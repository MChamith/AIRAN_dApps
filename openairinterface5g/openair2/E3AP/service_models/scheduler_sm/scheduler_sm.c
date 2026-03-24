#include "scheduler_sm.h"
#include "scheduler_enc.h"
#include "scheduler_dec.h"

#include "common/utils/LOG/log.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Default sampling threshold (collect stats every 5 scheduler invocations)
#define DEFAULT_SM_SAMPLING_THRESHOLD 5

// Scheduler SM RAN function configuration
static uint32_t scheduler_ran_functions[] = {SCHEDULER_SM_RAN_FUNCTION_ID};

e3_sm_scheduler_control_t *e3_sm_scheduler_control = NULL;

// Scheduler SM instance
e3_service_model_t scheduler_sm = {
    .name = "scheduler_sm",
    .version = 1,
    .ran_function_ids = scheduler_ran_functions,
    .ran_function_count = sizeof(scheduler_ran_functions) / sizeof(scheduler_ran_functions[0]),
    .init = scheduler_sm_init,
    .destroy = scheduler_sm_destroy,
    .thread_main = scheduler_sm_thread_main,
    .process_dapp_control_action = scheduler_sm_process_dapp_control_action,
    .thread_context = NULL,
    .is_running = false,
    .format = FORMAT_ASN1
};

// Global SM context
static scheduler_sm_context_t *scheduler_context = NULL;

/**
 * Initialize the scheduler SM
 */
int scheduler_sm_init(e3_service_model_t *sm) {
    LOG_D(E3AP, "[SCHEDULER] Initializing scheduler SM\n");
    
    if (scheduler_context && scheduler_context->initialized) {
        LOG_W(E3AP, "[SCHEDULER] SM already initialized\n");
        return SM_SUCCESS;
    }
    
    scheduler_context = malloc(sizeof(scheduler_sm_context_t));
    if (!scheduler_context) {
        LOG_E(E3AP, "[SCHEDULER] Failed to allocate scheduler SM context\n");
        return SM_ERROR_MEMORY;
    }
    
    memset(scheduler_context, 0, sizeof(scheduler_sm_context_t));
    
    // Initialize control structure
    e3_sm_scheduler_control = (e3_sm_scheduler_control_t *)malloc(sizeof(e3_sm_scheduler_control_t));
    if (!e3_sm_scheduler_control) {
        LOG_E(E3AP, "[SCHEDULER] Failed to allocate control structure\n");
        free(scheduler_context);
        scheduler_context = NULL;
        return SM_ERROR_MEMORY;
    }
    
    pthread_mutex_init(&e3_sm_scheduler_control->mutex, NULL);
    e3_sm_scheduler_control->policy_ready = 0;
    e3_sm_scheduler_control->stats_ready = 0;
    e3_sm_scheduler_control->requested_policy = SCHEDULER_POLICY_ROUND_ROBIN;
    e3_sm_scheduler_control->current_policy = SCHEDULER_POLICY_ROUND_ROBIN;
    
    // Sampling configuration: collect stats every 5 scheduler invocations
    e3_sm_scheduler_control->sampling_threshold = DEFAULT_SM_SAMPLING_THRESHOLD;
    e3_sm_scheduler_control->sampling_counter = 0;
    
    // Initialize statistics
    e3_sm_scheduler_control->last_frame = 0;
    e3_sm_scheduler_control->last_slot = 0;
    e3_sm_scheduler_control->num_ues = 0;
    
    scheduler_context->initialized = true;
    LOG_I(E3AP, "[SCHEDULER] Scheduler SM initialized successfully (sampling_threshold=%d)\n", 
          e3_sm_scheduler_control->sampling_threshold);
    return SM_SUCCESS;
}

/**
 * Destroy the scheduler SM
 */
int scheduler_sm_destroy(e3_service_model_t *sm) {
    LOG_D(E3AP, "[SCHEDULER] Destroying scheduler SM\n");
    
    if (!scheduler_context) {
        LOG_D(E3AP, "[SCHEDULER] SM context already destroyed\n");
        return SM_SUCCESS;
    }
    
    if (e3_sm_scheduler_control) {
        pthread_mutex_destroy(&e3_sm_scheduler_control->mutex);
        free(e3_sm_scheduler_control);
        e3_sm_scheduler_control = NULL;
    }
    
    free(scheduler_context);
    scheduler_context = NULL;
    
    LOG_D(E3AP, "[SCHEDULER] Scheduler SM destroyed successfully\n");
    return SM_SUCCESS;
}

/**
 * Main thread function for scheduler SM
 * Polls the ready flag and encodes indication messages when statistics are available
 */
void* scheduler_sm_thread_main(void *context) {
    sm_thread_context_t *thread_ctx = (sm_thread_context_t *)context;
    scheduler_sm_context_t *sm_ctx = scheduler_context;
    
    if (!sm_ctx || !sm_ctx->initialized) {
        LOG_E(E3AP, "[SCHEDULER] SM context not initialized, exiting main thread\n");
        return NULL;
    }
    
    LOG_I(E3AP, "[SCHEDULER] Main thread started\n");
    
    // Main event processing loop
    while (1) {
        // Check if we should stop
        pthread_mutex_lock(&thread_ctx->stop_mutex);
        bool should_stop = thread_ctx->should_stop;
        pthread_mutex_unlock(&thread_ctx->stop_mutex);
        
        if (should_stop) {
            LOG_I(E3AP, "[SCHEDULER] Main thread stopping\n");
            break;
        }
        
        // Check if statistics are ready
        pthread_mutex_lock(&e3_sm_scheduler_control->mutex);
        int ready = e3_sm_scheduler_control->stats_ready;
        pthread_mutex_unlock(&e3_sm_scheduler_control->mutex);
        
        if (ready == 1) {
            // Read statistics from control structure
            pthread_mutex_lock(&e3_sm_scheduler_control->mutex);
            
            scheduler_stats_t stats;
            stats.current_policy = e3_sm_scheduler_control->current_policy;
            stats.frame_number = e3_sm_scheduler_control->last_frame;
            stats.slot_number = e3_sm_scheduler_control->last_slot;
            stats.num_ues = e3_sm_scheduler_control->num_ues;
            
            // Copy per-UE statistics
            memcpy(stats.ue_stats, e3_sm_scheduler_control->ue_stats, 
                   sizeof(ue_scheduler_stats_t) * stats.num_ues);
            
            // Clear ready flag
            e3_sm_scheduler_control->stats_ready = 0;
            
            pthread_mutex_unlock(&e3_sm_scheduler_control->mutex);
            
            // Encode indication message
            uint8_t *encoded_data = NULL;
            size_t encoded_size = 0;
            
            int encode_result = scheduler_encode_indication(&stats, &encoded_data, &encoded_size);
            
            if (encode_result == SM_SUCCESS && encoded_data) {
                // Store in shared data structure for E3 agent to publish
                int set_result = sm_indication_data_set(thread_ctx->output_data, 
                                                       encoded_data, encoded_size, 0);
                if (set_result == SM_SUCCESS) {
                    LOG_D(E3AP, "[SCHEDULER] Indication data ready (%zu bytes, policy=%d, numUEs=%d)\n", 
                          encoded_size, stats.current_policy, stats.num_ues);
                } else {
                    LOG_E(E3AP, "[SCHEDULER] Failed to set indication data: %d\n", set_result);
                }
                
                free(encoded_data);
            } else {
                LOG_E(E3AP, "[SCHEDULER] Failed to encode scheduler indication: %d\n", encode_result);
            }
        }
        
        // Sleep briefly to avoid busy-waiting
        usleep(10000); // 10ms
    }
    
    LOG_I(E3AP, "[SCHEDULER] Main thread finished\n");
    return NULL;
}

/**
 * Process control messages for scheduler SM
 */
int scheduler_sm_process_dapp_control_action(uint32_t ran_function_id, uint8_t *encoded_data, size_t size) {
    if (ran_function_id != SCHEDULER_SM_RAN_FUNCTION_ID) {
        LOG_E(E3AP, "[SCHEDULER] Invalid RAN function ID %u\n", ran_function_id);
        return SM_ERROR_INVALID_PARAM;
    }
    
    if (!encoded_data || size == 0) {
        LOG_E(E3AP, "[SCHEDULER] Invalid control data parameters\n");
        return SM_ERROR_INVALID_PARAM;
    }
    
    LOG_I(E3AP, "[SCHEDULER] Processing control message (%zu bytes)\n", size);
    
    scheduler_control_t* control_payload = scheduler_decode_control(encoded_data, size);
    
    if (!control_payload) {
        LOG_E(E3AP, "[SCHEDULER] Failed to decode control message\n");
        return SM_ERROR_INVALID_PARAM;
    }
    
    // Validate policy value
    if (control_payload->requested_policy != SCHEDULER_POLICY_ROUND_ROBIN && 
        control_payload->requested_policy != SCHEDULER_POLICY_PROPORTIONAL_FAIR) {
        LOG_E(E3AP, "[SCHEDULER] Invalid policy value: %d (must be 0=RR or 1=PF)\n", 
              control_payload->requested_policy);
        scheduler_free_decoded_control(control_payload);
        return SM_ERROR_INVALID_PARAM;
    }
    
    LOG_I(E3AP, "[SCHEDULER] Requested policy: %d (%s)\n", 
          control_payload->requested_policy,
          control_payload->requested_policy == SCHEDULER_POLICY_ROUND_ROBIN ? "Round Robin" : "Proportional Fair");
    
    // Apply control action
    pthread_mutex_lock(&e3_sm_scheduler_control->mutex);
    
    e3_sm_scheduler_control->requested_policy = control_payload->requested_policy;
    
    // Update sampling threshold if specified
    if (control_payload->sampling_threshold > 0) {
        LOG_I(E3AP, "[SCHEDULER] Change sampling threshold from %d to %d\n", 
              e3_sm_scheduler_control->sampling_threshold, control_payload->sampling_threshold);
        e3_sm_scheduler_control->sampling_threshold = control_payload->sampling_threshold;
    }
    
    e3_sm_scheduler_control->policy_ready = 1; // Signal policy change to RAN
    
    pthread_mutex_unlock(&e3_sm_scheduler_control->mutex);
    
    LOG_I(E3AP, "[SCHEDULER] Control action applied successfully\n");
    scheduler_free_decoded_control(control_payload);
    
    return SM_SUCCESS;
}

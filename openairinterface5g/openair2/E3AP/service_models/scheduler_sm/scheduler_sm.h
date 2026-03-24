#ifndef SCHEDULER_SM_H
#define SCHEDULER_SM_H

#include "../sm_interface.h"
#include "common/utils/nr/nr_common.h"

// Forward declarations for scheduler SM functions
int scheduler_sm_init(e3_service_model_t *sm);
int scheduler_sm_destroy(e3_service_model_t *sm);
void* scheduler_sm_thread_main(void *context);
int scheduler_sm_process_dapp_control_action(uint32_t ran_function_id, uint8_t *encoded_data, size_t size);

// Scheduler SM RAN function ID
#define SCHEDULER_SM_RAN_FUNCTION_ID 2

// Scheduler policy enumeration
#define SCHEDULER_POLICY_ROUND_ROBIN 0
#define SCHEDULER_POLICY_PROPORTIONAL_FAIR 1

// Maximum UEs for statistics collection
#define MAX_UE_SCHEDULER_STATS 256

/**
 * @brief Per-UE scheduler statistics
 */
typedef struct ue_scheduler_stats {
  uint16_t rnti;
  uint32_t avg_throughput;  // in bps
  uint16_t bler;            // scaled by 100 (e.g., 1234 = 12.34%)
  uint8_t current_mcs;
  uint32_t pending_bytes;
  bool is_retx;
  uint16_t rbs_allocated;
  uint16_t beam_index;
} ue_scheduler_stats_t;

/**
 * @brief E3 Scheduler SM control structure
 * Shared between E3 SM (writer) and RAN scheduler (reader/writer)
 * Thread-safe communication using mutex protection
 */
typedef struct e3_sm_scheduler_control {
  pthread_mutex_t mutex;          ///< Mutex for thread-safe access
  int policy_ready;               ///< Flag: 1 = policy change available from E3 SM
  int stats_ready;                ///< Flag: 1 = statistics ready for E3 SM to read
  
  // Control fields (written by E3 SM, read by RAN)
  uint8_t requested_policy;       ///< Requested policy from dApp (0=RR, 1=PF)
  uint32_t sampling_threshold;    ///< Reporting interval (default 5 scheduler invocations)
  
  // Statistics fields (written by RAN, read by E3 SM)
  uint8_t current_policy;         ///< Currently active policy
  uint32_t sampling_counter;      ///< Current counter (incremented by RAN)
  frame_t last_frame;             ///< Frame number when stats were collected
  slot_t last_slot;               ///< Slot number when stats were collected
  uint8_t num_ues;                ///< Number of UEs in ue_stats array
  ue_scheduler_stats_t ue_stats[MAX_UE_SCHEDULER_STATS];  ///< Per-UE statistics
} e3_sm_scheduler_control_t;

extern e3_sm_scheduler_control_t* e3_sm_scheduler_control;

// Scheduler SM specific context
typedef struct {
  bool initialized;
} scheduler_sm_context_t;

// Export the scheduler SM instance
extern e3_service_model_t scheduler_sm;

#endif // SCHEDULER_SM_H

#include "scheduler_enc.h"
#include "common/utils/LOG/log.h"
#include "SchedulerPolicyIndication.h"
#include "UESchedulerStats.h"

#include <stdlib.h>
#include <string.h>

#define ENCODE_BUFFER_SIZE 65536  // Increased for per-UE stats

/**
 * Encode scheduler indication message using APER with per-UE statistics
 */
int scheduler_encode_indication(const scheduler_stats_t *stats, 
                                uint8_t **encoded_data, 
                                size_t *encoded_size)
{
    if (!stats || !encoded_data || !encoded_size) {
        LOG_E(E3AP, "[SCHEDULER_ENC] Invalid parameters\n");
        return SM_ERROR_INVALID_PARAM;
    }
    
    // Create ASN.1 structure
    SchedulerPolicyIndication_t *indication = calloc(1, sizeof(SchedulerPolicyIndication_t));
    if (!indication) {
        LOG_E(E3AP, "[SCHEDULER_ENC] Failed to allocate indication structure\n");
        return SM_ERROR_MEMORY;
    }
    
    // Populate scalar fields
    indication->currentPolicy = stats->current_policy;
    indication->frameNumber = stats->frame_number;
    indication->slotNumber = stats->slot_number;
    indication->numUEs = stats->num_ues;
    
    // Populate per-UE statistics SEQUENCE OF
    for (int i = 0; i < stats->num_ues; i++) {
        UESchedulerStats_t *ue_stat = calloc(1, sizeof(UESchedulerStats_t));
        if (!ue_stat) {
            LOG_E(E3AP, "[SCHEDULER_ENC] Failed to allocate UE stats structure\n");
            ASN_STRUCT_FREE(asn_DEF_SchedulerPolicyIndication, indication);
            return SM_ERROR_MEMORY;
        }
        
        const ue_scheduler_stats_t *src = &stats->ue_stats[i];
        
        ue_stat->rnti = src->rnti;
        ue_stat->avgThroughput = src->avg_throughput;
        ue_stat->bler = src->bler;
        ue_stat->currentMCS = src->current_mcs;
        ue_stat->pendingBytes = src->pending_bytes;
        ue_stat->isRetx = src->is_retx;
        ue_stat->rbsAllocated = src->rbs_allocated;
        ue_stat->beamIndex = src->beam_index;
        
        // Add to SEQUENCE OF
        ASN_SEQUENCE_ADD(&indication->ueStats.list, ue_stat);
    }
    
    // Encode with APER
    uint8_t *buffer = malloc(ENCODE_BUFFER_SIZE);
    if (!buffer) {
        LOG_E(E3AP, "[SCHEDULER_ENC] Failed to allocate encode buffer\n");
        ASN_STRUCT_FREE(asn_DEF_SchedulerPolicyIndication, indication);
        return SM_ERROR_MEMORY;
    }
    
    asn_enc_rval_t enc_ret = aper_encode_to_buffer(
        &asn_DEF_SchedulerPolicyIndication,
        NULL,
        indication,
        buffer,
        ENCODE_BUFFER_SIZE
    );
    
    if (enc_ret.encoded == -1) {
        LOG_E(E3AP, "[SCHEDULER_ENC] APER encoding failed for %s\n", enc_ret.failed_type->name);
        free(buffer);
        ASN_STRUCT_FREE(asn_DEF_SchedulerPolicyIndication, indication);
        return SM_ERROR_INVALID_PARAM;
    }
    
    // Calculate size in bytes (APER returns bits)
    *encoded_size = (enc_ret.encoded + 7) / 8;
    
    // Allocate output buffer and copy data
    *encoded_data = malloc(*encoded_size);
    if (!*encoded_data) {
        LOG_E(E3AP, "[SCHEDULER_ENC] Failed to allocate output buffer\n");
        free(buffer);
        ASN_STRUCT_FREE(asn_DEF_SchedulerPolicyIndication, indication);
        return SM_ERROR_MEMORY;
    }
    
    memcpy(*encoded_data, buffer, *encoded_size);
    
    LOG_D(E3AP, "[SCHEDULER_ENC] Encoded indication: policy=%d, frame=%d, slot=%d, numUEs=%d (%zu bytes)\n",
          stats->current_policy, stats->frame_number, stats->slot_number,
          stats->num_ues, *encoded_size);
    
    // Cleanup
    free(buffer);
    ASN_STRUCT_FREE(asn_DEF_SchedulerPolicyIndication, indication);
    
    return SM_SUCCESS;
}

#include "scheduler_dec.h"
#include "common/utils/LOG/log.h"
#include "SchedulerPolicyControl.h"

#include <stdlib.h>
#include <string.h>

/**
 * Decode scheduler control message using APER
 */
scheduler_control_t* scheduler_decode_control(const uint8_t *encoded_data, size_t size)
{
    if (!encoded_data || size == 0) {
        LOG_E(E3AP, "[SCHEDULER_DEC] Invalid parameters\n");
        return NULL;
    }
    
    // Decode ASN.1 structure
    SchedulerPolicyControl_t *asn_control = NULL;
    asn_dec_rval_t dec_ret = aper_decode(
        NULL,
        &asn_DEF_SchedulerPolicyControl,
        (void **)&asn_control,
        encoded_data,
        size,
        0,
        0
    );
    
    if (dec_ret.code != RC_OK || !asn_control) {
        LOG_E(E3AP, "[SCHEDULER_DEC] APER decoding failed (code=%d)\n", dec_ret.code);
        if (asn_control) {
            ASN_STRUCT_FREE(asn_DEF_SchedulerPolicyControl, asn_control);
        }
        return NULL;
    }
    
    // Extract fields into native structure
    scheduler_control_t *control = malloc(sizeof(scheduler_control_t));
    if (!control) {
        LOG_E(E3AP, "[SCHEDULER_DEC] Failed to allocate control structure\n");
        ASN_STRUCT_FREE(asn_DEF_SchedulerPolicyControl, asn_control);
        return NULL;
    }
    
    control->requested_policy = asn_control->requestedPolicy;
    
    // Extract optional sampling threshold
    control->sampling_threshold = 0;
    if (asn_control->samplingThreshold) {
        control->sampling_threshold = *(asn_control->samplingThreshold);
    }
    
    LOG_D(E3AP, "[SCHEDULER_DEC] Decoded control: policy=%d, sampling_threshold=%d\n",
          control->requested_policy, control->sampling_threshold);
    
    // Cleanup ASN.1 structure
    ASN_STRUCT_FREE(asn_DEF_SchedulerPolicyControl, asn_control);
    
    return control;
}

/**
 * Free decoded control structure
 */
void scheduler_free_decoded_control(scheduler_control_t *control)
{
    if (control) {
        free(control);
    }
}

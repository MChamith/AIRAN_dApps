#ifndef SCHEDULER_DEC_H
#define SCHEDULER_DEC_H

#include "../sm_interface.h"
#include <stdint.h>
#include <stddef.h>

/**
 * @brief Scheduler control structure for control messages
 */
typedef struct {
    uint8_t requested_policy;     ///< Requested policy (0=RR, 1=PF)
    uint32_t sampling_threshold;  ///< Sampling threshold (0 if not specified)
} scheduler_control_t;

/**
 * @brief Decode scheduler control message
 * 
 * @param encoded_data Input buffer with encoded control message
 * @param size Size of encoded data in bytes
 * @return Pointer to decoded control structure (caller must free with scheduler_free_decoded_control)
 */
scheduler_control_t* scheduler_decode_control(const uint8_t *encoded_data, size_t size);

/**
 * @brief Free decoded control structure
 * 
 * @param control Control structure to free
 */
void scheduler_free_decoded_control(scheduler_control_t *control);

#endif // SCHEDULER_DEC_H

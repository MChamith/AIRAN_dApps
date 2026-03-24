#ifndef SCHEDULER_ENC_H
#define SCHEDULER_ENC_H

#include "../sm_interface.h"
#include "scheduler_sm.h"
#include <stdint.h>
#include <stddef.h>

/**
 * @brief Scheduler statistics structure for indication messages
 */
typedef struct {
    uint8_t current_policy;                               ///< Current active policy (0=RR, 1=PF)
    uint16_t frame_number;                                ///< Frame number (0-1023)
    uint16_t slot_number;                                 ///< Slot number (0-319)
    uint8_t num_ues;                                      ///< Number of UEs in ue_stats array
    ue_scheduler_stats_t ue_stats[MAX_UE_SCHEDULER_STATS]; ///< Per-UE statistics
} scheduler_stats_t;

/**
 * @brief Encode scheduler indication message
 * 
 * @param stats Statistics to encode
 * @param encoded_data Output buffer (allocated by function, caller must free)
 * @param encoded_size Output size in bytes
 * @return SM_SUCCESS on success, error code otherwise
 */
int scheduler_encode_indication(const scheduler_stats_t *stats, 
                                uint8_t **encoded_data, 
                                size_t *encoded_size);

#endif // SCHEDULER_ENC_H

/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#ifndef OAI_VRTSIM_CIRDB_PROVIDER_H
#define OAI_VRTSIM_CIRDB_PROVIDER_H

#include <stdint.h>
#include "SIMULATION/TOOLS/sim.h"

/*
  Selection and timing options:

  want_model_id: TDL family index. Allowed: 0 (TDL-A), 1 (TDL-B), 2 (TDL-C), 3 (TDL-D), 4 (TDL-E).
                 Set < 0 to express no preference.
  want_ds_ns:    RMS delay spread in ns. Typical values: 30, 100, 300. Nonnegative float.
                 Set < 0 to express no preference.
  want_speed_mps: UE speed in m/s. Typical: walking ≈ 1.5, urban 3..10, vehicular 10..30.
                  Set < 0 to express no preference.
*/
typedef struct {
  const char *yaml_path; /* optional absolute path to vrtsim.yaml */
  const char *bin_path; /* optional absolute path to cir_db.bin  */

  int want_model_id; /* 0..4, or <0 for no preference */
  float want_ds_ns; /* nonnegative, or <0 for no preference */
  float want_speed_mps; /* nonnegative, or <0 for no preference */
} cirdb_select_opts_t;

/* Initialize provider and publish snapshot 0 through channel_desc_out. */
void cirdb_connect(int id,
                   int num_tx_antennas,
                   int num_rx_antennas,
                   const cirdb_select_opts_t *sel,
                   channel_desc_t **channel_desc_out);

/* Advance snapshot selection based on elapsed nanoseconds since the start. */
void cirdb_update(uint64_t ns_since_start);

/* Tear down and free resources. */
void cirdb_stop(void);

#endif /* OAI_VRTSIM_CIRDB_PROVIDER_H */

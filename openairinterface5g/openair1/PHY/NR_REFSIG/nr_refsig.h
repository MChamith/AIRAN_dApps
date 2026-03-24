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

/* Definitions for LTE Reference signals */
/* Author R. Knopp / EURECOM / OpenAirInterface.org */
#ifndef __NR_REFSIG__H__
#define __NR_REFSIG__H__

#include "PHY/nr_phy_common/inc/nr_phy_common.h"
#include "PHY/defs_nr_common.h"

uint32_t *gold_cache(uint32_t key, int length);
uint32_t *nr_gold_pbch(int Lmax, int Nid, int n_hf, int ssb);
uint32_t *nr_gold_pdcch(int N_RB_DL, int symbols_per_slot, unsigned short n_idDMRS, int ns, int l);
uint32_t *nr_gold_pdsch(int N_RB_DL, int symbols_per_slot, int nid, int nscid, int slot, int symbol);
uint32_t *nr_gold_pusch(int N_RB_UL, int symbols_per_slot, int Nid, int nscid, int slot, int symbol);
uint32_t *nr_gold_csi_rs(int N_RB_DL, int symbols_per_slot, int slot, int symb, uint32_t Nid);
uint32_t *nr_gold_prs(int nid, int slot, int symbol);

int nr_pdsch_dmrs_rx(nr_prefix_type_t Ncp,
                     unsigned int Ns,
                     const unsigned int *nr_gold_pdsch,
                     c16_t *output,
                     unsigned short p,
                     unsigned char lp,
                     unsigned short nb_pdsch_rb,
                     uint8_t config_type,
                     int16_t dmrs_scaling);

/*!\brief This function generates the NR Gold sequence (38-211, Sec 5.2.1) for the PBCH DMRS.
@param PHY_VARS_NR_UE* ue structure provides configuration, frame parameters and the pointers to the 32 bits sequence storage tables
 */
void nr_pbch_dmrs_rx(int dmrss, const unsigned int *nr_gold_pbch, c16_t *output, bool sidelink);

/*!\brief This function generates the NR Gold sequence (38-211, Sec 5.2.1) for the PDCCH DMRS.
@param PHY_VARS_NR_UE* ue structure provides configuration, frame parameters and the pointers to the 32 bits sequence storage tables
 */
void nr_pdcch_dmrs_ref(const unsigned int *nr_gold_pdcch, c16_t *output, unsigned short nb_rb_corset);

int nr_pusch_dmrs_rx(nr_prefix_type_t Ncp,
                     unsigned int Ns,
                     const uint32_t *nr_gold_pusch,
                     c16_t *output,
                     unsigned short p,
                     unsigned char lp,
                     unsigned short nb_pusch_rb,
                     uint32_t re_offset,
                     uint8_t dmrs_type,
                     int16_t dmrs_scaling);

void nr_generate_modulation_table(void);

extern simde__m128i byte2m128i[256];

int nr_pusch_lowpaprtype1_dmrs_rx(nr_prefix_type_t Ncp,
                                  unsigned int Ns,
                                  c16_t *dmrs_seq,
                                  c16_t *output,
                                  unsigned short p,
                                  unsigned char lp,
                                  unsigned short nb_pusch_rb,
                                  uint32_t re_offset,
                                  uint8_t dmrs_type);

#endif

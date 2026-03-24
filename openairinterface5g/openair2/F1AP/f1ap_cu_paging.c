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

/*! \file f1ap_cu_paging.c
 * \brief f1ap interface paging for CU
 * \author EURECOM/NTUST
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr, bing-kai.hong@eurecom.fr
 * \note
 * \warning
 */

#include "f1ap_common.h"
#include "f1ap_encoder.h"
#include "f1ap_itti_messaging.h"
#include "f1ap_cu_paging.h"
#include "lib/f1ap_paging.h"
#include "common/utils/ds/byte_array.h"

int CU_send_Paging(sctp_assoc_t assoc_id, const f1ap_paging_t *paging)
{
  F1AP_F1AP_PDU_t *pdu = encode_f1ap_paging(paging);
  if (pdu == NULL) {
    LOG_E(F1AP, "Failed to encode F1 Paging\n");
    return -1;
  }

  byte_array_t ba = {0};
  /* encode */
  if (f1ap_encode_pdu(pdu, &ba.buf, (uint32_t *)&ba.len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 Paging PDU\n");
    ASN_STRUCT_FREE(asn_DEF_F1AP_F1AP_PDU, pdu);
    return -1;
  }
  ASN_STRUCT_FREE(asn_DEF_F1AP_F1AP_PDU, pdu);
  /* Ownership of ba.buf is transferred to SCTP task; do not free here. */
  f1ap_itti_send_sctp_data_req(assoc_id, ba.buf, ba.len);
  return 0;
}

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

#include "f1ap_common.h"
#include "f1ap_du_paging.h"
#include "lib/f1ap_paging.h"
#include "conversions.h"
#include "oai_asn1.h"
#include "openair2/RRC/LTE/rrc_proto.h"

/* @brief Handle F1AP Paging message at DU */
int DU_handle_Paging(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  f1ap_paging_t decoded = {0};

  DevAssert(pdu);
  (void)instance;
  (void)assoc_id;
  (void)stream;

  if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
    xer_fprint(stdout, &asn_DEF_F1AP_F1AP_PDU, pdu);
  }

  if (!decode_f1ap_paging(&decoded, pdu)) {
    LOG_E(F1AP, "Failed to decode F1AP Paging\n");
    return -1;
  }

  /** @todo Build PCCH-Message (Paging) at DU per TS 38.331 §5.3.2; apply
   *  RRC padding per §8.5; deliver as RLC SDU per §8.2. For each
   *  cell in Paging Cell List that belongs to this DU, queue for MAC; MAC schedules
   *  at PF/PO per TS 38.304 §7. */

  free_f1ap_paging(&decoded);
  return 0;
}

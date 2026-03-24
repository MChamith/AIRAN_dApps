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

#ifndef NGAP_GNB_PDU_SESSION_MANAGEMENT_H_
#define NGAP_GNB_PDU_SESSION_MANAGEMENT_H_

#include <stdint.h>
#include "assertions.h"
#include "ngap_messages_types.h"
#include "ngap_msg_includes.h"

int ngap_gNB_pdusession_setup_resp(instance_t instance, ngap_pdusession_setup_resp_t *pdusession_setup_resp_p);

int ngap_gNB_pdusession_modify_resp(instance_t instance, ngap_pdusession_modify_resp_t *pdusession_modify_resp_p);

int ngap_gNB_pdusession_release_resp(instance_t instance, ngap_pdusession_release_resp_t *pdusession_release_resp_p);

#endif /* NGAP_GNB_PDU_SESSION_MANAGEMENT_H_ */

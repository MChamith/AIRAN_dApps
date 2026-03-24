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


#include "e2ap_criticality_diagnostics.h"
#include <assert.h>
#include <stdlib.h>

bool eq_criticality_diagnostics(const criticality_diagnostics_t* m0, const criticality_diagnostics_t* m1)
{
  if(m0 == m1) return true;

  if(m0 != NULL || m1 != NULL) return false;

  assert(0!=0 && "Not implemented");

  return true;
}

criticality_diagnostics_t copy_criticality_diagnostics(const criticality_diagnostics_t* src)
{
  criticality_diagnostics_t dst = {0};

  if (src->procedure_code != NULL) {
    dst.procedure_code = calloc(1, sizeof(uint8_t));
    assert(dst.procedure_code != NULL && "Memory exhausted");
    *dst.procedure_code = *src->procedure_code;
  }

  if (src->trig_msg != NULL) {
    dst.trig_msg = calloc(1, sizeof(triggering_message_e));
    assert(dst.trig_msg != NULL && "Memory exhausted");
    *dst.trig_msg = *src->trig_msg;
  }

  if (src->proc_crit != NULL) {
    dst.proc_crit = calloc(1, sizeof(criticality_e));
    assert(dst.proc_crit != NULL && "Memory exhausted");
    *dst.proc_crit = *src->proc_crit;
  }

  if (src->req_id != NULL) {
    dst.req_id = calloc(1, sizeof(ric_gen_id_t));
    assert(dst.req_id != NULL && "Memory exhausted");
    *dst.req_id = copy_ric_gen_id(src->req_id);
  }

  if (src->len_ie > 0) {
    dst.len_ie = src->len_ie;
    dst.ie = calloc(dst.len_ie, sizeof(ie_criticality_diagnostics_t));
    assert(dst.ie != NULL && "Memory exhausted");
    for (size_t i = 0; i < src->len_ie; i++) {
      dst.ie[i] = copy_ie_criticality_diagnostics(&src->ie[i]);
    }
  }
  return dst;
}

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

#ifndef FGS_NAS_UTILS_H
#define FGS_NAS_UTILS_H

#include <stdint.h>
#include <arpa/inet.h>
#include <string.h> // For memcpy
#include <stdio.h>
#include "utils.h" // text_info_t, TO_ENUM, TO_TEXT
#include "common/utils/eq_check.h"

#define GET_SHORT(input, size) ({           \
    uint16_t tmp16;                         \
    memcpy(&tmp16, (input), sizeof(tmp16)); \
    size += htons(tmp16);                   \
})

#endif // FGS_NAS_UTILS_H

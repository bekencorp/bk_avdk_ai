/*
 * Copyright 2025 Alibaba Group Holding Ltd.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *     http: *www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __UTIL_TYPE_TRANS_H__
#define __UTIL_TYPE_TRANS_H__

#ifdef __cplusplus
extern "C"{
#endif

#include "c_utils.h"

uint8_t util_char2hex(char c[2]);
uint32_t util_str2hex(uint8_t *hex, char *str, uint32_t str_len);
void util_random_bytes(uint8_t *buf, uint32_t len);
int32_t util_lsb_msb_trans(uint8_t *data, uint32_t data_size);

#ifdef __cplusplus
}
#endif

#endif

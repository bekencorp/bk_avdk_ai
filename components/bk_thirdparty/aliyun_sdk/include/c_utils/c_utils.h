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

#ifndef __C_UTILS_H__
#define __C_UTILS_H__

#ifdef __cplusplus
extern "C"{
#endif

// 需要确认平台 sizeof(int) sizeof(unsigned int) 是否都为4字节
#if CONFIG_PLATFORM_ESP32 || CONFIG_PLATFORM_AIW626X
typedef int int32_t;
#define _INT32_T_DECLARED
typedef unsigned int uint32_t;
#define _UINT32_T_DECLARED
#endif

#if CONFIG_PLATFORM_XR872
#undef __INT32_TYPE__
typedef int __INT32_TYPE__;
#undef __UINT32_TYPE__
typedef unsigned int __UINT32_TYPE__;
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// util log配置
#define CONFIG_UTIL_LOG_ENABLE      1

#include "util_log.h"

#include "hal_util_mem.h"
#include "hal_util_mutex.h"
#include "hal_util_random.h"
#include "hal_util_storage.h"
#include "hal_util_time.h"

#include "util_list.h"
#include "util_ringbuffer.h"
#include "util_string.h"
#include "util_type_trans.h"

#include "lib_c_sdk.h"

typedef enum _util_result_t {
    UTIL_SUCCESS = 0,
    UTIL_ERR_FAIL,
    UTIL_ERR_UNEXPECT,
    UTIL_ERR_UNDEFINE,
    UTIL_ERR_INVALID_PARAM,
    UTIL_ERR_NOT_FOUND,
    UTIL_ERR_NO_MEMORY,
    UTIL_ERR_ALREADY,
    UTIL_ERR_TIMEOUT,
    UTIL_ERR_NO_PERMISSION,
    UTIL_ERR_IO,
    UTIL_ERR_INIT_FAIL,
    UTIL_ERR_UNSUPPORT,
    UTIL_ERR_NO_INIT,
    UTIL_ERR_NO_READY,
    UTIL_ERR_BUSY,
} util_result_t;

#ifdef __cplusplus
}
#endif

#endif

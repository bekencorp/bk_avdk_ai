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

#ifndef _UTIL_RINGBUFFER_H_
#define _UTIL_RINGBUFFER_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "c_utils.h"

#define RINGBUFFER_MUTEX_ENABLE 0
#define UTIL_RINGBUFFER_OVERWRITE   1
#if UTIL_RINGBUFFER_OVERWRITE
#undef RINGBUFFER_MUTEX_ENABLE
#define RINGBUFFER_MUTEX_ENABLE 1
#endif

typedef struct _util_ringbuffer_t
{
    uint8_t *buffer;
    uint32_t size;
    uint32_t remain_size;   // 剩余可用缓冲大小
    uint32_t offset_read;
    uint32_t offset_write;
#if RINGBUFFER_MUTEX_ENABLE
    util_mutex_t *mutex;
#endif
} util_ringbuffer_t;

util_ringbuffer_t *util_ringbuffer_create(uint32_t size);
int32_t util_ringbuffer_delete(util_ringbuffer_t *rb);
uint32_t util_ringbuffer_put(util_ringbuffer_t *rb, uint8_t *data, uint32_t size);
uint32_t util_ringbuffer_get(util_ringbuffer_t *rb, uint8_t *data, uint32_t size);
uint8_t util_ringbuffer_writeable(util_ringbuffer_t *rb, uint32_t write_size);
int32_t util_ringbuffer_reset(util_ringbuffer_t *rb);

#ifdef __cplusplus
}
#endif

#endif

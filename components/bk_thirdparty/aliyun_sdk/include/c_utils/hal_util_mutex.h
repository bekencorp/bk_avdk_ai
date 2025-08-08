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

#ifndef __HAL_UTIL_MUTEX_H__
#define __HAL_UTIL_MUTEX_H__

#ifdef __cplusplus
extern "C"{
#endif

#include "c_utils.h"

#define MUTEX_WAIT_FOREVER  0x7FFFFFFF

/* 互斥锁结构体定义 */
typedef struct _util_mutex_t {
    void *mutex_handle; /* 互斥锁句柄，具体实现依赖于平台 */
} util_mutex_t;

/*****************************************************
 * Function: util_mutex_create
 * Description: 创建一个互斥锁对象。
 * Parameter: 无。
 * Return: util_mutex_t * --- 返回指向互斥锁结构体的指针。
 ****************************************************/
util_mutex_t * util_mutex_create(void);

/*****************************************************
 * Function: util_mutex_delete
 * Description: 删除指定的互斥锁对象。
 * Parameter:
 *     mutex --- 指向互斥锁结构体的指针。
 * Return: 无。
 ****************************************************/
void util_mutex_delete(util_mutex_t *mutex);

/*****************************************************
 * Function: util_mutex_lock
 * Description: 对指定的互斥锁进行加锁操作，带超时机制。
 * Parameter:
 *     mutex --- 指向互斥锁结构体的指针。
 *     timeout --- 加锁等待超时时间，单位为毫秒(ms)，可设为 MUTEX_WAIT_FOREVER 表示无限等待。
 * Return: int32_t --- 返回操作结果(util_result_t)。
 ****************************************************/
int32_t util_mutex_lock(util_mutex_t *mutex, int32_t timeout);

/*****************************************************
 * Function: util_mutex_unlock
 * Description: 对指定的互斥锁进行解锁操作。
 * Parameter:
 *     mutex --- 指向互斥锁结构体的指针。
 * Return: int32_t --- 返回操作结果(util_result_t)。
 ****************************************************/
int32_t util_mutex_unlock(util_mutex_t *mutex);

#ifdef __cplusplus
}
#endif

#endif

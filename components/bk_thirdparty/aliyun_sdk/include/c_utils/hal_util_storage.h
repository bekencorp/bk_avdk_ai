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

#ifndef __HAL_UTIL_STORAGE_H__
#define __HAL_UTIL_STORAGE_H__

#ifdef __cplusplus
extern "C"{
#endif

#include "c_utils.h"

/**
 * @brief擦除存储器
 * 
 * 该函数用于擦除存储器中的所有数据。在调用此函数之前，应确保不再需要存储器中的任何信息，
 * 因为擦除操作将删除所有数据，且此操作不可逆。
 * 
 * @return int32_t 返回擦除操作的结果。如果返回值为0，表示擦除成功；如果返回值非0，表示擦除过程中出现错误。
 */
int32_t util_storage_erase(void);

/**
 * @brief存储数据到存储器
 * 
 * 该函数将指定的数据存储到存储器中。在调用此函数之前，应确保数据的正确性和完整性，
 * 因为存储操作将覆盖存储器中的现有数据。
 * 
 * @param data 指向要存储的数据的指针。数据类型为uint8_t，即无符号的8位整数。
 * @param size 要存储的数据的大小，以字节为单位。数据类型为uint32_t，即无符号的32位整数。
 * @return int32_t 返回存储操作的结果。如果返回值为0，表示存储成功；如果返回值非0，表示存储过程中出现错误。
 */
int32_t util_storage_storage(uint8_t *data, uint32_t size);

/**
 * @brief从存储器加载数据
 * 
 * 该函数从存储器中加载指定大小的数据。在调用此函数之前，应确保提供的数据指针指向的内存区域足够大，
 * 以容纳从存储器加载的数据。
 * 
 * @param data 指向用于存储从存储器加载的数据的缓冲区的指针。数据类型为uint8_t，即无符号的8位整数。
 * @param size 要加载的数据的大小，以字节为单位。数据类型为uint32_t，即无符号的32位整数。
 * @return int32_t 返回加载操作的结果。如果返回值为0，表示加载成功；如果返回值非0，表示加载过程中出现错误。
 */
int32_t util_storage_load(uint8_t *data, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif

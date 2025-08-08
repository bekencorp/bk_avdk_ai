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

#ifndef __HAL_UTIL_TIME_H__
#define __HAL_UTIL_TIME_H__

#ifdef __cplusplus
extern "C"{
#endif

#include "c_utils.h"

/**
 * 获取当前时间戳（毫秒级）
 * 
 * 此函数用于获取当前的时间戳，精确到毫秒该时间戳通常用于计算时间差、
 * 记录事件发生时间等场景
 * 
 * 返回：当前时间戳（毫秒级）
 */
int64_t util_now_ms(void);

/**
 * 毫秒级睡眠函数
 * 
 * 此函数使当前线程暂停执行指定毫秒数，用于控制程序执行节奏、等待事件发生等
 * 
 * 参数 ms：需要暂停的毫秒数
 */
void util_msleep(uint32_t ms);

/**
 * 获取当前时间戳
 * 
 * 此函数用于获取当前的时间戳，即从1970年1月1日00:00:00 UTC开始到现在的毫秒数
 * 它没有输入参数，返回一个int64_t类型的值，代表当前的时间戳
 * 
 * @return int64_t 当前时间戳，单位为毫秒
 */
int64_t util_get_timestamp(void);

// 获取当前时间戳，单位为毫秒，即将废弃
int64_t util_get_timestamp_ms(void);

/**
 * 检查时间戳功能是否已初始化
 * 
 * 此函数用于检查时间戳相关功能是否已经初始化如果返回真（非零），则表示
 * 时间戳功能可用；如果返回假（零），则可能需要进行初始化操作或者避免使用时间戳功能
 * 
 * 返回：如果时间戳功能已初始化，则返回非零，否则返回零
 */
uint8_t util_timestamp_inited(void);

#ifdef __cplusplus
}
#endif

#endif

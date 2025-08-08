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

#ifndef __UTIL_LOG_H__
#define __UTIL_LOG_H__

#ifdef __cplusplus
extern "C"{
#endif

#include "c_utils.h"
#include <inttypes.h>

#define LOG_WITH_COLOR  1

#if LOG_WITH_COLOR
#define RED         "\033[31m"
#define GREEN       "\033[32m"
#define YELLOW      "\033[33m"
#define BLUE        "\033[34m"
#define CYAN        "\033[36m"
#define WHITE       "\033[37m"
#define COLOR_END   "\033[0m"
#else
#define RED
#define GREEN
#define YELLOW
#define BLUE
#define CYAN
#define WHITE
#define RESET
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))
#endif

enum {
    UTIL_LOG_LV_NONE = 0,
    UTIL_LOG_LV_ERR,
    UTIL_LOG_LV_WARN,
    UTIL_LOG_LV_INFO,
    UTIL_LOG_LV_HEX,
    UTIL_LOG_LV_DEBUG,
    UTIL_LOG_LV_ALL,
    UTIL_LOG_LV_DEFAULT = UTIL_LOG_LV_DEBUG,
};

#ifndef min
#define min(a, b)    ((a)<(b)?(a):(b))
#endif

#if CONFIG_UTIL_LOG_ENABLE
#define UTIL_LOG(G_LV, LV, TAG, COLOR, fmt, ...) do { if (min(G_LV, _log_level) >= LV) printf("[%08" PRIu64 "]" COLOR TAG "[%s]" fmt COLOR_END "\n", util_now_ms(), __func__, ##__VA_ARGS__); } while(0)

#define UTIL_HEX(G_LV, LV, TAG, head, data, len) do { \
    if (min(G_LV, _log_level) >= UTIL_LOG_LV_HEX) { \
        printf("[%09lld]" TAG "[%s][%d]%s: ", (long long int)util_now_ms(), __func__, len, head);\
        for(uint32_t __iii = 0; __iii < (uint32_t)len; __iii++) \
        { \
            printf("%02X ", ((uint8_t *)(data))[__iii]); \
        } \
        printf("\n");\
    } \
} while(0)
#else
#define UTIL_LOG
#define UTIL_HEX
#endif

#define UTIL_TAG        "[UT]"
#define UTIL_TAG_E      UTIL_TAG"[E]"
#define UTIL_TAG_W      UTIL_TAG"[W]"
#define UTIL_TAG_I      UTIL_TAG"[I]"
#define UTIL_TAG_H      UTIL_TAG"[H]"
#define UTIL_TAG_D      UTIL_TAG"[D]"

#define UTIL_LOG_E(format, ...)         UTIL_LOG(g_util_log_lv, UTIL_LOG_LV_ERR,   UTIL_TAG_E, RED,    format, ##__VA_ARGS__)
#define UTIL_LOG_W(format, ...)         UTIL_LOG(g_util_log_lv, UTIL_LOG_LV_WARN,  UTIL_TAG_W, YELLOW, format, ##__VA_ARGS__)
#define UTIL_LOG_I(format, ...)         UTIL_LOG(g_util_log_lv, UTIL_LOG_LV_INFO,  UTIL_TAG_I, GREEN,  format, ##__VA_ARGS__)
#define UTIL_LOG_H(str, data, size)     UTIL_HEX(g_util_log_lv, UTIL_LOG_LV_HEX,   UTIL_TAG_H, str, data, size)
#define UTIL_LOG_D(format, ...)         UTIL_LOG(g_util_log_lv, UTIL_LOG_LV_DEBUG, UTIL_TAG_D, WHITE,   format, ##__VA_ARGS__)

#define UTIL_LOG_IH(str, data, size)    UTIL_LOG_I(str, hex_str(data, size))

extern uint8_t g_util_log_lv;

int64_t util_now_ms(void);
void util_set_log_level(uint8_t log_lv);
const char *hex_str(const void *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif

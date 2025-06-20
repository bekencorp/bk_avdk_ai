// Copyright 2024-2025 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/* This file is used to debug uac work status by collecting statistics on the uac mic and speaker. */
#include "sdkconfig.h"
#include <os/os.h>
#include <os/mem.h>
#include <driver/uart.h>
#include "debug_dump.h"
#include "aud_intf_types.h"

#ifdef CONFIG_DEBUG_DUMP
#define UART_UTIL_TAG "d_dump"

#define LOGI(...) BK_LOGI(UART_UTIL_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(UART_UTIL_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(UART_UTIL_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(UART_UTIL_TAG, ##__VA_ARGS__)

#if CONFIG_DEBUG_DUMP_DATA_TYPE_EXTENSION
volatile debug_dump_data_header_t dump_header[HEADER_ARRAY_CNT] = 
{
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] =
         {
            .dump_type = DUMP_TYPE_AUD_MIC,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_RX_SPK,
            .dump_file_type = DUMP_FILE_TYPE_G722,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_TX_MIC,
            .dump_file_type = DUMP_FILE_TYPE_G722,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 3,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_AEC_MIC_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
         },
        .data_flow[1] = 
         {
            .dump_type = DUMP_TYPE_AEC_REF_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
         },
        .data_flow[2] = 
         {
            .dump_type = DUMP_TYPE_AEC_OUT_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_DEC_OUT_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
            .sample_rate = 16000,
            .frame_in_ms = 20,
         },
        .seq_no = 0
    },
};
#else
volatile debug_dump_data_header_t dump_header[HEADER_ARRAY_CNT] = 
{
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_AUD_MIC,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_RX_SPK,
            .dump_file_type = DUMP_FILE_TYPE_G722,
            .len = 0,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_TX_MIC,
            .dump_file_type = DUMP_FILE_TYPE_G722,
            .len = 0,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 3,
        .data_flow[0] = 
         {
            .dump_type = DUMP_TYPE_AEC_MIC_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
         },
        .data_flow[1] = 
         {
            .dump_type = DUMP_TYPE_AEC_REF_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
         },
        .data_flow[2] = 
         {
            .dump_type = DUMP_TYPE_AEC_OUT_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
         },
        .seq_no = 0
    },
    {
        .header_magicword_part1 = HEADER_MAGICWORD_PART1,
        .header_magicword_part2 = HEADER_MAGICWORD_PART2,
        .data_flow_num = 1,
        .data_flow[0] =
         {
            .dump_type = DUMP_TYPE_DEC_OUT_DATA,
            .dump_file_type = DUMP_FILE_TYPE_PCM,
            .len = 0,
         },
        .seq_no = 0
    },
};

#endif

const uint8_t g_dump_type2header_array_idx[DUMP_TYPE_MAX] = 
{
    DUMP_TYPE_AUD_MIC,
    DUMP_TYPE_RX_SPK,
    DUMP_TYPE_TX_MIC,
    DUMP_TYPE_AEC_MIC_DATA,
    DUMP_TYPE_AEC_MIC_DATA,
    DUMP_TYPE_AEC_MIC_DATA,
    DUMP_TYPE_DEC_OUT_DATA-2,//AEC MIC/REF/OUT DATA use same HEADER
};

/*map from aud_intf_voc_data_type_t to debug_dump_file_type_t*/
const uint8_t g_codec_type2dump_file_type2_mapping[DUMP_FILE_TYPE_MAX] = 
{
    DUMP_FILE_TYPE_G711A,
    DUMP_FILE_TYPE_PCM,
    DUMP_FILE_TYPE_G711U,
    DUMP_FILE_TYPE_G722,
    DUMP_FILE_TYPE_OPUS,
    DUMP_FILE_TYPE_MP3,
};

uart_util_t g_debug_data_uart_util = {0};

uint8_t dbg_dump_get_dump_file_type(uint8_t codec_type)
{
    if(DUMP_FILE_TYPE_MAX <= codec_type)
    {
        LOGE("%s,%d,codec type:%d is invalid!\n",__func__, __LINE__,codec_type);
        return DUMP_FILE_TYPE_PCM;
    }

    return g_codec_type2dump_file_type2_mapping[codec_type];
}

#endif

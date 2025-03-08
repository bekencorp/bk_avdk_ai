#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include "sdkconfig.h"
#include "uart_util.h"

#define HEADER_MAGICWORD_PART1    (0xDEADBEEF)
#define HEADER_MAGICWORD_PART2    (0x0F1001F0)
#define DEBUG_DUMP_MAX_DATA_FLOW_NO        (6)

typedef enum {
    DUMP_TYPE_AUD_MIC = 0,
    DUMP_TYPE_AGORA_RX_SPK,
    DUMP_TYPE_AGORA_TX_MIC,
    DUMP_TYPE_AEC_MIC_DATA,
    DUMP_TYPE_AEC_REF_DATA,
    DUMP_TYPE_AEC_OUT_DATA,
    DUMP_TYPE_MAX
} debug_dump_type_t;

#define HEADER_ARRAY_CNT (DUMP_TYPE_MAX-2)//AEC MIC/REF/OUT DATA use same HEADER

typedef enum {
    DUMP_FILE_TYPE_PCM = 0,
    DUMP_FILE_TYPE_G722,
    DUMP_FILE_TYPE_INVALID,
} debug_dump_file_type_t;

typedef struct
{
    uint8_t  dump_type;
    uint8_t  dump_file_type;
    uint16_t len;
}data_flow_t;

typedef struct
{
    uint32_t header_magicword_part1;
    uint32_t header_magicword_part2;
    uint32_t  data_flow_num;
    data_flow_t data_flow[DEBUG_DUMP_MAX_DATA_FLOW_NO];
    uint32_t seq_no;
    uint32_t timestamp;
} debug_dump_data_header_t;

#ifdef CONFIG_DEBUG_DUMP
extern uart_util_t g_debug_data_uart_util;
extern volatile debug_dump_data_header_t dump_header[HEADER_ARRAY_CNT];
extern const uint8_t g_dump_type2header_array_idx[DUMP_TYPE_MAX];

#define DEBUG_DATA_DUMP_UART_ID            (1)
#define DEBUG_DATA_DUMP_UART_BAUD_RATE     (2000000)

#define DEBUG_DATA_DUMP_BY_UART_OPEN()                        uart_util_create(&g_debug_data_uart_util, DEBUG_DATA_DUMP_UART_ID, DEBUG_DATA_DUMP_UART_BAUD_RATE)
#define DEBUG_DATA_DUMP_BY_UART_CLOSE()                       uart_util_destroy(&g_debug_data_uart_util)
#define DEBUG_DATA_DUMP_BY_UART_DATA(data_buf, len)           uart_util_tx_data(&g_debug_data_uart_util, data_buf, len)


#define DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW_NUM(dump_type,data_flow_num)              dump_header[g_dump_type2header_array_idx[dump_type]].data_flow_num = data_flow_num
#define DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW(type,data_flow_idx,file_type,length)      do\
                                                                                          {\
                                                                                              dump_header[g_dump_type2header_array_idx[dump_type]].data_flow[data_flow_idx].dump_type = type;\
                                                                                              dump_header[g_dump_type2header_array_idx[dump_type]].data_flow[data_flow_idx].dump_file_type = file_type;\
                                                                                              dump_header[g_dump_type2header_array_idx[dump_type]].data_flow[data_flow_idx].len = length;\
                                                                                          }while(0)
#define DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW_LEN(dump_type,data_flow_idx,length) dump_header[g_dump_type2header_array_idx[dump_type]].data_flow[data_flow_idx].len = length
#define DEBUG_DATA_DUMP_UPDATE_HEADER_TIMESTAMP(dump_type)            dump_header[g_dump_type2header_array_idx[dump_type]].timestamp = rtos_get_time()
#define DEBUG_DATA_DUMP_UPDATE_HEADER_SEQ_NUM(dump_type)              dump_header[g_dump_type2header_array_idx[dump_type]].seq_no++
#define DEBUG_DATA_DUMP_BY_UART_HEADER(dump_type)                     DEBUG_DATA_DUMP_BY_UART_DATA((void *)&dump_header[g_dump_type2header_array_idx[dump_type]], sizeof(debug_dump_data_header_t))

#else
#define DEBUG_DATA_DUMP_BY_UART_OPEN()
#define DEBUG_DATA_DUMP_BY_UART_CLOSE()
#define DEBUG_DATA_DUMP_BY_UART_DATA(data_buf, len)

#define DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW_NUM(dump_type,data_flow_num)
#define DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW(dump_type,data_flow_idx,dump_file_type,len)
#define DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW_LEN(dump_type,data_flow_idx,len)
#define DEBUG_DATA_DUMP_UPDATE_HEADER_TIMESTAMP(dump_type)
#define DEBUG_DATA_DUMP_UPDATE_HEADER_SEQ_NUM(dump_type)    
#define DEBUG_DATA_DUMP_BY_UART_HEADER(dump_type)

#endif  //CONFIG_DEBUG_DUMP

#ifdef __cplusplus
}
#endif


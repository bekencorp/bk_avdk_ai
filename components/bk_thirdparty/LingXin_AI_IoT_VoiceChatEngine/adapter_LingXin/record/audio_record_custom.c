#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <common/bk_typedef.h>
#include "chat_module_config.h"
#include "audio_buffer_play.h"
#include "chat_state_machine.h"
#include "audio_engine.h"

#define TAG "lx_record"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#ifdef CONFIG_RECORD_CUSTOM_ENABLE
uint16_t record_flag = 0;
// 实现自定义的逻辑
int bk_record_data_send(unsigned char *data_ptr, unsigned int data_len)
{
    if (record_flag == 1) {
        int rval = state_machine_post_record_data(data_ptr, data_len);
        LOGD("lingxin TX audio, data_ptr=%p, data_len=%d\r\n", data_ptr, (int)data_len);
        if (rval < 0)
        {
            LOGE("Failed to send record data, %d\n", rval);
            return BK_FAIL;
        }
        return rval;
    }
    else
        return 0;
}

int module_record_init()
{
    LOGI("module_record_init\n");
    record_flag = 1;
    audio_tras_register_tx_data_func(bk_record_data_send);
    state_machine_run_event(State_Event_Record_Ready);
    return BK_OK;
}

void module_record_stop()
{
    LOGI("module_record_stop\n");
    record_flag = 2;
    state_machine_run_event(State_Event_Record_Stop);
}

void module_record_terminate()
{
    LOGI("module_record_terminate\n");
    record_flag = 3;
    state_machine_run_event(State_Event_Record_TerminateEnd);
}

#endif
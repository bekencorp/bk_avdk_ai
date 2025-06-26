// Copyright 2020-2021 Beken
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

#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>

#include "media_core.h"
#include "media_evt.h"
#include "media_app.h"
#include "audio_act.h"
#include "aud_tras_drv.h"
#include "aud_intf_private.h"

#include <driver/media_types.h>


#define AUDIO_ACT_TAG "audio"

#define LOGI(...) BK_LOGI(AUDIO_ACT_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(AUDIO_ACT_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(AUDIO_ACT_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(AUDIO_ACT_TAG, ##__VA_ARGS__)


typedef struct
{
    uint32_t evt;
    void *data;
    uint16_t len;
} audio_msg_t;

static beken_thread_t s_audio_task = NULL;
static beken_semaphore_t s_audio_sema = NULL;


static beken_queue_t s_audio_msg_que = NULL;

static void audio_act_task(void *arg)
{
    int32_t ret = 0;
    uint32_t evt = 0;
    audio_msg_t msg;
    media_mailbox_msg_t *mb_msg = NULL;

    rtos_set_semaphore(&s_audio_sema);


    while (1)
    {
        ret = rtos_pop_from_queue(&s_audio_msg_que, &msg, BEKEN_WAIT_FOREVER);
        if (kNoErr == ret)
        {
            mb_msg = (typeof(mb_msg))msg.data;
            evt = msg.evt;
            ret = 0;

            switch (evt)
            {
                case EVENT_AUD_VOC_WRITE_MULTIPLE_SPK_SOURCE_DATA_REQ:
                {
                    audio_write_multiple_spk_data_req_t *req = (typeof(req))mb_msg->param;
                    uint32_t write_len = 0;
                    if (req->channel_num == 2)
                    {
                        /* change dual channel to mono channel */
                        int16_t *temp_buffer = psram_malloc(req->data_len/2);
                        if (!temp_buffer)
                        {
                            LOGE("%s, %d, malloc temp_buffer: %d fail\n", __func__, __LINE__, req->data_len/2);
                            write_len = aud_tras_drv_write_prompt_tone_data((char *)req->data, req->data_len/2, req->wait_time);//BEKEN_WAIT_FOREVER
                            break;
                        }

                        int16_t *src_buffer = (int16_t *)req->data;
                        for (uint32_t i = 0; i < req->data_len/4; i++)
                        {
                            temp_buffer[i] = src_buffer[2 * i];
                        }
                        write_len = aud_tras_drv_write_prompt_tone_data((char *)temp_buffer, req->data_len/2, req->wait_time);//BEKEN_WAIT_FOREVER
                        os_free(temp_buffer);
                    }
                    else
                    {
                        write_len = aud_tras_drv_write_prompt_tone_data((char *)req->data, req->data_len, req->wait_time);//BEKEN_WAIT_FOREVER
                    }

                    mb_msg->param = write_len;
                }
                    break;

                case EVENT_AUD_VOC_SET_SPK_SOURCE_REQ:
                    ret = aud_tras_drv_voc_set_spk_source_type((aud_spk_source_info_t *)mb_msg->param);
                    break;

                case EVENT_AUD_VOC_GET_SPK_SOURCE_REQ:
                    mb_msg->param = aud_tras_drv_get_spk_source_type();
                    ret = 0;
                    break;

                case EVENT_AUDIO_ACT_DEINIT_REQ:
                    goto exit;
                    break;

                default:
                    LOGE("unknow event 0x%x", evt);
                    ret = -1;
                    break;
            }
        }

        msg_send_rsp_to_media_major_mailbox(mb_msg, ret, APP_MODULE);
    }


exit:
    LOGI("exit");

    rtos_deinit_queue(&s_audio_msg_que);
    s_audio_msg_que = NULL;

    rtos_set_semaphore(&s_audio_sema);

    s_audio_task = NULL;
    rtos_delete_thread(NULL);
}

static bk_err_t audio_act_init_handle(media_mailbox_msg_t *msg)
{
    int ret = 0;

    LOGI("%s\n", __func__);

    if (s_audio_task)
    {
        LOGE("audio_task already init");
        goto fail;
    }

    if (!s_audio_sema)
    {
        ret = rtos_init_semaphore(&s_audio_sema, 1);
        if (ret)
        {
            LOGE("sema init failed");
            ret = -1;
            goto fail;
        }
    }

    ret = rtos_init_queue(&s_audio_msg_que,
                          "s_bt_audio_msg_que",
                          sizeof(audio_msg_t),
                          30);

    if (ret != kNoErr)
    {
        LOGE("bt_audio sink demo msg queue failed");
        ret = -1;
        goto fail;
    }

    ret = rtos_create_thread(&s_audio_task,
                             4,
                             "audio_act_task",
                             (beken_thread_function_t)audio_act_task,
                             1024 * 5,
                             (beken_thread_arg_t)NULL);
    if (ret)
    {
        LOGE("task init failed");
        ret = -1;
        goto fail;
    }


    ret = rtos_get_semaphore(&s_audio_sema, BEKEN_WAIT_FOREVER);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, rtos_get_semaphore\n", __func__, __LINE__);
        goto fail;
    }

    LOGI("%s, %d, create audio_act task complete\n", __func__, __LINE__);

    return BK_OK;

fail:

    LOGI("init stat %d", ret);

    if (s_audio_sema)
    {
        rtos_deinit_semaphore(&s_audio_sema);
        s_audio_sema = NULL;
    }

    if (s_audio_msg_que)
    {
        rtos_deinit_queue(&s_audio_msg_que);
        s_audio_msg_que = NULL;
    }

    return ret;
}


static bk_err_t audio_act_deinit_handle(media_mailbox_msg_t *msg)
{
    LOGI("%s\n", __func__);

    if (!s_audio_task)
    {
        LOGE("already deinit");
        return BK_OK;
    }

    audio_msg_t internal_msg = {0};
    internal_msg.data = NULL;
    internal_msg.evt = EVENT_AUDIO_ACT_DEINIT_REQ;
    rtos_push_to_queue(&s_audio_msg_que, &internal_msg, BEKEN_WAIT_FOREVER);

    rtos_get_semaphore(&s_audio_sema, BEKEN_WAIT_FOREVER);

    rtos_deinit_semaphore(&s_audio_sema);
    s_audio_sema = NULL;

    return BK_OK;
}

bk_err_t audio_act_event_handle(media_mailbox_msg_t *msg)
{
    bk_err_t ret = 0;

    switch (msg->event)
    {
        case EVENT_AUDIO_ACT_INIT_REQ:
            //sync op
            ret = audio_act_init_handle(msg);
            msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);
            break;

        case EVENT_AUDIO_ACT_DEINIT_REQ:
            //sync op
            ret = audio_act_deinit_handle(msg);
            msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);
            break;

        default:

            //async op
            if (!s_audio_task || !s_audio_sema)
            {
                LOGE("task not run");
                msg_send_rsp_to_media_major_mailbox(msg, -1, APP_MODULE);
                break;
            }

            if (EVENT_AUD_VOC_WRITE_MULTIPLE_SPK_SOURCE_DATA_REQ != msg->event)
            {
                LOGI("evt %d\n", msg->event);
            }

            audio_msg_t internal_msg = {0};

            internal_msg.data = (typeof(internal_msg.data))msg;
            internal_msg.evt = msg->event;

            ret = rtos_push_to_queue(&s_audio_msg_que, &internal_msg, BEKEN_WAIT_FOREVER);
            if (ret)
            {
                LOGE("send queue failed");
            }
            break;
    }

    return ret;
}


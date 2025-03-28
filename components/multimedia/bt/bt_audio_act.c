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
#include <components/log.h>

#include "media_core.h"
#include "media_evt.h"
#include "media_app.h"
#include "bt_audio_act.h"
#include "aud_tras_drv.h"

#include <driver/media_types.h>
#include <os/mem.h>
#include <modules/audio_rsp_types.h>
#include <modules/audio_rsp.h>
#include <modules/sbc_encoder.h>

#define TAG "bt_audio"

enum
{
    BT_AUDIO_DEBUG_LEVEL_ERROR,
    BT_AUDIO_DEBUG_LEVEL_WARNING,
    BT_AUDIO_DEBUG_LEVEL_INFO,
    BT_AUDIO_DEBUG_LEVEL_DEBUG,
    BT_AUDIO_DEBUG_LEVEL_VERBOSE,
};

#define BT_AUDIO_DEBUG_LEVEL BT_AUDIO_DEBUG_LEVEL_INFO

#define LOGE(format, ...) do{if(BT_AUDIO_DEBUG_LEVEL >= BT_AUDIO_DEBUG_LEVEL_ERROR)   BK_LOGE(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGW(format, ...) do{if(BT_AUDIO_DEBUG_LEVEL >= BT_AUDIO_DEBUG_LEVEL_WARNING) BK_LOGW(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGI(format, ...) do{if(BT_AUDIO_DEBUG_LEVEL >= BT_AUDIO_DEBUG_LEVEL_INFO)    BK_LOGI(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGD(format, ...) do{if(BT_AUDIO_DEBUG_LEVEL >= BT_AUDIO_DEBUG_LEVEL_DEBUG)   BK_LOGD(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGV(format, ...) do{if(BT_AUDIO_DEBUG_LEVEL >= BT_AUDIO_DEBUG_LEVEL_VERBOSE) BK_LOGV(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)

#define USE_QUEUE 1

typedef void (*camera_connect_state_t)(uint8_t state);

enum
{
    BT_AUDIO_ACTION_NONE,
    BT_AUDIO_ACTION_RESAMPLE,
    BT_AUDIO_ACTION_ENCODE,
} BT_AUDIO_ACTION;


enum
{
    TONE_STATUS_IDLE,
    TONE_STATUS_WAIT_ENABLE,
    TONE_STATUS_ENABLE,
};

typedef struct
{
    uint32_t ret;
    uint32_t in_len;
    uint32_t out_len;
} bt_audio_resample_result_t;

typedef struct
{
    uint32_t evt;
    void *data;
    uint16_t len;
} bt_audio_msg_t;

static uint8_t s_is_rsp_inited;


static beken_thread_t s_bt_audio_task = NULL;
static beken_semaphore_t s_bt_audio_sema = NULL;

#if USE_QUEUE
#else
    static volatile uint32_t s_bt_audio_action;
    static media_mailbox_msg_t *s_bt_audio_action_mailbox;
#endif

static uint8_t s_bt_audio_task_run;
static aud_rsp_cfg_t s_rsp_cfg_final;
static beken_queue_t s_bt_audio_msg_que = NULL;

static void bt_audio_task(void *arg)
{
    int32_t ret = 0;
    uint32_t write_count = 0;
    uint8_t tone_status = TONE_STATUS_IDLE;
    uint32_t evt = 0;
    bt_audio_msg_t msg;
    media_mailbox_msg_t *mb_msg = NULL;

    while (s_bt_audio_task_run)
    {
#if USE_QUEUE
        ret = rtos_pop_from_queue(&s_bt_audio_msg_que, &msg, 300);//BEKEN_WAIT_FOREVER);

        if (ret)
        {
            LOGV("pop queue err");
            continue;
        }

        mb_msg = (typeof(mb_msg))msg.data;
        evt = msg.evt;
#else
        rtos_get_semaphore(&s_bt_audio_sema, 300);//BEKEN_WAIT_FOREVER);
        evt = s_bt_audio_action;
        mb_msg = s_bt_audio_action_mailbox;
#endif
        ret = 0;


        switch (evt)
        {
        case 0:
            continue;
            break;

        case EVENT_BT_A2DP_STATUS_NOTI_REQ:
        {
            uint32_t a2dp_status_notif_status = (typeof(a2dp_status_notif_status))mb_msg->param;
            uint8_t final = (a2dp_status_notif_status ? 1 : 0);

            LOGW("EVENT_BT_A2DP_STATUS_NOTI_REQ %d status %d", final, tone_status);

            switch (tone_status)
            {
            case TONE_STATUS_IDLE:
                if (final)
                {
                    write_count = 0;
                    tone_status = TONE_STATUS_WAIT_ENABLE;
                }

                break;

            case TONE_STATUS_WAIT_ENABLE:
            case TONE_STATUS_ENABLE:
                if (!final)
                {
                    write_count = 0;
                    ret = aud_tras_drv_voc_set_spk_source_type(SPK_SOURCE_TYPE_VOICE);
                    tone_status = TONE_STATUS_IDLE;
                }

                break;
            }

            if (ret)
            {
                LOGE("aud_tras_drv_control_prompt_tone_play to %d err %d !!!", final, ret);
            }
        }
        break;

        case EVENT_BT_PCM_WRITE_REQ:
        {
            bt_audio_write_req_t *req = (typeof(req))mb_msg->param;

            switch (tone_status)
            {
            case TONE_STATUS_WAIT_ENABLE:
            case TONE_STATUS_ENABLE:
            {
                uint32_t w_len = 0;
                uint32_t err_count = 0;

                while (w_len < req->data_len && err_count <= 3)
                {
                    LOGV("start tone_data write len %d", req->data_len);
                    ret = aud_tras_drv_write_prompt_tone_data((char *)req->data + w_len, req->data_len - w_len, 200 / 2);//BEKEN_WAIT_FOREVER);
                    LOGV("end tone_data write ret %d", ret);

                    if (ret <= 0)
                    {
                        err_count++;
                        LOGE("aud_tras_drv_write_prompt_tone_data fail, ret: %d", ret);
                    }

                    if (ret >= 0)
                    {
                        w_len += ret;
                    }

                    ret = 0;

                    write_count++;
                }

                if (err_count > 3)
                {
                    LOGW("try write err_count reach limit %d !!", err_count);
                }

                if (write_count >= 2 && tone_status == TONE_STATUS_WAIT_ENABLE)
                {
                    LOGI("try aud_tras_drv_control_prompt_tone_play");
                    ret = aud_tras_drv_voc_set_spk_source_type(SPK_SOURCE_TYPE_A2DP);
                    if (ret)
                    {
                        LOGE("aud_tras_drv_control_prompt_tone_play enable err %d !!!", ret);
                    }

                    tone_status = TONE_STATUS_ENABLE;
                }
            }
            break;

            default:
                LOGV("tone status not match %d, len %d", tone_status, req->data_len);
                break;
            }
        }
        break;

        case EVENT_BT_PCM_RESAMPLE_INIT_REQ:
            do
            {
                if (s_is_rsp_inited)
                {
                    LOGE("resample already init");
                    break;
                }

                extern bk_err_t bk_audio_osi_funcs_init(void);
                bk_audio_osi_funcs_init();

                bt_audio_resample_init_req_t *cfg = (typeof(cfg))mb_msg->param;

                os_memset(&s_rsp_cfg_final, 0, sizeof(s_rsp_cfg_final));

                s_rsp_cfg_final.src_rate = cfg->src_rate;
                s_rsp_cfg_final.src_ch = cfg->src_ch;
                s_rsp_cfg_final.src_bits = cfg->src_bits;
                s_rsp_cfg_final.dest_rate = cfg->dest_rate;
                s_rsp_cfg_final.dest_ch = cfg->dest_ch;
                s_rsp_cfg_final.dest_bits = cfg->dest_bits;
                s_rsp_cfg_final.complexity = cfg->complexity;
                s_rsp_cfg_final.down_ch_idx = cfg->down_ch_idx;

                LOGI("resample init %p %d %d %d %d %d %d %d %d", cfg,
                     s_rsp_cfg_final.src_rate,
                     s_rsp_cfg_final.src_ch,
                     s_rsp_cfg_final.src_bits,
                     s_rsp_cfg_final.dest_rate,
                     s_rsp_cfg_final.dest_ch,
                     s_rsp_cfg_final.dest_bits,
                     s_rsp_cfg_final.complexity,
                     s_rsp_cfg_final.down_ch_idx);

                ret = bk_aud_rsp_init(s_rsp_cfg_final);

                if (ret)
                {
                    LOGE("bk_aud_rsp_init err %d !!", ret);
                    ret = -1;
                    break;
                }

                s_is_rsp_inited = 1;

            }
            while (0);

            if (ret)
            {
                if (s_is_rsp_inited)
                {
                    bk_aud_rsp_deinit();
                    s_is_rsp_inited = 0;
                }
            }

            break;

        case EVENT_BT_PCM_RESAMPLE_DEINIT_REQ:
            do
            {
                if (!s_is_rsp_inited)
                {
                    LOGE("resample already deinit");
                    break;
                }

                ret = bk_aud_rsp_deinit();

                if (ret)
                {
                    LOGE("bk_aud_rsp_deinit err %d !!", ret);
                    ret = -1;
                    break;
                }

                s_is_rsp_inited = 0;

            }
            while (0);

            if (ret)
            {
                if (s_is_rsp_inited)
                {
                    bk_aud_rsp_deinit();
                    s_is_rsp_inited = 0;
                }
            }

            break;

        case EVENT_BT_PCM_RESAMPLE_REQ:
        {
            bt_audio_resample_req_t *param = (typeof(param))(mb_msg->param);

            uint32_t in_len = *(param->in_bytes_ptr) / (s_rsp_cfg_final.src_bits / 8);
            uint32_t out_len = *(param->out_bytes_ptr) / (s_rsp_cfg_final.dest_bits / 8);

            LOGD("resample start %p %p %p %d %d", param, param->in_addr, param->out_addr, in_len, out_len);

            ret = bk_aud_rsp_process((int16_t *)param->in_addr, &in_len, (int16_t *)param->out_addr, &out_len);

            if (ret)
            {
                LOGE("bk_aud_rsp_process err %d !!", ret);
            }
            else
            {
                *(param->in_bytes_ptr) = in_len * (s_rsp_cfg_final.src_bits / 8);
                *(param->out_bytes_ptr) = out_len * (s_rsp_cfg_final.dest_bits / 8);
            }

            LOGD("resample done %d %d", in_len, out_len);
        }
        break;

        case EVENT_BT_PCM_ENCODE_INIT_REQ:
            ret = 0;
            break;

        case EVENT_BT_PCM_ENCODE_DEINIT_REQ:
            ret = 0;
            break;

        case EVENT_BT_PCM_ENCODE_REQ:
        {
            bt_audio_encode_req_t *param = (typeof(param))(mb_msg->param);

            if (!param || !param->handle || !param->in_addr || !param->out_len_ptr)
            {
                LOGE("encode req param err");
                ret = -1;
                break;
            }

            int32_t encode_len = 0;

            if (param->type != 0)
            {
                LOGE("type not match %d", param->type);
                ret = -1;
                break;
            }

            encode_len = sbc_encoder_encode((SbcEncoderContext *)param->handle, (const int16_t *)param->in_addr);

            if (!encode_len)
            {
                LOGE("encode err %d", encode_len);
                ret = -1;
                break;
            }

            *param->out_len_ptr = encode_len;
        }
        break;

        default:
            LOGE("unknow event 0x%x", evt);
            ret = -1;
            break;
        }

#if USE_QUEUE
#else
        s_bt_audio_action = 0;
#endif

        msg_send_rsp_to_media_major_mailbox(mb_msg, ret, APP_MODULE);
    }

    LOGI("exit");

    rtos_delete_thread(NULL);
}

static bk_err_t bt_audio_init_handle(media_mailbox_msg_t *msg)
{
    int ret = 0;

    LOGI("");

    if (s_bt_audio_task)
    {
        LOGE("already init");
        goto end;
    }

    if (!s_bt_audio_sema)
    {
        ret = rtos_init_semaphore(&s_bt_audio_sema, 1);
    }

    if (ret)
    {
        LOGE("sema init failed");
        ret = -1;
        goto end;
    }

    ret = rtos_init_queue(&s_bt_audio_msg_que,
                          "s_bt_audio_msg_que",
                          sizeof(bt_audio_msg_t),
                          60);

    if (ret != kNoErr)
    {
        LOGE("bt_audio sink demo msg queue failed");
        ret = -1;
        goto end;
    }

    s_bt_audio_task_run = 1;
    ret = rtos_create_thread(&s_bt_audio_task,
                             4,
                             "bt_audio_task",
                             (beken_thread_function_t)bt_audio_task,
                             1024 * 5,
                             (beken_thread_arg_t)NULL);

    if (ret)
    {
        LOGE("task init failed");
        ret = -1;
        goto end;
    }

end:

    LOGI("init stat %d", ret);

    if (ret)
    {
        if (s_bt_audio_task)
        {
            s_bt_audio_task_run = 0;
            rtos_thread_join(s_bt_audio_task);
            s_bt_audio_task = NULL;
        }

        if (s_bt_audio_sema)
        {
            rtos_deinit_semaphore(&s_bt_audio_sema);
            s_bt_audio_sema = NULL;
        }

        if (s_bt_audio_msg_que)
        {
            rtos_deinit_queue(&s_bt_audio_msg_que);
            s_bt_audio_msg_que = NULL;
        }
    }

    msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);

    return ret;
}


static bk_err_t bt_audio_deinit_handle(media_mailbox_msg_t *msg)
{
    int ret = 0;

    LOGI("");

    if (!s_bt_audio_task)
    {
        LOGE("already deinit");
        goto end;
    }

    s_bt_audio_task_run = 0;
    rtos_thread_join(s_bt_audio_task);
    s_bt_audio_task = NULL;

    if (s_bt_audio_sema)
    {
        rtos_deinit_semaphore(&s_bt_audio_sema);
        s_bt_audio_sema = NULL;
    }

    if (s_bt_audio_msg_que)
    {
        bt_audio_msg_t msg;

        while ( 0 == (ret = rtos_pop_from_queue(&s_bt_audio_msg_que, &msg, 0)))
        {
            if (ret)
            {
                break;
            }

            LOGI("free s_bt_audio_msg_que node");
            msg_send_rsp_to_media_major_mailbox((media_mailbox_msg_t *)msg.data, 0, APP_MODULE);
        }

        ret = 0;
        rtos_deinit_queue(&s_bt_audio_msg_que);
        s_bt_audio_msg_que = NULL;
    }

end:

    LOGI("deinit stat %d", ret);

    if (ret)
    {
        if (s_bt_audio_task)
        {
            s_bt_audio_task_run = 0;
            rtos_thread_join(s_bt_audio_task);
            s_bt_audio_task = NULL;
        }

        if (s_bt_audio_sema)
        {
            rtos_deinit_semaphore(&s_bt_audio_sema);
            s_bt_audio_sema = NULL;
        }
    }

    msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);

    return ret;
}

bk_err_t bt_audio_event_handle(media_mailbox_msg_t *msg)
{
    bk_err_t ret = 0;

    switch (msg->event)
    {
    case EVENT_BT_AUDIO_INIT_REQ:
        //sync op
        ret = bt_audio_init_handle(msg);
        break;

    case EVENT_BT_AUDIO_DEINIT_REQ:
        //sync op
        ret = bt_audio_deinit_handle(msg);
        break;

    default:

        //async op
        if (!s_bt_audio_task || !s_bt_audio_sema)
        {
            LOGE("task not run");
            msg_send_rsp_to_media_major_mailbox(msg, -1, APP_MODULE);
            break;
        }

        if (EVENT_BT_PCM_RESAMPLE_REQ != msg->event && EVENT_BT_PCM_ENCODE_REQ != msg->event && EVENT_BT_PCM_WRITE_REQ != msg->event)
        {
            LOGI("evt %d", msg->event);
        }

#if USE_QUEUE
        bt_audio_msg_t internal_msg = {0};

        internal_msg.data = (typeof(internal_msg.data))msg;
        internal_msg.evt = msg->event;

        ret = rtos_push_to_queue(&s_bt_audio_msg_que, &internal_msg, BEKEN_WAIT_FOREVER);

        if (ret)
        {
            LOGE("send queue failed");
        }

#else
        s_bt_audio_action_mailbox = msg;
        s_bt_audio_action = msg->event;

        if (s_bt_audio_sema)
        {
            rtos_set_semaphore(&s_bt_audio_sema);
        }

#endif
        break;
    }

    return ret;
}


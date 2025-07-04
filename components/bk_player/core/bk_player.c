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


#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <unistd.h>
#include "bk_player.h"
#include "bk_player_err.h"
#include "player_osal.h"
#include "source.h"
#include "codec.h"
#include "sink.h"


#define PLAYER_CHECK_NULL(ptr, act) do {\
        if (ptr == NULL) {\
            player_log(LOG_ERR, "PLAYER_CHECK_NULL fail \n");\
            {act;}\
        }\
    } while(0)

enum PLAYER_RESULT
{
    PLAYER_CONTINUE = 0,
    PLAYER_BREAKOUT,
    PLAYER_DO_PLAY,
    PLAYER_DO_SEEK,
    PLAYER_DO_EXIT,
};

typedef enum
{
    PLAYER_NULL = 0,
    PLAYER_PLAY,
    PLAYER_STOP,
    PLAYER_PAUSE,
    PLAYER_SEEK,
    PLAYER_EXIT,
} player_event_t;

typedef struct 
{
    beken_semaphore_t ack_sem;
    int result;
} player_msg_param_t;

typedef struct
{
    player_event_t event;
    void *param;
} player_msg_t;

struct player
{
    audio_source_t *source;
    audio_codec_t *codec;
    audio_sink_t *sink;

    char *buffer;
    int chunk_size;

    audio_info_t info;
    char *uri;
    bk_player_state_t state;

    beken_thread_t task_hdl;
    beken_queue_t queue;
    beken_semaphore_t sem;

    int consumed_bytes;
    int bytes_per_second;

    player_event_handle_cb event_handle;
    void *args;
};


static int player_event_handle(bk_player_handle_t player, int timeout_value);

static bk_err_t player_send_msg(beken_queue_t queue, player_event_t event, void *param)
{
    bk_err_t ret;
    player_msg_t msg;

    if (!queue)
    {
        player_log(LOG_ERR, "%s, %d, queue: %p \n", __func__, __LINE__, queue);
        return BK_FAIL;
    }

    msg.event = event;
    msg.param = param;
    ret = rtos_push_to_queue(&queue, &msg, BEKEN_NO_WAIT);
    if (ret != BK_OK)
    {
        player_log(LOG_ERR, "%s, %d, send message: %d fail, ret: %d\n", __func__, __LINE__, event, ret);
        return BK_FAIL;
    }

    return BK_OK;
}

static const char *player_state_strs[] =
{
    "STOPED",
    "PLAYING",
    "PAUSED",
};

static const char *get_state_str(int state)
{
    if (state <= PLAYER_STAT_PAUSED && state >= PLAYER_STAT_STOPPED)
    {
        return player_state_strs[state];
    }
    else
    {
        return "INVALID";
    }
}

static int player_pipeline_setup(bk_player_handle_t player)
{
    //int ret;
    audio_source_t *source = NULL;
    audio_codec_t *codec = NULL;
    audio_sink_t *sink = NULL;
    char *buffer = NULL;
    int codec_type;
    int chunk_size;
    audio_info_t info;
    bk_player_err_t err = PLAYER_OK;

    player_log(LOG_INFO, "pipeline setup\n");

    if (!player->uri)
    {
        err = PLAYER_ERR_OPERATION;
        goto out;
    }

    source = audio_source_open_url(player->uri);
    if (!source)
    {
        err = PLAYER_ERR_UNKNOWN;
        goto out;
    }

    codec_type = audio_source_get_codec_type(source);
    player_log(LOG_INFO, "codec_type :%d\n", codec_type);

    if (codec_type == AUDIO_CODEC_UNKNOWN)
    {
        err = PLAYER_ERR_INVALID_PARAM;
        goto out;
    }

    codec = audio_codec_open(codec_type, NULL, source);
    if (!codec)
    {
        err = PLAYER_ERR_UNKNOWN;
        goto out;
    }

    err = audio_codec_get_info(codec, &info);
    if (err)
    {
        player_log(LOG_ERR, "%s, get codec info fail, err=%d, %d \n", __func__, err, __LINE__);
        goto out;
    }
    player_log(LOG_INFO, "channel_number=%d, sample_rate=%d, sample_bits=%d, total_bytes=%d\n",
               info.channel_number, info.sample_rate, info.sample_bits, info.total_bytes);

    chunk_size = audio_codec_get_chunk_size(codec);
    player_log(LOG_INFO, "chunk_size is %d\n", chunk_size);
    //info.frame_size = chunk_size;

    buffer = player_malloc(chunk_size);
    if (!buffer)
    {
        err = PLAYER_ERR_NO_MEM;
        goto out;
    }

    //sink = audio_sink_open(AUDIO_SINK_ONBOARD_SPEAKER, &info);
    sink = audio_sink_open(AUDIO_SINK_VOICE_SPEAKER, &info);
    if (!sink)
    {
        err = PLAYER_ERR_UNKNOWN;
        goto out;
    }

    player->source = source;
    player->codec = codec;
    player->sink = sink;

    player->buffer = buffer;
    player->chunk_size = chunk_size;

    player->info = info;

    player->consumed_bytes = 0;
    player->bytes_per_second = info.channel_number * info.sample_rate * info.sample_bits / 8;
    if (player->bytes_per_second == 0)
    {
        player->bytes_per_second = 96000 * 2 * 2;
    }

#if 0
    player->spk_gain_internal = 0;
    //priv->gain_step = MAX_GAIN * 1000 * chunk_size / (priv->bytes_per_second * 200);  //MAX_GAIN / (bytes_per_second * 200 / 1000 / chunk_size)
    priv->gain_step = MAX_GAIN * 1000 * chunk_size / (priv->bytes_per_second * 200);    //MAX_GAIN / (bytes_per_second * 200 / 1000 / chunk_size)
    if (priv->gain_step == 0)
    {
        priv->gain_step = 1;
    }
    player_log(LOG_INFO, "gain_step is %d\n", priv->gain_step);
#endif

    if (player->event_handle)
    {
        player->event_handle(PLAYER_EVENT_START, NULL, player->args);
    }

    return PLAYER_OK;

out :
    player_log(LOG_ERR, "pipeline setup : failed with err=%d\n", err);

    if (codec)
    {
        audio_codec_close(codec);
        codec = NULL;
    }
    if (source)
    {
        audio_source_close(source);
        source = NULL;
    }
    if (sink)
    {
        audio_sink_close(sink);
        sink =NULL;
    }

    if (buffer)
    {
        player_free(buffer);
        buffer = NULL;
    }

    return err;
}

static void player_pipeline_teardown(bk_player_handle_t player)
{
    player_log(LOG_INFO, "pipeline teardown\n");

    /* mute audio dac before close audio to avoid "pop" voice */
    //audio_sink_control(player->sink, AUDIO_SINK_MUTE);

    /* stop sink onboard speaker */
    //audio_sink_control(player->sink, AUDIO_SINK_PAUSE);

    if (player->codec)
    {
        audio_codec_close(player->codec);
    }

    if (player->source)
    {
        audio_source_close(player->source);
    }

    if (player->sink)
    {
        audio_sink_close(player->sink);
    }

    if (player->buffer)
    {
        player_free(player->buffer);
    }

    player->source = NULL;
    player->codec = NULL;
    player->sink = NULL;

    player->buffer = NULL;

}

static int player_pipeline_pause(bk_player_handle_t player)
{
    player_log(LOG_INFO, "pipeline pause\n");

    audio_sink_control(player->sink, AUDIO_SINK_PAUSE);

    if (player->event_handle)
    {
        player->event_handle(PLAYER_EVENT_PAUSE, NULL, player->args);
    }

    return PLAYER_OK;
}

static int player_pipeline_resume(bk_player_handle_t player)
{
    player_log(LOG_INFO, "pipeline resume\n");

    audio_sink_control(player->sink, AUDIO_SINK_RESUME);

    if (player->event_handle)
    {
        player->event_handle(PLAYER_EVENT_RESUME, NULL, player->args);
    }

    return PLAYER_OK;
}

static void _play_sm_progress(bk_player_handle_t player)
{
    static int old_second = 0;
    int new_second;

    if (player->state != PLAYER_STAT_PLAYING && player->state != PLAYER_STAT_PAUSED)
    {
        return;
    }

    //player_log(LOG_INFO, "consumed_bytes: %d, bytes_per_second: %d\n", player->consumed_bytes, player->bytes_per_second);

    new_second = player->consumed_bytes / player->bytes_per_second;
    if (old_second != new_second)
    {
        old_second = new_second;
        if (player->event_handle)
        {
            player->event_handle(PLAYER_EVENT_TICK, NULL, player->args);
        }
    }
}

int player_pipeline_process(bk_player_handle_t player)
{
    int ret = 0;
    int len;
    int len2;
    int result = 0;
    int retry_cnt = CONFIG_PLAYER_READ_DATA_TIMEOUT_MS/100;

    //player_log(LOG_INFO, "%s\n", __func__);

__retry:
    result = player_event_handle(player, 0);
    if (result == PLAYER_BREAKOUT || result == PLAYER_EXIT)
    {
        player_log(LOG_ERR, "result: %d\n", result);
        return -1;
    }

    //player_log(LOG_INFO, "%s get data\n", __func__);

    len = audio_codec_get_data(player->codec, player->buffer, player->chunk_size);
    if (len < 0)
    {
        player_log(LOG_ERR, "%s, %d, codec return = %d\n", __func__, __LINE__, len);
        if (len == PLAYER_ERR_TIMEOUT)
        {
            if ((retry_cnt--) > 0)
            {
                player_log(LOG_WARN, "read data timeout. retry: %d\n", retry_cnt);
                goto __retry;
            }

            /* player read data timeout error, stop playing and report event */
            player_pipeline_teardown(player);
            player->state = PLAYER_STAT_STOPPED;
            if (player->event_handle)
            {
                player->event_handle(PLAYER_EVENT_READ_DATA_TIMEOUT, NULL, player->args);
            }
        }

        ret = -1;
    }
    else if (len == 0)
    {
        player_log(LOG_INFO, "%s, done\n", __func__);
        /* player complete */
        player_pipeline_teardown(player);
        player->state = PLAYER_STAT_STOPPED;
        if (player->event_handle)
        {
            player->event_handle(PLAYER_EVENT_FINISH, NULL, player->args);
        }
    }
    else
    {
        len2 = audio_sink_write_data(player->sink, player->buffer, len);
        if (len2 <= 0)
        {
            player_log(LOG_ERR, "%s, sink return = %d\n", __func__, len2);
            ret = -1;
        }
        else
        {
            player->consumed_bytes += len2;
            _play_sm_progress(player);
        }
    }

    return ret;
}

static int player_event_handle(bk_player_handle_t player, int timeout_value)
{
    bk_err_t ret = BK_OK;
    int result = PLAYER_CONTINUE;

    player_msg_t msg = {0};
    ret = rtos_pop_from_queue(&player->queue, &msg, timeout_value);
    if (ret == BK_OK)
    {
        int state_prev = player->state;

        /* handle player event */
        switch (msg.event)
        {
            case PLAYER_PLAY:
                switch (player->state)
                {
                    case PLAYER_STAT_STOPPED:
                        /* goto PLAYING state */
                        ret = player_pipeline_setup(player);
                        if (ret == PLAYER_OK)
                        {
                            player->state = PLAYER_STAT_PLAYING;
                            player_log(LOG_INFO, "player_state: %s --> %s\n", get_state_str(state_prev), get_state_str(player->state));
                            player_log(LOG_INFO, "player_play : play uri=%s\n", player->uri);
                            result = PLAYER_DO_PLAY;
                        }
                        break;

                    case PLAYER_STAT_PLAYING:
                        /* N/A */
                        ret = PLAYER_OK;
                        break;

                    case PLAYER_STAT_PAUSED:
                        ret = player_pipeline_resume(player);
                        if (ret == PLAYER_OK)
                        {
                            player->state = PLAYER_STAT_PLAYING;
                            player_log(LOG_INFO, "player_state: %s --> %s\n", get_state_str(state_prev), get_state_str(player->state));
                            player_log(LOG_INFO, "player_play : resume url=%s\n", player->uri);
                            result = PLAYER_DO_PLAY;
                        }
                        break;

                    default:
                        break;
                }
                break;

            case PLAYER_STOP:
                switch (player->state)
                {
                    case PLAYER_STAT_STOPPED:
                        /* N/A */
                        ret = PLAYER_OK;
                        break;

                    case PLAYER_STAT_PLAYING:
                    case PLAYER_STAT_PAUSED:
                        player_pipeline_teardown(player);
                        player->state = PLAYER_STAT_STOPPED;
                        if (player->event_handle)
                        {
                            player->event_handle(PLAYER_EVENT_STOP, NULL, player->args);
                        }
                        ret = PLAYER_OK;
                        player_log(LOG_INFO, "player_stop : stop url=%s\n", player->uri);
                        result = PLAYER_BREAKOUT;
                        break;

                    default:
                        break;
                }
                break;

            case PLAYER_PAUSE:
                switch (player->state)
                {
                    case PLAYER_STAT_STOPPED:
                        ret = BK_FAIL;
                        break;

                    case PLAYER_STAT_PLAYING:
                        ret = player_pipeline_pause(player);
                        if (ret == PLAYER_OK)
                        {
                            player->state = PLAYER_STAT_PAUSED;
                            player_log(LOG_INFO, "player_pause ok\n");
                            result = PLAYER_BREAKOUT;
                        }
                        break;

                    case PLAYER_STAT_PAUSED:
                        /* N/A */
                        ret = BK_OK;
                        break;

                    default:
                        break;
                }
                break;

            case PLAYER_EXIT:
                result = PLAYER_EXIT;
                break;

            default:
                break;
        }

        /* sync call */
        if (msg.param)
        {
            player_msg_param_t *param = (player_msg_param_t *)msg.param;
            param->result = ret;

            rtos_set_semaphore(&param->ack_sem);
        }

    }

    return result;
}



static void player_task_main(beken_thread_arg_t param_data)
{
    bk_err_t result = BK_OK;
    bk_err_t ret = 0;

    bk_player_handle_t player = (bk_player_handle_t)param_data;
    rtos_set_semaphore(&player->sem);

    while (1)
    {
        result = player_event_handle(player, BEKEN_WAIT_FOREVER);
        if (result != PLAYER_DO_PLAY && result != PLAYER_EXIT)
        {
            continue;
        }

        /* exit task */
        if (result == PLAYER_EXIT)
        {
            player_log(LOG_INFO, "%s, %d, breakout play\n", __func__, __LINE__);
            goto player_task_exit;
        }

        /* play music */
        while (1)
        {
            int ret = player_pipeline_process(player);
            if (ret == -1)
            {
                player_log(LOG_INFO, "%s, %d, breakout play\n", __func__, __LINE__);
                break;
            }
        }
    }

player_task_exit:
    /* delete msg queue */
    ret = rtos_deinit_queue(&player->queue);
    if (ret != BK_OK)
    {
        player_log(LOG_ERR, "%s, %d, delete message queue fail\n", __func__, __LINE__);
    }
    player->queue = NULL;

    /* delete task */
    player->task_hdl = NULL;

    rtos_set_semaphore(&player->sem);

    rtos_delete_thread(NULL);
}

bk_player_state_t bk_player_get_state(bk_player_handle_t player)
{
    PLAYER_CHECK_NULL(player, return PLAYER_ERR_INVALID_PARAM);

    return player->state;
}

bk_player_handle_t bk_player_create(void)
{
    bk_err_t ret = BK_OK;
    bk_player_handle_t player = NULL;

    player = player_malloc(sizeof(struct player));
    if (!player)
    {
        player_log(LOG_ERR, "malloc player handle fail\n");
        return NULL;
    }

    player->state = PLAYER_STAT_STOPPED;

    ret = rtos_init_semaphore(&player->sem, 1);
    if (ret != BK_OK)
    {
        player_log(LOG_ERR, "%s, %d, ceate semaphore fail\n", __func__, __LINE__);
        goto fail;
    }

    player_log(LOG_INFO, "%s, %d, player->sem: %p\n", __func__, __LINE__, player->sem);

    ret = rtos_init_queue(&player->queue,
                          "player_queue",
                          sizeof(player_msg_t),
                          5);
    if (ret != BK_OK)
    {
        player_log(LOG_ERR, "%s, %d, ceate player message queue fail\n", __func__, __LINE__);
        goto fail;
    }

    ret = rtos_create_thread(&player->task_hdl,
                             (BEKEN_DEFAULT_WORKER_PRIORITY - 1),
                             "player",
                             (beken_thread_function_t)player_task_main,
                             1024 * 8,
                             (beken_thread_arg_t)player);
    if (ret != BK_OK)
    {
        player_log(LOG_ERR, "%s, %d, create player task fail\n", __func__, __LINE__);
        goto fail;
    }

    rtos_get_semaphore(&player->sem, BEKEN_NEVER_TIMEOUT);

    player_log(LOG_INFO, "play_sm_init ok\n");

    return player;

fail:

    if (player->sem)
    {
        rtos_deinit_semaphore(&player->sem);
        player->sem = NULL;
    }

    if (player->queue)
    {
        rtos_deinit_queue(&player->queue);
        player->queue = NULL;
    }

    player_free(player);

    return NULL;
}

bk_player_err_t bk_player_destroy(bk_player_handle_t player)
{
    bk_err_t ret = BK_OK;
    player_log(LOG_INFO, "%s\n", __func__);

    PLAYER_CHECK_NULL(player, return PLAYER_ERR_INVALID_PARAM);

    /* run --> stop --> exit */
    bk_player_stop(player, 0);

    player_log(LOG_INFO, "%s stop\n", __func__);

    /* delete player task */
    ret = player_send_msg(player->queue, PLAYER_EXIT, NULL);
    if (ret != BK_OK)
    {
        player_log(LOG_ERR, "%s, %d, send message: PLAYER_EXIT fail\n", __func__, __LINE__);
        return PLAYER_ERR_UNKNOWN;
    }

    rtos_get_semaphore(&player->sem, BEKEN_NEVER_TIMEOUT);

    rtos_deinit_semaphore(&player->sem);
    player->sem = NULL;

    if (player->uri)
    {
        player_free(player->uri);
        player->uri = NULL;
    }

    player_free(player);

    player_log(LOG_INFO, "player destroy complete\n");

    return PLAYER_OK;
}

bk_player_err_t bk_player_register_event_handle(bk_player_handle_t player, player_event_handle_cb func, void *args)
{
    PLAYER_CHECK_NULL(player, return PLAYER_ERR_INVALID_PARAM);

    player->event_handle = func;
    player->args = args;

    return PLAYER_OK;
}

bk_player_err_t bk_player_play(bk_player_handle_t player, int param)
{
    int ret = BK_OK;
    PLAYER_CHECK_NULL(player, return PLAYER_ERR_INVALID_PARAM);

    bk_player_err_t ret_value = PLAYER_OK;
    player_msg_param_t msg_param = {0};
    void *msg_param_ptr = NULL;

    if (param == PLAYER_CTRL_SYNC)
    {
        ret = rtos_init_semaphore(&msg_param.ack_sem, 1);
        if (ret != BK_OK)
        {
            player_log(LOG_ERR, "%s, %d, ceate ack semaphore fail\n", __func__, __LINE__);
            return PLAYER_ERR_NO_MEM;
        }
        msg_param_ptr = &msg_param;
    }

    ret = player_send_msg(player->queue, PLAYER_PLAY, msg_param_ptr);
    if (ret != BK_OK)
    {
        player_log(LOG_ERR, "%s, %d, send play event fail\n", __func__, __LINE__);
        ret_value = PLAYER_ERR_UNKNOWN;
        goto exit;
    }

    if (param == PLAYER_CTRL_SYNC)
    {
        ret = rtos_get_semaphore(&msg_param.ack_sem, BEKEN_NEVER_TIMEOUT);
        if (ret == BK_OK)
        {
            ret_value = msg_param.result;
        }
        else if (ret == kTimeoutErr)
        {
            ret_value = PLAYER_ERR_TIMEOUT;
        }
        else
        {
            ret_value = PLAYER_ERR_UNKNOWN;
        }
    }

exit:
    if (msg_param.ack_sem)
    {
        rtos_deinit_semaphore(&msg_param.ack_sem);
        msg_param.ack_sem = NULL;
    }

    return ret_value;
}

bk_player_err_t bk_player_stop(bk_player_handle_t player, int param)
{
    int ret = BK_OK;
    PLAYER_CHECK_NULL(player, return PLAYER_ERR_INVALID_PARAM);

    bk_player_err_t ret_value = PLAYER_OK;
    player_msg_param_t msg_param = {0};
    void *msg_param_ptr = NULL;

    if (param == PLAYER_CTRL_SYNC)
    {
        ret = rtos_init_semaphore(&msg_param.ack_sem, 1);
        if (ret != BK_OK)
        {
            player_log(LOG_ERR, "%s, %d, ceate ack semaphore fail\n", __func__, __LINE__);
            return PLAYER_ERR_NO_MEM;
        }
        msg_param_ptr = &msg_param;
    }

    ret = player_send_msg(player->queue, PLAYER_STOP, msg_param_ptr);
    if (ret != BK_OK)
    {
        player_log(LOG_ERR, "%s, %d, send play event fail\n", __func__, __LINE__);
        ret_value = PLAYER_ERR_UNKNOWN;
        goto exit;
    }

    if (param == PLAYER_CTRL_SYNC)
    {
        ret = rtos_get_semaphore(&msg_param.ack_sem, BEKEN_NEVER_TIMEOUT);
        if (ret == BK_OK)
        {
            ret_value = msg_param.result;
        }
        else if (ret == kTimeoutErr)
        {
            ret_value = PLAYER_ERR_TIMEOUT;
        }
        else
        {
            ret_value = PLAYER_ERR_UNKNOWN;
        }
    }

exit:
    if (msg_param.ack_sem)
    {
        rtos_deinit_semaphore(&msg_param.ack_sem);
        msg_param.ack_sem = NULL;
    }

    return ret_value;
}

bk_player_err_t bk_player_pause(bk_player_handle_t player, int param)
{
    int ret = BK_OK;
    PLAYER_CHECK_NULL(player, return PLAYER_ERR_INVALID_PARAM);

    bk_player_err_t ret_value = PLAYER_OK;
    player_msg_param_t msg_param = {0};
    void *msg_param_ptr = NULL;

    if (param == PLAYER_CTRL_SYNC)
    {
        ret = rtos_init_semaphore(&msg_param.ack_sem, 1);
        if (ret != BK_OK)
        {
            player_log(LOG_ERR, "%s, %d, ceate ack semaphore fail\n", __func__, __LINE__);
            return PLAYER_ERR_NO_MEM;
        }
        msg_param_ptr = &msg_param;
    }

    ret = player_send_msg(player->queue, PLAYER_PAUSE, msg_param_ptr);
    if (ret != BK_OK)
    {
        player_log(LOG_ERR, "%s, %d, send play event fail\n", __func__, __LINE__);
        ret_value = PLAYER_ERR_UNKNOWN;
        goto exit;
    }

    if (param == PLAYER_CTRL_SYNC)
    {
        ret = rtos_get_semaphore(&msg_param.ack_sem, BEKEN_NEVER_TIMEOUT);
        if (ret == BK_OK)
        {
            ret_value = msg_param.result;
        }
        else if (ret == kTimeoutErr)
        {
            ret_value = PLAYER_ERR_TIMEOUT;
        }
        else
        {
            ret_value = PLAYER_ERR_UNKNOWN;
        }
    }

exit:
    if (msg_param.ack_sem)
    {
        rtos_deinit_semaphore(&msg_param.ack_sem);
        msg_param.ack_sem = NULL;
    }

    return ret_value;
}

bk_player_err_t bk_player_set_uri(bk_player_handle_t player, const char *uri)
{
    PLAYER_CHECK_NULL(player, return PLAYER_ERR_INVALID_PARAM);

    char *play_uri = NULL;
    char *old_uri = NULL;
    int result = PLAYER_OK;

    //STATE_LOCK_INIT;

    if (uri)
    {
        play_uri = player_strdup(uri);
        if (!play_uri)
        {
            result = PLAYER_ERR_NO_MEM;
        }
        //STATE_LOCK;

        old_uri = player->uri;
        player->uri = play_uri;

        //STATE_UNLOCK;

        if (old_uri)
        {
            player_free(old_uri);
        }
    }
    else
    {
        result = PLAYER_ERR_INVALID_PARAM;
    }

    return result;
}

const char *bk_player_get_uri(bk_player_handle_t player)
{
    PLAYER_CHECK_NULL(player, return NULL);

    return player->uri;
}

double bk_player_get_total_time(bk_player_handle_t player, int timeout)
{
    PLAYER_CHECK_NULL(player, return PLAYER_ERR_INVALID_PARAM);

    int file_size = 0;
    //int header_size = 0;

    if (player->state != PLAYER_STAT_PLAYING && player->state != PLAYER_STAT_PAUSED)
    {
        return PLAYER_ERR_OPERATION;
    }

    /* check whether duration was been caculated */
    if (player->codec->info.duration == 0)
    {
        file_size = audio_source_get_total_bytes(player->source);
        if (file_size == 0)
        {
            /* the url is not song in sdcard */
            player_log(LOG_INFO, "the url is not song in sdcard \n");
            return PLAYER_ERR_UNKNOWN;
        }

        /* CBR: total_duration = (total_bytes - header_bytes) / bitrate * 8000 ms */
        player_log(LOG_INFO, "file_size: %d, header_bytes: %d, bps: %d \n", file_size, player->codec->info.header_bytes, player->codec->info.bps);
        player->codec->info.duration = (double)(file_size - player->codec->info.header_bytes) / (double)player->codec->info.bps * 8000.0;
    }
    player_log(LOG_INFO, "the play url total time: %f ms\n", player->codec->info.duration);

    return player->codec->info.duration;
}


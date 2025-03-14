// Copyright 2023-2024 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//	   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stdio.h"
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <modules/pm.h>
#include "prompt_tone_play.h"
#include "aud_tras_drv.h"


#define PROMPT_TONE_PLAY_TAG  "rt_play"
#define LOGI(...) BK_LOGI(PROMPT_TONE_PLAY_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(PROMPT_TONE_PLAY_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(PROMPT_TONE_PLAY_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(PROMPT_TONE_PLAY_TAG, ##__VA_ARGS__)


struct prompt_tone_play
{
    audio_source_t *source;
    audio_codec_t *codec;
    prompt_tone_play_cfg_t config;
    beken_semaphore_t play_finish_sem;
};


static int source_out_data_handle_cb(char *buffer, uint32_t len, void *params)
{
    prompt_tone_play_handle_t handle = (prompt_tone_play_handle_t)params;

    uint32_t w_len = 0;
    int ret = 0;
    while (handle && handle->codec && w_len < len)
    {
        ret = audio_codec_write_data(handle->codec, buffer + w_len, len - w_len);
        if (ret <= 0)
        {
            LOGE("%s, %d, audio_codec_write_data fail, ret: %d\n", __func__, __LINE__, ret);
            break;
        }
        LOGD("%s, %d, ret: %d\n", __func__, __LINE__, ret);
        w_len += ret;
    }

    return w_len;
}

static int codec_out_data_handle_cb(audio_frame_info_t *frame_info, char *buffer, uint32_t len, void *params)
{
    LOGD("channel_number: %d, sample_rate: %d, sample_bits: %d\n", frame_info->channel_number, frame_info->sample_rate, frame_info->sample_bits);

    bool prompt_tonne_play_flag = false;

    uint32_t temp_w_len = 0;
    uint32_t w_len = 0;
    int ret = 0;
    while (w_len < len)
    {
        /* write speaker data to ringbuffer, aud_tras_drv read prompt tone data from the ringbuffer */
        if ((len - w_len) >= 640)
        {
            temp_w_len  = 640;
        }
        else
        {
            temp_w_len = len - w_len;
        }
        ret = aud_tras_drv_write_prompt_tone_data(buffer + w_len, temp_w_len, BEKEN_WAIT_FOREVER);
        if (ret <= 0)
        {
            LOGE("%s, %d, aud_tras_drv_write_prompt_tone_data fail, ret: %d\n", __func__, __LINE__, ret);
            break;
        }
        w_len += ret;

        /* start prompt tone play after write frame data to prompt tone ringbuffer pool to avoid read prompt tone fail */
        if (!prompt_tonne_play_flag)
        {
            aud_tras_drv_control_prompt_tone_play(true);
        }
    }

    return w_len;
}

static int prompt_tone_pool_empty_notify_cb(void *params)
{
    LOGI("%s, %d, params: %p\n", __func__, __LINE__, params);

    prompt_tone_play_handle_t handle = (prompt_tone_play_handle_t)params;

    if (handle && handle->play_finish_sem)
    {
        rtos_set_semaphore(&handle->play_finish_sem);
    }

    aud_tras_drv_control_prompt_tone_play(false);

    return BK_OK;
}


prompt_tone_play_handle_t prompt_tone_play_create(  prompt_tone_play_cfg_t *config)
{
    LOGI("%s\n", __func__);

    if (!config)
    {
        LOGE("%s, %d, config is NULL\n", __func__, __LINE__);
        return NULL;
    }

    prompt_tone_play_handle_t handle = (prompt_tone_play_handle_t)psram_malloc(sizeof(struct prompt_tone_play));
    if (!handle)
    {
        LOGE("%s, %d, malloc prompt tone play handle fail\n", __func__, __LINE__);
        return NULL;
    }
    os_memset(handle, 0, sizeof(struct prompt_tone_play));

#if 0
    if (BK_OK != rtos_init_semaphore(&prompt_tone_play->play_finish_sem, 1))
    {
        LOGE("%s, %d, ceate semaphore fail\n", __func__, __LINE__);
        goto fail;
    }
#endif

    /* create source */
    config->source_cfg.data_handle = source_out_data_handle_cb;
    config->source_cfg.usr_data = handle;
    handle->source = audio_source_create(config->source_type, &config->source_cfg);
    if (!handle->source)
    {
        LOGE("%s, %d, create audio source handle fail\n", __func__, __LINE__);
        goto fail;
    }

    /* create codec */
    config->codec_cfg.data_handle = codec_out_data_handle_cb;
    config->codec_cfg.usr_data = handle;
    config->codec_cfg.chunk_size = 640;
    config->codec_cfg.pool_size = config->codec_cfg.chunk_size * 4;
    handle->codec = audio_codec_create(config->codec_type, &config->codec_cfg);
    if (!handle->codec)
    {
        LOGE("%s, %d, create audio codec handle fail\n", __func__, __LINE__);
        goto fail;
    }

    os_memcpy(&handle->config, config, sizeof(prompt_tone_play_cfg_t));

    return handle;

fail:

#if 0
    if (mp3_play->play_finish_sem)
    {
        rtos_deinit_semaphore(&mp3_play->play_finish_sem);
        mp3_play->play_finish_sem = NULL;
    }
#endif

    if (handle->source)
    {
        audio_source_destroy(handle->source);
        handle->source = NULL;
    }

    if (handle->codec)
    {
        audio_codec_destroy(handle->codec);
        handle->codec = NULL;
    }

    if (handle)
    {
        psram_free(handle);
        handle = NULL;
    }

    return NULL;
}

bk_err_t prompt_tone_play_destroy(prompt_tone_play_handle_t handle)
{
    LOGI("%s\n", __func__);
    if (!handle)
    {
        LOGW("%s, %d, handle already destroy\n", __func__, __LINE__);
        return BK_OK;
    }

    if (handle->codec)
    {
        audio_codec_destroy(handle->codec);
        handle->codec = NULL;
    }

    if (handle->source)
    {
        audio_source_destroy(handle->source);
        handle->source = NULL;
    }

    psram_free(handle);

    return BK_OK;
}

bk_err_t prompt_tone_play_open(prompt_tone_play_handle_t handle)
{
    if (!handle)
    {
        LOGE("%s, %d, handle is NULL\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (!handle->source || !handle->codec)
    {
        LOGE("%s, %d, handle->source: %p, handle->codec: %p\n", __func__, __LINE__, handle->source, handle->codec);
        return BK_FAIL;
    }

    aud_tras_drv_register_prompt_tone_pool_empty_notify(prompt_tone_pool_empty_notify_cb, handle);

    audio_codec_open(handle->codec);

    audio_source_open(handle->source);

    return BK_OK;
}

bk_err_t prompt_tone_play_close(prompt_tone_play_handle_t handle, bool wait_play_finish)
{
    LOGI("%s\n", __func__);

    if (!handle)
    {
        LOGE("%s, %d, handle is NULL\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (wait_play_finish)
    {
        if (handle->play_finish_sem)
        {
            rtos_get_semaphore(&handle->play_finish_sem, BEKEN_NEVER_TIMEOUT);
        }
    }

    if (handle->play_finish_sem)
    {
        rtos_deinit_semaphore(&handle->play_finish_sem);
        handle->play_finish_sem = NULL;
    }

    if (handle->source)
    {
        audio_source_close(handle->source);
    }

    if (handle->codec)
    {
        audio_codec_close(handle->codec);
    }

    aud_tras_drv_register_prompt_tone_pool_empty_notify(NULL, NULL);

    return BK_OK;
}


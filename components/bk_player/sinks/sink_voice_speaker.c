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


#include "player_osal.h"
#include "source.h"
#include "codec.h"
#include "sink.h"
#include "bk_player_err.h"
#include <driver/uart.h>
#include "gpio_driver.h"
#include "aud_intf.h"
#include "aud_intf_types.h"


//#define SINK_ONBOARD_SPK_DEBUG

#ifdef SINK_ONBOARD_SPK_DEBUG

#define AUD_DAC_DMA_ISR_START()                 do { GPIO_DOWN(32); GPIO_UP(32); GPIO_DOWN(32);} while (0)

#define SINK_ONBOARD_SPK_WRITE_RB_START()       do { GPIO_DOWN(33); GPIO_UP(33);} while (0)
#define SINK_ONBOARD_SPK_WRITE_RB_END()         do { GPIO_DOWN(33); } while (0)

#define SINK_ONBOARD_SPK_READ_RB_START()        do { GPIO_DOWN(34); GPIO_UP(34);} while (0)
#define SINK_ONBOARD_SPK_READ_RB_END()          do { GPIO_DOWN(34); } while (0)

#define SINK_ONBOARD_SPK_WRITE_FIFO_START()     do { GPIO_DOWN(35); GPIO_UP(35);} while (0)
#define SINK_ONBOARD_SPK_WRITE_FIFO_END()       do { GPIO_DOWN(35); } while (0)

#else

#define AUD_DAC_DMA_ISR_START()

#define SINK_ONBOARD_SPK_WRITE_RB_START()
#define SINK_ONBOARD_SPK_WRITE_RB_END()

#define SINK_ONBOARD_SPK_READ_RB_START()
#define SINK_ONBOARD_SPK_READ_RB_END()

#define SINK_ONBOARD_SPK_WRITE_FIFO_START()
#define SINK_ONBOARD_SPK_WRITE_FIFO_END()

#endif


#define SINK_DEVICE_CHECK_NULL(ptr) do {\
        if (ptr == NULL) {\
            player_log(LOG_ERR, "SINK_DEVICE_CHECK_NULL fail \n");\
            return BK_FAIL;\
        }\
    } while(0)


typedef struct
{
//    uint32_t buad_rate;
//    uint8_t uart_id;

    audio_info_t info;
} voice_speaker_sink_priv_t;

static aud_spk_source_info_t gl_music_source_info = {0};

static int voice_speaker_sink_open(enum AUDIO_SINK_TYPE sink_type, void *param, audio_sink_t **sink_pp)
{
    audio_sink_t *sink;
    bk_err_t ret = BK_OK;
    //player_state_t *player;

    player_log(LOG_INFO, "%s \n", __func__);

    sink = audio_sink_new(sizeof(voice_speaker_sink_priv_t));
    if (!sink)
    {
        return PLAYER_ERR_NO_MEM;
    }

    voice_speaker_sink_priv_t *priv = (voice_speaker_sink_priv_t *)sink->sink_priv;

    audio_info_t *info = (audio_info_t *)param;

    priv->info.sample_rate = info->sample_rate;
    priv->info.sample_bits = info->sample_bits;
    priv->info.channel_number = info->channel_number;
    priv->info.bps = info->bps;

    sink->info.sampRate = priv->info.sample_rate;
    sink->info.bitsPerSample = priv->info.sample_bits;
    sink->info.nChans = priv->info.channel_number;

    ret = bk_aud_intf_voc_init_audio_act();
    if (ret != BK_OK)
    {
        player_log(LOG_ERR, "init audio act fail\n");
        goto fail;
    }

    gl_music_source_info.type = SPK_SOURCE_TYPE_MUSIC;
    gl_music_source_info.spk_info.channel_number = 1;
    gl_music_source_info.spk_info.sample_rate = info->sample_rate;
    gl_music_source_info.spk_info.sample_bits = info->sample_bits;
    ret = bk_aud_intf_voc_set_spk_source_type(&gl_music_source_info);
    if (ret != BK_OK)
    {
        player_log(LOG_ERR, "set speaker source type fail\n");
        goto fail;
    }

    *sink_pp = sink;

    player_log(LOG_ERR, "device_sink_open complete \n");

    return PLAYER_OK;

fail:
    if (sink)
    {
        os_free(sink);
        sink = NULL;
    }

    return PLAYER_ERR_UNKNOWN;
}

static int voice_speaker_sink_close(audio_sink_t *sink)
{
    //bk_err_t ret = BK_OK;
    player_log(LOG_INFO, "%s \n", __func__);

    gl_music_source_info.type = SPK_SOURCE_TYPE_VOICE;
    gl_music_source_info.spk_info.channel_number = 0;
    gl_music_source_info.spk_info.sample_rate = 0;
    gl_music_source_info.spk_info.sample_bits = 0;
    bk_aud_intf_voc_set_spk_source_type(&gl_music_source_info);

    bk_aud_intf_voc_deinit_audio_act();

    player_log(LOG_INFO, "%s complete\n", __func__);
    return PLAYER_OK;
}

static int voice_speaker_sink_write(audio_sink_t *sink, char *buffer, int len)
{
    voice_speaker_sink_priv_t *priv = (voice_speaker_sink_priv_t *)sink->sink_priv;
    bk_err_t ret = BK_OK;

    SINK_ONBOARD_SPK_WRITE_RB_START();

    audio_write_multiple_spk_data_req_t write_req = {0};
    write_req.data = (uint8_t *)buffer;
    write_req.data_len = (uint32_t)len;
    write_req.sampl_rate = priv->info.sample_rate;
    write_req.channel_num = priv->info.channel_number;
    write_req.wait_time = 3000;
    ret = bk_aud_intf_voc_write_multiple_spk_data(&write_req);

    SINK_ONBOARD_SPK_WRITE_RB_END();

    return ret;
}


static int voice_speaker_sink_control(audio_sink_t *sink, enum AUDIO_SINK_CONTROL control)
{
    int ret = PLAYER_OK;

    //uart_sink_priv_t *priv = (uart_sink_priv_t *)sink->sink_priv;

    if (control == AUDIO_SINK_PAUSE)
    {
        /* first mute audio dac, then pause audio pipeline */
        //ret = onboard_speaker_stop(priv);
        if (ret != BK_OK)
        {
            player_log(LOG_DEBUG, "%s, paly pipeline pause fail, ret: %d, %d \n", __func__, ret, __LINE__);
        }
    }
    else if (control == AUDIO_SINK_RESUME)
    {
        /* first resume audio pipeline, then unmute audio dac */
        //ret = onboard_speaker_start(priv);
        if (ret != BK_OK)
        {
            player_log(LOG_DEBUG, "%s, paly pipeline mute fail, ret: %d, %d \n", __func__, ret, __LINE__);
        }
        else
        {
            //audio_dac_mute_sta = false;
        }
    }
    else if (control == AUDIO_SINK_MUTE)
    {
        //ret = onboard_speaker_mute();
        if (ret != BK_OK)
        {
            player_log(LOG_DEBUG, "%s, paly pipeline mute fail, ret: %d, %d \n", __func__, ret, __LINE__);
        }
        else
        {
            //audio_dac_mute_sta = true;
        }
    }
    else if (control == AUDIO_SINK_UNMUTE)
    {
        //ret = onboard_speaker_unmute();
        if (ret != BK_OK)
        {
            player_log(LOG_DEBUG, "%s, paly pipeline mute fail, ret: %d, %d \n", __func__, ret, __LINE__);
        }
        else
        {
            //audio_dac_mute_sta = false;
        }
    }
    else if (control == AUDIO_SINK_FRAME_INFO_CHANGE)
    {
        //ret = onboard_speaker_change_frame_info(sink->info.sampRate, sink->info.nChans, sink->info.bitsPerSample);
        if (ret != BK_OK)
        {
            player_log(LOG_DEBUG, "%s, paly pipeline change frame info fail, ret: %d, %d \n", __func__, ret, __LINE__);
        }
    }
    else if (control == AUDIO_SINK_SET_VOLUME)
    {
        //ret = onboard_speaker_set_volume(sink->info.volume);
        if (ret != BK_OK)
        {
            player_log(LOG_DEBUG, "%s, paly pipeline set volume fail, ret: %d, %d \n", __func__, ret, __LINE__);
        }
    }
    else
    {
        //nothing to do
    }

    return ret;
}

audio_sink_ops_t voice_speaker_sink_ops =
{
    .open =    voice_speaker_sink_open,
    .write =   voice_speaker_sink_write,
    .control = voice_speaker_sink_control,
    .close =   voice_speaker_sink_close,
};

audio_sink_ops_t *get_voice_speaker_sink_ops(void)
{
    return &voice_speaker_sink_ops;
}


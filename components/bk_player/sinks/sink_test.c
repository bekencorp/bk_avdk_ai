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


#define SINK_ONBOARD_SPK_DEBUG

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
} uart_sink_priv_t;


static int uart_sink_open(enum AUDIO_SINK_TYPE sink_type, void *param, audio_sink_t **sink_pp)
{
    audio_sink_t *sink;
    //int ret;
    //player_state_t *player;

    sink = audio_sink_new(sizeof(uart_sink_priv_t));
    if (!sink)
    {
        return PLAYER_ERR_NO_MEM;
    }

    uart_sink_priv_t *priv = (uart_sink_priv_t *)sink->sink_priv;

//    audio_info_t *info = (audio_info_t *)param;

    *sink_pp = sink;

    player_log(LOG_ERR, "device_sink_open complete \n");

    return PLAYER_OK;
}

static int uart_sink_close(audio_sink_t *sink)
{
    //bk_err_t ret = BK_OK;
    player_log(LOG_INFO, "%s \n", __func__);

    player_log(LOG_INFO, "device_sink_close \n");
    return PLAYER_OK;
}

static int uart_sink_write(audio_sink_t *sink, char *buffer, int len)
{
    uart_sink_priv_t *priv = (uart_sink_priv_t *)sink->sink_priv;

    //int need_w_len = len;

    SINK_ONBOARD_SPK_WRITE_RB_START();

    //bk_uart_write_bytes(priv->uart_id, buffer, len);

    rtos_delay_milliseconds(100);

    SINK_ONBOARD_SPK_WRITE_RB_END();

    return len;
}


static int uart_sink_control(audio_sink_t *sink, enum AUDIO_SINK_CONTROL control)
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

audio_sink_ops_t test_sink_ops =
{
    .open =    uart_sink_open,
    .write =   uart_sink_write,
    .control = uart_sink_control,
    .close =   uart_sink_close,
};

audio_sink_ops_t *get_test_sink_ops(void)
{
    return &test_sink_ops;
}


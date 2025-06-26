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
    uint32_t buad_rate;
    uint8_t uart_id;

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

    audio_info_t *info = (audio_info_t *)param;

    //priv->info.frame_size = info->frame_size;
    priv->info.sample_rate = info->sample_rate;
    priv->info.sample_bits = info->sample_bits;
    priv->info.channel_number = info->channel_number;
    priv->info.bps = info->bps;

    sink->info.sampRate = priv->info.sample_rate;
    sink->info.bitsPerSample = priv->info.sample_bits;
    sink->info.nChans = priv->info.channel_number;

    //player = get_player();
    //gain = player->spk_gain * 63 / 100;

    priv->buad_rate = 2000000;
    priv->uart_id = 1;

    //bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_480M);

    uart_config_t config = {0};
    os_memset(&config, 0, sizeof(uart_config_t));
    if (priv->uart_id == 0)
    {
        gpio_dev_unmap(GPIO_10);
        gpio_dev_map(GPIO_10, GPIO_DEV_UART1_RXD);
        gpio_dev_unmap(GPIO_11);
        gpio_dev_map(GPIO_11, GPIO_DEV_UART1_TXD);
    }
    else if (priv->uart_id == 2)
    {
        gpio_dev_unmap(GPIO_40);
        gpio_dev_map(GPIO_40, GPIO_DEV_UART3_RXD);
        gpio_dev_unmap(GPIO_41);
        gpio_dev_map(GPIO_41, GPIO_DEV_UART3_TXD);
    }
    else
    {
        gpio_dev_unmap(GPIO_0);
        gpio_dev_map(GPIO_0, GPIO_DEV_UART2_TXD);
        gpio_dev_unmap(GPIO_1);
        gpio_dev_map(GPIO_1, GPIO_DEV_UART2_RXD);
    }

    config.baud_rate = priv->buad_rate;
    config.data_bits = UART_DATA_8_BITS;
    config.parity = UART_PARITY_NONE;
    config.stop_bits = UART_STOP_BITS_1;
    config.flow_ctrl = UART_FLOWCTRL_DISABLE;
    config.src_clk = UART_SCLK_XTAL_26M;

    if (bk_uart_init(priv->uart_id, &config) != BK_OK)
    {
        player_log(LOG_INFO, "%s, %d, init uart fail \n", __func__, __LINE__);
        return PLAYER_ERR_UNKNOWN;
    }
    player_log(LOG_INFO, "init uart: %d ok \n", priv->uart_id);


    *sink_pp = sink;

    player_log(LOG_ERR, "device_sink_open complete \n");

    return PLAYER_OK;
}

static int uart_sink_close(audio_sink_t *sink)
{
    //bk_err_t ret = BK_OK;
    player_log(LOG_INFO, "%s \n", __func__);

    uart_sink_priv_t *priv = (uart_sink_priv_t *)sink->sink_priv;

    if (priv->uart_id == 0)
    {
        gpio_dev_unmap(GPIO_10);
        gpio_dev_unmap(GPIO_11);
    }
    else if (priv->uart_id == 2)
    {
        gpio_dev_unmap(GPIO_40);
        gpio_dev_unmap(GPIO_41);
    }
    else
    {
        gpio_dev_unmap(GPIO_0);
        gpio_dev_unmap(GPIO_1);
    }

    if (bk_uart_deinit(priv->uart_id) != BK_OK)
    {
        player_log(LOG_INFO, "%s, %d, deinit uart: %d fail \n", __func__, __LINE__, priv->uart_id);
    }
    player_log(LOG_INFO, "deinit uart: %d ok \n", priv->uart_id);

    //bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_DEFAULT);

    player_log(LOG_INFO, "device_sink_close \n");
    return PLAYER_OK;
}

static int uart_sink_write(audio_sink_t *sink, char *buffer, int len)
{
    uart_sink_priv_t *priv = (uart_sink_priv_t *)sink->sink_priv;

    //int need_w_len = len;

    SINK_ONBOARD_SPK_WRITE_RB_START();

    bk_uart_write_bytes(priv->uart_id, buffer, len);

    rtos_delay_milliseconds(10);

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

audio_sink_ops_t uart_sink_ops =
{
    .open =    uart_sink_open,
    .write =   uart_sink_write,
    .control = uart_sink_control,
    .close =   uart_sink_close,
};

audio_sink_ops_t *get_uart_sink_ops(void)
{
    return &uart_sink_ops;
}


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
#include "bk_player_err.h"

#if CONFIG_PLAYER_ENABLE_SINK_ONBOARD_SPEAKER
#include "sink_onboard_speaker.h"
#include "sink_uart.h"
#include "sink_voice_speaker.h"
#endif

audio_sink_t *audio_sink_new(int priv_size)
{
    audio_sink_t *sink;

    sink = player_malloc(sizeof(audio_sink_t) + priv_size);
    if (!sink)
    {
        return NULL;
    }
    sink->sink_priv = sink + 1;
    return sink;
}

audio_sink_t *audio_sink_open(enum AUDIO_SINK_TYPE sink_type, void *param)
{
    audio_sink_ops_t *ops = NULL;
    audio_sink_t *sink = NULL;

    switch (sink_type)
    {
#if CONFIG_PLAYER_ENABLE_SINK_ONBOARD_SPEAKER
        case AUDIO_SINK_ONBOARD_SPEAKER:
            ops = get_onboard_speaker_sink_ops();
            //ops = get_uart_sink_ops();
            break;
#endif

#if CONFIG_PLAYER_ENABLE_SINK_VOICE_SPEAKER
        case AUDIO_SINK_VOICE_SPEAKER:
            ops = get_voice_speaker_sink_ops();
            break;
#endif

        default:
            break;
    }

    if (!ops)
        return sink;

    if (ops->open(sink_type, param, &sink) == PLAYER_OK)
    {
        sink->ops = ops;
    }

    return sink;
}

int audio_sink_close(audio_sink_t *sink)
{
    int ret = PLAYER_OK;

    if (sink && sink->ops->close)
    {
        ret = sink->ops->close(sink);
    }

    if (ret != PLAYER_OK)
    {
        //player_log(LOG_ERR, "%s, sink->ops->close fail, ret: %d, %d \n", __func__, ret, __LINE__);
    }

    if (sink)
    {
        player_free(sink);
    }

    return ret;
}

int audio_sink_write_data(audio_sink_t *sink, char *buffer, int len)
{
    if (!sink || !buffer || !len)
    {
        return PLAYER_ERR_INVALID_PARAM;
    }

    return sink->ops->write(sink, buffer, len);
}

int audio_sink_control(audio_sink_t *sink, enum AUDIO_SINK_CONTROL control)
{
    int ret;

    if (sink && sink->ops->control)
    {
        ret = sink->ops->control(sink, control);
    }
    else
    {
        ret = PLAYER_OK;
    }

    return ret;
}

int audio_sink_set_info(audio_sink_t *sink, int rate, int bits, int ch)
{
    if (!sink)
    {
        return PLAYER_ERR_INVALID_PARAM;
    }

    sink->info.sampRate = rate;
    sink->info.bitsPerSample = bits;
    sink->info.nChans = ch;

    return PLAYER_OK;
}

int audio_sink_set_volume(audio_sink_t *sink, int volume)
{
    if (!sink)
    {
        return PLAYER_ERR_INVALID_PARAM;
    }

    sink->info.volume = volume;

    return PLAYER_OK;
}


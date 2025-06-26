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
#include "codec.h"
#include "source.h"
#include <string.h>
#include "bk_player_err.h"

#if CONFIG_PLAYER_ENABLE_SOURCE_NET
#include "source_net.h"
#endif

static int audio_source_get_type(const char *URI)
{
    if (strstr(URI, "http://") && strstr(URI, ".m3u8"))   return AUDIO_SOURCE_M3U8;
    if (strstr(URI, "http://"))   return AUDIO_SOURCE_NET;
    if (strstr(URI, "1://"))      return AUDIO_SOURCE_FILE;

    return AUDIO_SOURCE_UNKNOWN;
}


audio_source_t *audio_source_new(int priv_size)
{
    audio_source_t *source;

    source = player_malloc(sizeof(audio_source_t) + priv_size);
    if (!source)
    {
        return NULL;
    }
    source->source_priv = source + 1;
    return source;
}

audio_source_t *audio_source_open_url(char *url)
{
    audio_source_ops_t *ops = NULL;
    audio_source_t *source = NULL;

    enum AUDIO_SOURCE_TYPE source_type = audio_source_get_type(url);

    switch (source_type)
    {
#if CONFIG_PLAYER_ENABLE_SOURCE_NET
        case AUDIO_SOURCE_NET:
            ops = get_net_source_ops();
            break;
#endif

        default:
           break;
    }

    if (!ops)
        return source;

    if (ops->open(url, &source) == PLAYER_OK)
    {
        source->ops = ops;
    }

    return source;
}

int audio_source_close(audio_source_t *source)
{
    int ret;

    if (source && source->ops->close)
    {
        ret = source->ops->close(source);
    }
    else
    {
        ret = PLAYER_OK;
    }

    if (source)
    {
        player_free(source);
    }

    return ret;
}

int audio_source_get_codec_type(audio_source_t *source)
{
    if (!source)
    {
        return AUDIO_CODEC_UNKNOWN;
    }

    return source->ops->get_codec_type(source);
}

uint32_t audio_source_get_total_bytes(audio_source_t *source)
{
    if (!source)
    {
        return AUDIO_CODEC_UNKNOWN;
    }

    if (source->ops->get_total_bytes)
    {
        return source->ops->get_total_bytes(source);
    }
    else
    {
        return 0;
    }
}

int audio_source_read_data(audio_source_t *source, char *buffer, int len)
{
    if (!source || !buffer || !len)
    {
        return PLAYER_ERR_INVALID_PARAM;
    }

    return source->ops->read(source, buffer, len);
}

int audio_source_seek(audio_source_t *source, int offset, uint32_t whence)
{
    if (!source || !source->ops->seek)
    {
        return PLAYER_ERR_INVALID_PARAM;
    }

    return source->ops->seek(source, offset, whence);
}

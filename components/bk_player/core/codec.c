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
#include <string.h>
#include "codec.h"
#include "bk_player_err.h"

#if CONFIG_PLAYER_ENABLE_CODEC_MP3
#include "codec_mp3.h"
#endif

struct codec_mapping
{
    char *ext_name;
    int codec_type;
};

static const struct codec_mapping codecs[] =
{
    {".wav", AUDIO_CODEC_WAV},
    {".mp3", AUDIO_CODEC_MP3},
    {".aac", AUDIO_CODEC_AAC},
    {".m4a", AUDIO_CODEC_AAC},
    {".ts", AUDIO_CODEC_TS},
    {".amr", AUDIO_CODEC_AMR},
    {".flac", AUDIO_CODEC_FLAC},
    {".opus", AUDIO_CODEC_OGG},
    {".ogg", AUDIO_CODEC_OGG},
};

#define CODECS_CNT sizeof(codecs)/sizeof(codecs[0])

int audio_codec_get_type(char *ext_name)
{
    int i;
    int ret = AUDIO_CODEC_UNKNOWN;

    if (!ext_name)
    {
        return ret;
    }

    for (i = 0; i < CODECS_CNT; i++)
    {
        if (strcasecmp(ext_name, codecs[i].ext_name) == 0)
        {
            ret = codecs[i].codec_type;
            break;
        }
    }

    return ret;
}

struct mime_mapping
{
    char *mime;
    int codec_type;
};

static const struct mime_mapping mimes[] =
{
    {"audio/wav", AUDIO_CODEC_WAV},
    {"audio/mp3", AUDIO_CODEC_MP3},
    {"audio/flac", AUDIO_CODEC_FLAC},
    {"audio/ogg", AUDIO_CODEC_OGG},
    {"audio/aac", AUDIO_CODEC_AAC},
    {"audio/mpeg", AUDIO_CODEC_MP3},
    /* special format */
    {"video/MP2T", AUDIO_CODEC_TS},
};

#define MIMES_CNT sizeof(mimes)/sizeof(mimes[0])

int audio_codec_get_mime_type(char *mime)
{
    int i;
    int ret = AUDIO_CODEC_UNKNOWN;

    if (!mime)
    {
        return ret;
    }

    for (i = 0; i < MIMES_CNT; i++)
    {
        if (strcasecmp(mime, mimes[i].mime) == 0)
        {
            ret = mimes[i].codec_type;
            break;
        }
    }

    return ret;
}

audio_codec_t *audio_codec_new(int priv_size)
{
    audio_codec_t *codec;

    codec = player_malloc(sizeof(audio_codec_t) + priv_size);
    if (!codec)
    {
        return NULL;
    }
    codec->codec_priv = codec + 1;
    return codec;
}

audio_codec_t *audio_codec_open(enum AUDIO_CODEC_TYPE codec_type, void *param, audio_source_t *source)
{
    audio_codec_ops_t *ops = NULL;
    audio_codec_t *codec = NULL;

    if (!source || codec_type == AUDIO_CODEC_UNKNOWN)
        return NULL;

    switch (codec_type)
    {
#if CONFIG_PLAYER_ENABLE_CODEC_MP3
        case AUDIO_CODEC_MP3:
            ops = get_mp3_codec_ops();
            break;
#endif

        default:
            break;
    }

    if (ops->open(codec_type, param, &codec) == PLAYER_OK)
    {
        codec->ops = ops;
        codec->source = source;
    }

    return codec;
}

int audio_codec_close(audio_codec_t *codec)
{
    int ret;

    if (codec && codec->ops->close)
    {
        ret = codec->ops->close(codec);
    }
    else
    {
        ret = PLAYER_OK;
    }

    player_free(codec);

    return ret;
}

int audio_codec_get_info(audio_codec_t *codec, audio_info_t *info)
{
    if (!codec || !info)
    {
        return PLAYER_ERR_INVALID_PARAM;
    }

    os_memset(info, 0, sizeof(audio_info_t));
    return codec->ops->get_info(codec, info);
}

int audio_codec_get_chunk_size(audio_codec_t *codec)
{
    if (codec && codec->ops->get_chunk_size)
    {
        return codec->ops->get_chunk_size(codec);
    }
    else
    {
        return DEFAULT_CHUNK_SIZE;
    }
}

int audio_codec_get_data(audio_codec_t *codec, char *buffer, int len)
{
    if (!codec || !buffer || !len)
    {
        return PLAYER_ERR_INVALID_PARAM;
    }

    return codec->ops->get_data(codec, buffer, len);
}


int audio_codec_calc_position(audio_codec_t *codec, int second)
{
    if (!codec)
    {
        return PLAYER_ERR_INVALID_PARAM;
    }

    return codec->ops->calc_position(codec, second);
}


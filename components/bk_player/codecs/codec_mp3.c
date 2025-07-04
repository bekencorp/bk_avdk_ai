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


#include <os/mem.h>
#include <os/str.h>
#include <string.h>
#include <unistd.h>
#include <modules/mp3dec.h>
#include "source.h"
#include "codec.h"
#include "sink.h"
#include "player_osal.h"
#include "bk_player_err.h"
#include <common/bk_assert.h>


//#define WIFI_READ_DATA_TIME_DEBUG
#ifdef WIFI_READ_DATA_TIME_DEBUG
#include <driver/aon_rtc.h>

#define WIFI_READ_DATA_TIMEOUT_NUM    (10 * 1000)      //unit us
#endif

#define MP3_AUDIO_BUF_SZ    (8 * 1024) /* feel free to change this, but keep big enough for >= one frame at high bitrates */

typedef struct
{
    char identifier[3];         // 必须是 "TAG"
    char title[30];             // 歌曲标题
    char artist[30];            // 艺术家
    char album[30];             // 专辑
    char year[4];               // 年份
    char comment[30];           // 注释
    unsigned char zeroByte;     // 必须是0
    unsigned char trackNum;     // 音轨号
    unsigned char genre;        // 流派代码
} id3v1_info_t;

typedef struct mp3_codec_priv_s
{
    /* mp3 information */
    HMP3Decoder decoder;
    MP3FrameInfo frame_info;
    uint32_t frames;
    uint32_t stream_offset;

    /* mp3 read session */
    uint8_t *read_buffer, *read_ptr;
    uint32_t bytes_left;

    int current_sample_rate;

    biterate_type_t biterate_type;

    uint32_t id3v2_bytes;
    uint32_t id3v1_bytes;

    uint32_t total_bytes;
    uint32_t total_frames;
    double total_duration;     // play duration of total bytes in milisecond (ms)

    id3v1_info_t id3v1_info;
} mp3_codec_priv_t;


static int mp3_codec_get_chunk_size(audio_codec_t *codec);

static int parse_id3v1_header(audio_codec_t *codec)
{
    int bytes_read;
    int ret = -1;

    mp3_codec_priv_t *priv = (mp3_codec_priv_t *)codec->codec_priv;

    /* reset read_ptr */
    priv->read_ptr = priv->read_buffer;
    priv->bytes_left = 0;

    if (0 != audio_source_seek(codec->source, -128, SEEK_END))
    {
        player_log(LOG_ERR, "%s, audio_source_seek fail, %d\n", __func__, __LINE__);
        return -1;
    }


    bytes_read = audio_source_read_data(codec->source, (char *)(priv->read_buffer), 128);
    if (bytes_read != 128)
    {
        player_log(LOG_ERR, "%s, read 'id3v1' fail, bytes_read: %d, %d\n", __func__, bytes_read, __LINE__);
        goto out;
    }

    /* check identifier "TAG" */
    if (os_strncasecmp((char *)priv->read_buffer, "TAG", os_strlen("TAG")) != 0)
    {
        /* not have ID3V1 */
        player_log(LOG_INFO, "not have ID3V1 \n");
        goto out;
    }
    else
    {
        priv->id3v1_bytes = 128;
        os_memcpy(&priv->id3v1_info, priv->read_buffer, 128);
    }

    ret = 0;
out:
    /* reset read_ptr */
    priv->read_ptr = priv->read_buffer;
    priv->bytes_left = 0;

    if (0 != audio_source_seek(codec->source, 0, SEEK_SET))
    {
        player_log(LOG_ERR, "%s, audio_source_seek fail, %d\n", __func__, __LINE__);
        return -1;
    }

    return ret;
}

/* must sure the buf_size is enough */
static int parse_vbr_xing_header(mp3_codec_priv_t *codec_mp3, char *xing_header_buf, int buf_size)
{
    /* check whether buffer size is enough */
    if (buf_size < 156)
    {
        return -1;
    }

    /* check header id of "XING" */
    if ((xing_header_buf[0] == 'x' || xing_header_buf[0] == 'X')
        && (xing_header_buf[1] == 'i' || xing_header_buf[1] == 'I')
        && (xing_header_buf[2] == 'n' || xing_header_buf[2] == 'N')
        && (xing_header_buf[3] == 'g' || xing_header_buf[3] == 'G')
       )
    {
        codec_mp3->biterate_type = BITERATE_TYPE_VBR_XING;
    }
    else if ((xing_header_buf[0] == 'i' || xing_header_buf[0] == 'I')
             && (xing_header_buf[1] == 'n' || xing_header_buf[1] == 'N')
             && (xing_header_buf[2] == 'f' || xing_header_buf[2] == 'F')
             && (xing_header_buf[3] == 'o' || xing_header_buf[3] == 'O')
            )
    {
        codec_mp3->biterate_type = BITERATE_TYPE_CBR_INFO;
    }
    else
    {
        return -1;
    }

    int offset = 8;

    unsigned char flags = xing_header_buf[7];
    if (flags & 0x01)   //Frames field is present
    {
        codec_mp3->total_frames = *((int *)(xing_header_buf + offset));
        offset += 4;
    }

    if (flags & 0x02)   //Bytes field is present
    {
        codec_mp3->total_bytes = *((int *)(xing_header_buf + offset));
        offset += 4;
    }

    return 0;
}

/* must sure the buf_size is enough */
static int parse_vbr_vbri_header(mp3_codec_priv_t *codec_mp3, char *vbri_header_buf, int buf_size)
{
    /* check whether buffer size is enough */
    if (buf_size < 156)
    {
        return -1;
    }

    /* check header id of "VBRI" */
    if ((vbri_header_buf[0] == 'v' || vbri_header_buf[0] == 'V')
        && (vbri_header_buf[1] == 'b' || vbri_header_buf[1] == 'B')
        && (vbri_header_buf[2] == 'r' || vbri_header_buf[2] == 'R')
        && (vbri_header_buf[3] == 'i' || vbri_header_buf[3] == 'I')
       )
    {
        codec_mp3->biterate_type = BITERATE_TYPE_VBR_VBRI;
    }
    else
    {
        return -1;
    }

    unsigned char *offset = (unsigned char *)(vbri_header_buf + 10);

    codec_mp3->total_bytes = *((int *)(offset));
    offset += 4;

    codec_mp3->total_frames = *((int *)(offset));

    return 0;
}

/* skip id3 tag */
static int codec_mp3_skip_idtag(audio_codec_t *codec)
{
    int  offset = 0;
    uint8_t *tag;

    mp3_codec_priv_t *codec_mp3 = (mp3_codec_priv_t *)codec->codec_priv;

    /* set the read_ptr to the read buffer */
    codec_mp3->read_ptr = codec_mp3->read_buffer;

    tag = codec_mp3->read_ptr;
    /* read idtag v2 */

    if (audio_source_read_data(codec->source, (char *)codec_mp3->read_ptr, 3) != 3)
    {
        goto __exit;
    }

    codec_mp3->bytes_left = 3;
    if (tag[0] == 'I' &&
        tag[1] == 'D' &&
        tag[2] == '3')
    {
        int  size;

        if (audio_source_read_data(codec->source, (char *)codec_mp3->read_ptr + 3, 7) != 7)
        {
            goto __exit;
        }

        size = ((tag[6] & 0x7F) << 21) | ((tag[7] & 0x7F) << 14) | ((tag[8] & 0x7F) << 7) | ((tag[9] & 0x7F));

        offset = size + 10;

        /* read all of idv3 */
        {
            int rest_size = size;
            while (1)
            {
                int length;
                int chunk;

                if (rest_size > MP3_AUDIO_BUF_SZ)
                {
                    chunk = MP3_AUDIO_BUF_SZ;
                }
                else
                {
                    chunk = rest_size;
                }

                length = audio_source_read_data(codec->source, (char *)codec_mp3->read_buffer, chunk);
                if (length > 0)
                {
                    rest_size -= length;
                }
                else
                {
                    break; /* read failed */
                }
            }

            codec_mp3->bytes_left = 0;
        }

        return offset;
    }

__exit:
    return offset;
}



static int check_mp3_sync_word(audio_codec_t *codec)
{
    int err;

    mp3_codec_priv_t *priv = (mp3_codec_priv_t *)codec->codec_priv;

    os_memset(&priv->frame_info, 0, sizeof(MP3FrameInfo));

    err = MP3GetNextFrameInfo(priv->decoder, &priv->frame_info, priv->read_ptr);
    if (err == ERR_MP3_INVALID_FRAMEHEADER)
    {
        player_log(LOG_ERR, "%s, ERR_MP3_INVALID_FRAMEHEADER, %d\n", __func__, __LINE__);
        goto __err;
    }
    else if (err != ERR_MP3_NONE)
    {
        player_log(LOG_ERR, "%s, MP3GetNextFrameInfo fail, err=%d, %d\n", __func__, err, __LINE__);
        goto __err;
    }
    else if (priv->frame_info.nChans != 1 && priv->frame_info.nChans != 2)
    {
        player_log(LOG_ERR, "%s, nChans is not 1 or 2, nChans=%d, %d\n", __func__, priv->frame_info.nChans, __LINE__);
        goto __err;
    }
    else if (priv->frame_info.bitsPerSample != 16 && priv->frame_info.bitsPerSample != 8)
    {
        player_log(LOG_ERR, "%s, bitsPerSample is not 16 or 8, bitsPerSample=%d, %d\n", __func__, priv->frame_info.bitsPerSample, __LINE__);
        goto __err;
    }
    else
    {
        //noting todo
    }

    return 0;

__err:
    return -1;
}


static int32_t codec_mp3_fill_buffer(audio_codec_t *codec)
{
    int bytes_read;
    size_t bytes_to_read;

    mp3_codec_priv_t *priv = (mp3_codec_priv_t *)codec->codec_priv;


    /* adjust read ptr */
    if (priv->bytes_left > 0xffff0000)
    {
        player_log(LOG_WARN, "read_ptr: %p, bytes_left: 0x%x \n", priv->read_ptr, priv->bytes_left);
        return -1;
    }

    if (priv->bytes_left > 0)
    {
        memmove(priv->read_buffer, priv->read_ptr, priv->bytes_left);
    }
    priv->read_ptr = priv->read_buffer;

    bytes_to_read = (MP3_AUDIO_BUF_SZ - priv->bytes_left) & ~(512 - 1);

    bytes_read = audio_source_read_data(codec->source, (char *)(priv->read_buffer + priv->bytes_left), bytes_to_read);
    if (bytes_read > 0)
    {
        priv->bytes_left = priv->bytes_left + bytes_read;
        return 0;
    }
    else
    {
        if (bytes_read == 0)
        {
            if (priv->bytes_left >= MAINBUF_SIZE)
            {
                return 0;
            }
            else
            {
                player_log(LOG_WARN, "can't read more data, end of stream. left=%d\n", priv->bytes_left);
                goto exit;
            }
        }

        if (bytes_read == PLAYER_ERR_TIMEOUT)
        {
            if (priv->bytes_left >= MAINBUF_SIZE)
            {
                return 0;
            }
            return PLAYER_ERR_TIMEOUT;
        }
        else
        {
            player_log(LOG_ERR, "read more data error. left=%d\n", priv->bytes_left);
            //TODO
        }
    }

exit:
    return PLAYER_ERR_UNKNOWN;
}



static int mp3_codec_open(enum AUDIO_CODEC_TYPE codec_type, void *param, audio_codec_t **codec_pp)
{
    audio_codec_t *codec;
    mp3_codec_priv_t *priv;

    if (codec_type != AUDIO_CODEC_MP3)
    {
        return PLAYER_ERR_INVALID_PARAM;
    }

    codec = audio_codec_new(sizeof(mp3_codec_priv_t));
    if (!codec)
    {
        return PLAYER_ERR_NO_MEM;
    }

    priv = (mp3_codec_priv_t *)codec->codec_priv;

    os_memset(priv, 0x0, sizeof(mp3_codec_priv_t));

    /* init read session */
    priv->read_ptr = NULL;
    priv->bytes_left = 0;
    priv->frames = 0;

    priv->read_buffer = player_malloc(MP3_AUDIO_BUF_SZ);
    if (priv->read_buffer == NULL)
    {
        player_log(LOG_ERR, "%s, priv->read_buffer malloc fail, %d\n", __func__, __LINE__);
        return PLAYER_ERR_NO_MEM;
    }

    priv->decoder = MP3InitDecoder();
    if (!priv->decoder)
    {
        player_log(LOG_ERR, "%s, MP3InitDecoder create fail, %d\n", __func__, __LINE__);
        return PLAYER_ERR_UNKNOWN;
    }

    *codec_pp = codec;

    return PLAYER_OK;
}

static int mp3_codec_get_info(audio_codec_t *codec, audio_info_t *info)
{
    mp3_codec_priv_t *priv;
    priv = (mp3_codec_priv_t *)codec->codec_priv;
    int read_offset;
    //    int read_retry_cnt = 5;
    int ret = 0;

    /* check and parse ID3V1 field */
    parse_id3v1_header(codec);

    if (!priv->current_sample_rate)
    {
        priv->stream_offset = codec_mp3_skip_idtag(codec);
    }

__retry:
    if ((priv->read_ptr == NULL) || priv->bytes_left < 2 * MAINBUF_SIZE)
    {
        if (codec_mp3_fill_buffer(codec) != 0)
        {
            player_log(LOG_ERR, "%s, codec_mp3_fill_buffer fail, %d \n", __func__, __LINE__);
            return PLAYER_ERR_UNKNOWN;
        }
    }

#if 0
    /* Protect mp3 decoder to avoid decoding assert when data is insufficient. */
    if (priv->bytes_left < MAINBUF_SIZE)
    {
        if ((read_retry_cnt--) > 0)
        {
            goto __retry;
        }
        else
        {
            player_log(LOG_ERR, "%s, connot read enough data, read: %d < %d, %d \n", __func__, priv->bytes_left, MAINBUF_SIZE);
            return PLAYER_ERR;
        }
    }
#endif

    read_offset = MP3FindSyncWord(priv->read_ptr, priv->bytes_left);
    if (read_offset < 0)
    {
        /* discard this data */
        player_log(LOG_ERR, "%s, MP3FindSyncWord fail, outof sync, byte left: %d, %d\n", __func__, priv->bytes_left, __LINE__);
        /* maybe bytes_left is not enough, need fill buffer again */
        //TODO
        priv->bytes_left = 0;
        return PLAYER_ERR_UNKNOWN;
    }

    if (read_offset > priv->bytes_left)
    {
        player_log(LOG_ERR, "%s, find sync exception, read_offset:%d > bytes_left:%d, %d\n", __func__, read_offset, priv->bytes_left, __LINE__);
        priv->read_ptr += priv->bytes_left;
        priv->bytes_left = 0;
    }
    else
    {
        priv->read_ptr += read_offset;
        priv->bytes_left -= read_offset;
    }

    priv->id3v2_bytes = read_offset;
    player_log(LOG_INFO, "id3v2_bytes: %d \n", priv->id3v2_bytes);

    ret = check_mp3_sync_word(codec);
    if (ret == -1)
    {
        if (priv->bytes_left > 0)
        {
            priv->bytes_left --;
            priv->read_ptr ++;
        }

        goto __retry;
    }

    priv->read_ptr -= read_offset;
    priv->bytes_left += read_offset;

    player_log(LOG_INFO, "bitrate:%d, nChans:%d, samprate:%d, bitsPerSample:%d, outputSamps:%d\n", priv->frame_info.bitrate, priv->frame_info.nChans, priv->frame_info.samprate, priv->frame_info.bitsPerSample, priv->frame_info.outputSamps);
    /* update audio frame info */
    codec->info.channel_number = priv->frame_info.nChans;
    codec->info.sample_rate = priv->frame_info.samprate;
    codec->info.sample_bits = priv->frame_info.bitsPerSample;
    codec->info.frame_size = 2 * priv->frame_info.outputSamps;
    codec->info.bps = priv->frame_info.bitrate;

    /* check whether mp3 file is VBR when the frame is first frame */
    if (priv->biterate_type == BITERATE_TYPE_UNKNOW)
    {
        if (0 == parse_vbr_xing_header(priv, (char *)(priv->read_ptr + read_offset), (int)(priv->bytes_left - read_offset)))
        {
            player_log(LOG_INFO, "[XING] biterate_type:%d, total_bytes:%d, total_frames:%d \n", priv->biterate_type, priv->total_bytes, priv->total_frames);
        }
        else if (0 == parse_vbr_vbri_header(priv, (char *)(priv->read_ptr + read_offset), (int)(priv->bytes_left - read_offset)))
        {
            player_log(LOG_INFO, "[VBRI] biterate_type:%d, total_bytes:%d, total_frames:%d \n", priv->biterate_type, priv->total_bytes, priv->total_frames);
        }
        else
        {
            player_log(LOG_INFO, "[CBR] biterate_type:%d \n", priv->biterate_type);
            priv->biterate_type = BITERATE_TYPE_CBR;
        }
    }

    /* calculate total duration of mp3 */
    switch (priv->biterate_type)
    {
        case BITERATE_TYPE_CBR:
        case BITERATE_TYPE_CBR_INFO:
            /* CBR: total_duration = total_bytes / bitrate * 8000 ms */
            //priv->total_duration = priv->total_bytes / priv->frame_info.bitrate * 8000;
            /* set codec info */
            codec->info.total_bytes = priv->total_bytes;
            codec->info.header_bytes = priv->id3v2_bytes + priv->id3v1_bytes;
            /* need user calculate */
            codec->info.duration = 0.0;
            player_log(LOG_INFO, "[CBR] total_bytes:%d \n", priv->total_bytes);
            break;

        case BITERATE_TYPE_VBR_XING:
        case BITERATE_TYPE_VBR_VBRI:
            /* VBR: total_duration = total_bytes / bitrate * 8000 ms */
            priv->total_duration = (double)priv->total_frames * 1152.0 * (1.0 / (double)priv->frame_info.samprate) * 1000.0;
            /* set codec info */
            codec->info.duration = priv->total_duration;
            player_log(LOG_INFO, "[VBR] total_duration:%f \n", priv->total_duration);
            break;

        default:
            break;
    }


    priv->current_sample_rate = priv->frame_info.samprate;

    info->channel_number = priv->frame_info.nChans;
    info->sample_rate = priv->frame_info.samprate;
    info->sample_bits = priv->frame_info.bitsPerSample;
    info->frame_size = 2 * priv->frame_info.outputSamps;

    /* update audio frame info */
    codec->info.channel_number = priv->frame_info.nChans;
    codec->info.sample_rate = priv->frame_info.samprate;
    codec->info.sample_bits = priv->frame_info.bitsPerSample;
    codec->info.frame_size = 2 * priv->frame_info.outputSamps;

    return PLAYER_OK;
}

int mp3_codec_get_data(audio_codec_t *codec, char *buffer, int len)
{
    int err;
    int read_offset;

    mp3_codec_priv_t *priv = (mp3_codec_priv_t *)codec->codec_priv;

retry:
    if ((priv->read_ptr == NULL) || priv->bytes_left < 2 * MAINBUF_SIZE)
    {

#ifdef WIFI_READ_DATA_TIME_DEBUG
        uint64_t start_time = 0;
        uint32_t start_bytes = priv->bytes_left;
#if CONFIG_ARCH_RISCV
        extern u64 riscv_get_mtimer(void);
        start_time = riscv_get_mtimer();
#elif CONFIG_ARCH_CM33

#if CONFIG_AON_RTC
        start_time = bk_aon_rtc_get_us();
#endif      //#if CONFIG_AON_RTC
#endif      //#if CONFIG_ARCH_RISCV
#endif      //#ifdef WIFI_READ_DATA_TIME_DEBUG

        int ret = codec_mp3_fill_buffer(codec);
        if (ret != 0)
        {
            if (ret == PLAYER_ERR_TIMEOUT)
            {
                return PLAYER_ERR_TIMEOUT;
            }
            else
            {
                /* play complete */
                player_log(LOG_INFO, "play complete\n");
                return 0;
            }
        }

#ifdef WIFI_READ_DATA_TIME_DEBUG
        uint64_t stop_time = 0;
        uint32_t stop_bytes = priv->bytes_left;
#if CONFIG_ARCH_RISCV
        extern u64 riscv_get_mtimer(void);
        stop_time = riscv_get_mtimer();
#elif CONFIG_ARCH_CM33

#if CONFIG_AON_RTC
        stop_time = bk_aon_rtc_get_us();
#endif      //#if CONFIG_AON_RTC
#endif      //#if CONFIG_ARCH_RISCV

        /* check wifi read data speed */
        if ((stop_time - start_time) > WIFI_READ_DATA_TIMEOUT_NUM)
        {
            player_log(LOG_ERR, "================== Notion =================\n");
            //player_log(LOG_ERR, "stop_time: 0x%04x%04x us, start_time: 0x%04x%04x us\n", (uint32_t)(stop_time>>32 & 0x00000000ffffffff), (uint32_t)(stop_time & 0x00000000ffffffff), (uint32_t)(start_time>>32 & 0x00000000ffffffff), (uint32_t)(start_time & 0x00000000ffffffff));
            player_log(LOG_ERR, "%s, wifi read %d bytes data use 0x%04x%04x us, %d ms, %d\n", __FUNCTION__, stop_bytes - start_bytes, (uint32_t)((stop_time - start_time) >> 32), (uint32_t)((stop_time - start_time) & 0x00000000ffffffff), (uint32_t)((stop_time - start_time) / 1000), __LINE__);
            player_log(LOG_ERR, "\n");
        }
#endif      //#ifdef WIFI_READ_DATA_TIME_DEBUG
    }

    /* Protect mp3 decoder to avoid decoding assert when data is insufficient. */
    if (priv->bytes_left < MAINBUF_SIZE)
    {
        player_log(LOG_ERR, "%s, connot read enough data, read: %d < %d, %d\n", __func__, priv->bytes_left, MAINBUF_SIZE, __LINE__);
        goto retry;
        //return PLAYER_ERR_UNKNOWN;
    }

    read_offset = MP3FindSyncWord(priv->read_ptr, priv->bytes_left);
    if (read_offset < 0)
    {
        /* discard this data */
        player_log(LOG_ERR, "%s, MP3FindSyncWord fail, outof sync, byte left: %d, %d\n", __func__, priv->bytes_left, __LINE__);

        priv->bytes_left = 0;
        return 0;
    }

    if (read_offset > priv->bytes_left)
    {
        player_log(LOG_ERR, "%s, find sync exception, read_offset:%d > bytes_left:%d, %d\n", __func__, read_offset, priv->bytes_left, __LINE__);
        priv->read_ptr += priv->bytes_left;
        priv->bytes_left = 0;
    }
    else
    {
        priv->read_ptr += read_offset;
        priv->bytes_left -= read_offset;
    }

    if (check_mp3_sync_word(codec) == -1)
    {
        if (priv->bytes_left > 0)
        {
            priv->bytes_left --;
            priv->read_ptr ++;
        }

        player_log(LOG_WARN, "check_mp3_sync_word fail \n");

        return 0;
    }

    /* check whether sample rate change */
    if (priv->frame_info.samprate != priv->current_sample_rate)
    {
        player_log(LOG_INFO, "================== Frame_info Change Notion =================\n");
        player_log(LOG_INFO, "new frame_info=>> bitrate:%d, nChans:%d, samprate:%d, bitsPerSample:%d, outputSamps:%d\n", priv->frame_info.bitrate, priv->frame_info.nChans, priv->frame_info.samprate, priv->frame_info.bitsPerSample, priv->frame_info.outputSamps);
        player_log(LOG_INFO, "new samprate:%d, current_sample_rate:%d\n", priv->frame_info.samprate, priv->current_sample_rate);
        player_log(LOG_INFO, "\n");

        /* update audio frame info */
        codec->info.channel_number = priv->frame_info.nChans;
        codec->info.sample_rate = priv->frame_info.samprate;
        codec->info.sample_bits = priv->frame_info.bitsPerSample;
        codec->info.frame_size = 2 * priv->frame_info.outputSamps;

        priv->current_sample_rate = priv->frame_info.samprate;
    }

#if 0
    if (priv->bytes_left < 1024)
    {
        /* fill more data */
        if (codec_mp3_fill_buffer(codec) != 0)
        {
            return -1;
        }
    }
#endif

    err = MP3Decode(priv->decoder, &priv->read_ptr, (int *)&priv->bytes_left, (short *)buffer, 0);
    priv->frames++;
    if (err != ERR_MP3_NONE)
    {
        player_log(LOG_ERR, "err: %d\n", err);
        switch (err)
        {
            case ERR_MP3_INDATA_UNDERFLOW:
                // LOG_E("ERR_MP3_INDATA_UNDERFLOW.");
                // codec_mp3->bytes_left = 0;
                if (codec_mp3_fill_buffer(codec) != 0)
                {
                    /* release this memory block */
                    return -1;
                }
                break;

            case ERR_MP3_MAINDATA_UNDERFLOW:
                /* do nothing - next call to decode will provide more mainData */
                // LOG_E("ERR_MP3_MAINDATA_UNDERFLOW.");
                goto retry;
                break;

            default:
                // LOG_E("unknown error: %d, left: %d.", err, codec_mp3->bytes_left);
                // LOG_D("stream position: %d.", codec_mp3->parent.stream->position);
                // stream_buffer(0, NULL);

                // skip this frame
                if (priv->bytes_left > 0)
                {
                    priv->bytes_left --;
                    priv->read_ptr ++;
                }
                else
                {
                    // TODO
                    BK_ASSERT(0);
                }
                break;
        }
    }
    else
    {
        int outputSamps;
        /* no error */
        MP3GetLastFrameInfo(priv->decoder, &priv->frame_info);

        /* write to sound device */
        outputSamps = priv->frame_info.outputSamps;
        if (outputSamps > 0)
        {
            /* check output pcm data length, for debug */
            uint32_t out_pcm_len = outputSamps * sizeof(uint16_t);
            if (out_pcm_len > 4608)
            {
                BK_ASSERT(0);
            }
            return outputSamps * sizeof(uint16_t);
        }
        else
        {
            player_log(LOG_ERR, "outputSamps: %d\n", outputSamps);
        }
    }

    return 0;
}

static int mp3_codec_close(audio_codec_t *codec)
{
    mp3_codec_priv_t *priv;
    priv = (mp3_codec_priv_t *)codec->codec_priv;

    player_log(LOG_INFO, "%s \n", __func__);

    if (priv && priv->decoder)
    {
        MP3FreeDecoder(priv->decoder);
        priv->decoder = NULL;
    }

    if (priv->read_buffer)
    {
        player_free(priv->read_buffer);
        priv->read_buffer = NULL;
    }

    return PLAYER_OK;
}

static int mp3_codec_get_chunk_size(audio_codec_t *codec)
{
#if 0
    mp3_codec_priv_t *priv;
    priv = (mp3_codec_priv_t *)codec->codec_priv;

    return 2 * priv->frame_info.outputSamps;
#endif

    (void)codec;
    /* set max value to avoid memory overflow when sample rate change */
    //TODO
    /* bitRate:64000, nChans:2, sampRate:48000, bitsPerSample:16, outputSamps:2304 */
    return 4608;
}

static int calc_mp3_position(audio_codec_t *codec, int second)
{
    mp3_codec_priv_t *priv = (mp3_codec_priv_t *)codec->codec_priv;
    int sample_rate = priv->frame_info.samprate;
    int bit_rate = priv->frame_info.bitrate;
    uint32_t offset = 0;

    /* CBR:                    number_of_frames         * frame_size                     */
    offset = (uint32_t)(second * sample_rate / 1152) * (4 + 144 * bit_rate / sample_rate);
    offset += priv->stream_offset;

    return offset;
}

audio_codec_ops_t mp3_codec_ops =
{
    .open = mp3_codec_open,
    .get_info = mp3_codec_get_info,
    .get_chunk_size = mp3_codec_get_chunk_size,
    .get_data = mp3_codec_get_data,
    .close = mp3_codec_close,
    .calc_position = calc_mp3_position,
};

audio_codec_ops_t *get_mp3_codec_ops(void)
{
    return &mp3_codec_ops;
}


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
#include <components/webclient.h>
#include "ring_buffer.h"
#include "sockets.h"
#include "bk_player_err.h"


#define NET_PIPE_SIZE   (8 * 1024)
#define NET_CHUNK_SIZE  (128)


typedef struct net_source_priv_s
{
    struct webclient_session *session;
    int content_length;
    int codec_type;
    int finish_bytes;

    osal_thread_t tid;
    int runing;
    osal_sema_t thread_sem;
    int result;
    ringbuf_handle_t pipe;
    char *net_buffer;
    int net_buffer_len;
    char *url;
} net_source_priv_t;


#define WEB_RETRY_COUNT         (10)

static void net_set_socket_timeout(struct webclient_session *session, uint32_t timeout_ms)
{
    //webclient_set_timeout(priv->session, 3 * 1000);
    webclient_set_timeout(session, timeout_ms);

    /* add keepalive */
    if (1)
    {
        int res;
        int keepalive = 1;      //Enable keepalive.
        int keepidle = 8;       //idle time is 60s.
        int keepinterval = 3;   //sending interval of detective packet
        int keepcount = 2;      //detective count.
        res = setsockopt(session->socket, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive, sizeof(keepalive));
        if (res < 0)
        {
            player_log(LOG_INFO, "SO_KEEPALIVE %d.", res);
        }
        res = setsockopt(session->socket, IPPROTO_TCP, TCP_KEEPIDLE, (void *)&keepidle, sizeof(keepidle));
        if (res < 0)
        {
            player_log(LOG_INFO, "TCP_KEEPIDLE %d.", res);
        }
        res = setsockopt(session->socket, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepinterval, sizeof(keepinterval));
        if (res < 0)
        {
            player_log(LOG_INFO, "TCP_KEEPINTVL %d.", res);
        }
        res = setsockopt(session->socket, IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepcount, sizeof(keepcount));
        if (res < 0)
        {
            player_log(LOG_INFO, "TCP_KEEPCNT %d.", res);
        }
    }
}


static void *_net_source_bg_thread(void *param)
{
    audio_source_t *source;
    net_source_priv_t *priv;
    int ret;
    int len;
    int pos;
    int retry;

    source = (audio_source_t *)param;
    priv = (net_source_priv_t *)source->source_priv;

    priv->runing = 1;
    retry = WEB_RETRY_COUNT;

    osal_post_sema(&priv->thread_sem);

    net_set_socket_timeout(priv->session, 500);

    while (priv->runing)
    {
        len = webclient_read(priv->session, priv->net_buffer, priv->net_buffer_len);
        if (len == -WEBCLIENT_TIMEOUT)
        {
            continue;
        }
        else if (len > 0)
        {
            priv->finish_bytes += len;
            pos = 0;
            while (pos < len)
            {
                ret = rb_write(priv->pipe, priv->net_buffer + pos, len - pos, 20 / portTICK_RATE_MS);
                if (ret > 0)
                {
                    pos += ret;
                }
                else if (ret == 0 || ret == RB_TIMEOUT)
                {
                    if (priv->runing)
                    {
                        continue;
                    }
                    else
                    {
                        player_log(LOG_INFO, "%s, break ret: %d\n", __func__, ret);
                        break;
                    }
                }
                else if (ret == RB_ABORT)
                {
                    player_log(LOG_INFO, "%s, rb_write abort and break\n", __func__);
                    break;
                }
                else
                {
                    continue;    // TODO
                }
            }
        }
        else if (len == 0)
        {
            priv->result = 1;

            player_log(LOG_INFO, "%s, socket close , content_length=%d, actual_length=%d\n", __func__, priv->content_length, priv->finish_bytes);

            if (priv->content_length > priv->finish_bytes &&  0 < retry--)
            {
                player_log(LOG_WARN, "try again retry:%d\r\n", retry);
                if (priv->session)
                {
                    webclient_close(priv->session);
                    priv->session = webclient_session_create(2048);
                    webclient_get_position(priv->session, priv->url, priv->finish_bytes);
                }
                continue;
            }
            break;
        }
        else
        {
            priv->result = -1;
            player_log(LOG_ERR, "net_source_read failed, ret = %d\n", len);
            break;
        }
    }

    player_log(LOG_INFO, "%s %d thread exit \r\n", __func__, __LINE__);
    //rb_done_write(priv->pipe);
    osal_post_sema(&priv->thread_sem);
    osal_delete_thread(NULL);
    return NULL;
}

static int net_source_start_worker(audio_source_t *source)
{
    int ret = BK_OK;
    net_source_priv_t *priv;

    priv = (net_source_priv_t *)source->source_priv;

    priv->net_buffer_len = NET_CHUNK_SIZE;
    priv->net_buffer = player_malloc(priv->net_buffer_len);
    if (!priv->net_buffer)
    {
        player_log(LOG_ERR, "net_source_start_worker : can't malloc net_buffer\n");
        return PLAYER_ERR_NO_MEM;
    }

    priv->pipe = rb_create_by_mem_type(NET_PIPE_SIZE, RB_MEM_TYPE_PSRAM);
    if (!priv->pipe)
    {
        player_log(LOG_ERR, "net_source_start_worker : can't create pipe\n");
        player_free(priv->net_buffer);
        priv->net_buffer = NULL;
        return PLAYER_ERR_NO_MEM;
    }

    osal_init_sema(&priv->thread_sem, 1, 0);

    ret = osal_create_thread(&priv->tid, (osal_thread_func)_net_source_bg_thread, 4096, "net", source);
    if (ret != BK_OK)
    {
        player_log(LOG_ERR, "net_source_start_worker : can't create thread\n");
        player_free(priv->net_buffer);
        priv->net_buffer = NULL;
        rb_destroy(priv->pipe);
        priv->pipe = NULL;
        return PLAYER_ERR_NO_MEM;
    }

    osal_wait_sema(&priv->thread_sem, BEKEN_WAIT_FOREVER);

    player_log(LOG_INFO, "net_source_start_worker : create thread ok\n");
    return PLAYER_OK;
}

static int net_source_stop_worker(audio_source_t *source)
{
    net_source_priv_t *priv;

    priv = (net_source_priv_t *)source->source_priv;

    priv->runing = 0;
    /* abort write */
    //TODO
    //rb_abort(priv->pipe);
    rb_abort_write(priv->pipe);
    osal_wait_sema(&priv->thread_sem, BEKEN_WAIT_FOREVER);
    osal_deinit_sema(&priv->thread_sem);

    if (priv->net_buffer)
    {
        player_free(priv->net_buffer);
        priv->net_buffer = NULL;
    }
    rb_destroy(priv->pipe);
    return PLAYER_OK;
}


static int net_source_open(char *url, audio_source_t **source_pp)
{
    audio_source_t *source;
    net_source_priv_t *priv;
    int code;
    char *ext_name;
    char *mime;
    int ret;

    if (strncmp(url, "http://", 7) != 0
        || strncmp(&url[os_strlen(url) - 6], ".m3u8", 5) == 0
        || strncmp(&url[os_strlen(url) - 5], ".m3u", 4) == 0
        || strncmp(&url[os_strlen(url) - 5], "m3u8", 4) == 0
        || strncmp(&url[os_strlen(url) - 4], "m3u", 3) == 0)
    {
        return PLAYER_ERR_INVALID_PARAM;
    }

    player_log(LOG_INFO, "net_source_open : %s\n", url);
    source = audio_source_new(sizeof(net_source_priv_t));
    if (!source)
    {
        return PLAYER_ERR_NO_MEM;
    }

    priv = (net_source_priv_t *)source->source_priv;

    priv->finish_bytes = 0;
    priv->session = webclient_session_create(2048);
    if (!priv->session)
    {
        player_log(LOG_ERR, "webclient_session_create() for %s failed\n", url);
        player_free(source);
        source = NULL;
        return PLAYER_ERR_NO_MEM;
    }

    priv->url = player_strdup(url);

    code = webclient_get(priv->session, (const char *)url);
    if (code != 200)
    {
        player_log(LOG_ERR, "webclient_get() for %s failed, code=%d\n", url, code);
        if (priv->session)
        {
            webclient_close(priv->session);
            priv->session = NULL;
        }
        player_free(source);
        source = NULL;
        return PLAYER_ERR_UNKNOWN;
    }
    priv->content_length = webclient_content_length_get(priv->session);
    mime = (char *)webclient_header_fields_get(priv->session, "Content-Type");

    ext_name = os_strrchr(url, '.');
    priv->codec_type = audio_codec_get_type(ext_name);
    if (priv->codec_type == AUDIO_CODEC_UNKNOWN)
    {
        priv->codec_type = audio_codec_get_mime_type(mime);
        if (priv->codec_type == AUDIO_CODEC_UNKNOWN)
        {
            player_log(LOG_ERR, "mime type %s not supported\n", mime);
            if (priv->session)
            {
                webclient_close(priv->session);
                priv->session = NULL;
            }
            player_free(source);
            source = NULL;
            return PLAYER_ERR_UNKNOWN;
        }
    }

    ret = net_source_start_worker(source);
    if (ret != PLAYER_OK)
    {
        if (priv->session)
        {
            webclient_close(priv->session);
            priv->session = NULL;
        }
        player_free(source);
        source = NULL;
        return ret;
    }

    *source_pp = source;

    return PLAYER_OK;
}

static int net_source_close(audio_source_t *source)
{
    net_source_priv_t *priv;

    priv = (net_source_priv_t *)source->source_priv;
    player_log(LOG_INFO, "net_source_close\n");

    net_source_stop_worker(source);

    if (priv->session)
    {
        webclient_close(priv->session);
        priv->session = NULL;
    }

    if (priv->url)
    {
        player_free(priv->url);
        priv->url = NULL;
    }

    return PLAYER_OK;
}

static int net_source_get_codec_type(audio_source_t *source)
{
    net_source_priv_t *priv;

    priv = (net_source_priv_t *)source->source_priv;

    return priv->codec_type;
}

static int net_source_read(audio_source_t *source, char *buffer, int len)
{
    int ret;
    net_source_priv_t *priv;
    priv = (net_source_priv_t *)source->source_priv;

    /*  Check whether net source data read finish.
        If net source data read finish, return 0.
        Otherwise read ringbuffer with timeout.
     */
    if ((priv->content_length > 0) && (priv->content_length <= priv->finish_bytes))
    {
        return 0;
    }
    else
    {
        ret = rb_read(priv->pipe, buffer, len, 100 / portTICK_RATE_MS);
    }

    if (ret > 0)
    {
        return ret;
    }
    else if (ret == RB_TIMEOUT)
    {
        player_log(LOG_WARN, "net_source_read: RB_TIMEOUT\n");
        return PLAYER_ERR_TIMEOUT;
    }
    else
    {
        player_log(LOG_ERR, "net_source_read, ret: %d\n", ret);
        return PLAYER_ERR_UNKNOWN;
    }
}

static int net_source_seek(audio_source_t *source, int offset, uint32_t whence)
{
    net_source_priv_t *priv;
    priv = (net_source_priv_t *)source->source_priv;
    int ret = PLAYER_OK;
    int seek_offset = offset;

    if (whence == SEEK_CUR)
    {
        seek_offset = offset + rb_bytes_filled(priv->pipe);
    }
    else if (whence == SEEK_END)
    {
        if (priv->content_length != -1)
        {
            seek_offset = priv->content_length - offset;
        }
    }

    if (seek_offset > priv->content_length)
    {
        seek_offset = priv->content_length;
    }

    player_log(LOG_DEBUG, "seek offset:%d,offset:%d,whence:%d\r\n", seek_offset, offset, whence);

    if (!priv->runing)
    {
        player_log(LOG_DEBUG, "no runing seek return \r\n");
        return PLAYER_ERR_OPERATION;
    }

    net_source_stop_worker(source);

    if (priv->session)
    {
        webclient_close(priv->session);
        priv->session = webclient_session_create(2048);
        priv->finish_bytes =  seek_offset;
        webclient_get_position(priv->session, priv->url, seek_offset);
    }

    ret = net_source_start_worker(source);
    if (ret)
    {
        if (priv->session)
        {
            webclient_close(priv->session);
            priv->session = NULL;
        }
        player_free(source);
    }
    return ret;
}

audio_source_ops_t net_source_ops =
{
    .open = net_source_open,
    .get_codec_type = net_source_get_codec_type,
    .get_total_bytes = NULL,
    .read = net_source_read,
    .seek = net_source_seek,
    .close = net_source_close,
};

audio_source_ops_t *get_net_source_ops(void)
{
    return &net_source_ops;
}


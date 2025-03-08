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


#ifndef __AUDIO_SOURCE_H__
#define __AUDIO_SOURCE_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    AUDIO_SOURCE_UNKNOWN = 0,

    AUDIO_SOURCE_FILE,
    AUDIO_SOURCE_ARRAY,
} audio_source_type_t;

typedef enum
{
    AUDIO_SOURCE_EVENT_EMPTY = 0,
    AUDIO_SOURCE_EVENT_FAIL,
    AUDIO_SOURCE_EVENT_MAX,
} audio_source_event_t;


typedef struct audio_source_s audio_source_t;

typedef int (*source_out_data_handle)(char *buffer, uint32_t len, void *params);
typedef int (*source_notify)(void *play_ctx, void *params);

typedef struct
{
    char *url;                              /*!< the url need to read, for example: array, file */
    uint32_t total_size;
    uint32_t frame_size;                    /*!< the size (unit byte) of every frame read */
    source_out_data_handle data_handle;
    source_notify notify;
    void *usr_data;
} audio_source_cfg_t;

#define DEFAULT_FRAME_SIZE      (640)

#define DEFAULT_AUDIO_SOURCE_CONFIG() {         \
    .url = NULL,                                \
    .total_size = 0,                            \
    .frame_size = DEFAULT_FRAME_SIZE,           \
    .data_handle = NULL,                        \
    .notify = NULL,                             \
    .usr_data = NULL,                           \
}


typedef struct audio_source_ops_s
{
    int (*open)(audio_source_t *codec, audio_source_cfg_t *config);
//    int (*read)(audio_source_t *source, char *buffer, uint32_t len);
    int (*seek)(audio_source_t *source, int offset, uint32_t whence);
    int (*close)(audio_source_t *source);
} audio_source_ops_t;

struct audio_source_s
{
    audio_source_ops_t *ops;

    audio_source_cfg_t config;

    void *source_ctx;
};


audio_source_t *audio_source_create(  audio_source_type_t source_type, audio_source_cfg_t *config);

bk_err_t audio_source_destroy(audio_source_t *source);

bk_err_t audio_source_open(audio_source_t *source);

bk_err_t audio_source_close(audio_source_t *source);

int audio_source_read_data(audio_source_t *source, char *buffer, int len);

int audio_source_seek(audio_source_t *source, int offset, uint32_t whence);

#ifdef __cplusplus
}
#endif
#endif /* __AUDIO_SOURCE_H__ */


// Copyright 2020-2021 Beken
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

#ifndef __PROMPT_TONE_PLAY_H__
#define __PROMPT_TONE_PLAY_H__


#ifdef __cplusplus
extern "C" {
#endif

#include "audio_source.h"
#include "audio_codec.h"


typedef struct
{
    audio_source_cfg_t source_cfg;
    audio_codec_cfg_t codec_cfg;
} prompt_tone_play_cfg_t;

typedef struct prompt_tone_play *prompt_tone_play_handle_t;

#define DEFAULT_PROMPT_TONE_PLAY_CONFIG() {     \
    .source_cfg = {                             \
        .url = NULL,                            \
        .frame_size = DEFAULT_FRAME_SIZE,       \
        .data_handle = NULL,                    \
        .notify = NULL,                         \
        .usr_data = NULL,                       \
    },                                          \
    .codec_cfg = {                              \
        .chunk_size = DEFAULT_CHUNK_SIZE,       \
        .pool_size = DEFAULT_POOL_SIZE,         \
        .data_handle = NULL,                    \
        .empty_cb = NULL,                       \
        .usr_data = NULL,                       \
    },                                          \
}

/**
 * @brief     Create mp3 player with config
 *
 * This API create mp3 play handle according to config.
 * This API should be called before other api.
 *
 * @param[in] config    Mp3 play config
 *
 * @return
 *    - Not NULL: success
 *    - NULL: failed
 */
prompt_tone_play_handle_t prompt_tone_play_create(  prompt_tone_play_cfg_t *config);

/**
 * @brief      Destroy mp3 play
 *
 * This API Destroy mp3 play according to mp3 play handle.
 *
 *
 * @param[in] mp3_play  The mp3 play handle
 *
 * @return
 *    - BK_OK: success
 *    - Others: failed
 */
bk_err_t prompt_tone_play_destroy(prompt_tone_play_handle_t handle);

/**
 * @brief      Open mp3 play
 *
 * This API open mp3 play and start decode speaker data to play.
 *
 *
 * @param[in] mp3_play  The mp3 play handle
 *
 * @return
 *    - BK_OK: success
 *    - Others: failed
 */
bk_err_t prompt_tone_play_open(prompt_tone_play_handle_t handle);

/**
 * @brief      Close mp3 play
 *
 * This API close mp3 play
 *
 *
 * @param[in] mp3_play  The mp3 play handle
 * @param[in] wait_play_finish  The flag to declare whether waiting play pool data finish
 *
 * @return
 *    - BK_OK: success
 *    - Others: failed
 */
bk_err_t prompt_tone_play_close(prompt_tone_play_handle_t handle, bool wait_play_finish);

#ifdef __cplusplus
}
#endif
#endif /* __PROMPT_TONE_PLAY_H__ */


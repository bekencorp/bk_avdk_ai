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


#ifndef __BK_PLAYER__
#define __BK_PLAYER__

#ifdef __cplusplus
extern "C" {
#endif


#include "bk_player_types.h"
#include "bk_player_err.h"


/**
 * @brief     Create network player
 *
 * This API create network play handle
 * This API should be called before other api.
 *
 *
 * @return
 *    - Not NULL: success
 *    - NULL: failed
 */
bk_player_handle_t bk_player_create(void);

bk_player_state_t bk_player_get_state(bk_player_handle_t player);

/**
 * @brief      Destroy network player
 *
 * This API Destroy network player according to network player handle.
 *
 *
 * @param[in] player  The network player handle
 *
 * @return
 *    - PLAYER_OK: success
 *    - Others: failed
 */
bk_player_err_t bk_player_destroy(bk_player_handle_t player);

/**
 * @brief      Start network player to play music
 *
 * This API Start network player to play uri music.
 *
 * @param[in] player  The network player handle
 * @param[in] param   The control parameter. PLAYER_CTRL_SYNC: sync call, PLAYER_CTRL_ASYNC: async call
 *
 * @return
 *    - PLAYER_OK: success
 *    - Others: failed
 */
bk_player_err_t bk_player_play(bk_player_handle_t player, int param);

/**
 * @brief      Stop network player
 *
 * This API stop network player.
 *
 * @param[in] player  The network player handle
 * @param[in] param   The control parameter. PLAYER_CTRL_SYNC: sync call, PLAYER_CTRL_ASYNC: async call
 *
 * @return
 *    - PLAYER_OK: success
 *    - Others: failed
 */
bk_player_err_t bk_player_stop(bk_player_handle_t player, int param);

/**
 * @brief      Pause network player playback
 *
 * This API pause network player.
 *
 * @param[in] player  The network player handle
 * @param[in] param   The control parameter. PLAYER_CTRL_SYNC: sync call, PLAYER_CTRL_ASYNC: async call
 *
 * @return
 *    - PLAYER_OK: success
 *    - Others: failed
 */
bk_player_err_t bk_player_pause(bk_player_handle_t player, int param);

/**
 * @brief      Set music uri
 *
 * This API set music uri need to play.
 *
 * @param[in] player  The network player handle
 * @param[in] uri     The uri of music
 *
 * @return
 *    - PLAYER_OK: success
 *    - Others: failed
 */
bk_player_err_t bk_player_set_uri(bk_player_handle_t player, const char *uri);

/**
 * @brief      Get music uri
 *
 * This API get music uri that network player play currently.
 *
 * @param[in] player  The network player handle
 *
 * @return      uri of music
 *
 */
const char *bk_player_get_uri(bk_player_handle_t player);

/**
 * @brief      Get music total time
 *
 * This API get music total time.
 *
 * @param[in] player  The network player handle
 * @param[in] timeout The timeout
 *
 * @return      total time
 *
 */
double bk_player_get_total_time(bk_player_handle_t player, int timeout);

/**
 * @brief      Register network player event callback
 *
 * This API register callback to handle event of network player.
 *
 * @param[in] player  The network player handle
 * @param[in] func    The event handle callback
 * @param[in] args    The paramters ptr of callback
 *
 * @return
 *    - PLAYER_OK: success
 *    - Others: failed
 */
bk_player_err_t bk_player_register_event_handle(bk_player_handle_t player, player_event_handle_cb func, void *args);

#ifdef __cplusplus
}
#endif
#endif /* __BK_PLAYER__ */


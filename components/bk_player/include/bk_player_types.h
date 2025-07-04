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


#ifndef __BK_PLAYER_TYPES__
#define __BK_PLAYER_TYPES__

enum player_param
{
    PLAYER_CTRL_ASYNC = 0,
    PLAYER_CTRL_SYNC = 1,
};

typedef enum
{
    PLAYER_STAT_STOPPED = 0,
    PLAYER_STAT_PLAYING,
    PLAYER_STAT_PAUSED,
} bk_player_state_t;

typedef enum
{
    PLAYER_EVENT_START = 0,
    PLAYER_EVENT_STOP,
    PLAYER_EVENT_FAILURE,
    PLAYER_EVENT_PAUSE,
    PLAYER_EVENT_RESUME,
    PLAYER_EVENT_FINISH,

    PLAYER_EVENT_TICK,
    PLAYER_EVENT_READ_DATA_TIMEOUT,
} bk_player_event_t;

typedef struct player *bk_player_handle_t;
typedef int (*player_event_handle_cb)(int, void *, void *);

#endif

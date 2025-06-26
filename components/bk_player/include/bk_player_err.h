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


#ifndef __BK_PLAYER_ERR__
#define __BK_PLAYER_ERR__

/* bk_player error code */
typedef enum
{
    PLAYER_OK                = BK_OK,        /*!< excute success */
    PLAYER_ERR_UNKNOWN       = BK_FAIL,       /*!< unknown error */
    PLAYER_ERR_INVALID_PARAM = -2,       /*!< invalid parameter */
    PLAYER_ERR_NO_MEM        = -3,       /*!< no memory */
    PLAYER_ERR_BUSY          = -4,       /*!< system busy */
    PLAYER_ERR_OPERATION     = -5,       /*!< illegal operation */
    PLAYER_ERR_TIMEOUT       = -6,       /*!< excute timeout */
} bk_player_err_t;

#endif

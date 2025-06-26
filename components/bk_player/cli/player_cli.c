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

#include <os/os.h>
#include <os/str.h>
#include <components/log.h>
#include <driver/pwr_clk.h>
#include "cli.h"
#include "bk_player.h"

#define TAG "player_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


static bk_player_handle_t gl_player = NULL;


static int player_event_handle(int event, void *params, void *args)
{
    os_printf("++>event: %d\n", event);
    return BK_OK;
}

void cli_player_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    int ret = 0;
    int param = 0;

    if (os_strcmp(argv[1], "init") == 0)
    {
        gl_player = bk_player_create();
        if (!gl_player)
        {
            LOGE("player_create fail\n");
        }
        bk_player_register_event_handle(gl_player, player_event_handle, NULL);
    }
    else if (os_strcmp(argv[1], "deinit") == 0)
    {
        bk_player_destroy(gl_player);
        gl_player = NULL;
    }
    else if (os_strcmp(argv[1], "gain") == 0)
    {
    }
    else if (os_strcmp(argv[1], "play") == 0)
    {
        param = os_strtoul(argv[2], NULL, 10);
        ret = bk_player_play(gl_player, param);
    }
    else if (os_strcmp(argv[1], "total_time") == 0)
    {
        //ret = player_get_total_time(gl_player, 0);
    }
    else if (os_strcmp(argv[1], "stop") == 0)
    {
        param = os_strtoul(argv[2], NULL, 10);
        ret = bk_player_stop(gl_player, param);
    }
    else if (os_strcmp(argv[1], "pause") == 0)
    {
        param = os_strtoul(argv[2], NULL, 10);
        ret = bk_player_pause(gl_player, param);
    }
    else if (os_strcmp(argv[1], "resume") == 0)
    {
        param = os_strtoul(argv[2], NULL, 10);
        ret = bk_player_play(gl_player, param);
    }
    else if (os_strcmp(argv[1], "set_url") == 0)
    {
        ret = bk_player_set_uri(gl_player, argv[2]);
    }
    else if (os_strcmp(argv[1], "get_url") == 0)
    {
        const char *uri = bk_player_get_uri(gl_player);
        LOGI("uri: %s\n", uri);
    }
    else if (os_strcmp(argv[1], "get_state") == 0)
    {
        bk_player_state_t state = bk_player_get_state(gl_player);
        LOGI("state: %d\n", state);
    }
    else
    {
        //nothing
    }

    LOGI("play ret=%d\n", ret);
}

#define VOICE_CMD_CNT  (sizeof(s_player_commands) / sizeof(struct cli_command))

static const struct cli_command s_player_commands[] =
{
    {"player", "player", cli_player_cmd},
};

int cli_player_init(void)
{
    return cli_register_commands(s_player_commands, VOICE_CMD_CNT);
}


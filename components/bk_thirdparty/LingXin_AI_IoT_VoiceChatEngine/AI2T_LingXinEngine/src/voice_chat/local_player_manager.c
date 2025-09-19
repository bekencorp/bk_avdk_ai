#include "lingxin_local_player.h"
#include "lingxin_log.h"
#include "chat_state_machine.h"

static int initial_volume = 80;
static lingxin_local_player_t welcome_audio_player = NULL;
static lingxin_local_player_t terminate_audio_player = NULL;
static lingxin_local_player_t continue_audio_player = NULL;
static char *welcome_audio_path = NULL;
static char *terminate_audio_path = NULL;
static char *continue_audio_path = NULL;

void module_local_play_set_welcome_audio_path(char *audio_path) {
    welcome_audio_path = audio_path;
}
void module_local_play_set_terminate_audio_path(char *audio_path) {
    terminate_audio_path = audio_path;
}
void module_local_play_set_continue_audio_path(char *audio_path) {
    continue_audio_path = audio_path;
}
static void create_player_and_start(lingxin_local_player_t player, char *audio_path, lingxin_local_player_callback_t callback) {
    if (audio_path) {
        lingxin_local_player_play_param_t param = {
            .audio_path = audio_path,
            .initial_volume = initial_volume,
        };
        player = lingxin_local_player_create();
        lingxin_local_player_play(player, &param, callback);
    } else {
        callback(0); // 如果audio_path为空，则直接执行回调函数
    }
}
static void destory_player(lingxin_local_player_t player) {
    if (player) {
        lingxin_local_player_destory(player);
        player = NULL;
    }
}

static void play_welcome_audio_callback(int result) {
    lingxin_log_debug("%s 播放成功 State_Event_Welcome_Play_End", __func__);
    state_machine_run_event(State_Event_Welcome_Play_End);
    destory_player(welcome_audio_player);
}
static void play_terminate_audio_callback(int result) {
    lingxin_log_debug("%s 播放成功 State_Event_TerminatePrompt_PlayEnd", __func__);
    state_machine_run_event(State_Event_TerminatePrompt_PlayEnd);
    destory_player(terminate_audio_player);
}
static void play_continue_audio_callback(int result) {
    lingxin_log_debug("%s 播放成功 State_Event_ContinuePrompt_PlayEnd", __func__);
    state_machine_run_event(State_Event_ContinuePrompt_PlayEnd);
    destory_player(continue_audio_player);
}

void module_local_play_welcome_audio() {
    create_player_and_start(welcome_audio_player, welcome_audio_path, play_welcome_audio_callback);
}
void module_local_play_terminate_audio() {
    create_player_and_start(terminate_audio_player, terminate_audio_path, play_terminate_audio_callback);
}
void module_local_play_continue_audio() {
    create_player_and_start(continue_audio_player, continue_audio_path, play_continue_audio_callback);
}

void module_local_play_set_volume(int volume) 
{
    lingxin_log_debug("local play set volume to %d", volume);
    initial_volume = volume;
    
    if (welcome_audio_player) {
        lingxin_local_player_set_volume(welcome_audio_player, volume);
    }
    if (terminate_audio_player) {
        lingxin_local_player_set_volume(terminate_audio_player, volume);
    }
    if (continue_audio_player) {
        lingxin_local_player_set_volume(continue_audio_player, volume);
    }
}


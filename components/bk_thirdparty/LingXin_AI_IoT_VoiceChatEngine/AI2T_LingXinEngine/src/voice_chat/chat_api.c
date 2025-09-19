#include "chat_state_machine.h"
#include "schedule_timer_manager.h"
#include "lingxin_log.h"
#include <stdio.h>
#include <string.h>
#include "lingxin_auth_config.h"
#include "lingxin_recorder_manager.h"
#define LINGXIN_PROPS_INIT_TAG "lingxin_props_init_tag"

// 标记是否初始化
static int is_inited = 0;

// 初始化方法
VoiceChatInitProps get_voice_chat_init_default_props() {
    VoiceChatInitProps props = {0};
    props.is_schedule_task_on = 1;
    props.props_init_tag = LINGXIN_PROPS_INIT_TAG;
    return props;
}
int voice_chat_init(VoiceChatInitProps *init_props) {
    lingxin_log_debug("对话模式初始化");

    // 检测传参是否合法
    if (init_props && strcmp(init_props->props_init_tag, LINGXIN_PROPS_INIT_TAG) != 0) {
        lingxin_log_error("对话模式初始化传参不合法");
        return -1;
    }
    // 检测传参是否为空
    if (!init_props) {
        lingxin_log_error("对话模式初始化参数为空");
        return -1;
    }

    /******************************** 鉴权初始化 ********************************/
    // 注册动态获取appId、appKey、sn、agentCode的函数
    if (init_props && init_props->auth_app_id_get_func
        && init_props->auth_app_key_get_func
        && init_props->auth_sn_get_func
        && init_props->auth_agent_code_get_func) {
        inner_register_auth_get_func(
            init_props->auth_app_id_get_func,
            init_props->auth_app_key_get_func,
            init_props->auth_sn_get_func,
            init_props->auth_agent_code_get_func);
    } else {
        lingxin_log_error("存在空的鉴权获取函数，对话模式初始化失败");
        return -1;
    }
    /******************************** 状态机初始化 ********************************/
    bool enable_terminate_audio = init_props->terminate_audio_path != NULL;
    bool enable_continue_audio = init_props->continue_audio_path != NULL;
    // 注册chat生命周期监听函数
    if (init_props->chat_life_cycle_event_listener) {
        voice_chat_machine_init(init_props->chat_life_cycle_event_listener, enable_terminate_audio, enable_continue_audio);
    } else {
        voice_chat_machine_init(NULL, enable_terminate_audio, enable_continue_audio);
    }

    /******************************** 录音模块初始化 ********************************/
    module_record_manager_init(init_props->send_uni_size, init_props->send_cbuf_scale);

    /******************************** 本地播放模块初始化 ********************************/
    // 设置开场白音频的路径
    if (init_props->welcome_audio_path) {
        module_local_play_set_welcome_audio_path(init_props->welcome_audio_path);
    }
    // 设置打断时音频的路径
    if (init_props->terminate_audio_path) {
        module_local_play_set_terminate_audio_path(init_props->terminate_audio_path);
    }
    // 设置连续对话进入下一轮对话前音频的路径
    if (init_props->continue_audio_path) {
        module_local_play_set_continue_audio_path(init_props->continue_audio_path);
    }

    /******************************** 定时任务模块初始化 ********************************/
    // 设置是否开启定时任务
    if(init_props->is_schedule_task_on) {
        lingxin_log_debug("定时任务开启");
        module_schedule_init();
    } else {
        lingxin_log_debug("定时任务关闭");
    }

    // 标记初始化完成
    is_inited = 1;
    return 0;
}

/**
 * 进入对话模式
 */
StartNewChatProps get_start_new_chat_default_props() {
    StartNewChatProps props = {0};
    props.props_init_tag = LINGXIN_PROPS_INIT_TAG;
    return props;
}
int start_new_chat(StartNewChatProps *start_props) {
    lingxin_log_debug("进入触发唤醒事件");

    // 检测是否初始化
    if (!is_inited) {
        lingxin_log_error("对话模式未初始化，请先进行初始化");
        return -2;
    }

    // 检测传参是否合法
    if (start_props && strcmp(start_props->props_init_tag, LINGXIN_PROPS_INIT_TAG) != 0) {
        lingxin_log_error("进入对话模式传参不合法");
        return -1;
    }

    // 构造payload
    WakeupDetectedPayload wakeup_detected_payload = {0}; 
    if (start_props) {
        wakeup_detected_payload.disable_welcome_audio = start_props->disable_welcome_audio;
        wakeup_detected_payload.disable_vad = start_props->disable_vad;
        wakeup_detected_payload.task_id = start_props->task_id;
    }
    StateEventPayload payload = {
        .wakeup_detected_payload = &wakeup_detected_payload
    };

    // 发送事件
    state_machine_run_event_with_payload(State_Event_Wakeup_Detected, &payload); 
    return 0;
}

/**
 * 用户主动调用停止录音
 */
int stop_chat_record(StopChatRecordProps *stop_record_props) {
    lingxin_log_debug("用户主动调用停止录音");
    // 构造payload
    VadStopPayload vad_stop_payload = {
        .disable_vad = true
    };
    StateEventPayload payload = {
        .vad_stop_payload = &vad_stop_payload
    };
    state_machine_run_event_with_payload(State_Event_Vad_Stop, &payload);
    return 0;
}

/**
 * 退出对话模式
 */
ExitChatProps get_exit_chat_default_props() {
    ExitChatProps props = {0};
    props.props_init_tag = LINGXIN_PROPS_INIT_TAG;
    return props;
}
int exit_chat(ExitChatProps *exit_props) {
    lingxin_log_debug("退出对话模式");

    // 检测传参是否合法
    if (exit_props && strcmp(exit_props->props_init_tag, LINGXIN_PROPS_INIT_TAG) != 0) {
        lingxin_log_error("退出对话模式传参不合法");
        return -1;
    }

    // 构造payload
    WillExitPayload will_exit_payload = {0};
    if (exit_props) {
        will_exit_payload.disable_close_ws_immediately = exit_props->disable_close_ws_immediately;
    }
    StateEventPayload payload = {
        .will_exit_payload = &will_exit_payload
    };

    // 发送事件
    state_machine_run_event_with_payload(State_Event_WillExit, &payload);
    return 0;
}

/**
 * 加音量
 */
int set_volume(int volume) {
    module_bufferPlay_setVolume(volume);
    module_local_play_set_volume(volume);
    return 0;
}
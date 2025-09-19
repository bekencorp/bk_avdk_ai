#include "chat_state_machine.h"
#include "schedule_timer_manager.h"
#include "lingxin_semaphore.h"
#include "lingxin_common.h"
#include <stddef.h>
#include <stdio.h>
#include "lingxin_log.h"
#include "lingxin_recorder_manager.h"


// 全局上下文访问
static ChatStateRuntimeContext g_context = {0};

typedef enum
{
    CONTROL_AUTHORITY_STATE_MACHINE, // 主控权在状态机
    CONTROL_AUTHORITY_MODULE         // 主控权在模块（如SDK）
} ControlAuthority;
static ControlAuthority control_state = CONTROL_AUTHORITY_STATE_MACHINE;

static ChatState chat_current_state = State_Idle;
static ChatState chat_last_state = State_Idle;


static ChatContinueParams voice_chat_continue_params = {true, true, NULL};

static ChatLifeCycleEventListener chat_event_listenner = NULL;
static ChatLifeCycleEventListener emit_chat_event(ChatLifeCycleEvent event, void *payload) {
    if (chat_event_listenner) {
        chat_event_listenner(event, payload); // 给对话生命周期监听函数加一层非空校验
    }
    return chat_event_listenner;
}

typedef struct
{
    bool record_started;
    bool voice_chat_started;
} InnerStateForStart;
static InnerStateForStart inner_state_for_start = {false, false};

typedef struct
{
    bool record_terminated;
    bool buffer_play_terminated;
} InnerStateForTerminate;
static InnerStateForTerminate inner_state_for_terminate = {false, false};
typedef struct
{
    bool record_terminated;
    bool buffer_play_terminated;
} InnerStateForNoVoiceTerminate;
static InnerStateForNoVoiceTerminate inner_state_for_novoice_terminate = {false, false};

typedef struct
{
    bool play_terminate_prompted;
    bool record_restarted;
    bool voice_chat_terminated;
    bool voice_chat_restarted;
} InnerStateForTerminateRestart;
static InnerStateForTerminateRestart inner_state_for_terminate_restart = {false, false, false};
typedef struct
{
    bool voice_chat_terminated;
    bool voice_chat_restarted;
    bool buffer_play_ended;
} InnerStateForNoVoiceTerminateRestart;
static InnerStateForNoVoiceTerminateRestart inner_state_for_novoice_terminate_restart = {false, false, false};

typedef struct
{
    bool record_terminated;
    bool buffer_play_terminated;
    bool voice_chat_terminated;
    bool voice_chat_exited;
    bool is_normal_exit; // 是否是用户主动退出对话模式
} InnerStateForExit;
static InnerStateForExit inner_state_for_exit = {false, false, false, false, true};

// 内部状态
typedef struct
{
    InnerStateForExit *inner_state_for_exit; // 退出中状态的内部状态
} InnerStateCollection;

//static char *current_task_id = NULL; // 注释掉未使用的变量

/**
 * voice chat交互逻辑
 */

// 开始对话逻辑
static void wanson_voiceChatContinueCallback()
{
    lingxin_log_debug("dlu:2-1 wanson_voiceChatContinueCallback");

    state_machine_run_event(State_Event_VoiceChat_StartEnd);
}
// 在定时任务触发失败时回调拉起一轮新的voice循环
static void noVoiceErrorCallback()
{
    lingxin_log_debug("noVoiceErrorCallback\n");
    // 通知定时任务模块定时任务触发失败
    recieve_schedule_task_error();
}
// 向用户暴露关闭tts/asr的时机
static void noVoiceTerminateEndCallback()
{
    lingxin_log_debug("noVoiceTerminateEndCallback\n");
    state_machine_run_event(State_Event_NoVoice_TerminateEnd);
}
// 请求服务端连续对话
static void voice_chat_continue(bool is_create_voice_task)
{
    lingxin_log_debug("voice_chat_continue");
    voice_chat_continue_params.isCreateVoiceTask = is_create_voice_task;
    if (is_create_voice_task) {
        voiceChatContinueCheck(&voice_chat_continue_params, wanson_voiceChatContinueCallback, NULL); // 普通连续对话
    } else {
        voiceChatContinueCheck(&voice_chat_continue_params, wanson_voiceChatContinueCallback, noVoiceErrorCallback); // 定时任务对话
    }
    // 重置连续对话参数
    voice_chat_continue_params.isCreateVoiceTask = true;
    voice_chat_continue_params.useServerVad = true;
    voice_chat_continue_params.taskId = NULL;
}
void set_voice_chat_continue_params(StateEventPayload *payload)
{
    lingxin_log_debug("set_voice_chat_continue_params");
    if (payload && payload->wakeup_detected_payload) {
        voice_chat_continue_params.useServerVad = !payload->wakeup_detected_payload->disable_vad;
        voice_chat_continue_params.taskId = payload->wakeup_detected_payload->task_id;
    } else {
        voice_chat_continue_params.useServerVad = true;
        voice_chat_continue_params.taskId = NULL;
    }
}

// 根据枚举值返回描述字符串
static const inline char *get_chat_state_description(ChatState state)
{
    switch (state)
    {
    case State_Idle:
        return "State_Idle  等待唤醒态";
    case State_Welcome:
        return "State_Welcome  欢迎语播放态";
    case State_Terminate:
        return "State_Terminate 打断状态";
    case State_Start:
        return "State_Start 新一轮对话开始，录音开始";
    case State_Binary_Transfer:
        return "State_Binary_Transfer 录音传输态";
    case State_End_Audio:
        return "State_End_Audio 结束录音态";
    case State_BufferPlay_Start:
        return "State_BufferPlay_Start 流式播放开始态";
    case State_BufferPlay_Play:
        return "State_BufferPlay_Play 流式播放中";
    case State_BufferPlay_EndTask:
        return "State_BufferPlay_EndTask 结束一轮对话";
    case State_Exit:
        return "State_Exit 主动结束对话";
    case State_NoVoice_Start:
        return "State_NoVoice_Start 新一轮noVoice循环开始";
    case State_NoVoice_Terminate:
        return "State_NoVoice_Terminate 打断当前循环，开启新一轮noVoice循环";
    default:
        lingxin_log_warn("%s 未知状态 %d", __func__, state);
        return "未知状态";
    }
}

static const inline char *get_wakeup_event_description(StateEvent event)
{
    switch (event)
    {
    case State_Event_Wakeup_Detected:
        return "State_Event_Wakeup_Detected 唤醒事件";
    case State_Event_NoVoice_Error:
        return "State_Event_NoVoice_Error noVoice下行阶段服务端推送error";
    case State_Event_NoVoice_Start:
        return "State_Event_NoVoice_Start 开启新一轮noVoice循环";
    case State_Event_Welcome_Play_End:
        return "State_Event_Welcome_Play_End 播放欢迎语结束事件";
    case State_Event_VoiceChat_StartEnd:
        return "State_Event_VoiceChat_StartEnd  voice chat 对话成功事件";
    case State_Event_VoiceChat_TerminateEnd:
        return "State_Event_VoiceChat_TerminateEnd voice chat voice chat 打断成功事件";
    case State_Event_NoVoice_TerminateEnd:
        return "State_Event_NoVoice_TerminateEnd 拉起新一轮noVoice前打断成功事件";
    case State_Event_VoiceChat_AIStart:
        return "State_Event_VoiceChat_AIStart 服务端开始推音频流事件";
    case State_Event_VoiceChat_AIEnd:
        return "State_Event_VoiceChat_AIEnd 服务端结束推音频流事件";
    case State_Event_Vad_Stop:
        return "State_Event_Vad_Stop vad 停止";
    case State_Event_Vad_Exit:
        return "State_Event_Vad_Exit 退出唤醒";
    case State_Event_BufferPlay_AudioInitEnd:
        return "State_Event_BufferPlay_AudioInitEnd 流式播放初始化结束";
    case State_Event_BufferPlay_PlayEnd:
        return "State_Event_BufferPlay_PlayEnd 流式播放结束";
    case State_Event_BufferPlay_ServerClosed:
        return "State_Event_BufferPlay_ServerClosed audio_server服务关闭";
    case State_Event_BufferPlay_TerminateEnd:
        return "State_Event_BufferPlay_TerminateEnd 流式播放打断后暂停";
    case State_Event_BufferPlay_Error:
        return "State_Event_BufferPlay_Error 流式播放模块出错";
    case State_Event_Record_Ready:
        return "State_Event_Record_Ready 录音模块初始化成功";
    case State_Event_Record_Stop:
        return "State_Event_Record_Stop 录音模块停止录音";
    case State_Event_Record_TerminateEnd:
        return "State_Event_Record_TerminateEnd 录音模块打断事件";
    case State_Event_VoiceChat_ExitEnd:
        return "State_Event_VoiceChat_ExitEnd voice chat引擎销毁成功";
    case State_Event_WillExit:
        return "State_Event_WillExit 主动退出对话事件";
    case State_Event_TerminatePrompt_PlayEnd:
        return "State_Event_TerminatePrompt_PlayEnd 打断唤醒提示音播放成功";  // 增加打断唤醒提示音
    case State_Event_ContinuePrompt_PlayEnd:
        return "State_Event_ContinuePrompt_PlayEnd 连续对话提示音播放成功";   // 增加连续对话提示音
    default:
        lingxin_log_warn("%s 未知事件 %d", __func__, event);
        return "未知事件";
    }
}

// 更新主控权逻辑
static void updateControl(ControlAuthority control)
{
    control_state = control;

    if (control_state == CONTROL_AUTHORITY_STATE_MACHINE)
    {
        // TODO：主控权在状态机的时候，处理打断逻辑
    }
}

static void updateChatState(ChatState state, StateEvent event)
{
    chat_last_state = chat_current_state;

    chat_current_state = state;
    lingxin_log_debug("%s 从 %s 更新到 %s, 触发事件: %s", __func__, get_chat_state_description(chat_last_state), get_chat_state_description(chat_current_state), get_wakeup_event_description(event));
}

ChatState get_current_chat_state()
{
    return chat_current_state;
}

bool get_chat_state_terminate()
{
    return (chat_current_state == State_Terminate && !inner_state_for_terminate_restart.voice_chat_terminated) || chat_current_state == State_Exit || (chat_current_state == State_NoVoice_Terminate && !inner_state_for_novoice_terminate_restart.voice_chat_terminated);
}

/**
 * 新重构之后的接口
 */
// 定时任务触发事件
void schedule_emit() {
    emit_chat_event(CHAT_LIFE_CYCLE_EVENT_SCHEDULE_EMIT, NULL);
}
// 初始化 machine
void voice_chat_machine_init(ChatLifeCycleEventListener event_listener, bool need_terminate_prompt, bool need_continue_prompt)
{
    chat_event_listenner = event_listener;
    setNoVoiceListner(schedule_emit); // 将用户在拉起noVoice前的自定义二开方法注到schedule中
    g_context.need_terminate_prompt = need_terminate_prompt;
    g_context.need_continue_prompt = need_continue_prompt;
}
static void clear_all_inner_state()
{
    memset(&inner_state_for_start, 0, sizeof(InnerStateForStart));
    memset(&inner_state_for_terminate, 0, sizeof(InnerStateForTerminate));
    memset(&inner_state_for_terminate_restart, 0, sizeof(InnerStateForTerminateRestart));

    memset(&inner_state_for_novoice_terminate, 0, sizeof(InnerStateForNoVoiceTerminate));
    memset(&inner_state_for_novoice_terminate_restart, 0, sizeof(InnerStateForNoVoiceTerminateRestart));

    memset(&inner_state_for_exit, 0, sizeof(InnerStateForExit));
}
static void set_preset_inner_state(InnerStateCollection *preset_inner_state)
{
    if (preset_inner_state) {
        if (preset_inner_state->inner_state_for_exit) {
            if (preset_inner_state->inner_state_for_exit->voice_chat_exited) {
                inner_state_for_exit.voice_chat_exited = true;
            }
            if (preset_inner_state->inner_state_for_exit->voice_chat_terminated) {
                inner_state_for_exit.voice_chat_terminated = true;
            }
        }
    }
    return;
}

static void turn_to_with_preset_inner_state(ChatState state, StateEvent event, InnerStateCollection *preset_inner_state)
{
    updateControl(CONTROL_AUTHORITY_STATE_MACHINE);

    if (preset_inner_state && preset_inner_state->inner_state_for_exit && preset_inner_state->inner_state_for_exit->is_normal_exit) {
        lingxin_log_debug("%s 当前是用户主动退出对话模式, is_normal_exit: %d", __func__, preset_inner_state->inner_state_for_exit->is_normal_exit);
        g_context.is_normal_exit = true;
    }
    else {
        g_context.is_normal_exit = false;
    }
    clear_all_inner_state(); // 状态机切换状态时，清除所有内部状态
    set_preset_inner_state(preset_inner_state);
    updateChatState(state, event);
    switch (state)
    {
    case State_Idle:
    {
        lingxin_websocket_control_del();
        break;
    }
    case State_Welcome:
    {
        lingxin_websocket_control_create();
        module_local_play_welcome_audio();
        break;
    }
    case State_Start:
    {
        lingxin_log_debug("从%s 调用 module_record_init调用", __func__);
        module_record_init();      // 初始化录音
        voice_chat_continue(true); // 请求服务端连续对话
        break;
    }
    case State_NoVoice_Start:
    {
        noVoiceTerminateEndCallback();
        // 传入待触发的定时任务id，获取config
        voice_chat_continue(false); // 请求服务端连续对话
        break;
    }
    case State_Binary_Transfer:
    {
        module_record_start_send(); // 录音开始
        break;
    }
    case State_NoVoice_Terminate:
    case State_Terminate:
    case State_Exit:
    {
        module_record_terminate();     // 先停止录音
        module_bufferPlay_terminate(); // 暂停播放
        break;
    }
    default:
        break;
    }
    updateControl(CONTROL_AUTHORITY_MODULE);
}
void turn_to(ChatState state, StateEvent event) {
    turn_to_with_preset_inner_state(state, event, NULL);
}

// 从 State_Start 到 State_Binary_Transfer 的中间状态处理
static void from_start_to_binary_transfer(InnerStateForStart *inner_state, StateEvent event)
{
    switch (event)
    {
    case State_Event_Record_Ready:
        inner_state->record_started = true;
        break;
    case State_Event_VoiceChat_StartEnd:
        inner_state->voice_chat_started = true;
        break;
    default:
        break;
    }
    if (inner_state->record_started && inner_state->voice_chat_started)
    {
        turn_to(State_Binary_Transfer, event);
    }
}
// 从 State_Terminate 到 State_Binary_Transfer 的2个中间状态处理
static void from_terminate_to_restart(InnerStateForTerminate *inner_state, StateEvent event)
{
    switch (event)
    {
    case State_Event_Record_TerminateEnd:
        inner_state->record_terminated = true;
        break;
    case State_Event_BufferPlay_TerminateEnd:
        inner_state->buffer_play_terminated = true;
        break;
    default:
        break;
    }

    if (inner_state->record_terminated && inner_state->buffer_play_terminated)
    {
        
        lingxin_websocket_control_create();
        lingxin_unlock_write_websocket_controle();
        if (g_context.need_terminate_prompt) {
            module_local_play_terminate_audio();
        }
        else {
            lingxin_log_debug("从%s 调用 module_record_init 调用", __func__);
            module_record_init(); // 初始化录音
            module_voiceChat_terminate();
        }
    }
}

static void from_terminate_restart_to_binary_transfer(InnerStateForTerminateRestart *inner_state, StateEvent event)
{
    switch (event)
    {
    case State_Event_TerminatePrompt_PlayEnd:
        inner_state->play_terminate_prompted = true;
        lingxin_log_debug("从%s 调用 module_record_init 调用", __func__);
        module_record_init(); // 初始化录音
        module_voiceChat_terminate();
        break;
    case State_Event_Record_Ready:
        inner_state->record_restarted = true;
        break;
    case State_Event_VoiceChat_TerminateEnd:
        inner_state->voice_chat_terminated = true;
        voice_chat_continue(true);
        break;
    case State_Event_VoiceChat_StartEnd:
        inner_state->voice_chat_restarted = true;
    default:
        break;
    }
    if (inner_state->record_restarted && inner_state->voice_chat_terminated && inner_state->voice_chat_restarted)
    {
        turn_to(State_Binary_Transfer, event);
    }
}
// 从State_NoVoice_Start到等待服务端发送音频数据
static void from_novoice_start_to_wait_buffer()
{
    lingxin_websocket_control_create();
}
// 从 State_NoVoice_Terminate 到 等待服务端发送音频数据 的2个中间状态处理
static void from_novoice_terminate_to_restart(InnerStateForNoVoiceTerminate *inner_state, StateEvent event)
{
    switch (event)
    {
    case State_Event_Record_TerminateEnd:
        inner_state->record_terminated = true;
        break;
    case State_Event_BufferPlay_TerminateEnd:
        inner_state->buffer_play_terminated = true;
        break;
    default:
        break;
    }
    if (inner_state->record_terminated && inner_state->buffer_play_terminated)
    {
        
        module_voiceChat_terminate();
    }
}

static void from_novoice_terminate_restart_to_wait_buffer(InnerStateForNoVoiceTerminateRestart *inner_state, StateEvent event)
{
    switch (event)
    {
    case State_Event_VoiceChat_TerminateEnd:
        lingxin_log_debug("接收到terminateEnd");
        if (inner_state->voice_chat_terminated != true)
        {
            inner_state->voice_chat_terminated = true;
        }
        break;
    case State_Event_BufferPlay_ServerClosed:
        inner_state->buffer_play_ended = true;
        break;
    case State_Event_VoiceChat_StartEnd:
        inner_state->voice_chat_restarted = true;
    default:
        break;
    }

    if (inner_state->voice_chat_terminated && inner_state->buffer_play_ended)
    {
        lingxin_log_debug("发送启动noVoice的start_task");
        noVoiceTerminateEndCallback();
        // 传入当前待触发的定时任务id
        voice_chat_continue(false);
    }
}
// 从 State_Exit 到 State_Idle 的中间状态处理
static void from_exit_to_idle(InnerStateForExit *inner_state, StateEvent event)
{
    switch (event)
    {
    case State_Event_Record_TerminateEnd:
        inner_state->record_terminated = true;
        break;
    case State_Event_BufferPlay_TerminateEnd:
        inner_state->buffer_play_terminated = true;
        if (!inner_state->voice_chat_terminated) {
            module_voiceChat_terminate();
        }

        break;
    case State_Event_VoiceChat_TerminateEnd:
        inner_state->voice_chat_terminated = true;
        break;
    case State_Event_VoiceChat_ExitEnd:
        inner_state->voice_chat_exited = true;
    default:
        break;
    }
    if (inner_state->record_terminated && inner_state->buffer_play_terminated && inner_state->voice_chat_terminated)
    {
        if (inner_state->voice_chat_exited)
        {
            turn_to(State_Idle, event);
            ExitPayload payload = {EXIT_REASON_USER_INITIATED};
            if (g_context.exit_code >= 0) {
                lingxin_log_debug("使用退出时的错误信息 exit_code %d", g_context.exit_code);
                payload.exit_code = g_context.exit_code; // 使用退出时的错误信息

                g_context.exit_code = EXIT_REASON_USER_INITIATED; // 清除退出时的错误信息
            }
            emit_chat_event(CHAT_LIFE_CYCLE_EVENT_EXIT, &payload); // 添加结束回调
            g_context.is_normal_exit = false;                      // 重置主动退出标志
        }
        else
        {
            module_voiceChat_exit();
        }
    }
}

void state_machine_receive_error(ExitCode exit_code)
{
    if (g_context.is_normal_exit) {
        lingxin_log_debug("%s 当前是用户主动退出对话模式，不处理错误事件 %d", __func__, exit_code);
        return; // 如果当前是用户主动退出对话模式，不处理错误事件
    }

    if (exit_code == EXIT_REASON_WEBSOCKET_DISCONNECT || exit_code == EXIT_REASON_WEBSOCKET_CONNECTION_FAILED) {
        
        lingxin_log_error("赋值之前的 g_context.exit_code %d", g_context.exit_code);

        g_context.exit_code = exit_code; // 保存退出时的错误信息

        lingxin_log_error("赋值之后的 g_context.exit_code %d", g_context.exit_code);


        // 如果是websocket连接失败或者销毁，直接进入退出状态
        lingxin_log_error("接收到websocket连接失败或者销毁的错误，进入退出状态 %d", g_context.exit_code);
        InnerStateForExit inner_state = {false, false, true, true, false};
        InnerStateCollection inner_state_collection = {
            .inner_state_for_exit = &inner_state,
        };
        turn_to_with_preset_inner_state(State_Exit, State_Event_WillExit, &inner_state_collection);
    }
}

static int module_timeout_need_callback = 0;

static void module_timeout_handler() {
    // 改成分步执行
    InnerStateForExit inner_state = {false, false, false, true, true};
    InnerStateCollection inner_state_collection = {
        .inner_state_for_exit = &inner_state,
    };
    turn_to_with_preset_inner_state(State_Exit, State_Event_WillExit, &inner_state_collection);
}

static void timer_callback() {
    lingxin_log_debug("定时任务执行成功");

    if (module_timeout_need_callback) {
        lingxin_log_debug("打断功能超时，定时任务执行退出成功");
        module_timeout_handler();
    }

    delete_lingxin_chat_timer();
}

// 外部调用：兼容老的不带payload的调用
void state_machine_run_event(StateEvent event)
{
    if (chat_current_state == State_Idle && (event != State_Event_Wakeup_Detected && event != State_Event_NoVoice_Start ))
    {
        lingxin_log_debug("State_Idle 不再接受别的事件, %s 接受到事件 %s，当前状态 %s, ", __func__, get_wakeup_event_description(event), get_chat_state_description(chat_current_state));
        return;
    }
    state_machine_run_event_with_payload(event, NULL);
}
// 外部调用：事件流转
void state_machine_run_event_with_payload(StateEvent event, StateEventPayload *payload)
{
    lingxin_log_debug("%s 接受到事件 %s，当前状态 %s", __func__, get_wakeup_event_description(event), get_chat_state_description(chat_current_state));
    switch (event)
    {

    // 唤醒事件
    case State_Event_Wakeup_Detected:
    {
        if (chat_current_state == State_Exit || chat_current_state == State_Welcome || chat_current_state == State_NoVoice_Terminate || chat_current_state == State_NoVoice_Start)
        {
            lingxin_log_debug("不允许唤醒打断, 当前状态为%s", get_chat_state_description(chat_current_state));
            return;
        }
        else if (chat_current_state == State_Idle)
        {
            lingxin_log_debug("首次唤醒 %s", get_wakeup_event_description(event));  // 首次唤醒
            g_context.is_normal_exit = false;                                      // 重置主动退出标志
            set_voice_chat_continue_params(payload);
            bool disable_welcome_audio = payload && payload->wakeup_detected_payload && payload->wakeup_detected_payload->disable_welcome_audio;
            disable_welcome_audio ? turn_to(State_Start, event) : turn_to(State_Welcome, event);
        }
        else
        {
            // TODO: 撤掉注释
            if (isVoiceChatInited() == false) {
                lingxin_log_warn("voice chat未初始化完成，不允许唤醒打断");
                return; // 如果voice chat没有初始化完成，则不允许唤醒打断
            }
            module_timeout_need_callback = 1;
            if (get_chat_state_terminate())
            {
                lingxin_log_warn("重复打断唤醒 %s", get_wakeup_event_description(event));
                reset_lingxin_chat_timer_run(); // 重置定时任务
            }
            else {
                
                bool resetSuccess =  reset_lingxin_chat_timer_run();
                if (!resetSuccess) {
                    lingxin_log_debug("首次打断唤醒，初始化定时器 %s", get_wakeup_event_description(event));
                    init_lingxin_chat_timer(NULL, timer_callback);
                }
            }

            lingxin_log_debug("打断唤醒 %s", get_wakeup_event_description(event)); // 打断唤醒
            set_voice_chat_continue_params(payload);

            turn_to(State_Terminate, State_Event_Wakeup_Detected);
        }
        break;
    }
    case State_Event_NoVoice_Error:
    {
        // noVoice下行error时，重启一轮chat循环
        if (chat_current_state == State_NoVoice_Terminate || chat_current_state == State_NoVoice_Start)
        {
            lingxin_log_debug("noVoice下行error时，重启一轮chat循环"); // 打断唤醒
            setWaitTerminateOrEndSuccess(false);                       // 弥补定时任务触发失败时未回复task_started, 未将waitTerminateOrEndSuccess置为false
            turn_to(State_Terminate, event);
        }
        break;
    }
    // 开启一轮novoice循环
    case State_Event_NoVoice_Start:
    {
        if (chat_current_state == State_Exit || chat_current_state == State_NoVoice_Terminate || chat_current_state == State_NoVoice_Start)
        {
            lingxin_log_debug("当前状态为%s,不允许触发当前定时任务", get_chat_state_description(chat_current_state));
            return; // 退出状态，不可触发定时任务
        }

        if (chat_current_state == State_Idle)
        {
            lingxin_log_debug("从Idle态开始noVoice");
            turn_to(State_NoVoice_Start, event);
        }
        else
        {
            lingxin_log_debug("从chat中开始或者在定时任务触发的下行阶段");
            turn_to(State_NoVoice_Terminate, event);
        }
        break;
    }

    // 欢迎语播放结束事件
    case State_Event_Welcome_Play_End:
    {
        if (chat_current_state == State_NoVoice_Start || chat_current_state == State_NoVoice_Terminate)
        {
            lingxin_log_debug("进入定时任务触发阶段时，不允许播放欢迎语结束的事件");
            return; // 进入定时任务触发阶段时，不允许播放欢迎语结束的事件
        }
        turn_to(State_Start, event); // 进入新一轮对话的初始化
        break;
    }

    // 录音开启成功事件
    case State_Event_Record_Ready:
    {
        if (chat_current_state == State_Start)
        {
            from_start_to_binary_transfer(&inner_state_for_start, event);
        }
        else if (chat_current_state == State_Terminate)
        {
            from_terminate_restart_to_binary_transfer(&inner_state_for_terminate_restart, event);
        }
        else if (chat_current_state == State_NoVoice_Terminate)
        {
            // 打断State_Terminate状态下录音初始化操作
            lingxin_log_debug("打断State_Terminate状态下录音初始化操作");
            return;
        }
        break;
    }

    // 服务端新一轮对话开启完成事件
    case State_Event_VoiceChat_StartEnd:
    {
        if (chat_current_state == State_Start)
        {
            from_start_to_binary_transfer(&inner_state_for_start, event);
        }
        else if (chat_current_state == State_NoVoice_Start)
        {
            from_novoice_start_to_wait_buffer(); // 保持当前状态不变，等待服务端推送audio/error, 初始化信号量
        }
        else if (chat_current_state == State_Terminate)
        {
            from_terminate_restart_to_binary_transfer(&inner_state_for_terminate_restart, event);
        }
        // else if (chat_current_state == State_NoVoice_Terminate)
        // {
        //     from_novoice_terminate_restart_to_wait_buffer(&inner_state_for_novoice_terminate_restart, event);
        // }
        break;
    }

    case State_Event_Record_Stop:
    {
        if (chat_current_state == State_Idle)
        {
            return; // TODO: State_Idle这类特殊状态是不是可以单独做处理，其他的状态也可能需要相同的处理
        }
        else
        {
            voiceChatStopSendAudio(); // TODO: 确认是不是其他所有状态下都可以调用这个函数
            if (chat_current_state == State_Binary_Transfer)
            {
                // 4.1 事件更新状态
                turn_to(State_End_Audio, event);
            }
        }

        break;
    }
    case State_Event_Vad_Stop:
    {
        if (payload && payload->vad_stop_payload && payload->vad_stop_payload->disable_vad) {
            if ((chat_current_state == State_Binary_Transfer) ||
                (chat_current_state == State_Start && inner_state_for_start.record_started) ||
                (chat_current_state == State_Terminate && inner_state_for_terminate_restart.record_restarted))
            {
                module_record_stop(1);
            }
        } else {
            // 启用云端vad时，只有录音的时候才可以停止
            if (chat_current_state == State_Binary_Transfer)
            {
                updateControl(CONTROL_AUTHORITY_STATE_MACHINE);
                // 先停止录音
                module_record_stop(0);
                updateControl(CONTROL_AUTHORITY_MODULE);
            }
        }

        break;
    }
    case State_Event_Vad_Exit:
    {
        // 退出逻辑， 只有在录音传输的过程才可以执行退出
        if (chat_current_state == State_Binary_Transfer)
        {
            // 10.1 vad 退出唤醒
            turn_to(State_BufferPlay_EndTask, event);

            // 停止录音
            module_record_stop(0);
        }

        break;
    }
    case State_Event_VoiceChat_AIStart:
    {
        // 初始化audio init
        if (chat_current_state == State_BufferPlay_EndTask || chat_current_state == State_Idle)
        {
            // 初始化audio init
            module_bufferPlay_audioInit();
            // 此时不更新状态
            return;
        }

        // 6.1
        turn_to(State_BufferPlay_Start, event);

        // 初始化audio init
        module_bufferPlay_audioInit();
        lingxin_lock_write_websocket_control();

        break;
    }
    case State_Event_BufferPlay_AudioInitEnd:
    {
        if (chat_current_state == State_BufferPlay_EndTask)
        {
            // 退出状态下，不再调用状态更新的逻辑

            return;
        }

        lingxin_unlock_write_websocket_controle();
        // 7.1 事件
        turn_to(State_BufferPlay_Play, event);

        break;
    }
    case State_Event_VoiceChat_AIEnd:
    {
        if (chat_current_state == State_NoVoice_Terminate)
        {
            return;
        }

        // 播放结束事件
        // 更新主控权
        updateControl(CONTROL_AUTHORITY_STATE_MACHINE);

        // 播放结束事件
        module_bufferPlay_audioEnd();

        // 更新主控权
        updateControl(CONTROL_AUTHORITY_MODULE);

        break;
    }
    case State_Event_BufferPlay_PlayEnd:
    {
        emit_chat_event(CHAT_LIFE_CYCLE_EVENT_PLAY_END, NULL);
        if (chat_current_state == State_BufferPlay_EndTask || chat_current_state == State_Idle)
        {

            // 完整的停止逻辑
            lingxin_log_debug("播放退出唤醒语音，对话结束");
            turn_to(State_Idle, event);

            return;
        }
        else if (chat_current_state == State_BufferPlay_Play)
        {
            if (g_context.need_continue_prompt) {
                lingxin_log_debug("播放跟进提示音");
                module_local_play_continue_audio();
            }
            else {
                lingxin_log_debug("不播放跟进提示音，直接进入下一轮对话");
                turn_to(State_Start, event);
            }
        }

        break;
    }
    case State_Event_ContinuePrompt_PlayEnd:
    {
        turn_to(State_Start, event);
        break;
    }
    case State_Event_BufferPlay_ServerClosed:
    {
        if (chat_current_state == State_NoVoice_Terminate)
        {
            from_novoice_terminate_restart_to_wait_buffer(&inner_state_for_novoice_terminate_restart, event);
        }
        break;
    }
    case State_Event_VoiceChat_TerminateEnd:
    {
        // TODO: 撤掉注释
        module_timeout_need_callback = 0;
        if (chat_current_state == State_BufferPlay_EndTask)
        {
            return;
        }
        else if (chat_current_state == State_Terminate)
        {
            lingxin_log_debug("VoiceChat打断结束");
            from_terminate_restart_to_binary_transfer(&inner_state_for_terminate_restart, event);
        }
        else if (chat_current_state == State_NoVoice_Terminate)
        {
            from_novoice_terminate_restart_to_wait_buffer(&inner_state_for_novoice_terminate_restart, event);
        }
        else if (chat_current_state == State_Exit)
        {
            from_exit_to_idle(&inner_state_for_exit, event); // 确保录音、流式播放、对话退出并断联后再进入Idle
        }
        break;
    }
    case State_Event_NoVoice_TerminateEnd:
    {
        novoice_user_custom_listener(event);
        break;
    }
    case State_Event_BufferPlay_TerminateEnd:
    {
        if (chat_current_state == State_BufferPlay_EndTask)
        {
            return;
        }
        else if (chat_current_state == State_Terminate)
        {
            from_terminate_to_restart(&inner_state_for_terminate, event); // 确保录音和流式播放都打断成功后再进入新对话初始化
        }
        else if (chat_current_state == State_NoVoice_Terminate)
        {
            from_novoice_terminate_to_restart(&inner_state_for_novoice_terminate, event);
        }
        else if (chat_current_state == State_Exit)
        {
            from_exit_to_idle(&inner_state_for_exit, event); // 确保录音、流式播放、对话退出并断联后再进入Idle
        }
        break;
    }
    case State_Event_BufferPlay_Error:
    {
        break;
    }

    case State_Event_Record_TerminateEnd:
    {
        if (chat_current_state == State_Terminate)
        {
            from_terminate_to_restart(&inner_state_for_terminate, event); // 确保录音和流式播放都打断成功后再进入新对话初始化
        }
        else if (chat_current_state == State_NoVoice_Terminate)
        {
            from_novoice_terminate_to_restart(&inner_state_for_novoice_terminate, event);
        }
        else if (chat_current_state == State_Exit)
        {
            from_exit_to_idle(&inner_state_for_exit, event); // 确保录音、流式播放、对话退出并断联后再进入Idle
        }
        break;
    }

    case State_Event_WillExit:
    {
        // 避免重复触发exit
        if (chat_current_state == State_Idle)
        {
            return;
        }
        if (payload && payload->will_exit_payload && payload->will_exit_payload->disable_close_ws_immediately)
{
            InnerStateForExit inner_state = {false, false, false, true, true};
            InnerStateCollection inner_state_collection = {
                .inner_state_for_exit = &inner_state,
            };
            turn_to_with_preset_inner_state(State_Exit, State_Event_WillExit, &inner_state_collection);
        } 
        else {
            turn_to(State_Exit, State_Event_WillExit);
        }
        
        break;
    }

    case State_Event_VoiceChat_ExitEnd:
    {
        // sdk发送打断指令
        if (chat_current_state == State_Exit)
        {

            if (g_context.is_normal_exit) {
                lingxin_log_debug("%s 主动调用对话结束", __func__);
                from_exit_to_idle(&inner_state_for_exit, event); // 确保录音、流式播放、对话退出并断联后再进入Idle
            }
            else {
                lingxin_log_debug("%s 异常webSocket导致对话结束", __func__);
                
                state_machine_receive_error(EXIT_REASON_WEBSOCKET_DISCONNECT);
            }
        } else if (chat_current_state == State_Idle) {
            lingxin_log_debug("等待唤醒态websocket断开");
        } else {
            lingxin_log_debug("websocket异常断开");
            // 找产品对一下产品逻辑

            if (g_context.is_normal_exit) {
                lingxin_log_debug("%s 主动调用对话结束", __func__);
                from_exit_to_idle(&inner_state_for_exit, event); // 确保录音、流式播放、对话退出并断联后再进入Idle
            }
            else {
                lingxin_log_debug("%s 异常webSocket导致对话结束", __func__);
                
                state_machine_receive_error(EXIT_REASON_WEBSOCKET_DISCONNECT);
            }
        }
        break;
    }
    case State_Event_TerminatePrompt_PlayEnd:
    {
        from_terminate_restart_to_binary_transfer(&inner_state_for_terminate_restart, event);
        break;
    }

    default:
    {
        lingxin_log_debug("%s 事件 %s", __func__, get_wakeup_event_description(event));
        break;
    }
    }
}

// 接收SDK的mp3数据
void state_machine_receive_mp3_data(void *buf, int rlen)
{
    if (chat_current_state == State_BufferPlay_Start || chat_current_state == State_BufferPlay_Play || chat_current_state == State_BufferPlay_EndTask || chat_current_state == State_Terminate || chat_current_state == State_NoVoice_Terminate || chat_current_state == State_Exit)
    {
        module_bufferPlay_data(buf, (int)rlen);
        return;
    }

    // 做数据缓存处理，同时阻塞写线程保证数据不再写入，防止内存爆
    lingxin_lock_write_websocket_control();
    lingxin_log_warn("%s 状态值不对，当前状态是 %s", __func__, get_chat_state_description(chat_current_state));
}

// 接收录音模块的录音数据
void state_machine_post_record_data(void *buf, int rlen)
{
    if (chat_current_state != State_Binary_Transfer)
    {
        lingxin_log_error("%s 状态值不对，当前状态是 %s", __func__, get_chat_state_description(chat_current_state));
        return;
    }
    voiceChatSendAudio(buf, (int)rlen);
}

// 接收SDK的定时任务数据
void state_machine_receive_schedule_data(void *scheduleStr)
{
    initTimerTaskList(scheduleStr);
}

// 接收SDK的文本数据
void state_machine_receive_text_data(const char* text) {
    emit_chat_event(CHAT_LIFE_CYCLE_EVENT_TEXT_OUT, (void *)text);
}

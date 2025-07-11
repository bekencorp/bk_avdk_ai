#include "chat_state_machine.h"
#include "lingxin_semaphore.h"
#include <stddef.h>
#include <stdio.h>
typedef enum
{
    CONTROL_AUTHORITY_STATE_MACHINE, // 主控权在状态机
    CONTROL_AUTHORITY_MODULE         // 主控权在模块（如SDK）
} ControlAuthority;



static ControlAuthority control_state = CONTROL_AUTHORITY_STATE_MACHINE;

static ChatState chat_current_state = State_Idle;

static ChatState chat_last_state = State_Idle;

static ChatEventListener chat_event_listenner = NULL;

static ChatUserCommandListener chat_user_command_listener = NULL;

/**
 * voice chat交互逻辑
 */

// 开始对话逻辑
static void wanson_voiceChatContinueCallback()
{
    printf("0.0.6 dlu:2-1 wanson_voiceChatContinueCallback\n");

    state_machine_run_event(State_Event_VoiceChat_StartEnd);
}

// 根据枚举值返回描述字符串
static const inline char *get_chat_state_description(ChatState state)
{
    switch (state)
    {
    case State_Idle:
        return "State_Idle  等待唤醒态";
    case State_Terminate:
        return "State_Terminate 打断状态";
    case State_Start:
        return "State_Start 新一轮对话开始";
    case State_Started:
        return "State_Started voice chat已经发完start指令";
    case State_Binary_Transfe:
        return "State_Binary_Transfe 录音传输态";
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
    default:
        printf("%s 未知状态 %d", __func__, state);
        return "未知状态";
    }
}

static const inline char *get_wakeup_event_description(StateEvent event)
{
    switch (event)
    {
    case State_Event_Wakeup_Detected:
        return "State_Event_Wakeup_Detected 唤醒事件";
    case State_Event_VoiceChat_StartEnd:
        return "State_Event_VoiceChat_StartEnd  voice chat 对话成功事件";
    case State_Event_VoiceChat_TerminateEnd:
        return "State_Event_VoiceChat_TerminateEnd voice chat voice chat 打断成功事件";
    case State_Event_VoiceChat_AIStart:
        return "State_Event_VoiceChat_AIStart 服务端开始推音频流事件";
    case State_Event_VoiceChat_AIEnd:
        return "State_Event_VoiceChat_AIEnd 服务端结束推音频流事件";
    case State_Event_Play_DingDong:
        return "State_Event_Play_DingDong 播放开始说话提示音";
    case State_Event_Vad_Stop:
        return "State_Event_Vad_Stop vad 停止";
    case State_Event_Vad_Exit:
        return "State_Event_Vad_Exit 退出唤醒";
    case State_Event_BufferPlay_AudioInitEnd:
        return "State_Event_BufferPlay_AudioInitEnd 流式播放初始化结束";
    case State_Event_BufferPlay_PlayEnd:
        return "State_Event_BufferPlay_PlayEnd 流式播放结束";
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
    default:
        printf("%s 未知事件 %d", __func__, event);
        return "未知事件";
    }
}

static bool isTerminate = false;
static bool isSdkTerminate = false;
static bool isRecordTerminate = false;
static bool isAudioBufferTerminate = false;

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
    if (state == State_Idle)
    {
        lingxin_semaphore_del();
    }
    chat_last_state = chat_current_state;

    chat_current_state = state;
    printf("%s 从 %s 更新到 %s, 触发事件: %s", __func__, get_chat_state_description(chat_last_state), get_chat_state_description(chat_current_state), get_wakeup_event_description(event));
}

static void startTerminate(StateEvent event)
{
    if (isTerminate && isSdkTerminate && isRecordTerminate && isAudioBufferTerminate)
    {
        if (chat_current_state == State_Exit) {
            module_voiceChat_exit();

            isTerminate = isSdkTerminate = isRecordTerminate = isAudioBufferTerminate = false;
        }
        else {
            // 执行打断逻辑

            // 唤醒态 更新到 Start
            updateChatState(State_Start, event);

            // 调用start指令
            voiceChatContinueCheck(wanson_voiceChatContinueCallback, NULL);

            isTerminate = isSdkTerminate = isRecordTerminate = isAudioBufferTerminate = false;
        }
        
        return;
    }
    else
    {
        printf("打断事件未执行完, isTerminate:%d ,isSdkTerminate:%d ,isRecordTerminate:%d ,isAudioBufferTerminate:%d ,", isTerminate, isSdkTerminate, isRecordTerminate, isAudioBufferTerminate);
    }
}

bool get_chat_state_terminate()
{
    return isTerminate;
}

/**
 * 新重构之后的接口
 */
// 初始化 machine
void voice_chat_machine_init(ChatEventListener event_listener, ChatUserCommandListener user_command_listener)
{
    initVoiceChat();
    chat_event_listenner = event_listener;

    chat_user_command_listener = user_command_listener;
}

// 事件流转
void state_machine_run_event(StateEvent event)
{
    printf("%s 接受到事件 %s，当前状态 %s", __func__, get_wakeup_event_description(event), get_chat_state_description(chat_current_state));
    switch (event)
    {
    case State_Event_Wakeup_Detected:
    {
        // 唤醒事件
        if (chat_current_state == State_Exit) {
            // 退出状态。不可唤醒
            return;
        }
        if (chat_current_state == State_Idle)
        {
            // 首次唤醒
            printf("首次唤醒 %s", get_wakeup_event_description(event));

            lingxin_semaphore_create();

            // 唤醒态 更新到 Start
            updateChatState(State_Start, event);

            // 调用start指令
            voiceChatContinueCheck(wanson_voiceChatContinueCallback, NULL);

            // 更新主控权
            updateControl(CONTROL_AUTHORITY_MODULE);
        }
        else
        {
            if (isTerminate || chat_current_state == State_BufferPlay_EndTask) {
                printf("重复打断唤醒 %s", get_wakeup_event_description(event));
                // return;
                if (chat_current_state == State_BufferPlay_EndTask) {
                    updateChatState(State_BufferPlay_Play, event);
                }
            }
            printf("打断唤醒 %s", get_wakeup_event_description(event));
            // 打断唤醒 群发消息
            isTerminate = true;

            // lingxin_pend_write_semaphore();

            // 先停止录音
            module_record_terminate();

            // 暂停播放 由于要安全停止播放，所以需要在流式播放结束之后，再调用打断指令，暂停语音数据传输
            module_bufferPlay_terminate();
        }
        break;
    }
    case State_Event_VoiceChat_StartEnd:
    {
        updateControl(CONTROL_AUTHORITY_STATE_MACHINE);
        // 接受 SDK 成功start的指令

        // 1.1 事件更新状态
        updateChatState(State_Started, event);

        // 播放叮咚
        printf("%s 调用 playDingDongAudio", __func__);
        playDingDongAudio();

        // 更新主控权
        updateControl(CONTROL_AUTHORITY_MODULE);
        break;
    }

    case State_Event_Play_DingDong:
    {
        updateControl(CONTROL_AUTHORITY_STATE_MACHINE);

        // 初始化录音
        module_record_init();

        // 更新主控权
        updateControl(CONTROL_AUTHORITY_MODULE);
        break;
    }

    case State_Event_Record_Ready:
    {
        updateControl(CONTROL_AUTHORITY_STATE_MACHINE);

        // 3.1 事件更新状态
        updateChatState(State_Binary_Transfe, event);

        // 更新主控权
        updateControl(CONTROL_AUTHORITY_MODULE);
        break;
    }

    case State_Event_Record_Stop:
    {
        if (chat_current_state == State_BufferPlay_EndTask || chat_current_state == State_Exit)
        {
            voiceChatStopSendAudio();
        }
        else if (chat_current_state != State_Idle)
        {
            voiceChatStopSendAudio();

            // 4.1 事件更新状态
            updateChatState(State_End_Audio, event);
        }

        break;
    }
    case State_Event_Vad_Stop:
    {
        // 录音的时候才可以停止
        if (chat_current_state == State_Binary_Transfe)
        {
            //
            updateControl(CONTROL_AUTHORITY_STATE_MACHINE);

            // 先停止录音
            module_record_stop();

            // 更新主控权
            updateControl(CONTROL_AUTHORITY_MODULE);
        }

        break;
    }
    case State_Event_Vad_Exit:
    {
        // 退出逻辑， 只有在录音传输的过程才可以执行退出
        if (chat_current_state == State_Binary_Transfe)
        {
            // 更新主控权
            updateControl(CONTROL_AUTHORITY_STATE_MACHINE);

            // 10.1 vad 退出唤醒
            updateChatState(State_BufferPlay_EndTask, event);

            // 停止录音
            module_record_stop();
        }

        break;
    }
    case State_Event_VoiceChat_AIStart:
    {
        // 初始化audio init
        // 更新主控权
        updateControl(CONTROL_AUTHORITY_STATE_MACHINE);

        if (chat_current_state == State_BufferPlay_EndTask || chat_current_state == State_Idle)
        {

            // 初始化audio init
            module_bufferPlay_audioInit();
            // 此时不更新状态
            return;
        }

        // 6.1
        updateChatState(State_BufferPlay_Start, event);

        // 初始化audio init
        module_bufferPlay_audioInit();
        lingxin_pend_write_semaphore();

        break;
    }
    case State_Event_BufferPlay_AudioInitEnd:
    {
        if (chat_current_state == State_BufferPlay_EndTask)
        {
            // 退出状态下，不再调用状态更新的逻辑

            return;
        }

        lingxin_post_write_semaphore();
        // 7.1 事件
        updateChatState(State_BufferPlay_Play, event);

        break;
    }
    case State_Event_VoiceChat_AIEnd:
    {

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
        if (chat_event_listenner)
        {
            chat_event_listenner(event);
        }
        if (chat_current_state == State_BufferPlay_EndTask || chat_current_state == State_Idle)
        {

            // 完整的停止逻辑
            printf("播放退出唤醒语音，对话结束");
            updateChatState(State_Idle, event);

            return;
        }
        else
        {
            // 更新主控权
            updateControl(CONTROL_AUTHORITY_STATE_MACHINE);

            // 唤醒态 更新到 Start
            updateChatState(State_Start, event);

            // 更新主控权
            updateControl(CONTROL_AUTHORITY_MODULE);

            // 调用start指令
            voiceChatContinueCheck(wanson_voiceChatContinueCallback, NULL);
        }

        break;
    }
    case State_Event_VoiceChat_TerminateEnd:
    {
        if (chat_current_state == State_BufferPlay_EndTask)
        {
            return;
        }

        printf("VoiceChat打断结束");
        isSdkTerminate = true;

        startTerminate(event);

        break;
    }

    case State_Event_BufferPlay_TerminateEnd:
    {
        
        if (chat_current_state == State_BufferPlay_EndTask)
        {
            return;
        }

        // 解锁
        // lingxin_pend_write_semaphore();

        isAudioBufferTerminate = true;

        module_voiceChat_terminate();
        break;
    }
    case State_Event_BufferPlay_Error:
    {
        break;
    }

    case State_Event_Record_TerminateEnd:
    {
        // sdk发送打断指令
        isRecordTerminate = true;
        startTerminate(event);
        break;
    }

    case State_Event_WillExit:
    {
        // 触发内部事件退出逻辑
        updateChatState(State_Exit, State_Event_WillExit);

        // 打断唤醒 群发消息
        isTerminate = true;

        // 先停止录音
        module_record_terminate();

        // 暂停播放 由于要安全停止播放，所以需要在流式播放结束之后，再调用打断指令，暂停语音数据传输
        module_bufferPlay_terminate();
        break;
    }

    case State_Event_VoiceChat_ExitEnd:
    {
        // sdk发送打断指令
        if (chat_current_state == State_Exit)
        {

            // 完整的停止逻辑
            printf("主动调用对话结束");
            updateChatState(State_Idle, event);

            // 添加结束回调
            chat_user_command_listener(Chat_User_Command_Stop);

            return;
        }
        break;
    }

    default:
    {
        printf("%s 事件 %s", __func__, get_wakeup_event_description(event));
        break;
    }
    }
}

void state_machine_receive_mp3_data(void *buf, int rlen)
{
    if (chat_current_state == State_BufferPlay_Start || chat_current_state == State_BufferPlay_Play || chat_current_state == State_BufferPlay_EndTask)
    {
        module_bufferPlay_data(buf, (int)rlen);
        return;
    }

    // 做数据缓存处理，同时阻塞写线程保证数据不再写入，防止内存爆
    lingxin_pend_write_semaphore();
    printf("%s 状态值不对，当前状态是 %s", __func__, get_chat_state_description(chat_current_state));
}

int state_machine_post_record_data(void *buf, int rlen)
{
    if (chat_current_state != State_Binary_Transfe)
    {
        printf("%s 状态值不对，当前状态是 %s", __func__, get_chat_state_description(chat_current_state));
        return -1;
    }
    return voiceChatSendAudio(buf, (int)rlen);
}

/**
 * 老的代码逻辑
 */

// 静态变量：当前状态
// static WakeupState current_state = STATE_END;

// struct timeval machine_state_change_time;

//  // 根据枚举值返回描述字符串
// static const inline char* get_wakeup_state_description(WakeupState state) {
//     switch (state) {
//         case STATE_INIT:
//             return "STATE_INIT 初始化状态（首次唤醒）";
//         case STATE_START:
//             return "STATE_START 开始状态（首次唤醒之后到 vad stop的状态）";
//         case STATE_VOICE_CHAT_CHECK:
//             return "STATE_VOICE_CHAT_CHECK voice chat 状态（voice chat事件调用成功）";
//         case STATE_PLAY_DINGDONG:
//             return "STATE_PLAY_DINGDONG 播放叮咚（叮咚播放成功）";
//         case STATE_RECORD_INIT:
//             return "STATE_RECORD_INIT 录音初始化（初始化成功）";
//         case STATE_RECORDING:
//             return "STATE_RECORDING 录音中";
//         case STATE_STOP_RECORDING:
//             return "STATE_STOP_RECORDING 结束录音";
//         case STATE_INTERRUPT_WAKEUP:
//             return "STATE_INTERRUPT_WAKEUP 打断唤醒状态 (获取到你好小兔的打断语音)";
//         case STATE_TERMINATE:
//             return "STATE_TERMINATE 打断状态(获取到你好小兔到第一次vad stop之间)";
//         case STATE_ERROR_RETRY:
//             return "STATE_ERROR_RETRY 错误重试（接受到服务端voice chat错误）";
//         case STATE_AI_VOICE:
//             return "STATE_AI_VOICE 服务端首包";
//         case STATE_END:
//             return "STATE_END 结束状态（退出唤醒）";
//         default:
//             printf("%s 未知状态 %d",__func__, state);
//             return "未知状态";
//     }
// }

// static void set_state_change_time() {
//     // 调用 gettimeofday()
//     // int result =
//      gettimeofday(&machine_state_change_time, NULL);

//     // if (result == 0) {
//     //     // 成功记录时间
//     //     printf("%s 时间记录成功.",__func__);
//     //     printf("Seconds: %ld, Microseconds: %ld\n",
//     //            machine_state_change_time.tv_sec,
//     //            machine_state_change_time.tv_usec);
//     // } else {
//     //     // 记录时间失败
//     //     // 成功记录时间
//     //     printf("%s 时间记录失败.",__func__);
//     //     printf("Seconds: %ld, Microseconds: %ld\n",
//     //            machine_state_change_time.tv_sec,
//     //            machine_state_change_time.tv_usec);
//     // }

// }
// // 测试时间问题
// static double wakeup_state_change_time(struct timeval *start_time, WakeupState lastState, WakeupState temp_current_state)
// {
//     struct timeval end_time;
//     long seconds, microseconds;
//     double elapsed_ms;

//     // 记录结束时间
//     gettimeofday(&end_time, NULL);

//     // printf("end Seconds: %ld, Microseconds: %ld\n",
//     //     end_time.tv_sec,
//     //     end_time.tv_usec);

//     // 计算时间差
//     seconds = end_time.tv_sec - start_time->tv_sec;
//     microseconds = end_time.tv_usec - start_time->tv_usec;

//     // printf("%s 差值 Seconds: %ld, Microseconds: %ld\n",
//     //     __func__,
//     //     seconds,
//     //     microseconds);
//     elapsed_ms = seconds * 1000.0 + microseconds / 1000.0;

//     printf("%s 从 %s 切换到  %s 消耗的时间  %.2f ms", __func__, get_wakeup_state_description(lastState), get_wakeup_state_description(temp_current_state), elapsed_ms);

//     return elapsed_ms;
// }

// static bool change_wake_up_state(WakeupState new_state)
// {
//     // 不再依赖STATE_INTERRUPT_WAKEUP状态来进行更新了
//     // if (current_state == STATE_INTERRUPT_WAKEUP) {
//     //     // 如果当前是打断状态，下一个只能是 STATE_TERMINATE
//     //     if (new_state != STATE_TERMINATE) {
//     //         printf("%s 如果当前是打断状态，下一个只能是 STATE_TERMINATE， new_state: %s",__func__,get_machine_state_string(new_state));
//     //         return false;
//     //     }
//     // }
//     wakeup_state_change_time(&machine_state_change_time, current_state, new_state);
//     printf("%s", __func__);

//     current_state = new_state;

//      // 重置当前状态时间
//     set_state_change_time();

//     return true;
// }

// // 获取当前状态
// WakeupState get_machine_current_state()
// {
//     return current_state;
// }

// char * get_machine_state_string()
// {
//     return get_wakeup_state_description(current_state);
// }

// bool state_machine_is_wakeup()
// {
//     return current_state != STATE_END;
// }

// // 初始化状态机
// bool state_machine_init() {
//     if (current_state == STATE_END || current_state == STATE_INIT) {
//         current_state = STATE_INIT; // 重置状态
//         // 重置当前状态时间
//         set_state_change_time();
//         printf("%s 状态机初始化成功",__func__);

//         return true;
//     }
//     else
//     {
//         printf("%s 状态机初始化异常插入, 当前状态 %s 无法切换到  %s", __func__, get_wakeup_state_description(current_state), get_wakeup_state_description(STATE_INIT));

//         return false;
//     }
// }

// bool state_machine_exit() {
//     // 重置重试计数

//     printf("%s 状态机结束成功成功",__func__);

//     return change_wake_up_state(STATE_END);;
// }

// // 启动状态机
// bool state_machine_run(WakeupState event) {
//     if (event == current_state) {
//         if (event != STATE_AI_VOICE) {
//             printf("%s 状态值重复设置, %s", __func__, get_wakeup_state_description(event));
//         }
//         return false;
//     }

//     switch (event) {
//         case STATE_INIT:
//             printf("不允许在run的过程中设置init状态");
//             return false;

//         case STATE_START:
//             return change_wake_up_state(event);

//         case STATE_PLAY_DINGDONG:
//             return change_wake_up_state(event);

//         case STATE_RECORD_INIT:
//             return change_wake_up_state(event);

//         case STATE_STOP_RECORDING:
//             return change_wake_up_state(event);

//         case STATE_INTERRUPT_WAKEUP:
//             return change_wake_up_state(event);

//         case STATE_TERMINATE:
//             return change_wake_up_state(event);

//         case STATE_ERROR_RETRY:
//             return change_wake_up_state(event);

//         case STATE_END:
//             printf("不允许在run的过程中设置 end 状态");
//             return change_wake_up_state(event);

//         default:
//             return change_wake_up_state(event);
//     }
// }

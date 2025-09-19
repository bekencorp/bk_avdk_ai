#ifndef CHAT_STATE_MACHINE_H
#define CHAT_STATE_MACHINE_H

// 添加模块函数依赖
#include "audio_buffer_play.h"
#include "lingxin_recorder.h"
#include "lingxin_local_player_manager.h"
#include "voice_chat_manager.h"
#include "lingxin_time_task.h"

// 添加事件定义
#include "chat_state_machine_event.h"

// 添加对外暴露函数定义
#include "chat_api.h"


// 状态机状态
typedef enum
{
  State_Idle,                   // 等待唤醒态
  State_Welcome,                // 欢迎语播放态

  State_Start,                  // 新一轮对话初始化(开始录音和请求连续对话)
  State_Binary_Transfer,        // 录音传输态
  State_End_Audio,              // 结束录音态
  State_BufferPlay_Start,       // 流式播放开始态
  State_BufferPlay_Play,        // 流式播放中
  State_BufferPlay_EndTask,     // 结束当前轮对话并退出

  State_NoVoice_Start,          // noVoice循环初始化
  State_NoVoice_Terminate,      // 打断当前对话，开启新一轮noVoice循环
  
  State_Terminate,              // 打断状态
  State_Exit,                   // 主动退出
} ChatState;

// typedef enum 
// {
//   Chat_VoiceChat_WS_Destory,        // voice chat ws 销毁，可能原因：断网
//   Chat_VoiceChat_WS_Connect_Failed, // voice chat ws 连接失败
//   Chat_VoiceChat_WS_User_Exit_Destory, // 用户主动退出导致 ws 销毁
// } ChatError;

// typedef struct {
//   ChatError chatErrorType;   // chat error 类型
//   char *externData;                // chat error 附带的外部数据
// } ChatErrorPlayload;

// 运行时上下文
typedef struct {
    ExitCode exit_code; // 退出时的错误信息
    bool is_normal_exit;       // 是否是主动退出对话模式, 主要用来区别是不是错误导致退出
    bool need_terminate_prompt; // 打断唤醒是否需要播放提示音
    bool need_continue_prompt;  // 连续对话前是否需要播放提示音
} ChatStateRuntimeContext;

// 携带payload向状态机发送事件
typedef struct {
  bool disable_welcome_audio;   // 首次唤醒是否需要开场白
  bool disable_vad;             // 本轮对话是否启用云端VAD
  char *task_id;                // 本轮ß对话是否指定task_id
} WakeupDetectedPayload;
typedef struct {
  bool disable_close_ws_immediately;    // 是否立即关闭websocket
} WillExitPayload;
typedef struct {
  bool disable_vad;             // 本次事件是否为禁用云端VAD的场景
} VadStopPayload;
typedef struct {
  WakeupDetectedPayload *wakeup_detected_payload;
  WillExitPayload *will_exit_payload;
  VadStopPayload *vad_stop_payload;
} StateEventPayload;

void state_machine_run_event_with_payload(StateEvent event, StateEventPayload *payload);

// 获取状态机是否为终止状态
bool get_chat_state_terminate();

// 接收SDK的mp3数据
void state_machine_receive_mp3_data(void *buf, int rlen);

// 接收SDK的定时任务数据
void state_machine_receive_schedule_data(void *scheduleStr);

// 接收SDK的文本数据
void state_machine_receive_text_data(const char *text);

void state_machine_receive_error(ExitCode exit_code);

void turn_to(ChatState state, StateEvent event);

void voice_chat_machine_init(ChatLifeCycleEventListener event_listener, bool need_terminate_prompt, bool need_continue_prompt);
#endif // CHAT_STATE_MACHINE_H
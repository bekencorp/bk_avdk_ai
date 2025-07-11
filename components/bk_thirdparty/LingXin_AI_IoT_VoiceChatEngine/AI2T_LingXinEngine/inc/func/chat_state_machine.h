#ifndef VOICE_CHAT_MACHINE_H
#define VOICE_CHAT_MACHINE_H

// 添加模块函数依赖
#include "audio_buffer_play.h"
#include "audio_recorder.h"
#include "local_audio_play.h"
#include "voice_chat_manager.h"

typedef enum
{
  State_Idle,               // 等待唤醒态
  State_Terminate,          // 打断状态
  State_Start,              // 新一轮对话开始
  State_Started,            // voice chat已经发完start指令
  State_Binary_Transfe,     // 录音传输态
  State_End_Audio,          // 结束录音态
  State_BufferPlay_Start,   // 流式播放开始态
  State_BufferPlay_Play,    // 流式播放中
  State_BufferPlay_EndTask, // 结束当前轮对话
  State_Exit,               // 主动退出
} ChatState;

// 事件是模块回调到状态机
typedef enum
{
  State_Event_Wakeup_Detected,         // 唤醒事件 0
  State_Event_VoiceChat_StartEnd,      // voice chat 对话成功事件 1
  State_Event_VoiceChat_TerminateEnd,  // voice chat 打断成功事件 2
  State_Event_Play_DingDong,           // 播放开始说话提示音 3
  State_Event_VoiceChat_AIStart,       // voice chat 开始推送语音流 4
  State_Event_VoiceChat_AIEnd,         // voice chat 开始推送语音流 5
  State_Event_Vad_Stop,                // vad 停止 6
  State_Event_Vad_Exit,                // vad 退出唤醒 7
  State_Event_BufferPlay_AudioInitEnd, // 流式播放初始化结束 8
  State_Event_BufferPlay_PlayEnd,      // 流式播放结束 9
  State_Event_BufferPlay_TerminateEnd, // 流式播放打断后暂停 10
  State_Event_BufferPlay_Error,        // 流式播放模块出错 11
  State_Event_Record_Ready,            // 录音模块初始化成功 12
  State_Event_Record_Stop,             // 录音模块停止录音 13
  State_Event_Record_TerminateEnd,     // 录音模块打断事件 14

  State_Event_VoiceChat_ExitEnd, // voice chat 对话退出成功 15
  State_Event_WillExit, // 用户调用退出
} StateEvent;

// 用户指令事件回调
typedef enum
{
  Chat_User_Command_Stop, // 用户停止命令
} ChatUserCommand;

// 对外暴露的状态机接口
bool get_chat_state_terminate();
/**
 * 模块调用状态机的方法
 */
void state_machine_run_event(StateEvent event);

// 录音数据转发
int state_machine_post_record_data(void *buf, int rlen);

// 接受SDK的数据
void state_machine_receive_mp3_data(void *buf, int rlen);

// 事件监听器
typedef void (*ChatEventListener)(StateEvent event);

typedef void (*ChatUserCommandListener)(ChatUserCommand user_command);

// 初始化方法
void voice_chat_machine_init(ChatEventListener event_listener,
                            ChatUserCommandListener user_command_listener);


#endif // VOICE_CHAT_MACHINE_H
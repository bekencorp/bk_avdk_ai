
#ifndef CHAT_STATE_MACHINE_EVENT_H
#define CHAT_STATE_MACHINE_EVENT_H

// 模块抛给状态机的事件
typedef enum
{
  State_Event_Wakeup_Detected,          // 唤醒事件 0
  State_Event_Welcome_Play_End,         // 播放欢迎语结束事件 1

  State_Event_VoiceChat_StartEnd,       // voice chat 对话成功事件 2
  State_Event_VoiceChat_TerminateEnd,   // voice chat 打断成功事件 3
  State_Event_VoiceChat_AIStart,        // voice chat 开始推送语音流 4
  State_Event_VoiceChat_AIEnd,          // voice chat 开始推送语音流 5
  State_Event_Vad_Stop,                 // vad 停止 6
  State_Event_Vad_Exit,                 // vad 退出唤醒 7

  State_Event_BufferPlay_AudioInitEnd,  // 流式播放初始化结束 8
  State_Event_BufferPlay_PlayEnd,       // 流式播放结束 9
  State_Event_BufferPlay_TerminateEnd,  // 流式播放打断后暂停 10
  State_Event_BufferPlay_Error,         // 流式播放模块出错 11

  State_Event_Record_Ready,             // 录音模块初始化成功 12
  State_Event_Record_Stop,              // 录音模块停止录音 13
  State_Event_Record_TerminateEnd,      // 录音模块打断事件 14

  State_Event_VoiceChat_ExitEnd,        // voice chat 对话退出成功 15
  State_Event_WillExit,                 // 用户调用退出 16

  State_Event_NoVoice_Start,            // 开启新一轮novoice模式 17
  State_Event_NoVoice_Error,            // noVoice下行阶段服务端推送error 18
  State_Event_NoVoice_TerminateEnd,
  State_Event_BufferPlay_ServerClosed,  // audio_server关闭事件

  State_Event_TerminatePrompt_PlayEnd,  // 增加打断唤醒提示音
  State_Event_ContinuePrompt_PlayEnd,   // 增加连续对话提示音
} StateEvent;

/******************** 在chat套件内核中已实现，客户调用 ********************/
// 模块抛出事件给调用状态机
void state_machine_run_event(StateEvent event); 
// 录音模块把录音数据发送给状态机
void state_machine_post_record_data(void *buf, int rlen); 

#endif // CHAT_STATE_MACHINE_EVENT_H
#ifndef __CHAT_API_H__
#define __CHAT_API_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>

typedef char* (*AuthAppIdGetFunc)(void);
typedef char* (*AuthAppKeyGetFunc)(void);
typedef char* (*AuthSnGetFunc)(void);
typedef char* (*AuthAgentCodeGetFunc)(void);

// 退出完成Code
typedef enum {
  EXIT_REASON_USER_INITIATED,               // 0. 主动退出
  EXIT_REASON_WEBSOCKET_DISCONNECT,         // 1. websocket异常断开，可能原因：断网
  EXIT_REASON_WEBSOCKET_CONNECTION_FAILED,  // 2. websocket建联失败
} ExitCode;
// 退出完成事件载荷
typedef struct {
  ExitCode exit_code;
} ExitPayload;

// 对话模式生命周期事件
typedef enum {
  CHAT_LIFE_CYCLE_EVENT_EXIT,           // 0. 退出完成
  CHAT_LIFE_CYCLE_EVENT_SCHEDULE_EMIT,  // 1. 定时任务触发
  CHAT_LIFE_CYCLE_EVENT_TEXT_OUT,       // 2. 指令+文本
  CHAT_LIFE_CYCLE_EVENT_PLAY_END,       // 3. 播放完成事件
} ChatLifeCycleEvent;
typedef void (*ChatLifeCycleEventListener)(ChatLifeCycleEvent event, void *payload);

// 对话模式初始化方法与参数
typedef struct {
  AuthAppIdGetFunc auth_app_id_get_func;
  AuthAppKeyGetFunc auth_app_key_get_func;
  AuthSnGetFunc auth_sn_get_func; 
  AuthAgentCodeGetFunc auth_agent_code_get_func;

  ChatLifeCycleEventListener chat_life_cycle_event_listener;

  int send_uni_size;          // 设置录音单次发送的大小（字节）
  int send_cbuf_scale;        // 设置录音缓冲区大小对于单次发送大小的倍数
  char *welcome_audio_path;   // 设置首次唤醒后播放的音频
  char *terminate_audio_path; // 设置打断时播放的音频
  char *continue_audio_path;  // 设置连续对话进入下一轮对话前播放的音频
  int is_schedule_task_on;    // 是否开启定时任务

  char *props_init_tag;       // 标记是否经过灵芯自带的初始化，用户无需关心
} VoiceChatInitProps;
VoiceChatInitProps get_voice_chat_init_default_props();
int voice_chat_init(VoiceChatInitProps *init_props);

// 对话模式新一轮对话方法与参数
typedef struct {
  bool disable_welcome_audio; // 首次唤醒是否需要开场白
  bool disable_vad;           // 本轮对话是否启用云端VAD
  char *task_id;              // 本轮ß对话是否指定task_id

  char *props_init_tag;       // 标记是否经过灵芯自带的初始化，用户无需关心
} StartNewChatProps;
StartNewChatProps get_start_new_chat_default_props();
int start_new_chat(StartNewChatProps *start_props);

// 对话模式主动停止录音方法与参数
typedef struct {
} StopChatRecordProps;        // 用于后续拓展
int stop_chat_record(StopChatRecordProps *stop_record_props);

// 对话模式退出方法与参数
typedef struct {
  bool disable_close_ws_immediately;  // 是否立即关闭websocket
  char *props_init_tag;               // 标记是否经过灵芯自带的初始化，用户无需关心
} ExitChatProps;
ExitChatProps get_exit_chat_default_props();
int exit_chat(ExitChatProps *exit_props);

// 加音量
int set_volume(int volume);

#ifdef __cplusplus
}
#endif

#endif // __CHAT_API_H__
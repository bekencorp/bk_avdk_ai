#ifndef AI_IOT_SDK_VOICE_CHAT_H
#define AI_IOT_SDK_VOICE_CHAT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>

  typedef enum
  {
    VOICECHAT_EVENT_ON_VOIC_SEND_READY,              // 当voicechat准备好接收音频输入时触发
    VOICECHAT_EVENT_ON_RESULT_AI_VOICE_START,        // 当服务端准备推音频流时触发
    VOICECHAT_EVENT_ON_RESULT_AI_VOICE,              // 当服务端推音频流时触发
    VOICECHAT_EVENT_ON_RESULT_AI_VOICE_READY_TO_END, // 当服务端推音频流结束,开始清理操作时触发
    VOICECHAT_EVENT_ON_RESULT_AI_VOICE_END,          // 当服务端推音频流结束,且清理操作完成后触发
    VOICECHAT_EVENT_ON_TERMINAL,                     // 当voicechat打断时触发
    VOICECHAT_EVENT_ON_TEXTOUT,                      // 当voicechat有信息透传时触发
    VOICECHAT_EVENT_ON_VAD_END,                      // server vad场景下，当有断句时触发
    VOICECHAT_EVENT_ON_VAD_EXIT,                     // server vad场景下，当收到vad退出指令时触发
    VOICECHAT_EVENT_ON_ERROR,                        // 当voicechat发生错误时触发
    VOICECHAT_EVENT_ON_DESTROY                       // 当voicechat销毁时触发
  } VoiceChatEventType;

  typedef struct VoiceChatHandler VoiceChatHandler;

  typedef struct
  {
    char *taskId;
    char *requestId; // 请求ID
    char *instanceId;
  } VoiceChatExtraInfo;

  typedef void (*VoiceChatEventListener)(VoiceChatEventType event,
                                         const char *data, const size_t len, VoiceChatExtraInfo *extraInfo);

  typedef struct
  {
    bool showLog;
    const char *serverPath;
    const char *payload;
    const char *taskId;
    const char *appKey;
    const char *appId;
    const char *sn;
  } VoiceChatConfig;

  char *voiceChatCreate(VoiceChatHandler **handlerAddress, VoiceChatConfig *config, VoiceChatEventListener listener);

  int voiceChatSend(VoiceChatHandler *handler, const char *audioData,
                    size_t dataSize);

  bool voiceChatSendStop(VoiceChatHandler *handler);

  bool voiceChatTerminal(VoiceChatHandler *handler);

  bool voiceChatContinue(VoiceChatHandler *handler);

  bool voiceChatStartNewChat(VoiceChatHandler *handler, VoiceChatConfig *config);

  void voiceChatDestroy(VoiceChatHandler *handler);

#ifdef __cplusplus
}
#endif
#endif // AI_IOT_SDK_VOICE_CHAT_H
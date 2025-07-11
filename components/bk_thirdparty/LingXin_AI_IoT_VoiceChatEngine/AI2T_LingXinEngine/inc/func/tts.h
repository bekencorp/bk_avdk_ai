#ifndef AI_IOT_SDK_TTS_H
#define AI_IOT_SDK_TTS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>

  // 定义tts事件类型的枚举
  typedef enum
  {
    TTS_EVENT_ON_READY,       // 当tts准备就绪时触发
    TTS_EVENT_ON_SEND_START,  // 当tts可以接收文本时触发
    TTS_EVENT_ON_SEND_RESULT, // 当tts结果回调时触发
    TTS_EVENT_ON_SEND_END,    // 当tts结果回调完成时触发
    TTS_EVENT_ON_ERROR,       // 当tts发生错误时触发
    TTS_EVENT_ON_DESTROY      // 当tts对象被销毁时触发
  } TTSEventType;

  typedef struct
  {
    bool showLog;
    const char *appKey;
    const char *sn;
    const char *appId;
  } TTSConfig;

  typedef struct TTSHandler TTSHandler;

  typedef struct
  {
    char *taskId;
    char *requestId; // 请求ID
    char *instanceId;
  } TTSExtraInfo;

  typedef void (*TTSEventListener)(TTSEventType event, const char *data,
                                   const size_t len, TTSExtraInfo *extraInfo);

  char *ttsCreate(TTSHandler **handlerAddress, TTSConfig *config, const char *payload, TTSEventListener listener);

  bool ttsSendStart(TTSHandler *handler, const char *taskId);

  int ttsSend(TTSHandler *handler, const char *taskId, const char *text);

  bool ttsSendStop(TTSHandler *handler, const char *taskId);

  void ttsDestroy(TTSHandler *handler);

#ifdef __cplusplus
}
#endif
#endif // AI_IOT_SDK_TTS_H
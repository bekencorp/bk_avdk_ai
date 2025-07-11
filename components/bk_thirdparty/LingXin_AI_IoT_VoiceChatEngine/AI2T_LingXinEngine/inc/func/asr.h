#ifndef AI_IOT_SDK_ASRT_H
#define AI_IOT_SDK_ASRT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>

  // 定义ASR事件类型的枚举
  typedef enum
  {
    ASR_EVENT_ON_READY,       // 当ASR准备就绪时触发
    ASR_EVENT_ON_SEND_START,  // 当ASR可以接收音频时触发
    ASR_EVENT_ON_SEND_RESULT, // 当ASR结果回调时触发
    ASR_EVENT_ON_SEND_END,    // 当ASR结果回调完成时触发
    ASR_EVENT_ON_ERROR,       // 当ASR发生错误时触发
    ASR_EVENT_ON_DESTROY      // 当ASR对象被销毁时触发
  } ASREventType;

  typedef struct ASRHandler ASRHandler;

  typedef struct
  {
    char *taskId;
    char *requestId; // 请求ID
    char *instanceId;
  } ASRExtraInfo;

  typedef void (*ASREventListener)(ASREventType event, const char *data,
                                   size_t dataSize, ASRExtraInfo *extraInfo);

  typedef struct
  {
    bool showLog;
    const char *appKey;
    const char *sn;
    const char *appId;
  } ASRConfig;

  char *asrCreate(ASRHandler **handlerAddress, ASRConfig *config, ASREventListener listener);

  bool asrSendStart(ASRHandler *handler, const char *taskId, const char *payload);

  int asrSend(ASRHandler *handler, const char *audioData, size_t dataSize);

  bool asrSendStop(ASRHandler *handler, const char *taskId);

  void asrDestroy(ASRHandler *handler);

#ifdef __cplusplus
}
#endif

#endif // AI_IOT_SDK_ASRT_H
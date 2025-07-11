#include "tts.h"
#include "lingxin_common.h"
#include "lingxin_json_util.h"
#include "lingxin_websocket.h"

#ifdef LINGXI_USE_VOICE_QUEUE
#include "lingxin_event_queue.h"
#include "lingxin_voice_queue.h"
#endif // LINGXI_USE_VOICE_QUEUE

struct TTSHandler
{
  const char *payload;
  WebsocketClient *websocket;
  TTSEventListener listener;
  void *voiceQueue;
  bool canRequestNextVoice;
  bool isVoiceEnd;
  void *notifyQueue;
  TTSExtraInfo *extraInfo;
  struct TTSHandler **selfPointer; // 存储双指针的引用
};

static char *getTaskId(TTSHandler *handler)
{
  return handler->extraInfo ? handler->extraInfo->taskId : "";
}

static char *getReqId(TTSHandler *handler)
{
  return handler->extraInfo ? handler->extraInfo->requestId : "";
}

static char *getTTSLogPre(TTSHandler *handler)
{
  char *logInstanceId = NULL;
  if (handler && handler->selfPointer && handler->extraInfo)
  {
    logInstanceId = handler->extraInfo->instanceId;
  }
  return (!logInstanceId || strlen(logInstanceId) == 0) ? "" : logInstanceId;
}

static void triggerCallbackOrEnqueue(TTSHandler *handler, TTSEventType eventType, const char *data, const size_t len)
{
  if (!handler->listener)
  {
    logPrintf("[%s], triggerCallback listener null", getTTSLogPre(handler));
    return;
  }
  if (handler->notifyQueue)
  {
#ifdef LINGXI_USE_VOICE_QUEUE
    eventQueueEnqueue(handler->notifyQueue, eventType, data, len);
#endif // LINGXI_USE_VOICE_QUEUE
    return;
  }
  handler->listener(eventType, data, len, handler->extraInfo);
}

#ifdef LINGXI_USE_VOICE_QUEUE
static void onEventQueueCallback(void *userContext, int event, const char *data,
                                 const size_t len)
{
  if (!userContext)
  {
    logPrintf("onEventQueueCallback params null");
    return;
  }
  TTSHandler *handler = (TTSHandler *)userContext;
  if (!handler)
  {
    logPrintf("onEventQueueCallback handler null");
    return;
  }
  if (!handler->listener)
  {
    return;
  }
  handler->listener(event, data, len, handler->extraInfo);
}

static bool requestNextAudioPackets(TTSHandler *handler, size_t space)
{
  logPrintf("[%s], requestNextAudioPackets", getTTSLogPre(handler));
  if (!handler || !handler->voiceQueue)
  {
    logPrintf("[%s], requestNextAudioPackets params null", getTTSLogPre(handler));
    return false;
  }
  char *message =
      snprintfWithMalloc("{\"header\":{\"action\":\"request_audio_packets\","
                         "\"task_id\":\"%s\"\"request_id\":\"%s\"},\"payload\":"
                         "{\"flow_control_parameters\":{\"data_size\":%zu}}}",
                         getTaskId(handler), getReqId(handler), space);
  bool result = websocketSendText(handler->websocket, message);
  free(message);
  return result;
}

static void checkNextVoiceRequest(TTSHandler *handler, size_t space)
{
  if (handler->canRequestNextVoice)
  {
    handler->canRequestNextVoice = !requestNextAudioPackets(handler, space);
  }
}

static bool continueWaitCheck(void *userContext, size_t space)
{
  TTSHandler *handler = (TTSHandler *)userContext;

  if (handler->isVoiceEnd)
  {
    triggerCallbackOrEnqueue(handler, TTS_EVENT_ON_SEND_END, NULL, 0);
    return false;
  }
  checkNextVoiceRequest(handler, space);
  return true;
}

static void initFlowControl(TTSHandler *handler)
{
  int poolSize = parsePoolSize(handler->payload);
  if (!poolSize)
  {
    logPrintf("[%s], initFlowControl poolSize null", getTTSLogPre(handler));
    return;
  }
  handler->notifyQueue = eventQueueCreate(handler, onEventQueueCallback);
  if (!handler->notifyQueue)
  {
    logPrintf("[%s], handler->notifyQueue poolSize null", getTTSLogPre(handler));
    return;
  }

  handler->voiceQueue = voiceQueueCreate(poolSize);
  if (!handler->voiceQueue)
  {
    logPrintf("[%s], initFlowControl voiceQueueCreate fail", getTTSLogPre(handler));
    eventQueueDestroy(handler->notifyQueue);
  }
  logPrintf("[%s], initFlowControl voiceQueueCreate after", getTTSLogPre(handler));
}

#endif // LINGXI_USE_VOICE_QUEUE

// 服务端新代码不兼容按需拉取的流控策略，先注释
// bool ttsGetNextFlow(TTSHandler *handler)
// {
// #ifdef LINGXI_USE_VOICE_QUEUE
//   logPrintf("[%s], ttsGetNextFlow begin", getTTSLogPre(handler));
//   if (!handler)
//   {
//     logPrintf("[%s], handler null", getTTSLogPre(handler));
//     return false;
//   }
//   if (!handler->voiceQueue)
//   {
//     logPrintf("[%s], ttsGetNextFlow params null", getTTSLogPre(handler));
//     return false;
//   }
//   logPrintf("[%s], getNextFlow: calloc VoiceChunk", getTTSLogPre(handler));

//   VoiceChunk *chunk = (VoiceChunk *)calloc(1, sizeof(VoiceChunk));
//   if (!chunk)
//   {
//     logPrintf("[%s], getNextFlow: Failed to allocate memory for voice chunk", getTTSLogPre(handler));
//     return false;
//   }
//   logPrintf("[%s], getNextFlow: begin, %d", getTTSLogPre(handler), handler->isVoiceEnd);
//   bool result =
//       voiceDequeue(handler->voiceQueue, chunk, continueWaitCheck, handler);
//   logPrintf("[%s], getNextFlow: result length: %d", getTTSLogPre(handler), chunk->length);

//   if (result)
//   {
//     triggerCallbackOrEnqueue(handler, TTS_EVENT_ON_SEND_RESULT, chunk->data,
//                              chunk->length);
//     if (isVoiceQueueSpaceEnough(handler->voiceQueue))
//     {
//       checkNextVoiceRequest(handler,
//                             getRemainSpaceOfVoiceQueue(handler->voiceQueue));
//     }
//     return true;
//   }
//   logPrintf("[%s], getNextFlow: finish", getTTSLogPre(handler));
// #endif // LINGXI_USE_VOICE_QUEUE
//   return false;
// }

static void dealEventFromServer(TTSHandler *handler, const char *event,
                                cJSON *message)
{
  logPrintf("[%s], dealEventFromServer: %s", getTTSLogPre(handler), event);

  if (!event)
  {
    return;
  }

  if (strcmp(event, "task_started") == 0)
  {
    char *reqId = parseRequestId(message);
    if (reqId && handler->extraInfo)
    {
      handler->extraInfo->requestId = reqId;
      char *copyReqId = strdup(reqId);
      if (copyReqId)
      {
        handler->extraInfo->requestId = copyReqId;
      }
    }
    triggerCallbackOrEnqueue(handler, TTS_EVENT_ON_SEND_START, NULL, 0);
  }
  else if (strcmp(event, "audio_packets_responded") == 0)
  {
    if (handler->voiceQueue)
    {
#ifdef LINGXI_USE_VOICE_QUEUE
      handler->canRequestNextVoice = true;
#endif // LINGXI_USE_VOICE_QUEUE
    }
  }
  else if (strcmp(event, "task_ended") == 0)
  {
    if (handler->voiceQueue)
    {
#ifdef LINGXI_USE_VOICE_QUEUE
      handler->isVoiceEnd = true;
      handler->canRequestNextVoice = false;
#endif // LINGXI_USE_VOICE_QUEUE
    }
    else
    {
      triggerCallbackOrEnqueue(handler, TTS_EVENT_ON_SEND_END, NULL, 0);
    }
  }
  else if (strcmp(event, "error") == 0)
  {
    char *errorInfo = parseErrorInfo(message);
    triggerCallbackOrEnqueue(handler, TTS_EVENT_ON_ERROR, errorInfo,
                             strlen(errorInfo));
  }
}

static void onEventMessageReceived(TTSHandler *handler, const char *message)
{
  cJSON *jsonMessage = cJSON_Parse(message);
  if (!jsonMessage)
  {
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr)
    {
      logPrintf("[%s], Error json: %s", getTTSLogPre(handler), message);
    }
    return;
  }
  // 解析 event
  const char *eventStr = parseEvent(jsonMessage);
  dealEventFromServer(handler, eventStr, jsonMessage);
  // Free the JSON object
  cJSON_Delete(jsonMessage);
}

static void freeTTS(TTSHandler **handlerAddress)
{
  if (!handlerAddress)
  {
    logPrintf("freeTTS handlerAddress null");
    return;
  }
  TTSHandler *handler = *handlerAddress;
  if (!handler)
  {
    logPrintf("freeTTS handler null");
    return;
  }
  logPrintf("[%s], freeTTS begin", getTTSLogPre(handler));

#ifdef LINGXI_USE_VOICE_QUEUE
  eventQueueDestroy(handler->notifyQueue);
  destroyVoiceQueue(handler->voiceQueue);
#endif // LINGXI_USE_VOICE_QUEUE

  if (handler->websocket && handler->websocket->config)
  {
    free(handler->websocket->config);
    handler->websocket->config = NULL;
  }
  if (handler->extraInfo)
  {
    free(handler->extraInfo);
    handler->extraInfo = NULL;
  }
  free(handler);
  *handlerAddress = NULL;

  logPrintf("freeTTS after");
}

static void onWebSocketEvent(WebSocketEventType event, const char *data,
                             const size_t len, const int isBinary,
                             void *userData)
{
  TTSHandler *handler = (TTSHandler *)userData;
  if (!handler)
  {
    logPrintf("onWebSocketEvent handler null");
    return;
  }
  switch (event)
  {
  case ON_WEBSOCKET_CONNECTION_SUCCESS:
    logPrintf("[%s], TTS CONNECTION_SUCCESS", getTTSLogPre(handler));
    triggerCallbackOrEnqueue(handler, TTS_EVENT_ON_READY, NULL, 0);
    break;
  case ON_WEBSOCKET_DATA_RECEIVED:
    logPrintf("[%s], TTS REVEIVE: %d, %d", getTTSLogPre(handler), len, isBinary);
    if (isBinary)
    {
      if (handler->voiceQueue)
      {
#ifdef LINGXI_USE_VOICE_QUEUE
        bool result = voiceEnqueue(handler->voiceQueue, data, len);
        logPrintf("[%s], onWebSocketEvent: enqueue result: %d", getTTSLogPre(handler), result);
#endif // LINGXI_USE_VOICE_QUEUE
      }
      else
      {
        triggerCallbackOrEnqueue(handler, TTS_EVENT_ON_SEND_RESULT, data, len);
      }
    }
    else
    {
      onEventMessageReceived(handler, data);
    }

    break;
  case ON_WEBSOCKET_CONNECTION_ERROR:
    triggerCallbackOrEnqueue(handler, TTS_EVENT_ON_ERROR, data, len);
    break;
  case ON_WEBSOCKET_DESTROY:
    logPrintf("[%s], TTS_EVENT_ON_DESTROY", getTTSLogPre(handler));
    triggerCallbackOrEnqueue(handler, TTS_EVENT_ON_DESTROY, data, len);
    freeTTS(handler->selfPointer);
    break;
  default:
    break;
  }
}

char *ttsCreate(TTSHandler **handlerAddress, TTSConfig *config, const char *payload, TTSEventListener listener)

{
  setLogEnable(config->showLog);
  logPrintf("ttsCreate  begin");

  if (!config || !config->sn || !config->appKey || !config->appId)
  {
    logPrintf("ttsCreate config params error!");
    return NULL;
  }

  TTSHandler *handler = (TTSHandler *)calloc(1, sizeof(struct TTSHandler));
  if (!handler)
  {
    logPrintf("Failed to allocate memory for TTS handler");
    return NULL;
  }
  handler->listener = listener;
  handler->canRequestNextVoice = false;
  handler->isVoiceEnd = false;
  handler->voiceQueue = NULL;
  handler->notifyQueue = NULL;

  WebsocketConfig *websocketConfig =
      createWebsocketConfig(handler, config->sn, config->appKey, config->appId,
                            WEBSOCKET_TTS_PATH, onWebSocketEvent);
  if (!websocketConfig)
  {
    logPrintf("Failed to create WebsocketConfig");
    free(handler);
    return NULL;
  }
  handler->websocket = initWebsocket(websocketConfig);
  if (!handler->websocket)
  {
    logPrintf("Failed to initialize WebSocket client");
    free(websocketConfig);
    free(handler);
    return NULL;
  }
  handler->payload = payload;
#ifdef LINGXI_USE_VOICE_QUEUE
  initFlowControl(handler);
#endif // LINGXI_USE_VOICE_QUEUE

  handler->extraInfo = NULL;
  TTSExtraInfo *extraInfo = (TTSExtraInfo *)calloc(1, sizeof(TTSExtraInfo));
  if (extraInfo)
  {
    extraInfo->taskId = "";
    extraInfo->requestId = "";
    extraInfo->instanceId = generateUUID(16);
    handler->extraInfo = extraInfo;
  }

  startWebsocket(handler->websocket);

  handler->selfPointer = handlerAddress; // 设置 selfPointer
  *handlerAddress = handler;             // 返回 handler

  logPrintf("[%s], asrCreate finish", getTTSLogPre(handler));
  return extraInfo ? extraInfo->instanceId : "";
}

bool ttsSendStart(TTSHandler *handler, const char *taskId)
{
  logPrintf("[%s], ttsSendStart begin", getTTSLogPre(handler));

  if (!handler)
  {
    logPrintf("handler null");
    return false;
  }
  if (!taskId || !handler->payload)
  {
    logPrintf("[%s], ttsSendStart params null", getTTSLogPre(handler));
    return false;
  }
  if (handler->extraInfo)
  {
    handler->extraInfo->taskId = (char *)taskId;
  }

  handler->isVoiceEnd = false;
  handler->canRequestNextVoice = false;
  char *message = snprintfWithMalloc("{\"header\":{\"action\":\"start_task\","
                                     "\"task_id\":\"%s\"},\"payload\":%s}",
                                     taskId, handler->payload);
  bool result = websocketSendText(handler->websocket, message);
  free(message);
  logPrintf("[%s], ttsSendStart result: %s", getTTSLogPre(handler), result ? "true" : "false");
  return result;
}

int ttsSend(TTSHandler *handler, const char *taskId, const char *text)
{
  logPrintf("[%s], ttsSend begin", getTTSLogPre(handler));

  if (!handler)
  {
    logPrintf("[%s], handler null", getTTSLogPre(handler));
    return 0;
  }
  if (!taskId || !text)
  {
    logPrintf("[%s], ttsSend params null", getTTSLogPre(handler));
    return 0;
  }
  if (handler->extraInfo)
  {
    handler->extraInfo->taskId = (char *)taskId;
  }

  char *message = snprintfWithMalloc(
      "{\"header\":{\"action\":\"send_text\",\"task_id\":\"%s\",\"request_id\":"
      "\"%s\"},\"payload\":{\"input\":{\"text\":\"%s\"}}}",
      taskId, getReqId(handler), text);
  int result = websocketSendText(handler->websocket, message);
  free(message); // 释放分配的内存
  logPrintf("[%s], ttsSend result: %d", getTTSLogPre(handler), result);

  return result;
}

bool ttsSendStop(TTSHandler *handler, const char *taskId)
{
  logPrintf("[%s], ttsSendStop begin", getTTSLogPre(handler));
  if (!handler)
  {
    logPrintf("handler null");
    return false;
  }
  if (!taskId)
  {
    logPrintf("[%s], ttsSendStop params null", getTTSLogPre(handler));
    return false;
  }

  if (handler->extraInfo)
  {
    handler->extraInfo->taskId = (char *)taskId;
  }

  char *message =
      snprintfWithMalloc("{\"header\":{\"action\":\"end_task\",\"task_id\":\"%"
                         "s\",\"request_id\":\"%s\"},\"payload\":{}}",
                         taskId, getReqId(handler));
  bool result = websocketSendText(handler->websocket, message);
  free(message);
  logPrintf("[%s], ttsSendStop result: %s", getTTSLogPre(handler), result ? "true" : "false");
  return result;
}

void ttsDestroy(TTSHandler *handler)
{
  logPrintf("[%s], ttsDestroy begin", getTTSLogPre(handler));
  if (!handler || !handler->selfPointer || !*handler->selfPointer)
  {
    logPrintf("handler or handler->selfPointer null");
    return;
  }
  closeWebsocket(handler->websocket);
  logPrintf("ttsDestroy after");
}

#include "voice_chat.h"
#include "cJSON.h"
#include "lingxin_common.h"
#include "lingxin_json_util.h"
#include "lingxin_websocket.h"
#include "chat_state_machine.h"
#ifdef LINGXI_USE_VOICE_QUEUE
#include "lingxin_event_queue.h"
#include "lingxin_voice_queue.h"
#endif // LINGXI_USE_VOICE_QUEUE

struct VoiceChatHandler
{
  VoiceChatConfig *config;
  WebsocketClient *websocket;
  VoiceChatEventListener listener;
  void *voiceQueue;
  bool canRequestNextVoice;
  bool isVoiceRespondingEnd;
  bool waitTerminate;
  void *notifyQueue;
  VoiceChatExtraInfo *extraInfo;
  struct VoiceChatHandler **selfPointer; // 存储双指针的引用
};
static char *getVoiceChatLogPre(VoiceChatHandler *handler)
{
  char *logInstanceId = NULL;
  if (handler && handler->selfPointer && handler->extraInfo)
  {
    logInstanceId = handler->extraInfo->instanceId;
  }
  return (!logInstanceId || strlen(logInstanceId) == 0) ? "" : logInstanceId;
}

static char *getReqId(VoiceChatHandler *handler)
{
  return handler->extraInfo ? handler->extraInfo->requestId : "";
}

static void triggerCallbackOrEnqueue(VoiceChatHandler *handler,
                                     VoiceChatEventType eventType,
                                     const char *data, const size_t len)
{
  if (!handler->listener)
  {
    logPrintf("[%s], triggerCallbackOrEnqueue listener invalid", getVoiceChatLogPre(handler));
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

static bool sendStartTask(VoiceChatHandler *handler)
{
  if (!handler)
  {
    logPrintf("sendStartTask handler null");
    return false;
  }

  if (!handler->config || !handler->config->taskId || !handler->config->payload)
  {
    logPrintf("sendStartTask  params valid");
    return false;
  }
  char *message =
      snprintfWithMalloc("{\"header\":{\"action\":\"start_task\",\"task_id\":"
                         "\"%s\"},\"payload\":%s}",
                         handler->config->taskId, handler->config->payload);
  logPrintf("sendStartTask begin: %s", message);
  bool result = websocketSendText(handler->websocket, message);
  free(message);
  logPrintf("sendStartTask result: %s", result ? "true" : "false");
  return result;
}
static void sendEndTask(VoiceChatHandler *handler)
{
  if (!handler)
  {
    logPrintf(" sendStartTask handler null");
    return;
  }

  if (!handler->config || !handler->config->taskId)
  {
    logPrintf("[%s], sendEndTask params null", getVoiceChatLogPre(handler));
    return;
  }
  if (handler->waitTerminate)
  {
    logPrintf("[%s], waitTerminate, can not send end_task", getVoiceChatLogPre(handler));
    return;
  }

  // 方法引用自chat_state_machince.c 后续改成状态获取方法
  if (get_chat_state_terminate()) {
    logPrintf("[%s], termianling, can not send end_task", getVoiceChatLogPre(handler));
    return;
  }

  char *message =
      snprintfWithMalloc("{\"header\":{\"action\":\"end_task\",\"task_id\":\"%"
                         "s\",\"request_id\":\"%s\"},\"payload\":{}}",
                         handler->config->taskId, getReqId(handler));
  bool result = websocketSendText(handler->websocket, message);
  if (result)
  {
    triggerCallbackOrEnqueue(handler, VOICECHAT_EVENT_ON_RESULT_AI_VOICE_READY_TO_END, NULL, 0);
  }
  free(message);
  logPrintf("[%s], sendEndTask result: %s", getVoiceChatLogPre(handler), result ? "true" : "false");
}

static char *getPayloadString(cJSON *message)
{
  cJSON *payload = cJSON_GetObjectItemCaseSensitive(message, "payload");
  if (!payload)
  {
    logPrintf("getPayloadString: Error getting payload from JSON");
    return NULL;
  }
  // 获取 payload 字符串
  char *payloadString = cJSON_PrintUnformatted(payload);
  if (!payloadString)
  {
    logPrintf("getPayloadString:", "Error printing payload as string");
    return NULL;
  }
  return payloadString;
}

static void dealEventFromServer(VoiceChatHandler *handler, const char *event,
                                cJSON *message)
{
  logPrintf("[%s], dealEventFromServer: %s", getVoiceChatLogPre(handler), event);
  if (!event)
  {
    return;
  }
  // 等待打断期间，只接收task_terminated和error
  if (handler->waitTerminate && strcmp(event, "task_terminated") &&
      strcmp(event, "error"))
  {
    logPrintf("[%s], waitTerminate, discard data  %s", getVoiceChatLogPre(handler), event);
    return;
  }
  if (strcmp(event, "task_started") == 0)
  {
    char *reqId = parseRequestId(message);
    logPrintf("[%s], dealEventFromServer requesId:  %s", getVoiceChatLogPre(handler), reqId);

    if (reqId && handler->extraInfo)
    {
      handler->extraInfo->requestId = reqId;
      char *copyReqId = strdup(reqId);
      if (copyReqId)
      {
        handler->extraInfo->requestId = copyReqId;
      }
    }
    // 复位，防止异常
    handler->waitTerminate = false;
    triggerCallbackOrEnqueue(handler, VOICECHAT_EVENT_ON_VOIC_SEND_READY, NULL, 0);
  }
  else if (strcmp(event, "audio_response_start") == 0)
  {
    triggerCallbackOrEnqueue(handler, VOICECHAT_EVENT_ON_RESULT_AI_VOICE_START, NULL, 0);
  }
  else if (strcmp(event, "audio_packets_responded") == 0)
  {
    if (handler->voiceQueue != NULL)
    {
#ifdef LINGXI_USE_VOICE_QUEUE
      handler->canRequestNextVoice = true;
#endif // LINGXI_USE_VOICE_QUEUE
    }
  }
  else if (strcmp(event, "audio_response_end") == 0)
  {
    sendEndTask(handler);
  }
  else if (strcmp(event, "task_terminated") == 0)
  {
    handler->waitTerminate = false;
    triggerCallbackOrEnqueue(handler, VOICECHAT_EVENT_ON_TERMINAL, NULL, 0);
  }
  else if (strcmp(event, "task_ended") == 0)
  {
    if (handler->voiceQueue != NULL)
    {
#ifdef LINGXI_USE_VOICE_QUEUE
      handler->isVoiceRespondingEnd = true;
      handler->canRequestNextVoice = false;
#endif // LINGXI_USE_VOICE_QUEUE
    }
    else
    {
      triggerCallbackOrEnqueue(handler, VOICECHAT_EVENT_ON_RESULT_AI_VOICE_END, NULL, 0);
    }
  }
  else if (strcmp(event, "text_output") == 0)
  {
    const char *payload = parsePayloadStr(message);
    logPrintf("[%s], onEventMessageReceived: %s", getVoiceChatLogPre(handler), payload);

    triggerCallbackOrEnqueue(handler, VOICECHAT_EVENT_ON_TEXTOUT, payload, strlen(payload));
  }
  else if (strcmp(event, "error") == 0)
  {
    char *errorInfo = parseErrorInfo(message);
    triggerCallbackOrEnqueue(handler, VOICECHAT_EVENT_ON_ERROR, errorInfo, strlen(errorInfo));
  }
  else if (strcmp(event, "vad_end") == 0)
  {
    triggerCallbackOrEnqueue(handler, VOICECHAT_EVENT_ON_VAD_END, NULL, 0);
  }
  else if (strcmp(event, "vad_exit") == 0)
  {
    triggerCallbackOrEnqueue(handler, VOICECHAT_EVENT_ON_VAD_EXIT, NULL, 0);
  }
}

static void onEventMessageReceived(VoiceChatHandler *handler,
                                   const char *message)
{
  logPrintf("[%s], onEventMessageReceived: %s", getVoiceChatLogPre(handler), message);
  cJSON *jsonMessage = cJSON_Parse(message);
  if (!jsonMessage)
  {
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr)
    {
      logPrintf("[%s], onEventMessageReceived: Error json: %s", getVoiceChatLogPre(handler), message);
    }
    return;
  }
  // 解析 event
  const char *eventStr = parseEvent(jsonMessage);
  dealEventFromServer(handler, eventStr, jsonMessage);
  cJSON_Delete(jsonMessage);
}

static void freeVoiceChat(VoiceChatHandler **handlerAddress)
{

  if (!handlerAddress)
  {
    logPrintf("freeVoiceChat handlerAddress null");
    return;
  }

  VoiceChatHandler *handler = *handlerAddress;
  if (!handler)
  {
    logPrintf("freeVoiceChat handler null");
    return;
  }
  logPrintf("[%s], freeVoiceChat begin", getVoiceChatLogPre(handler));
#ifdef LINGXI_USE_VOICE_QUEUE
  eventQueueDestroy(handler->notifyQueue);
  destroyVoiceQueue(handler->voiceQueue);
#endif // LINGXI_USE_VOICE_QUEUE
  if (handler->websocket && handler->websocket->config)
  {
    free(handler->websocket->config);
  }
  if (handler->extraInfo)
  {
    free(handler->extraInfo);
    handler->extraInfo = NULL;
  }
  free(handler);
  *handlerAddress = NULL;

  logPrintf("freeVoiceChat after");
}

static void onWebSocketEvent(WebSocketEventType event, const char *data,
                             const size_t len, const int isBinary,
                             void *userData)
{
  VoiceChatHandler *handler = (VoiceChatHandler *)userData;
  if (!handler)
  {
    logPrintf("onWebSocketEvent handler null");
    return;
  }
  switch (event)
  {
  case ON_WEBSOCKET_CONNECTION_SUCCESS:
    logPrintf("[%s], ON_WEBSOCKET_CONNECTION_SUCCESS", getVoiceChatLogPre(handler));
    sendStartTask(handler);
    break;
  case ON_WEBSOCKET_DATA_RECEIVED:
    if (isBinary)
    {
      if (!handler->waitTerminate)
      {
        if (handler->voiceQueue)
        {
#ifdef LINGXI_USE_VOICE_QUEUE
          voiceEnqueue(handler->voiceQueue, data, len);
#endif // LINGXI_USE_VOICE_QUEUE
        }
        else
        {
          triggerCallbackOrEnqueue(handler, VOICECHAT_EVENT_ON_RESULT_AI_VOICE,
                                   data, len);
        }
      }
      else
      {
        logPrintf("[%s], waitTerminate, discard data length %d", getVoiceChatLogPre(handler), len);
      }
    }
    else
    {
      onEventMessageReceived(handler, data);
    }
    break;
  case ON_WEBSOCKET_CONNECTION_ERROR:
  {
    logPrintf(data);
    char *errorData = (char *)malloc(len + 1);
    if (!errorData)
    {
      logPrintf("[%s], Failed to allocate memory for error data", getVoiceChatLogPre(handler));
      return;
    }
    // 复制数据
    memcpy(errorData, data, len);
    // 添加字符串结束符
    errorData[len] = '\0';
    triggerCallbackOrEnqueue(handler, VOICECHAT_EVENT_ON_ERROR, errorData,
                             len + 1);
  }
  break;
  case ON_WEBSOCKET_DESTROY:
    if (handler->notifyQueue)
    {
#ifdef LINGXI_USE_VOICE_QUEUE
      eventQueueEnqueue(handler->notifyQueue, EVENT_QUEUE_FINISH_FLAG, NULL, 0);
#endif // LINGXI_USE_VOICE_QUEUE
    }
    else
    {
      handler->listener(VOICECHAT_EVENT_ON_DESTROY, NULL, 0, handler->extraInfo);
      freeVoiceChat(handler->selfPointer);
    }
    break;
  default:
    break;
  }
}

#ifdef LINGXI_USE_VOICE_QUEUE
static void onEventQueueCallback(void *userContext, int event, const char *data,
                                 const size_t len)
{
  if (!userContext)
  {
    logPrintf("onEventQueueCallback userContext null");
    return;
  }
  VoiceChatHandler *handler = (VoiceChatHandler *)userContext;
  if (!handler)
  {
    logPrintf("onEventQueueCallback handler null");
    return;
  }
  // 事件队列可以销毁了
  if (event == EVENT_QUEUE_FINISH_FLAG)
  {
    logPrintf("[%s], onEventQueueCallback EVENT_QUEUE_FINISH_FLAG", getVoiceChatLogPre(handler));
    if (handler->listener)
    {
      handler->listener(VOICECHAT_EVENT_ON_DESTROY, NULL, 0, handler->extraInfo);
    }
    freeVoiceChat(handler->selfPointer);
    return;
  }
  if (handler->listener)
  {
    handler->listener(event, data, len, handler->extraInfo);
  }
}

static bool requestNextAudioPackets(VoiceChatHandler *handler, size_t space)
{
  if (!handler)
  {
    logPrintf("requestNextAudioPackets handler null");
    return false;
  }
  if (!handler->voiceQueue)
  {
    logPrintf("[%s], requestNextAudioPackets params null", getVoiceChatLogPre(handler));
    return false;
  }
  char *message =
      snprintfWithMalloc("{\"header\":{\"action\":\"request_audio_packets\","
                         "\"task_id\":\"%s\"\"request_id\":\"%s\"},\"payload\":"
                         "{\"flow_control_parameters\":{\"data_size\":%zu}}}",
                         handler->config->taskId, getReqId(handler), space);
  bool result = websocketSendText(handler->websocket, message);
  free(message);
  return result;
}

static bool checkNextVoiceRequest(VoiceChatHandler *handler, size_t space)
{
  if (handler->canRequestNextVoice)
  {
    handler->canRequestNextVoice = !requestNextAudioPackets(handler, space);
    return handler->canRequestNextVoice;
  }
  return false;
}

static bool continueWaitCheck(void *userContext, size_t space)
{
  VoiceChatHandler *handler = (VoiceChatHandler *)userContext;
  if (!handler)
  {
    logPrintf("onEventQueueCallback handler is invalid");
    return false;
  }
  if (handler->isVoiceRespondingEnd)
  {
    eventQueueEnqueue(handler->notifyQueue,
                      VOICECHAT_EVENT_ON_RESULT_AI_VOICE_END, NULL, 0);
    return false;
  }
  checkNextVoiceRequest(handler, space);
  return true;
}

static void initFlowControl(VoiceChatHandler *handler)
{

  int poolSize = parsePoolSize(handler->config->payload);
  if (!poolSize)
  {
    logPrintf("[%s], dynamic flow control", getVoiceChatLogPre(handler));
    return;
  }
  handler->notifyQueue = eventQueueCreate(handler, onEventQueueCallback);
  if (!handler->notifyQueue)
  {
    return;
  }
  handler->voiceQueue = voiceQueueCreate(poolSize);
  if (!handler->voiceQueue)
  {
    logPrintf("[%s], initFlowControl voiceQueueCreate fail", getVoiceChatLogPre(handler));
    eventQueueDestroy(handler->notifyQueue);
  }
  logPrintf("[%s], initFlowControl voiceQueueCreate after", getVoiceChatLogPre(handler));
}

#endif // LINGXI_USE_VOICE_QUEUE

// 服务端新代码不兼容按需拉取的流控策略，先注释
// bool voiceChatGetNextFlow(VoiceChatHandler *handler)
// {
// #ifdef LINGXI_USE_VOICE_QUEUE
//   logPrintf("[%s], voiceChatGetNextFlow begin", getVoiceChatLogPre(handler));

//   if (!handler)
//   {
//     logPrintf(" handler null");
//     return false;
//   }
//   if (!handler->voiceQueue)
//   {
//     logPrintf("[%s], voiceChatGetNextFlow params null", getVoiceChatLogPre(handler));
//     return false;
//   }
//   VoiceChunk *chunk = (VoiceChunk *)calloc(1, sizeof(VoiceChunk));
//   if (!chunk)
//   {
//     logPrintf("[%s], voiceChatGetNextFlow: Failed to allocate memory for voice chunk", getVoiceChatLogPre(handler));
//     return false;
//   }
//   logPrintf("[%s], voiceChatGetNextFlow: begin, %d", getVoiceChatLogPre(handler), handler->isVoiceRespondingEnd);
//   bool result =
//       voiceDequeue(handler->voiceQueue, chunk, continueWaitCheck, handler);
//   logPrintf("[%s], voiceChatGetNextFlow: result length: %d", getVoiceChatLogPre(handler), chunk->length);
//   if (result)
//   {
//     eventQueueEnqueue(handler->notifyQueue, VOICECHAT_EVENT_ON_RESULT_AI_VOICE,
//                       chunk->data, chunk->length);
//     if (isVoiceQueueSpaceEnough(handler->voiceQueue))
//     {
//       return checkNextVoiceRequest(
//           handler, getRemainSpaceOfVoiceQueue(handler->voiceQueue));
//     }
//   }
//   logPrintf("[%s], voiceChatGetNextFlow: finish", getVoiceChatLogPre(handler));
// #endif // LINGXI_USE_VOICE_QUEUE
//   return false;
// }

char *voiceChatCreate(VoiceChatHandler **handlerAddress, VoiceChatConfig *config,
                      VoiceChatEventListener listener)
{
  setLogEnable(config->showLog);

  logPrintf("voiceChatCreate  begin");

  if (!config || !config->sn || !config->appKey || !config->appId)
  {
    logPrintf("voiceChatCreate config params error!");
    return NULL;
  }
  VoiceChatHandler *handler =
      (VoiceChatHandler *)calloc(1, sizeof(struct VoiceChatHandler));
  if (!handler)
  {
    logPrintf("Failed to allocate memory for voicechat handler");
    return NULL;
  }
  handler->listener = listener;
  handler->canRequestNextVoice = false;
  handler->isVoiceRespondingEnd = false;
  handler->waitTerminate = false;

  handler->voiceQueue = NULL;
  handler->notifyQueue = NULL;

  handler->config = config;
  WebsocketConfig *websocketConfig =
      createWebsocketConfig(handler, config->sn, config->appKey, config->appId,
                            config->serverPath, onWebSocketEvent);
  if (!websocketConfig)
  {
    logPrintf("Failed to create WebsocketConfig");
    free(handler);
    return NULL;
  }

  handler->websocket = initWebsocket(websocketConfig);
  if (!handler->websocket)
  {
    free(websocketConfig);
    free(handler);
    return NULL;
  }

#ifdef LINGXI_USE_VOICE_QUEUE
  initFlowControl(handler);
#endif // LINGXI_USE_VOICE_QUEUE

  handler->extraInfo = NULL;
  VoiceChatExtraInfo *extraInfo = (VoiceChatExtraInfo *)calloc(1, sizeof(VoiceChatExtraInfo));
  if (extraInfo)
  {
    extraInfo->taskId = (char *)config->taskId;
    extraInfo->requestId = "";
    extraInfo->instanceId = generateUUID(16);
    handler->extraInfo = extraInfo;
  }

  handler->selfPointer = handlerAddress; // 设置 selfPointer
  *handlerAddress = handler;             // 返回 handler
  //modified by beken
  startWebsocket(handler->websocket);

  logPrintf("[%s], voiceChatCreate finish", getVoiceChatLogPre(handler));
  return extraInfo ? extraInfo->instanceId : "";
}

int voiceChatSend(VoiceChatHandler *handler, const char *audioData,
                  size_t dataSize)
{
  logPrintf("[%s], voiceChatSend begin", getVoiceChatLogPre(handler));

  if (!handler)
  {
    logPrintf("handler null");
    return 0;
  }
  if (!audioData || dataSize == 0)
  {
    logPrintf("[%s], voiceChatSend params null", getVoiceChatLogPre(handler));
    return 0;
  }
  handler->isVoiceRespondingEnd = false;
  handler->canRequestNextVoice = false;
  int result = websocketSendBinary(handler->websocket, audioData, dataSize);
  logPrintf("[%s], voiceChatSend result: %d", getVoiceChatLogPre(handler), result);
  return result;
}

bool voiceChatSendStop(VoiceChatHandler *handler)
{
  logPrintf("[%s], voiceChatSendStop begin", getVoiceChatLogPre(handler));

  if (!handler)
  {
    logPrintf("handler null");
    return false;
  }
  if (!handler->config || !handler->config->taskId)
  {
    logPrintf("[%s], voiceChatSendStop params null", getVoiceChatLogPre(handler));
    return false;
  }
  char *message =
      snprintfWithMalloc("{\"header\":{\"action\":\"end_audio\",\"task_id\":\"%"
                         "s\",\"request_id\":\"%s\"},\"payload\":{}}",
                         handler->config->taskId, getReqId(handler));
  bool result = websocketSendText(handler->websocket, message);
  free(message);
  logPrintf("[%s], voiceChatSendStop result: %s", getVoiceChatLogPre(handler), result ? "true" : "false");
  return result;
}

bool voiceChatTerminal(VoiceChatHandler *handler)
{
  logPrintf("[%s], voiceChatTerminal begin", getVoiceChatLogPre(handler));

  if (!handler)
  {
    logPrintf(" handler null");
    return false;
  }
  if (!handler->config || !handler->config->taskId)
  {
    logPrintf("[%s], voiceChatTerminal params null", getVoiceChatLogPre(handler));
    return false;
  }
  char *message =
      snprintfWithMalloc("{\"header\":{\"action\":\"terminate_task\",\"task_"
                         "id\":\"%s\",\"request_id\":\"%s\"},\"payload\":{}}",
                         handler->config->taskId, getReqId(handler));
  bool result = websocketSendText(handler->websocket, message);
  if (result)
  {
    handler->waitTerminate = true;
  }
  free(message);
  logPrintf("[%s], voiceChatTerminal result: %s", getVoiceChatLogPre(handler), result ? "true" : "false");
  return result;
}

bool voiceChatContinue(VoiceChatHandler *handler)
{
  logPrintf("[%s], voiceChatContinue begin", getVoiceChatLogPre(handler));

  if (!handler)
  {
    logPrintf(" handler null");
    return false;
  }
  bool result = sendStartTask(handler);
  handler->isVoiceRespondingEnd = false;
  logPrintf("[%s], voiceChatContinue result: %s", getVoiceChatLogPre(handler), result ? "true" : "false");
  return result;
}

bool voiceChatStartNewChat(VoiceChatHandler *handler, VoiceChatConfig *config)
{
  logPrintf("[%s], voiceChatStartNewChat begin", getVoiceChatLogPre(handler));

  if (!handler)
  {
    logPrintf("handler null");
    return false;
  }
  bool result = sendStartTask(handler);
  handler->config = config;
  if (handler->extraInfo)
  {
    handler->extraInfo->taskId = (char *)config->taskId;
  }
  handler->isVoiceRespondingEnd = false;
  logPrintf("[%s], voiceChatStartNewChat result: %s", getVoiceChatLogPre(handler), result ? "true" : "false");
  return result;
}

void voiceChatDestroy(VoiceChatHandler *handler)
{
  logPrintf("[%s], voiceChatDestroy begin", getVoiceChatLogPre(handler));

  if (!handler || !handler->selfPointer || !*handler->selfPointer)
  {
    logPrintf("handler or handler->selfPointer null");
    return;
  }
  closeWebsocket(handler->websocket);
  logPrintf("voiceChatDestroy after");
}
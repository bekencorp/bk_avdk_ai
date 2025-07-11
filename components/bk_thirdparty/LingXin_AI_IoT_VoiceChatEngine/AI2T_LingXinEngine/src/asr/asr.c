#include "asr.h"
#include "cJSON.h"
#include "lingxin_common.h"
#include "lingxin_json_util.h"
#include "lingxin_websocket.h"
#include <stdarg.h>

struct ASRHandler
{
  WebsocketClient *websocket;
  ASREventListener listener;
  ASRExtraInfo *extraInfo;
  struct ASRHandler **selfPointer; // 存储双指针的引用
};

static char *getASRLogPre(ASRHandler *handler)
{
  char *logInstanceId = NULL;
  if (handler && handler->selfPointer && handler->extraInfo)
  {
    logInstanceId = handler->extraInfo->instanceId;
  }
  return (!logInstanceId || strlen(logInstanceId) == 0) ? "" : logInstanceId;
}

static void triggerCallback(ASRHandler *handler, ASREventType eventType,
                            const char *data, const size_t len)
{
  if (!handler)
  {
    logPrintf("triggerCallback handler null");
    return;
  }
  if (!handler->listener)
  {
    logPrintf("[%s], triggerCallback listener null", getASRLogPre(handler));
    return;
  }
  handler->listener(eventType, data, len, handler->extraInfo);
}

static void dealEventFromServer(ASRHandler *handler, const char *event,
                                cJSON *message)
{
  if (!event)
  {
    logPrintf("[%s], aaa", "event is null", getASRLogPre(handler));
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
    triggerCallback(handler, ASR_EVENT_ON_SEND_START, NULL, 0);
  }
  else if (strcmp(event, "text_result_generated") == 0)
  {
    const char *textResult = parsePayloadStr(message);
    if (!textResult)
    {
      logPrintf("[%s], Failed to parse text result", getASRLogPre(handler));
      return;
    }
    const int length = strlen(textResult);
    logPrintf("[%s], dealEventFromServer: %d", getASRLogPre(handler), length);

    triggerCallback(handler, ASR_EVENT_ON_SEND_RESULT, textResult, length);
  }
  else if (strcmp(event, "task_ended") == 0)
  {
    triggerCallback(handler, ASR_EVENT_ON_SEND_END, NULL, 0);
  }
  else if (strcmp(event, "error") == 0)
  {
    char *errorInfo = parseErrorInfo(message);
    triggerCallback(handler, ASR_EVENT_ON_ERROR, errorInfo, strlen(errorInfo));
  }
}

static void onMessageReceived(ASRHandler *handler, const char *message)
{
  logPrintf("[%s], asr onMessageReceived, eventStr = %s", getASRLogPre(handler), message);

  cJSON *jsonMessage = cJSON_Parse(message);
  if (!jsonMessage)
  {
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr)
    {
      logPrintf("[%s], Error json: %s", getASRLogPre(handler), message);
    }
    return;
  }
  // 解析 event
  const char *eventStr = parseEvent(jsonMessage);
  dealEventFromServer(handler, eventStr, jsonMessage);
  // Free the JSON object
  cJSON_Delete(jsonMessage);
}

static void freeASR(ASRHandler **handlerAddress)
{
  if (!handlerAddress)
  {
    logPrintf("freeASR handlerAddress null");
    return;
  }
  ASRHandler *handler = *handlerAddress;
  if (!handler)
  {
    logPrintf("freeASR handler null");
    return;
  }
  logPrintf("[%s], freeASR begin", getASRLogPre(handler));

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
  logPrintf("freeASR finish");
}

static void onWebSocketEvent(WebSocketEventType event, const char *data,
                             const size_t len, const int isBinary,
                             void *userData)
{
  ASRHandler *handler = (ASRHandler *)userData;
  if (!handler)
  {
    logPrintf("onWebSocketEvent handler null");
    return;
  }
  switch (event)
  {
  case ON_WEBSOCKET_CONNECTION_SUCCESS:
    triggerCallback(handler, ASR_EVENT_ON_READY, NULL, 0);
    break;
  case ON_WEBSOCKET_DATA_RECEIVED:
    onMessageReceived(handler, data);
    break;
  case ON_WEBSOCKET_CONNECTION_ERROR:
    logPrintf("[%s], triggerCallback ON_WEBSOCKET_CONNECTION_ERROR", getASRLogPre(handler));
    triggerCallback(handler, ASR_EVENT_ON_ERROR, data, len);
    break;
  case ON_WEBSOCKET_DESTROY:
    logPrintf("[%s], triggerCallback ON_WEBSOCKET_DESTROY", getASRLogPre(handler));
    triggerCallback(handler, ASR_EVENT_ON_DESTROY, data, len);
    freeASR(handler->selfPointer);
    break;
  default:
    break;
  }
}

char *asrCreate(ASRHandler **handlerAddress, ASRConfig *config, ASREventListener listener)
{
  setLogEnable(config->showLog);

  logPrintf("asrCreate  begin");

  ASRHandler *handler = (ASRHandler *)calloc(1, sizeof(ASRHandler));

  if (!handler)
  {
    logPrintf("Failed to allocate memory for ASR handler");
    return NULL;
  }
  WebsocketConfig *websocketConfig =
      createWebsocketConfig(handler, config->sn, config->appKey, config->appId,
                            WEBSOCKET_ASR_PATH, onWebSocketEvent);
  if (!websocketConfig)
  {
    logPrintf("Failed to create WebsocketConfig");
    free(handler);
    return NULL;
  }
  WebsocketClient *client = initWebsocket(websocketConfig);

  if (!client)
  {
    logPrintf("Failed to initialize WebSocket client");
    free(websocketConfig);
    free(handler);
    return NULL;
  }
  handler->websocket = client;
  handler->listener = listener;
  handler->extraInfo = NULL;

  ASRExtraInfo *extraInfo = (ASRExtraInfo *)calloc(1, sizeof(ASRExtraInfo));

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

  logPrintf("[%s], asrCreate finish", getASRLogPre(handler));
  return extraInfo ? extraInfo->instanceId : "";
}

bool asrSendStart(ASRHandler *handler, const char *taskId, const char *payload)
{
  logPrintf("asrSendStart begin");

  if (!handler)
  {
    logPrintf("handler null");
    return false;
  }
  if (!taskId || !payload)
  {
    logPrintf("[%s], taskId or payload null", getASRLogPre(handler));
    return false;
  }

  if (handler->extraInfo)
  {
    handler->extraInfo->taskId = (char *)taskId;
  }

  char *message = snprintfWithMalloc("{\"header\":{\"action\":\"start_task\","
                                     "\"task_id\":\"%s\"},\"payload\":%s}",
                                     taskId, payload);
  if (!message)
  {
    logPrintf("[%s],Failed to allocate memory for message", getASRLogPre(handler));
    return false;
  }
  bool result = websocketSendText(handler->websocket, message);
  free(message);
  logPrintf("[%s],asrSendStart result: %s", getASRLogPre(handler), result ? "true" : "false");
  return result;
}

int asrSend(ASRHandler *handler, const char *audioData, size_t dataSize)
{
  logPrintf("[%s], asrSend begin", getASRLogPre(handler));

  if (!handler)
  {
    logPrintf("handler null");
    return 0;
  }
  if (!audioData || !dataSize)
  {
    logPrintf("[%s], audioData or dataSize null", getASRLogPre(handler));
    return 0;
  }
  int result = websocketSendBinary(handler->websocket, audioData, dataSize);
  logPrintf("[%s], asrSend result: %d", getASRLogPre(handler), result);
  return result;
}

void asrDestroy(ASRHandler *handler)
{
  logPrintf("[%s], asrDestroy begin", getASRLogPre(handler));

  if (!handler || !handler->selfPointer || !*handler->selfPointer)
  {
    logPrintf("handler or handler->selfPointer null");
    return;
  }
  closeWebsocket(handler->websocket);
  logPrintf("asrDestroy after");
}

bool asrSendStop(ASRHandler *handler, const char *taskId)
{
  logPrintf("[%s], asrSendStop begin", getASRLogPre(handler));

  if (!handler)
  {
    logPrintf("handler null");
    return false;
  }
  if (!taskId)
  {
    logPrintf("[%s], taskId null", getASRLogPre(handler));
    return false;
  }

  if (handler->extraInfo)
  {
    handler->extraInfo->taskId = (char *)taskId;
  }

  char *message = snprintfWithMalloc(
      "{\"header\":{\"action\":\"end_task\",\"task_id\":\"%s\",\"request_id\":"
      "\"%s\"},\"payload\":{}}",
      taskId, handler->extraInfo ? handler->extraInfo->requestId : "");
  if (!message)
  {
    logPrintf("[%s], Failed to allocate memory for message", getASRLogPre(handler));
    return false;
  }
  bool result = websocketSendText(handler->websocket, message);
  free(message);
  logPrintf("[%s], asrSendStop result: %s", getASRLogPre(handler), result ? "true" : "false");
  return result;
}
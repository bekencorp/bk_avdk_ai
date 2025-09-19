#include "schedule_first_ws.h"
#include "cJSON.h"
#include "lingxin_common.h"
#include "lingxin_json_util.h"
#include "lingxin_websocket.h"
#include "chat_state_machine.h"
#include "lingxin_timer.h"
#include "lingxin_log.h"

#ifdef LINGXI_USE_VOICE_QUEUE
#include "lingxin_event_queue.h"
#include "lingxin_voice_queue.h"
#endif // LINGXI_USE_VOICE_QUEUE

struct ScheduleChatHandler
{
  ScheduleChatConfig *config;
  WebsocketClient *websocket;
  SystemEventListener listener;
  VoiceChatExtraInfo *extraInfo;
  bool isRecievedSystemEvent;
  bool isWsConnectSuccess;
};

static char *getReqId(ScheduleChatHandler *handler)
{
  return handler->extraInfo ? handler->extraInfo->requestId : "";
}


static void dealEventFromServer(ScheduleChatHandler *handler, const char *event,
                                cJSON *message)
{
  lingxin_log_debug("dealEventFromServer: %s", event);
  if (!event)
  {
    lingxin_log_debug("没有event");
    return;
  }
  // 等待打断期间，只接收task_terminated和error
  if (strcmp(event, "error") == 0)
  {
    char *errorInfo = parseErrorInfo(message);
    lingxin_log_debug("error: %s", errorInfo);
    cJSON_free(errorInfo);
  }
  else if (strcmp(event, "system_event") == 0)
  {
    // TODO: 撤掉注释
    const char *payload = parsePayloadStr(message);
    lingxin_log_debug("system_event: %s", payload);
    handler->isRecievedSystemEvent = true;
    handler->listener(handler, payload);
    if (handler->isWsConnectSuccess)
    {
      scheduleWsDestroy(handler);
    }
    cJSON_free((char*)payload);
  }
}

static void onEventMessageReceived(ScheduleChatHandler *handler,
                                   const char *message)
{
  lingxin_log_debug(" onEventMessageReceived: %s", message);
  cJSON *jsonMessage = cJSON_Parse(message);
  if (!jsonMessage)
  {
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr)
    {
      lingxin_log_error(" onEventMessageReceived: Error json: %s", message);
    }
    return;
  }
  // 解析 event
  const char *eventStr = parseEvent(jsonMessage);
  dealEventFromServer(handler, eventStr, jsonMessage);
  cJSON_Delete(jsonMessage);
}

static void freeScheduleWs(ScheduleChatHandler **handlerAddress)
{

  if (!handlerAddress)
  {
    lingxin_log_error("freeScheduleWs handlerAddress null");
    return;
  }

  ScheduleChatHandler *handler = *handlerAddress;
  if (!handler)
  {
    lingxin_log_error("freeScheduleWs handler null");
    return;
  }
  lingxin_log_debug(" freeScheduleWs begin");
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

  lingxin_log_debug("freeScheduleWs after");
}

static void onWebSocketEvent(WebSocketEventType event, const char *data,
                             const size_t len, const int isBinary,
                             void *userData)
{
  ScheduleChatHandler *handler = (ScheduleChatHandler *)userData;
  if (!handler)
  {
    lingxin_log_error("onWebSocketEvent handler null");
    return;
  }
  switch (event)
  {
  case ON_WEBSOCKET_CONNECTION_SUCCESS:
    lingxin_log_debug("ON_WEBSOCKET_CONNECTION_SUCCESS");
    handler->isWsConnectSuccess = true;
    if (handler->isRecievedSystemEvent)
    {
      // TODO: 撤掉注释
      scheduleWsDestroy(handler);
    }
    break;
  case ON_WEBSOCKET_DATA_RECEIVED:
    lingxin_log_debug("FirstScheduleConnect ON_WEBSOCKET_DATA_RECEIVED");
    if (isBinary)
    {
      lingxin_log_debug("服务端推送音频数据");
    }
    else
    {
      onEventMessageReceived(handler, data);
    }
    break;
  case ON_WEBSOCKET_CONNECTION_ERROR:
  {
    lingxin_log_debug(data);
    char *errorData = (char *)malloc(len + 1);
    if (!errorData)
    {
      lingxin_log_error(" Failed to allocate memory for error data");
      return;
    }
    // 复制数据
    memcpy(errorData, data, len);
    // 添加字符串结束符
    errorData[len] = '\0';
    lingxin_log_debug("Websocket Error data: %s", errorData);
  }
  break;
  case ON_WEBSOCKET_DESTROY:
    lingxin_log_debug("销毁初始化时用于定时任务同步的ws连接");
    freeScheduleWs(&handler);
    break;
  default:
    break;
  }
}
char *startFirstScheduleConnect(ScheduleChatConfig *config,
                                SystemEventListener listener)
{

  lingxin_log_debug("startFirstScheduleConnect  begin");

  if (!config || !config->sn || !config->appKey || !config->appId)
  {
    lingxin_log_error("startFirstScheduleConnect config params error!");
    return NULL;
  }
  ScheduleChatHandler *handler =
      (ScheduleChatHandler *)calloc(1, sizeof(struct ScheduleChatHandler));
  if (!handler)
  {
    lingxin_log_error("Failed to allocate memory for startFirstScheduleConnect handler");
    return NULL;
  }
  handler->listener = listener;
  handler->isRecievedSystemEvent = false;
  handler->isWsConnectSuccess = false;
  handler->config = config;
  WebsocketConfig *websocketConfig =
      createWebsocketConfig(handler, config->sn, config->appKey, config->appId,
                            config->serverPath, onWebSocketEvent);
  if (!websocketConfig)
  {
    lingxin_log_error("Failed to create WebsocketConfig");
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

  handler->extraInfo = NULL;
  VoiceChatExtraInfo *extraInfo = (VoiceChatExtraInfo *)calloc(1, sizeof(VoiceChatExtraInfo));
  if (extraInfo)
  {
    extraInfo->taskId = (char *)config->taskId;
    extraInfo->requestId = "";
    extraInfo->instanceId = generateUUID(16);
    handler->extraInfo = extraInfo;
  }
  startWebsocket(handler->websocket);

  lingxin_log_debug("startFirstScheduleConnect finish");
  return extraInfo ? extraInfo->instanceId : "";
}

void scheduleWsDestroy(ScheduleChatHandler *handler)
{
  lingxin_log_debug("scheduleWsDestroy begin");

  if (!handler)
  {
    lingxin_log_debug("handler null");
    return;
  }
  closeWebsocket(handler->websocket);
  lingxin_log_debug("scheduleWsDestroy after");
}
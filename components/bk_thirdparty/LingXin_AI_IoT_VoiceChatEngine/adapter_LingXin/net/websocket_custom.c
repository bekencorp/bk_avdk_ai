#include "lingxin_websocket.h"
#include "bk_websocket_client.h"
#include "components/log.h"
#include <locale.h>

#define BK_SEND_TIMEOUT 10 * 1000
#define TAG "ws_cust"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

extern  char *snprintfWithMalloc(const char *format, ...);

typedef struct
{
  transport bk_rtc_client;
  bool wsiDestroyFromClose;
} WebsocketClientHandler;

static WebsocketClient *globalClient;

static WebsocketClientHandler *getClientHandler(WebsocketClient *client)
{
  if (!client)
  {
    LOGE("getClientHandler client  null\r\n");
    return NULL;
  }
  if (!client->clientHandler)
  {
    LOGE("getClientHandler clientHandler null\r\n");
    return NULL;
  }
  return (WebsocketClientHandler *)client->clientHandler;
}

static void freeWebsocketClient(WebsocketClient *client)
{
  if (!client)
  {
    LOGE("freeWebsocketClient client null");
    return;
  }
  LOGI("free clientHandler\r\n");
  if (client->clientHandler)
  {
    free(client->clientHandler);
    client->clientHandler = NULL;
  }
  LOGD("free client\r\n");
  free(client);
  //client = NULL;
  LOGD("after free client\r\n");
}

static WebSocketEventListener getWebSocketEventListener(WebsocketClient *client)
{
  if (!client)
  {
    LOGE("getListener client null\r\n");
    return NULL;
  }
  if (!client->config)
  {
    LOGE("getListener client config null\r\n");
    return NULL;
  }
  if (!client->config->listener)
  {
    LOGE("getListener client config listener null\r\n");
    return NULL;
  }
  return client->config->listener;
}

void websocket_event_handler(void *event_handler_arg, char *event_base, int32_t event_id, void *event_data)
{
  bk_websocket_event_data_t *data = (bk_websocket_event_data_t *)event_data;
  WebsocketClient *client = data->user_context;

  if (client)
  {
    LOGD("data->user_contex not null, %0x\r\n", client);
  }
  else
  {
    LOGD("data->user_contex  null\r\n");
  }
  WebsocketClient *client1 = ((transport)event_handler_arg)->config->user_context;
  if (client1)
  {
    LOGD("client->user_contex not null, %0x\r\n", client1);
  }
  else
  {
    LOGD("client->user_contex  null\r\n");
  }

  switch (event_id)
  {
  case WEBSOCKET_EVENT_CONNECTED:
  {
    LOGI("Connected to WebSocket server\r\n");
    WebSocketEventListener listener = getWebSocketEventListener(globalClient);
    if (listener)
    {
      listener(ON_WEBSOCKET_CONNECTION_SUCCESS, NULL, 0, 0,
               globalClient->config->userContext);
    }

    break;
  }

  case WEBSOCKET_EVENT_DISCONNECTED:
  {
    LOGI("WEBSOCKET_EVENT_DISCONNECTED\r\n");
    WebsocketClientHandler *clientHandler = getClientHandler(globalClient);
    if (clientHandler)
    {
      if (clientHandler->wsiDestroyFromClose)
      {
        // closeWebsocket触发回调
        WebSocketEventListener listener = getWebSocketEventListener(globalClient);
        if (listener)
        {
          listener(ON_WEBSOCKET_DESTROY, NULL, 0, 0,
                   globalClient->config->userContext);
          freeWebsocketClient(globalClient);
          globalClient = NULL;
        }
      }
      else
      {
        if (!client->isWebsocketDestroyed)
        {
          client->isWebsocketDestroyed = true;
          WebSocketEventListener listener = getWebSocketEventListener(globalClient);
          if (listener)
          {
            listener(ON_WEBSOCKET_DESTROY, NULL, 0, 0, globalClient->config->userContext);
            freeWebsocketClient(globalClient);
            globalClient = NULL;
          }
        }
      }
    }
    break;
  }

  case WEBSOCKET_EVENT_DATA:
  {
    LOGD("data from WebSocket server, len:%d op:%d\r\n", data->data_len, data->op_code);
    WebSocketEventListener listener = getWebSocketEventListener(globalClient);
    if (listener)
    {
      if (data->op_code == WS_TRANSPORT_OPCODES_BINARY)
      {
        listener(ON_WEBSOCKET_DATA_RECEIVED, data->data_ptr, data->data_len, 1, globalClient->config->userContext);
      }
      else if (data->op_code == WS_TRANSPORT_OPCODES_TEXT)
      {
        listener(ON_WEBSOCKET_DATA_RECEIVED, data->data_ptr, data->data_len, 0, globalClient->config->userContext);
      }
    }
    break;
  }
  case WEBSOCKET_EVENT_CLOSED:
    LOGI("WEBSOCKET_EVENT_CLOSED\r\n");
    break;
  default:
    break;
  }
}

WebsocketClient *initWebsocket(WebsocketConfig *config)
{
  globalClient = (WebsocketClient *)malloc(sizeof(WebsocketClient));
  if (!globalClient)
  {
    LOGE("Failed to allocate memory for WebSocket client\r\n");
    return NULL;
  }

  WebsocketClientHandler *handler =
      (WebsocketClientHandler *)malloc(sizeof(WebsocketClientHandler));
  if (!handler)
  { // 添加空指针检查
    LOGE("Failed to allocate memory for WebSocket client handler\r\n");
    free(globalClient);
    globalClient = NULL;
    return NULL;
  }

  globalClient->clientHandler = handler;
  globalClient->config = config;
  globalClient->isWebsocketDestroyed = false; // Initialize running state

  char url[256];
  snprintf((char *)url, sizeof(url), "%s://%s:%d/%s", config->protocol,
           config->host, config->port, config->path);

  websocket_client_input_t websocket_cfg = {0};
  websocket_cfg.uri = url;
  websocket_cfg.headers = snprintfWithMalloc(
      "app_id:%s\r\nsn:%s\r\nsignature:%s\r\ntimestamp:%s\r\n",
      config->header_app_id, config->header_sn,
      config->header_signature, config->header_timestamp);
  websocket_cfg.ws_event_handler = websocket_event_handler;
  websocket_cfg.user_context = globalClient;
  websocket_cfg.disable_auto_reconnect = true;
  handler->wsiDestroyFromClose = false;
  handler->bk_rtc_client = websocket_client_init(&websocket_cfg);

  LOGI("success connect to %s:%d/%s\r\n", config->host, config->port, config->path);
  return globalClient;
}

void startWebsocket(WebsocketClient *client)
{
  WebsocketClientHandler *clientHandler = getClientHandler(client);
  if (!clientHandler)
  {
    LOGE("startWebsocket, clientHandler or context null\r\n");
    return;
  }

  int result = websocket_client_start(clientHandler->bk_rtc_client);
  LOGD("startWebsocket, result: %d\r\n", result);
}

bool websocketSendText(WebsocketClient *client, const char *message)
{
  WebsocketClientHandler *clientHandler = getClientHandler(client);

  if (!clientHandler || !clientHandler->bk_rtc_client)
  {
    LOGE("websocketSendText, clientHandler or bk_rtc_client null\r\n");
    return false;
  }
  if (client->isWebsocketDestroyed)
  {
    LOGE("WebsocketClient has been destroyed\r\n");
    return false;
  }
  LOGD("websocketSendText begin: %s\r\n", message);
  size_t message_len = strlen(message);
  int result = websocket_client_send_text(clientHandler->bk_rtc_client, message, message_len, BK_SEND_TIMEOUT);

  LOGD("websocketSendText finish, result: %d\r\n", result);
  return result > 0;
}

void closeWebsocket(WebsocketClient *client)
{
  LOGD("closeWebsocket before\r\n");

  WebsocketClientHandler *clientHandler = getClientHandler(client);
  if (!clientHandler)
  {
    LOGE("closeWebsocket params null\r\n");
    return;
  }
  if (client->isWebsocketDestroyed)
  {
    LOGE("client has been destroyed\r\n");
    return;
  }
  clientHandler->wsiDestroyFromClose = true;
  client->isWebsocketDestroyed = true;
  websocket_client_destroy(clientHandler->bk_rtc_client);
  LOGD("closeWebsocket after websocket_client_destroy\r\n");
}

int websocketSendBinary(WebsocketClient *client, const char *audioData, size_t dataSize)
{
  if (!audioData || !dataSize)
  {
    LOGE("websocketSendBinary Invalid parameters\r\n");
    return 0;
  }

  WebsocketClientHandler *clientHandler = getClientHandler(client);
  if (!clientHandler || !clientHandler->bk_rtc_client)
  {
    LOGE("websocketSendBinary Invalid clientHandler\r\n");
    return 0;
  }
  if (client->isWebsocketDestroyed)
  {
    LOGE("websocketSendBinary has been destroyed\r\n");
    return 0;
  }
  int bytesWritten = websocket_client_send_binary(clientHandler->bk_rtc_client, audioData, dataSize, BK_SEND_TIMEOUT);
  if (bytesWritten < 0)
  {
    LOGE("Failed to send binary data， %d\r\n", bytesWritten);
    return 0;
  }
  return (int)dataSize;
}

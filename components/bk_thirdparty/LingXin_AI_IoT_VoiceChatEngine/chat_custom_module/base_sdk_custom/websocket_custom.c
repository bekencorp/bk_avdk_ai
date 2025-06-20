#include "sdk_websocket.h"
#include "bk_websocket_client.h"

#include <locale.h>

#define BK_SEND_TIMEOUT 10 * 1000
#define logOut bk_printf
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
    logOut("getClientHandler client  null");
    return NULL;
  }
  if (!client->clientHandler)
  {
    logOut("getClientHandler clientHandler null");
    return NULL;
  }
  return (WebsocketClientHandler *)client->clientHandler;
}

static void freeWebsocketClient(WebsocketClient *client)
{
  if (!client)
  {
    logOut("freeWebsocketClient client null");
    return;
  }
  logOut("free clientHandler");
  if (client->clientHandler)
  {
    free(client->clientHandler);
    client->clientHandler = NULL;
  }
  logOut("free client");
  free(client);
  //client = NULL;
  logOut("after free client");
}

static WebSocketEventListener getWebSocketEventListener(WebsocketClient *client)
{
  if (!client)
  {
    logOut("getListener client null");
    return NULL;
  }
  if (!client->config)
  {
    logOut("getListener client config null");
    return NULL;
  }
  if (!client->config->listener)
  {
    logOut("getListener client config listener null");
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
    logOut("data->user_contex not null, %0x", client);
  }
  else
  {
    logOut("data->user_contex  null");
  }
  WebsocketClient *client1 = ((transport)event_handler_arg)->config->user_context;
  if (client1)
  {
    logOut("client->user_contex not null, %0x", client1);
  }
  else
  {
    logOut("client->user_contex  null");
  }

  switch (event_id)
  {
  case WEBSOCKET_EVENT_CONNECTED:
  {
    logOut("Connected to WebSocket server");
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
        }
      }
      else
      {
        // websocket库内部主动回调
        closeWebsocket(globalClient);
        WebSocketEventListener listener = getWebSocketEventListener(globalClient);
        if (listener)
        {
          listener(ON_WEBSOCKET_DESTROY, NULL, 0, 0,
                   globalClient->config->userContext);
          freeWebsocketClient(globalClient);
        }
      }
    }
    break;
  }

  case WEBSOCKET_EVENT_DATA:
  {
    logOut("data from WebSocket server, len:%d op:%d", data->data_len, data->op_code);
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
    logOut("WEBSOCKET_EVENT_CLOSED");
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
    logOut("Failed to allocate memory for WebSocket client");
    return NULL;
  }

  WebsocketClientHandler *handler =
      (WebsocketClientHandler *)malloc(sizeof(WebsocketClientHandler));
  if (!handler)
  { // 添加空指针检查
    logOut("Failed to allocate memory for WebSocket client handler");
    free(globalClient);
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

  handler->wsiDestroyFromClose = false;
  handler->bk_rtc_client = websocket_client_init(&websocket_cfg);

  logOut("success connect to %s:%d/%s", config->host, config->port, config->path);
  return globalClient;
}

void startWebsocket(WebsocketClient *client)
{
  WebsocketClientHandler *clientHandler = getClientHandler(client);
  if (!clientHandler)
  {
    logOut("startWebsocket, clientHandler or context null");
    return;
  }

  int result = websocket_client_start(clientHandler->bk_rtc_client);
  logOut("startWebsocket, result: %d", result);
}

bool websocketSendText(WebsocketClient *client, const char *message)
{
  WebsocketClientHandler *clientHandler = getClientHandler(client);

  if (!clientHandler || !clientHandler->bk_rtc_client)
  {
    logOut("websocketSendText, clientHandler or bk_rtc_client null");
    return false;
  }
  if (client->isWebsocketDestroyed)
  {
    logOut("WebsocketClient has been destroyed");
    return false;
  }
  logOut("websocketSendText begin: %s", message);
  size_t message_len = strlen(message);
  int result = websocket_client_send_text(clientHandler->bk_rtc_client, message, message_len, BK_SEND_TIMEOUT);

  logOut("websocketSendText finish, result: %d", result);
  return result > 0;
}

void closeWebsocket(WebsocketClient *client)
{
  logOut("closeWebsocket before");

  WebsocketClientHandler *clientHandler = getClientHandler(client);
  if (!clientHandler)
  {
    logOut("closeWebsocket params null");
    return;
  }
  if (client->isWebsocketDestroyed)
  {
    logOut("client has been destroyed");
    return;
  }
  clientHandler->wsiDestroyFromClose = true;
  client->isWebsocketDestroyed = true;
  websocket_client_destroy(clientHandler->bk_rtc_client);
  logOut("closeWebsocket after websocket_client_destroy");
}

int websocketSendBinary(WebsocketClient *client, const char *audioData, size_t dataSize)
{
  if (!audioData || !dataSize)
  {
    logOut("websocketSendBinary Invalid parameters");
    return 0;
  }

  WebsocketClientHandler *clientHandler = getClientHandler(client);
  if (!clientHandler || !clientHandler->bk_rtc_client)
  {
    logOut("websocketSendBinary Invalid clientHandler");
    return 0;
  }
  if (client->isWebsocketDestroyed)
  {
    logOut("websocketSendBinary has been destroyed");
    return 0;
  }
  int bytesWritten = websocket_client_send_binary(clientHandler->bk_rtc_client, audioData, dataSize, BK_SEND_TIMEOUT);
  if (bytesWritten < 0)
  {
    logOut("Failed to send binary data， %d", bytesWritten);
    return 0;
  }
  return (int)dataSize;
}

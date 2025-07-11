#ifdef USE_LINGXIN_NET_LIB
#include "lingxin_websocket.h"
#include "lingxin_common.h"
#include <libwebsockets.h>
#include <locale.h>
#include <pthread.h>

typedef struct
{
  struct lws_context *context;
  struct lws *lws;
  pthread_t thread_id;
  bool wsiDestroyFromClose;
} WebsocketClientHandler;

static WebsocketClientHandler *getClientHandler(WebsocketClient *client)
{
  if (!client)
  {
    logPrintf("getClientHandler client  null");
    return NULL;
  }
  if (!client->clientHandler)
  {
    logPrintf("getClientHandler clientHandler null");
    return NULL;
  }
  return (WebsocketClientHandler *)client->clientHandler;
}

static void freeWebsocketClient(WebsocketClient *client)
{
  if (!client)
  {
    logPrintf("freeWebsocketClient client null");
    return;
  }
  logPrintf("free clientHandler");
  if (client->clientHandler)
  {
    free(client->clientHandler);
    client->clientHandler = NULL;
  }
  logPrintf("free client");
  free(client);
  client = NULL;
  logPrintf("after free client");
}

static int callback_ws(struct lws *wsi, enum lws_callback_reasons reason,
                       void *clientSelf, void *in, const size_t len)
{
  unsigned char **p = (unsigned char **)in;
  unsigned char *end;
  int isBinary;
  WebsocketClient *client = (WebsocketClient *)clientSelf;
  logPrintf("WebSocket callback_ws:%d", reason);
  if (!client)
  {
    logPrintf("callback_ws client null");
    return 0;
  }
  if (!client->config)
  {
    logPrintf("callback_ws config null");
    return 0;
  }
  if (!client->config->listener)
  {
    logPrintf("callback_ws listener null");
    return 0;
  }
  switch (reason)
  {
  case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
  {
    end = (*p) + len;

    if (!client->config->header_app_id ||
        lws_add_http_header_by_name(
            wsi, (const unsigned char *)"app_id",
            (const unsigned char *)client->config->header_app_id,
            strlen(client->config->header_app_id), p, end))
    {
      logPrintf("Failed to add HTTP header: app_id");
      return -1;
    }
    logPrintf("success add HTTP header app_id: %s",
              client->config->header_app_id);

    if (!client->config->header_sn ||
        lws_add_http_header_by_name(
            wsi, (const unsigned char *)"sn",
            (const unsigned char *)client->config->header_sn,
            strlen(client->config->header_sn), p, end))
    {
      logPrintf("Failed to add HTTP header: sn");
      return -1;
    }
    logPrintf("success add HTTP header sn: %s", client->config->header_sn);
    if (!client->config->header_signature ||
        lws_add_http_header_by_name(
            wsi, (const unsigned char *)"signature",
            (const unsigned char *)client->config->header_signature,
            strlen(client->config->header_signature), p, end))
    {
      logPrintf("Failed to add HTTP header: signature");
      return -1;
    }
    logPrintf("success add HTTP header signature: %s",
              client->config->header_signature);

    if (!client->config->header_timestamp ||
        lws_add_http_header_by_name(
            wsi, (const unsigned char *)"timestamp",
            (const unsigned char *)client->config->header_timestamp,
            strlen(client->config->header_timestamp), p, end))
    {
      logPrintf("Failed to add HTTP header : timestamp");
      return -1;
    }
    logPrintf("success add HTTP header timestamp: %s",
              client->config->header_timestamp);
    break;
  }
  case LWS_CALLBACK_CLIENT_ESTABLISHED:
  {
    logPrintf("WebSocket connection established");
    WebsocketClientHandler *clientHandler = getClientHandler(client);
    if (clientHandler)
    {
      clientHandler->lws = wsi;
      client->config->listener(ON_WEBSOCKET_CONNECTION_SUCCESS, NULL, 0, 0,
                               client->config->userContext);
    }
    break;
  }
  case LWS_CALLBACK_CLIENT_RECEIVE:
  {
    logPrintf("WebSocket REVEIVE", (char *)in);
    isBinary = lws_frame_is_binary(wsi) ? 1 : 0;
    client->config->listener(ON_WEBSOCKET_DATA_RECEIVED, in, len, isBinary,
                             client->config->userContext);
    break;
  }
  case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
  {
    logPrintf("LWS_CALLBACK_WS_PEER_INITIATED_CLOSE");
    client->config->listener(ON_WEBSOCKET_CONNECTION_ERROR, (const char *)in, len, 0,
                             client->config->userContext);
    break;
  }
  case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
  {
    logPrintf("LWS_CALLBACK_CLIENT_CONNECTION_ERROR");

    char buffer[256];                         // 增大 buffer 大小
    const char *headerKey = "x-auth-reason:"; // 自定义 header 名称
    int lenOfHeaderKey = strlen(headerKey);   // 动态计算 header 名称长度

    // 获取自定义 header 的长度
    int lenOfHeaderValue = lws_hdr_custom_length(wsi, headerKey, lenOfHeaderKey);
    if (lenOfHeaderValue < 0)
    {
      logPrintf("Can't find %s", headerKey);
    }
    else
    {
      // 拷贝自定义 header 的内容到 buffer
      int copyResult = lws_hdr_custom_copy(wsi, buffer, sizeof(buffer), headerKey, lenOfHeaderKey);
      if (copyResult < 0)
      {
        logPrintf("Custom header too long: %s", headerKey);
      }
      else
      {
        logPrintf("Custom header: %s", buffer);
      }
    }
    char *errorMessage = (char *)in;
    if (strlen(buffer) > 0)
    {
      errorMessage = buffer;
    }
    client->config->listener(ON_WEBSOCKET_CONNECTION_ERROR, (const char *)errorMessage, len, 0,
                             client->config->userContext);
    break;
  }
  // 这个回调有两个触发的地方：closeWebsocket触发回调 和websocket库内部主动回调
  case LWS_CALLBACK_WSI_DESTROY:
  {
    logPrintf("LWS_CALLBACK_WSI_DESTROY");

    WebsocketClientHandler *clientHandler = getClientHandler(client);
    if (clientHandler)
    {
      if (clientHandler->wsiDestroyFromClose)
      {
        // closeWebsocket触发回调
        client->config->listener(ON_WEBSOCKET_DESTROY, NULL, 0, 0,
                                 client->config->userContext);
        freeWebsocketClient(client);
      }
      else
      {
        // websocket库内部主动回调
        closeWebsocket(client);
        client->config->listener(ON_WEBSOCKET_DESTROY, NULL, 0, 0,
                                 client->config->userContext);
        freeWebsocketClient(client);
      }
    }
    break;
  }

  default:
    break;
  }
  return 0;
}

static struct lws_protocols protocols[] = {
    {
        .name = "my-protocol",
        .callback = callback_ws,
        .per_session_data_size = sizeof(WebsocketClient),
        .rx_buffer_size = 1024 * 1024,
        .user = 0,
        .id = 0,
    },
    {NULL, NULL, 0, 0, 0, 0} // 结束标记
};

static void customLogEmit(int level, const char *line)
{
  logPrintf("customLogEmit: %d, %s", level, line);
  // Do nothing, effectively disabling all logs
}

WebsocketClient *initWebsocket(WebsocketConfig *config)
{
  lws_set_log_level(LLL_ERR | LLL_WARN, customLogEmit);
  // lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG,
  //                   customLogEmit);

  struct lws_context_creation_info info;
  struct lws_client_connect_info ccinfo = {0};

  memset(&info, 0, sizeof(info));
  info.port = CONTEXT_PORT_NO_LISTEN;
  info.protocols = protocols;
  // info.timeout_secs_ah_idle = 60;
  info.gid = -1;
  info.uid = -1;
  // info.connect_timeout_secs = 5;

  logPrintf("lws_create_context");

  struct lws_context *context = lws_create_context(&info);
  if (!context)
  {
    logPrintf("Failed to create libwebsockets context");
    return NULL;
  }

  memset(&ccinfo, 0, sizeof(ccinfo));
  ccinfo.context = context;
  ccinfo.address = config->host;
  ccinfo.port = config->port;
  ccinfo.path = config->path;
  ccinfo.host = config->host;
  ccinfo.protocol = protocols[0].name;
  ccinfo.ssl_connection =
      strcmp(config->protocol, "ws") == 0 ? 0 : LCCSCF_USE_SSL;

  logPrintf("before lws_client_connect_via_info");

  struct lws *lws = lws_client_connect_via_info(&ccinfo);
  logPrintf("after lws_client_connect_via_info");

  if (!lws)
  {
    logPrintf("Failed to initiate WebSocket connection");
    lws_context_destroy(context);
    return NULL;
  }

  WebsocketClient *client = (WebsocketClient *)malloc(sizeof(WebsocketClient));
  if (!client)
  {
    logPrintf("Failed to allocate memory for WebSocket client");
    lws_context_destroy(
        context); // Add this line to destroy context if client allocation fails
    return NULL;
  }

  WebsocketClientHandler *handler =
      (WebsocketClientHandler *)malloc(sizeof(WebsocketClientHandler));
  if (!handler)
  { // 添加空指针检查
    logPrintf("Failed to allocate memory for WebSocket client handler");
    lws_context_destroy(context);
    free(client);
    return NULL;
  }

  handler->context = context;
  handler->lws = lws;
  handler->wsiDestroyFromClose = false;

  client->clientHandler = handler;
  client->config = config;
  client->isWebsocketDestroyed = false; // Initialize running state
  ccinfo.userdata = client;             // Set client as userdata
  lws_set_wsi_user(lws, client);        // Ensure client is set correctly

  logPrintf("success connect to %s:%d/%s", config->host, config->port,
            config->path);

  return client;
}

void *websocketThread(void *arg)
{
  if (!arg)
  {
    logPrintf("websocketThread arg null");
    return NULL;
  }
  WebsocketClient *client = (WebsocketClient *)arg;
  WebsocketClientHandler *clientHandler = getClientHandler(client);
  if (!clientHandler)
  {
    return NULL;
  }
  logPrintf("websocketThread begin");
  while (!client->isWebsocketDestroyed)
  {
    if (lws_service(clientHandler->context, 1000) < 0)
    {
      logPrintf("lws_service fail,then break");
      break;
    }
  }
  logPrintf("websocketThread end");
  return NULL;
}

void startWebsocket(WebsocketClient *client)
{
  WebsocketClientHandler *clientHandler = getClientHandler(client);
  if (!clientHandler)
  {
    logPrintf("startWebsocket, clientHandler or context null");
    return;
  }

  if (pthread_create(&clientHandler->thread_id, NULL, websocketThread,
                     (void *)client) != 0)
  {
    logPrintf("Failed to create WebSocket thread");
    return;
  }
}

bool websocketSendText(WebsocketClient *client, const char *message)
{
  WebsocketClientHandler *clientHandler = getClientHandler(client);

  if (!clientHandler || !clientHandler->lws)
  {
    logPrintf("websocketSendText, clientHandler or lws null");
    return false;
  }
  if (client->isWebsocketDestroyed)
  {
    logPrintf("WebsocketClient has been destroyed");
    return false;
  }
  logPrintf("websocketSendText begin: %s", message);
  size_t message_len = strlen(message);
  unsigned char *buf = (unsigned char *)malloc(LWS_PRE + message_len);
  if (!buf)
  {
    logPrintf("Failed to allocate memory for buffer");
    return false;
  }
  memcpy(&buf[LWS_PRE], message, message_len);

  int result =
      lws_write(clientHandler->lws, &buf[LWS_PRE], message_len, LWS_WRITE_TEXT);
  logPrintf("websocketSendText finish, result: %d", result);
  free(buf);
  return result > 0;
}

void closeWebsocket(WebsocketClient *client)
{
  logPrintf("closeWebsocket before");

  WebsocketClientHandler *clientHandler = getClientHandler(client);
  if (!clientHandler || !clientHandler->context)
  {
    logPrintf("closeWebsocket params null");
    return;
  }
  if (client->isWebsocketDestroyed)
  {
    logPrintf("client has been destroyed");
    return;
  }
  clientHandler->wsiDestroyFromClose = true;
  client->isWebsocketDestroyed = true;
  logPrintf("set client->isWebsocketDestroyed");
  lws_cancel_service(clientHandler->context);
  logPrintf("closeWebsocket after lws_cancel_service");
  pthread_join(clientHandler->thread_id, NULL);
  logPrintf("closeWebsocket after pthread_join");
  lws_context_destroy(clientHandler->context);
  logPrintf("closeWebsocket after lws_context_destroy");
}

int websocketSendBinary(WebsocketClient *client, const char *audioData,
                        size_t dataSize)
{
  if (!audioData || !dataSize)
  {
    logPrintf("websocketSendBinary Invalid parameters");
    return 0;
  }

  WebsocketClientHandler *clientHandler = getClientHandler(client);
  if (!clientHandler || !clientHandler->lws)
  {
    logPrintf("websocketSendBinary Invalid clientHandler");
    return 0;
  }

  if (client->isWebsocketDestroyed)
  {
    logPrintf("websocketSendBinary has been destroyed");
    return 0;
  }

  size_t bufferSize = LWS_PRE + dataSize;
  unsigned char *buf = (unsigned char *)malloc(bufferSize);
  if (!buf)
  {
    logPrintf("Failed to allocate memory for buffer");
    return 0;
  }

  memcpy(&buf[LWS_PRE], audioData, dataSize);
  int bytesWritten = lws_write(clientHandler->lws, &buf[LWS_PRE], dataSize, LWS_WRITE_BINARY);

  free(buf);

  if (bytesWritten < 0)
  {
    logPrintf("Failed to send binary data， %d", bytesWritten);
    free(buf);
    return 0;
  }
  return (int)dataSize;
}
#endif // USE_LINGXIN_NET_LIB
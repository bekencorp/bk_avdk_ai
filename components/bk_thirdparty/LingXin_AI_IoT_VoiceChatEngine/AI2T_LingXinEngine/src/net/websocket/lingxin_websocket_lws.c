#ifdef USE_LINGXIN_NET_LIB
#include "lingxin_websocket.h"
#include "lingxin_common.h"
#include <libwebsockets.h>
#include <locale.h>
#include <pthread.h>
#include "lingxin_log.h"

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
    lingxin_log_error("getClientHandler client  null");
    return NULL;
  }
  if (!client->clientHandler)
  {
    lingxin_log_error("getClientHandler clientHandler null");
    return NULL;
  }
  return (WebsocketClientHandler *)client->clientHandler;
}

static void freeWebsocketClient(WebsocketClient *client)
{
  if (!client)
  {
    lingxin_log_error("freeWebsocketClient client null");
    return;
  }
  lingxin_log_debug("free clientHandler");
  if (client->clientHandler)
  {
    free(client->clientHandler);
    client->clientHandler = NULL;
  }
  lingxin_log_debug("free client");
  free(client);
  client = NULL;
  lingxin_log_debug("after free client");
}

static int callback_ws(struct lws *wsi, enum lws_callback_reasons reason,
                       void *clientSelf, void *in, const size_t len)
{
  unsigned char **p = (unsigned char **)in;
  unsigned char *end;
  int isBinary;
  WebsocketClient *client = (WebsocketClient *)clientSelf;
  lingxin_log_debug("WebSocket callback_ws:%d", reason);
  if (!client)
  {
    lingxin_log_warn("callback_ws client null");
    return 0;
  }
  if (!client->config)
  {
    lingxin_log_error("callback_ws config null");
    return 0;
  }
  if (!client->config->listener)
  {
    lingxin_log_error("callback_ws listener null");
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
      lingxin_log_error("Failed to add HTTP header: app_id");
      return -1;
    }
    lingxin_log_debug("success add HTTP header app_id: %s",
              client->config->header_app_id);

    if (!client->config->header_sn ||
        lws_add_http_header_by_name(
            wsi, (const unsigned char *)"sn",
            (const unsigned char *)client->config->header_sn,
            strlen(client->config->header_sn), p, end))
    {
      lingxin_log_error("Failed to add HTTP header: sn");
      return -1;
    }
    lingxin_log_debug("success add HTTP header sn: %s", client->config->header_sn);
    if (!client->config->header_signature ||
        lws_add_http_header_by_name(
            wsi, (const unsigned char *)"signature",
            (const unsigned char *)client->config->header_signature,
            strlen(client->config->header_signature), p, end))
    {
      lingxin_log_error("Failed to add HTTP header: signature");
      return -1;
    }
    lingxin_log_debug("success add HTTP header signature: %s",
              client->config->header_signature);

    if (!client->config->header_timestamp ||
        lws_add_http_header_by_name(
            wsi, (const unsigned char *)"timestamp",
            (const unsigned char *)client->config->header_timestamp,
            strlen(client->config->header_timestamp), p, end))
    {
      lingxin_log_error("Failed to add HTTP header : timestamp");
      return -1;
    }
    lingxin_log_debug("success add HTTP header timestamp: %s",
              client->config->header_timestamp);
    break;
  }
  case LWS_CALLBACK_CLIENT_ESTABLISHED:
  {
    lingxin_log_debug("WebSocket connection established");
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
    lingxin_log_debug("WebSocket REVEIVE", (char *)in);
    isBinary = lws_frame_is_binary(wsi) ? 1 : 0;
    client->config->listener(ON_WEBSOCKET_DATA_RECEIVED, in, len, isBinary,
                             client->config->userContext);
    break;
  }
  case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
  {
    lingxin_log_warn("LWS_CALLBACK_WS_PEER_INITIATED_CLOSE");
    client->config->listener(ON_WEBSOCKET_CONNECTION_ERROR, (const char *)in, len, 0,
                             client->config->userContext);
    break;
  }
  case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
  {
    lingxin_log_error("LWS_CALLBACK_CLIENT_CONNECTION_ERROR");

    char buffer[256];                         // 增大 buffer 大小
    const char *headerKey = "x-auth-reason:"; // 自定义 header 名称
    int lenOfHeaderKey = strlen(headerKey);   // 动态计算 header 名称长度

    // 获取自定义 header 的长度
    int lenOfHeaderValue = lws_hdr_custom_length(wsi, headerKey, lenOfHeaderKey);
    if (lenOfHeaderValue < 0)
    {
      lingxin_log_error("Can't find %s", headerKey);
    }
    else
    {
      // 拷贝自定义 header 的内容到 buffer
      int copyResult = lws_hdr_custom_copy(wsi, buffer, sizeof(buffer), headerKey, lenOfHeaderKey);
      if (copyResult < 0)
      {
        lingxin_log_error("Custom header too long: %s", headerKey);
      }
      else
      {
        lingxin_log_debug("Custom header: %s", buffer);
      }
    }
    char *errorMessage = (char *)in;
    if (strlen(buffer) > 0)
    {
      errorMessage = buffer;
    }
    client->config->listener(ON_WEBSOCKET_CONNECTION_ERROR, (const char *)errorMessage, strlen(errorMessage), 0,
                             client->config->userContext);
    break;
  }
  // 这个回调有两个触发的地方：closeWebsocket触发回调 和websocket库内部主动回调
  case LWS_CALLBACK_WSI_DESTROY:
  {
    lingxin_log_debug("LWS_CALLBACK_WSI_DESTROY");

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
  lingxin_log_debug("customLogEmit: %d, %s", level, line);
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

  lingxin_log_debug("lws_create_context");

  struct lws_context *context = lws_create_context(&info);
  if (!context)
  {
    lingxin_log_error("Failed to create libwebsockets context");
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

  lingxin_log_debug("before lws_client_connect_via_info");

  struct lws *lws = lws_client_connect_via_info(&ccinfo);
  lingxin_log_debug("after lws_client_connect_via_info");

  if (!lws)
  {
    lingxin_log_error("Failed to initiate WebSocket connection");
    lws_context_destroy(context);
    return NULL;
  }

  WebsocketClient *client = (WebsocketClient *)malloc(sizeof(WebsocketClient));
  if (!client)
  {
    lingxin_log_error("Failed to allocate memory for WebSocket client");
    lws_context_destroy(
        context); // Add this line to destroy context if client allocation fails
    return NULL;
  }

  WebsocketClientHandler *handler =
      (WebsocketClientHandler *)malloc(sizeof(WebsocketClientHandler));
  if (!handler)
  { // 添加空指针检查
    lingxin_log_error("Failed to allocate memory for WebSocket client handler");
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

  lingxin_log_debug("success connect to %s:%d/%s", config->host, config->port,
            config->path);

  return client;
}

void *websocketThread(void *arg)
{
  if (!arg)
  {
    lingxin_log_error("websocketThread arg null");
    return NULL;
  }
  WebsocketClient *client = (WebsocketClient *)arg;
  WebsocketClientHandler *clientHandler = getClientHandler(client);
  if (!clientHandler)
  {
    return NULL;
  }
  lingxin_log_debug("websocketThread begin");
  while (!client->isWebsocketDestroyed)
  {
    if (lws_service(clientHandler->context, 1000) < 0)
    {
      lingxin_log_warn("lws_service fail,then break");
      break;
    }
  }
  // Android需要已经做过AttachCurrentThread操作的native线程退出的时候做清理操作，其他平台暂未发现需要，所以这里先作为内部使用，暂不对外
#if defined(__ANDROID__) && defined(USE_THREAD_EXIT_CLEANUP)
    // 这个方法在适配套件的Android module中有实现
    extern void websocketThreadExitCleanup();
    websocketThreadExitCleanup();
#endif
  lingxin_log_debug("websocketThread end");
  return NULL;
}

void startWebsocket(WebsocketClient *client)
{
  WebsocketClientHandler *clientHandler = getClientHandler(client);
  if (!clientHandler)
  {
    lingxin_log_error("startWebsocket, clientHandler or context null");
    return;
  }

  if (pthread_create(&clientHandler->thread_id, NULL, websocketThread,
                     (void *)client) != 0)
  {
    lingxin_log_error("Failed to create WebSocket thread");
    return;
  }
}

bool websocketSendText(WebsocketClient *client, const char *message)
{
  WebsocketClientHandler *clientHandler = getClientHandler(client);

  if (!clientHandler || !clientHandler->lws)
  {
    lingxin_log_error("websocketSendText, clientHandler or lws null");
    return false;
  }
  if (client->isWebsocketDestroyed)
  {
    lingxin_log_error("WebsocketClient has been destroyed");
    return false;
  }
  lingxin_log_debug("websocketSendText begin: %s", message);
  size_t message_len = strlen(message);
  unsigned char *buf = (unsigned char *)malloc(LWS_PRE + message_len);
  if (!buf)
  {
    lingxin_log_error("Failed to allocate memory for buffer");
    return false;
  }
  memcpy(&buf[LWS_PRE], message, message_len);

  int result =
      lws_write(clientHandler->lws, &buf[LWS_PRE], message_len, LWS_WRITE_TEXT);
  lingxin_log_debug("websocketSendText finish, result: %d", result);
  free(buf);
  return result > 0;
}

void closeWebsocket(WebsocketClient *client)
{
  lingxin_log_debug("closeWebsocket before");

  WebsocketClientHandler *clientHandler = getClientHandler(client);
  if (!clientHandler || !clientHandler->context)
  {
    lingxin_log_error("closeWebsocket params null");
    return;
  }
  if (client->isWebsocketDestroyed)
  {
    lingxin_log_error("client has been destroyed");
    return;
  }
  clientHandler->wsiDestroyFromClose = true;
  client->isWebsocketDestroyed = true;
  lingxin_log_debug("set client->isWebsocketDestroyed");
  lws_cancel_service(clientHandler->context);
  lingxin_log_debug("closeWebsocket after lws_cancel_service");
  pthread_join(clientHandler->thread_id, NULL);
  lingxin_log_debug("closeWebsocket after pthread_join");
  lws_context_destroy(clientHandler->context);
  lingxin_log_debug("closeWebsocket after lws_context_destroy");
}

int websocketSendBinary(WebsocketClient *client, const char *audioData,
                        size_t dataSize)
{
  if (!audioData || !dataSize)
  {
    lingxin_log_error("websocketSendBinary Invalid parameters");
    return 0;
  }

  WebsocketClientHandler *clientHandler = getClientHandler(client);
  if (!clientHandler || !clientHandler->lws)
  {
    lingxin_log_error("websocketSendBinary Invalid clientHandler");
    return 0;
  }

  if (client->isWebsocketDestroyed)
  {
    lingxin_log_error("websocketSendBinary has been destroyed");
    return 0;
  }

  size_t bufferSize = LWS_PRE + dataSize;
  unsigned char *buf = (unsigned char *)malloc(bufferSize);
  if (!buf)
  {
    lingxin_log_error("Failed to allocate memory for buffer");
    return 0;
  }

  memcpy(&buf[LWS_PRE], audioData, dataSize);
  int bytesWritten = lws_write(clientHandler->lws, &buf[LWS_PRE], dataSize, LWS_WRITE_BINARY);

  free(buf);

  if (bytesWritten < 0)
  {
    lingxin_log_error("Failed to send binary data， %d", bytesWritten);
    free(buf);
    return 0;
  }
  return (int)dataSize;
}
#endif // USE_LINGXIN_NET_LIB
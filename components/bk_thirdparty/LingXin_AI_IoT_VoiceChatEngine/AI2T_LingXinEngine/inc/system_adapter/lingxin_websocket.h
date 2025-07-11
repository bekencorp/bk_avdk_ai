#ifndef AI_IOT_SDK_WEBSOCKET_H
#define AI_IOT_SDK_WEBSOCKET_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>

  typedef enum
  {
    ON_WEBSOCKET_CONNECTION_SUCCESS,
    ON_WEBSOCKET_DATA_RECEIVED,
    ON_WEBSOCKET_DESTROY,
    ON_WEBSOCKET_CONNECTION_ERROR
  } WebSocketEventType;

  typedef void (*WebSocketEventListener)(WebSocketEventType event, const char *data, size_t data_len, const int isBinary, void *userContext);

  typedef struct
  {
    const char *protocol; // ws or wss
    const char *host;
    const char *path;
    const char *header_signature;
    const char *header_sn;
    const char *header_app_id;
    const char *header_timestamp;
    int port;
    void *userContext;
    WebSocketEventListener listener;
  } WebsocketConfig;

  typedef struct
  {
    void *clientHandler;
    WebsocketConfig *config;
    bool isWebsocketDestroyed;
  } WebsocketClient;

  WebsocketClient *initWebsocket(WebsocketConfig *config);

  void startWebsocket(WebsocketClient *client);

  bool websocketSendText(WebsocketClient *client, const char *message);

  int websocketSendBinary(WebsocketClient *client, const char *audioData, size_t dataSize);

  void closeWebsocket(WebsocketClient *client);
#ifdef __cplusplus
}
#endif

#endif // AI_IOT_SDK_WEBSOCKET_H
// httpclient.h
#ifndef AI_IOT_SDK_HTTP_HTTPCLIENT_H
#define AI_IOT_SDK_HTTP_HTTPCLIENT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>

  typedef void (*RequestCallback)(void *contents, size_t size, void *userp);

  typedef struct
  {
    const char *signature;
    const char *sn;
    const char *app_id;
    const char *timestamp;
  } HttpHeader;

  typedef struct
  {
    const char *protocol; // http or https
    const char *host;
    const char *path;
    int port;
    const char *post_data;
    HttpHeader *headers;
  } HttpConfig;

  int http_post(HttpConfig *config, RequestCallback userCallback, void *userData);
#ifdef __cplusplus
}
#endif
#endif // AI_IOT_SDK_HTTP_HTTPCLIENT_H

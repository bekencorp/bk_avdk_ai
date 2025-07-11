#ifndef AI_IOT_SDK_COMMON_COMMON_H
#define AI_IOT_SDK_COMMON_COMMON_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "lingxin_http.h"
#include "lingxin_websocket.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROTOCOL_HTTP "http"
#define PROTOCOL_WEBSOCKET "ws"

#ifdef ENV_DAILY
#define REQUEST_URL "101.132.68.167"
#define REQUEST_PORT 30002
// #define REQUEST_URL "30.233.0.184"
// #define REQUEST_PORT 7001
#else
#define REQUEST_URL "eagent.edu-aliyun.com"
#define REQUEST_PORT 80
#endif

#define WEBSOCKET_ASR_PATH "gw/ws/open/api/v1/asr"
#define WEBSOCKET_TTS_PATH "gw/ws/open/api/v1/tts"
#define LLM_TEXT_PATH "smart/api/v1/llm/compatible-mode/chat"
#define LLM_IMAGE_PATH "/gw/d/api/v1/text2image/createImage"
#define LLM_IMAGE_RESULT_PATH "/gw/d/api/v1/text2image/getImageCreateResult"

    // 打印日志
    extern bool enableLog;
    void setLogEnable(bool enabled);
    bool isLogEnabled();

    WebsocketConfig *createWebsocketConfig(void *handler, const char *sn, const char *appKey, const char *appId, const char *path, WebSocketEventListener listener);
    HttpConfig *createHttpConfig(const char *appId, const char *sn, const char *appKey, const char *path, const char *reqBody, HttpHeader *headers);
    char *generateUUID(int length);
    char *snprintfWithMalloc(const char *format, ...);

    void logPrintf(const char *format, ...);

#ifdef __cplusplus
}
#endif
#endif // AI_IOT_SDK_COMMON_COMMON_H
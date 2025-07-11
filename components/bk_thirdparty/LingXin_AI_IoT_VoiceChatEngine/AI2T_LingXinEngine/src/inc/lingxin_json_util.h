#ifndef AI_IOT_SDK_UTIL_JSON_UTIL_H
#define AI_IOT_SDK_UTIL_JSON_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cJSON.h"

const char *parsePayloadStr(cJSON *json);
char *parseRequestId(cJSON *json);
const char *parseEvent(cJSON *json);
const int isUseLLMStreaming(const char *message);
const int isJSON(const char *message);
const int parsePoolSize(const char *message);
char *parseErrorInfo(cJSON *json);
#ifdef __cplusplus
}
#endif
#endif // AI_IOT_SDK_UTIL_JSON_UTIL_H
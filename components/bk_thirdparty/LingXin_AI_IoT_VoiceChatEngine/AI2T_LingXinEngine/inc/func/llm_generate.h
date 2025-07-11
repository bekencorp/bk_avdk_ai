#ifndef AI_IOT_SDK_GENERATE_BY_LLM_H
#define AI_IOT_SDK_GENERATE_BY_LLM_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

  typedef void (*GenerateTextRequestCallback)(char *contents, int finish);

  void generateText(const char *appId, const char *sn, const char *appKey,
                    bool showLog, const char *input,
                    GenerateTextRequestCallback callback);

  void generateImage(const char *appId, const char *sn, const char *appKey,
                     bool showLog, const char *requestParams, char **response);

  void queryGenerateImageResult(const char *appId, const char *sn,
                                const char *appKey, bool showLog,
                                const char *requestParams, char **response);

#ifdef __cplusplus
}
#endif

#endif // AI_IOT_SDK_GENERATE_BY_LLM_H
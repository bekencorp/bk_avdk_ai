#ifndef LINGXIN_GENERATE_BY_LLM_H
#define LINGXIN_GENERATE_BY_LLM_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

  typedef void (*GenerateTextRequestCallback)(char *contents, int finish);

  void generateText(const char *appId, const char *sn, const char *appKey, const char *input,
                    GenerateTextRequestCallback callback);

  void generateImage(const char *appId, const char *sn, const char *appKey, const char *requestParams, char **response);

  void queryGenerateImageResult(const char *appId, const char *sn,
                                const char *appKey, const char *requestParams, char **response);

#ifdef __cplusplus
}
#endif

#endif // LINGXIN_GENERATE_BY_LLM_H
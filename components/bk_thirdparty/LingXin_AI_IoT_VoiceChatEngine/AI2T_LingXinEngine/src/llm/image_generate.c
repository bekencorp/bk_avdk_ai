#include "cJSON.h"
#include "lingxin_common.h"
#include "lingxin_http.h"
#include "llm_generate.h"
#include "lingxin_log.h"

static void callbackFromHttpRequest(void *contents, size_t size, void *userp)
{
  char **response = (char **)userp;
  // 计算现有数据长度
  size_t existingLength = *response ? strlen(*response) : 0;

  // 重新分配内存以容纳现有数据和新数据
  char *newResponse = (char *)realloc((void *)(intptr_t)*response, existingLength + size + 1);
  if (!newResponse)
  {
    lingxin_log_error("Memory allocation failed");
    return;
  }
  *response = newResponse;

  // 拷贝新数据到响应缓冲区
  memcpy(*response + existingLength, contents, size);
  (*response)[existingLength + size] = '\0'; // 添加字符串结束符
}

void generateImage(const char *appId, const char *sn, const char *appKey, const char *requestParams, char **response)
{
  if (!appId || !sn || !appKey || !requestParams)
  {
    lingxin_log_error("generateImage: Invalid input parameters");
    return;
  }
  HttpHeader *headers = (HttpHeader *)malloc(sizeof(HttpHeader));
  if (!headers)
  {
    lingxin_log_error("Failed to allocate memory for HttpHeader");
    return;
  }
  HttpConfig *config = createHttpConfig(appId, sn, appKey, LLM_IMAGE_PATH, requestParams, headers);
  http_post(config, callbackFromHttpRequest, response);
  free(headers);
  free(config);
}

void queryGenerateImageResult(const char *appId, const char *sn, const char *appKey, const char *requestParams, char **response)
{
  if (!appId || !sn || !appKey || !requestParams)
  {
    lingxin_log_error("queryGenerateImageResult: Invalid input parameters");
    return;
  }
  HttpHeader *headers = (HttpHeader *)malloc(sizeof(HttpHeader));
  if (!headers)
  {
    lingxin_log_error("Failed to allocate memory for HttpHeader");
    return;
  }
  HttpConfig *config =
      createHttpConfig(appId, sn, appKey, LLM_IMAGE_RESULT_PATH, requestParams, headers);
  http_post(config, callbackFromHttpRequest, response);
  free(headers);
  free(config);
}
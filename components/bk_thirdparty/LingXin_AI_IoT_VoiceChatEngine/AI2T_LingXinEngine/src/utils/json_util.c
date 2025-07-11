#include "lingxin_json_util.h"
#include "lingxin_common.h"

const char *parsePayloadStr(cJSON *json)
{
  if (!json)
  {
    return "";
  }

  cJSON *payload = cJSON_GetObjectItemCaseSensitive(json, "payload");
  if (!cJSON_IsObject(payload))
  {
    logPrintf("payload not  an object");
    return "";
  }
  return cJSON_PrintUnformatted(payload);
}
const char *parseEvent(cJSON *json)
{
  const char *event = NULL;
  // 获取 header 对象
  cJSON *header = cJSON_GetObjectItemCaseSensitive(json, "header");
  if (!header)
  {
    const char *event = NULL;
    cJSON *eventJSON = cJSON_GetObjectItemCaseSensitive(json, "action");
    if (cJSON_IsString(eventJSON) && eventJSON->valuestring)
    {
      event = eventJSON->valuestring;
    }
    return event;
  }
  if (!cJSON_IsObject(header))
  {
    logPrintf("Header not found or is not an object");
    return event;
  }
  // 获取 event 字段
  cJSON *eventJSON = cJSON_GetObjectItemCaseSensitive(header, "action");
  if (cJSON_IsString(eventJSON) && eventJSON->valuestring)
  {
    // logPrintf("parseEvent header event found");
    event = eventJSON->valuestring;
  }
  return event;
}

char *parseRequestId(cJSON *json)
{
  char *event = "";
  // 获取 header 对象
  cJSON *header = cJSON_GetObjectItemCaseSensitive(json, "header");
  if (!cJSON_IsObject(header))
  {
    logPrintf("Header not found or is not an object");
    return event;
  }
  // 获取 event 字段
  cJSON *eventJSON = cJSON_GetObjectItemCaseSensitive(header, "request_id");
  if (cJSON_IsString(eventJSON) && eventJSON->valuestring)
  {
    event = eventJSON->valuestring;
  }
  return event;
}

char *parseErrorInfo(cJSON *json)
{
  // 获取 header 对象
  cJSON *header = cJSON_GetObjectItemCaseSensitive(json, "header");
  if (!cJSON_IsObject(header))
  {
    logPrintf("Header not found or is not an object");
    return "";
  }
  cJSON *errorInfoObj = cJSON_CreateObject();
  // 获取 action 字段
  cJSON *actionObj = cJSON_GetObjectItemCaseSensitive(header, "action");
  if (cJSON_IsString(actionObj) && actionObj->valuestring && errorInfoObj)
  {
    cJSON_AddStringToObject(errorInfoObj, "action", actionObj->valuestring);
  } // 获取 event 字段
  cJSON *codeObj = cJSON_GetObjectItemCaseSensitive(header, "code");
  if (cJSON_IsString(codeObj) && codeObj->valuestring && errorInfoObj)
  {
    cJSON_AddStringToObject(errorInfoObj, "code", codeObj->valuestring);
  }
  // 获取 event 字段
  cJSON *msgObj = cJSON_GetObjectItemCaseSensitive(header, "err_msg");
  if (cJSON_IsString(msgObj) && msgObj->valuestring && errorInfoObj)
  {
    cJSON_AddStringToObject(errorInfoObj, "err_msg", msgObj->valuestring);
  }
  return cJSON_PrintUnformatted(errorInfoObj);
}

const int isUseLLMStreaming(const char *message)
{
  cJSON *jsonMessage = cJSON_Parse(message);
  if (!jsonMessage)
  {
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr)
    {
      logPrintf("Error json: %s\n", message);
    }
    return 0;
  }
  cJSON *stream = cJSON_GetObjectItemCaseSensitive(jsonMessage, "stream");
  if (cJSON_IsBool(stream))
  {
    int result = cJSON_IsTrue(stream);
    cJSON_Delete(jsonMessage); // 释放解析后的 JSON 对象
    return result;
  }

  cJSON_Delete(jsonMessage);
  return 0;
}

const int isJSON(const char *message)
{
  cJSON *jsonMessage = cJSON_Parse(message);
  if (!jsonMessage)
  {
    return 0;
  }
  return 1;
}

const int parsePoolSize(const char *message)
{
  cJSON *payloadObject = cJSON_Parse(message);
  if (!payloadObject)
  {
    logPrintf("appendParamsToStartPayload: Error json: %s", message);
    return 0;
  }
  cJSON *paramObject = cJSON_GetObjectItemCaseSensitive(
      payloadObject, "flow_control_parameters");
  if (!paramObject)
  {
    cJSON_Delete(payloadObject);
    return 0;
  }
  cJSON *strategy =
      cJSON_GetObjectItemCaseSensitive(paramObject, "flow_control_strategy");
  if (!cJSON_IsString(strategy) || !strategy->valuestring ||
      strcmp(strategy->valuestring, "dynamic") != 0)
  {
    cJSON_Delete(payloadObject);
    return 0;
  }
  cJSON *poolSizeObj =
      cJSON_GetObjectItemCaseSensitive(paramObject, "buffer_pool_size");
  int poolSize = 0;
  if (cJSON_IsString(poolSizeObj) && poolSizeObj->valuestring)
  {
    poolSize = atoi(poolSizeObj->valuestring); // 将字符串转换为整数
  }
  else if (cJSON_IsNumber(poolSizeObj))
  {
    poolSize = poolSizeObj->valueint;
  }
  else
  {
    logPrintf("Key 'buffer_pool_size' is neither a number nor a string.");
    return 0;
  }
  return poolSize;
}
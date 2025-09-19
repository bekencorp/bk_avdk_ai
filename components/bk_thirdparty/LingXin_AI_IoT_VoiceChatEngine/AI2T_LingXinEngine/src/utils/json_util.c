#include "lingxin_json_util.h"
#include "lingxin_common.h"
#include "lingxin_timer.h"
#include "schedule_timer_manager.h"


#include "lingxin_log.h"

const char *parsePayloadStr(cJSON *json)
{
  if (!json)
  {
    return "";
  }

  cJSON *payload = cJSON_GetObjectItemCaseSensitive(json, "payload");
  if (!cJSON_IsObject(payload))
  {
    lingxin_log_error("payload not an object");
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
    lingxin_log_error("Header not found or is not an object");
    return event;
  }
  // 获取 event 字段
  cJSON *eventJSON = cJSON_GetObjectItemCaseSensitive(header, "action");
  if (cJSON_IsString(eventJSON) && eventJSON->valuestring)
  {
    // lingxin_log_debug("parseEvent header event found");
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
    lingxin_log_error("Header not found or is not an object");
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
    lingxin_log_error("Header not found or is not an object");
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
      lingxin_log_debug("Error json: %s", message);
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
    lingxin_log_error("appendParamsToStartPayload: Error json: %s", message);
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
    lingxin_log_error("Key 'buffer_pool_size' is neither a number nor a string.");
    return 0;
  }
  return poolSize;
}
int parseScheduleTaskList(const char *jsonStr, ScheduleTaskList *outList)
{
  if (!jsonStr || !outList)
  {
    lingxin_log_error("Invalid input parameters");
    return -1;
  }

  cJSON *root = cJSON_Parse(jsonStr);
  if (!root)
  {
    lingxin_log_error("Failed to parse JSON string");
    return -1;
  }

  cJSON *dataObj = cJSON_GetObjectItemCaseSensitive(root, "data");
  if (!dataObj || !cJSON_IsObject(dataObj))
  {
    lingxin_log_error("data not found or is not an object");
    cJSON_Delete(root);
    return -1;
  }

  // 检查 type 是否为 "schedule_task_list"
  cJSON *typeObj = cJSON_GetObjectItemCaseSensitive(dataObj, "type");
  if (!typeObj || !cJSON_IsString(typeObj) || strcmp(typeObj->valuestring, "schedule_task_list") != 0)
  {
    lingxin_log_error("type not found or not 'schedule_task_list'");
    cJSON_Delete(root);
    return -1;
  }

  // 解析 task_list 数组
  cJSON *taskListArray = cJSON_GetObjectItemCaseSensitive(dataObj, "task_list");
  if (!taskListArray || !cJSON_IsArray(taskListArray))
  {
    lingxin_log_error("task_list not found or is not an array");
    cJSON_Delete(root);
    return -1;
  }

  int taskCount = cJSON_GetArraySize(taskListArray);
  TaskItem *tasks = NULL;
  if (taskCount > 0)
  {
    tasks = (TaskItem *)calloc(taskCount, sizeof(TaskItem));
    if (!tasks)
    {
      lingxin_log_error("Memory allocation failed for tasks");
      cJSON_Delete(root);
      return -1;
    }

    for (int i = 0; i < taskCount; i++)
    {
      cJSON *taskObj = cJSON_GetArrayItem(taskListArray, i);
      if (!taskObj || !cJSON_IsObject(taskObj))
      {
        lingxin_log_error("task_list[%d] is not a valid object", i);
        continue;
      }

      cJSON *taskIdObj = cJSON_GetObjectItemCaseSensitive(taskObj, "schedule_task_id");
      cJSON *countdownObj = cJSON_GetObjectItemCaseSensitive(taskObj, "trigger_time");

      tasks[i].taskId = taskIdObj && cJSON_IsString(taskIdObj) ? strdup(taskIdObj->valuestring) : strdup("");

      tasks[i].countdown = countdownObj && cJSON_IsNumber(countdownObj) ? countdownObj->valueint : 0;
    }
  }
  else
  {
    tasks = NULL;
  }

  // 解析 schedule_task_config 对象
  cJSON *configObj = cJSON_GetObjectItemCaseSensitive(dataObj, "schedule_task_config");
  int advanceConnectTime = 0;
  if (configObj && cJSON_IsObject(configObj))
  {
    cJSON *advanceTimeObj = cJSON_GetObjectItemCaseSensitive(configObj, "advance_connect_time");
    if (advanceTimeObj && cJSON_IsNumber(advanceTimeObj))
    {
      advanceConnectTime = advanceTimeObj->valueint;
    }
  }

  // 填充输出结构体
  outList->tasks = tasks;
  outList->taskCount = taskCount;
  outList->advanceConnectTime = advanceConnectTime;
  lingxin_log_debug("Task list解析成功:\n");

  cJSON_Delete(root);
  return 0;
}
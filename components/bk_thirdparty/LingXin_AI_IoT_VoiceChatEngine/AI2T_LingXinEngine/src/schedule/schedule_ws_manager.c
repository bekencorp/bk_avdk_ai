#include "schedule_first_ws.h"
#include "schedule_ws_manager.h"
#include "chat_state_machine.h"
#include "lingxin_auth_config.h"
#include "chat_api.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "lingxin_common.h"
#include "lingxin_log.h"

extern char *generateUUID(int length);

static ScheduleChatConfig *scheduleConfig = NULL;
//static ScheduleChatHandler *globalHandler = NULL; 未使用的全局变量
static bool isFirstExec = true;

static void getConfig()
{
  if (scheduleConfig == NULL)
  {
    scheduleConfig = (ScheduleChatConfig *)calloc(1, sizeof(ScheduleChatConfig));
  }
  AuthAppKeyGetFunc appKeyFunc = lingxin_auth_appKey_get();
  if (!appKeyFunc) {
    lingxin_log_error("appKeyFunc is null, please check lingxin_auth_appKey_get() implementation\n");
    return;
  }
  char *appKey = appKeyFunc();
  if (!appKey || strlen(appKey) == 0)
  {
    lingxin_log_error("appKey is null, please check lingxin_auth_appKey_get() implementation\n");
    return;
  }
  AuthSnGetFunc snFunc = lingxin_auth_sn_get();
  if (!snFunc) {
    lingxin_log_error("snFunc is null, please check lingxin_auth_sn_get() implementation\n");
    return;
  }
  char *sn = snFunc();
  if (!sn || strlen(sn) == 0)
  {
    lingxin_log_error("sn is null, please check lingxin_auth_sn_get() implementation\n");
    return;
  }
  AuthAppIdGetFunc appIdFunc = lingxin_auth_appId_get();
  if (!appIdFunc) {
    lingxin_log_error("appIdFunc is null, please check lingxin_auth_appId_get() implementation\n");
    return;
  }
  char *appId = appIdFunc();
  if (!appId || strlen(appId) == 0)
  {
    lingxin_log_error("appId is null, please check lingxin_auth_appId_get() implementation\n");
    return;
  }
  AuthAgentCodeGetFunc agentCodeFunc = lingxin_auth_agentCode_get();
  if (!agentCodeFunc) {
    lingxin_log_error("agentCodeFunc is null, please check lingxin_auth_agentCode_get() implementation\n");
    return;
  }
  char *agentCode = agentCodeFunc();
  if (!agentCode || strlen(agentCode) == 0)
  {
    lingxin_log_error("agentCode is null, please check lingxin_auth_agentCode_get() implementation\n");
    return;
  }

  scheduleConfig->serverPath = WEBSOCKET_CHAT_PATH;
  scheduleConfig->appKey = appKey;
  scheduleConfig->sn = sn;
  scheduleConfig->appId = appId;
  scheduleConfig->showLog = true;
  scheduleConfig->taskId = generateUUID(32);
}

static void systemEventListener(ScheduleChatHandler *globalHandler, const char *data)
{
  lingxin_log_debug("-----SCHEDULECHAT_EVENT_ON_SYSTEM_EVENT-----%s\n", data);
  state_machine_receive_schedule_data((void *)data);
}

static void doCreate()
{
  getConfig();
  char *instanceId = startFirstScheduleConnect(scheduleConfig, systemEventListener);
  if (!instanceId)
  {
    lingxin_log_error(" Failed to initialize First PowerOn ScheduleChat handler\n");
    free(scheduleConfig);
    scheduleConfig = NULL;
    return;
  }
  lingxin_log_debug("-----First PowerOn ScheduleChat create finish-----\n");
}

void initScheduleChat()
{
  if (isFirstExec)
  {
    isFirstExec = false;
    doCreate();
  }
}

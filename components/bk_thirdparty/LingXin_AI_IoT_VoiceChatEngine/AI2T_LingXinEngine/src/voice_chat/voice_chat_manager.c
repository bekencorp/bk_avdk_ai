#include "audio_buffer_play.h"
#include "voice_chat.h"
#include "voice_chat_manager.h"
#include "chat_state_machine.h"
#include "lingxin_auth_config.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "lingxin_semaphore.h"
#include "lingxin_common.h"
#include "lingxin_log.h"

extern char *generateUUID(int length);
extern char *snprintfWithMalloc(const char *format, ...);

// static char *filePath = NULL;
VoiceChatConfig *config = NULL;
// 语音进语音出
static const char *inputMode = "voice"; // no_voice
static char *scheduleTaskId;            // 待触发的定时任务id
// 开场白
static const char *playPrologue = "false"; // false

static const char *userInput = ""; //"\"user_input\":\"今天星期几\"";
static char *payload;

static VoiceChatHandler *globalHandler = NULL;
static bool isFirstContinueAfterCreate = true;
static ContinueCheckCallback continueCheckCallback = NULL;
static TerminateCheckCallback terminateCheckCallback = NULL;
static ErrorCallback errorCallback = NULL;

static bool isAIResponseding = false; // 已给服务端发请求，带响应，或者 已在响应，YES 才能发打断
static bool isAudioSending = false;
static bool isVadExit = false;
static bool waitTerminateOrEndSuccess = false;
void setWaitTerminateOrEndSuccess(bool target)
{
  waitTerminateOrEndSuccess = target;
}

void initScheduleConfig(char *taskId)
{
  if (taskId != NULL)
  {
    scheduleTaskId = strdup(taskId);
    if (scheduleTaskId == NULL)
    {
      lingxin_log_error("current_task_id内存分配失败！\n");
    }
  }
  inputMode = "no_voice";
}
static void initVoiceChatConfig()
{
  scheduleTaskId = "";
  inputMode = "voice";
}
static void getConfig(ChatContinueParams *params)
{
  if (config == NULL)
  {
    config = (VoiceChatConfig *)calloc(1, sizeof(VoiceChatConfig));
  }
  AuthAppKeyGetFunc appKeyFunc = lingxin_auth_appKey_get();
  if (!appKeyFunc) {
    lingxin_log_error("appKeyFunc is null, please check lingxin_auth_appKey_get() implementation");
    return;
  }
  char *appKey = appKeyFunc();
  if (!appKey || strlen(appKey) == 0)
  {
    lingxin_log_error("appKey is null, please check lingxin_auth_appKey_get() implementation");
    return;
  }
  AuthSnGetFunc snFunc = lingxin_auth_sn_get();
  if (!snFunc) {
    lingxin_log_error("snFunc is null, please check lingxin_auth_sn_get() implementation");
    return;
  }
  char *sn = snFunc();
  if (!sn || strlen(sn) == 0)
  {
    lingxin_log_error("sn is null, please check lingxin_auth_sn_get() implementation");
    return;
  }
  AuthAppIdGetFunc appIdFunc = lingxin_auth_appId_get();
  if (!appIdFunc) {
    lingxin_log_error("appIdFunc is null, please check lingxin_auth_appId_get() implementation");
    return;
  }
  char *appId = appIdFunc();
  if (!appId || strlen(appId) == 0)
  {
    lingxin_log_error("appId is null, please check lingxin_auth_appId_get() implementation");
    return;
  }
  AuthAgentCodeGetFunc agentCodeFunc = lingxin_auth_agentCode_get();
  if (!agentCodeFunc) {
    lingxin_log_error("agentCodeFunc is null, please check lingxin_auth_agentCode_get() implementation");
    return;
  }
  char *agentCode = agentCodeFunc();
  if (!agentCode || strlen(agentCode) == 0)
  {
    lingxin_log_error("agentCode is null, please check lingxin_auth_agentCode_get() implementation");
    return;
  }

  config->serverPath = WEBSOCKET_CHAT_PATH;
  config->appKey = appKey;
  config->sn = sn;
  config->appId = appId;
  if (!params->taskId || strlen(params->taskId) == 0)
  {
    config->taskId =  config->taskId ? config->taskId : generateUUID(32);
  }
  else
  {
    config->taskId = strdup(params->taskId);
  }
  char *vadParams = params->useServerVad ? "chat_vad" : "chat";
  payload = snprintfWithMalloc(
      "{\"task\":\"%s\",\"input_mode\":\"%s\",\"schedule_task_id\":\"%s\",\"agent_code\":\"%s\",\"agent_basic_inputs\":{%s}"
      ",\"agent_ext_inputs\":{\"play_prologue\":%s},\"user_id\":\"111\"}",
      vadParams, inputMode, scheduleTaskId, agentCode, userInput, playPrologue);
  config->payload = payload;
}

static void vadEndRecorderExitCallback()
{
  lingxin_log_debug("vadEndRecorderExitCallback");

  // state_machine_run(STATE_STOP_RECORDING);
  voiceChatStopSendAudio();
  lingxin_log_debug("%s 调用了audio init", __func__);
  audioInit();
}

static void onVoiceChatEvent(VoiceChatEventType event,
                             const char *data, const size_t len, VoiceChatExtraInfo *extraInfo)
{
  // TODO: webSocket 做成单实例 或者 使用 extraInfo.instanceId 来做区分，只响应后一个 instanceId 
  switch (event)
  {
  case VOICECHAT_EVENT_ON_VOIC_SEND_READY:
    lingxin_log_debug("-----VOICECHAT_EVENT_ON_VOIC_SEND_READY-----");
    isVadExit = false;
    waitTerminateOrEndSuccess = false;
    if (continueCheckCallback)
    {
      continueCheckCallback();
    }
    break;
  case VOICECHAT_EVENT_ON_VAD_EXIT:
    lingxin_log_debug("-----VOICECHAT_EVENT_ON_VAD_EXIT-----");
    isVadExit = true;
    // recorderModeExit(vadEndRecorderExitCallback);
    state_machine_run_event(State_Event_Vad_Exit);
    break;
  case VOICECHAT_EVENT_ON_VAD_END:
    lingxin_log_debug("-----VOICECHAT_EVENT_ON_VAD_END-----");
    // recorderModeExit(vadEndRecorderExitCallback);
    state_machine_run_event(State_Event_Vad_Stop);
    break;
  case VOICECHAT_EVENT_ON_RESULT_AI_VOICE_START:
    lingxin_log_debug("-----VOICECHAT_EVENT_ON_RESULT_AI_VOICE_START-----");
    state_machine_run_event(State_Event_VoiceChat_AIStart);
    break;
  case VOICECHAT_EVENT_ON_TEXTOUT:
    lingxin_log_debug("-----VOICECHAT_EVENT_ON_TEXTOUT-----%s", data);
    state_machine_receive_text_data(data);

    // if (strstr(data, "{\"id\": 2000}") ||
    //     strstr(data, "{\"id\":2000}"))
    // {
    //   // 执行退出唤醒
    //   lingxin_log_debug("收到退出唤醒指令，触发退出事件");
    //   state_machine_run_event(State_Event_WillExit);
    // }
    break;
  case VOICECHAT_EVENT_ON_SYSTEM_EVENT:
    lingxin_log_debug("0.0.6 -----VOICECHAT_EVENT_ON_SYSTEM_EVENT-----%s\n", data);
    state_machine_receive_schedule_data((void *)data);
    break;
  case VOICECHAT_EVENT_ON_RESULT_AI_VOICE:
    lingxin_log_debug("-----VOICECHAT_EVENT_ON_RESULT_AI_VOICE-----%d", (int)len);
    // lingxin_log_debug("mp3数据开始，长度:%d",len);

    // // 打印太长，会导致CPU过高
    // int newLen = len > 16 ? 16 : len;

    // for (int i = 0; i < newLen; i++) {
    //     unsigned char byte = data[i]; // 强制转换为无符号类型
    //     lingxin_log_debug("%02X ", byte);        // 打印 16 进制值
    //     if (isprint(byte)) {          // 判断是否为可打印字符
    //         lingxin_log_debug("('%c') ", byte);
    //     } else {
    //         lingxin_log_debug("(.) ");           // 非可打印字符用 '.' 表示
    //     }
    // }
    // lingxin_log_debug("mp3数据结束");

    // audioBufferPlay(data, (int)len);
    state_machine_receive_mp3_data((void *)data, (int)len);
    break;
  case VOICECHAT_EVENT_ON_RESULT_AI_VOICE_READY_TO_END:
    lingxin_log_debug("-----VOICECHAT_EVENT_ON_RESULT_AI_VOICE_READY_TO_END-----");
    waitTerminateOrEndSuccess = true;
    break;
  case VOICECHAT_EVENT_ON_RESULT_AI_VOICE_END:
    lingxin_log_debug("-----VOICECHAT_EVENT_ON_RESULT_AI_VOICE_END-----");
    lingxin_log_debug("zzz: 播放结束啦，需要停止播放啦");
    isAIResponseding = false;
    waitTerminateOrEndSuccess = false;
    // audioBufferEnd();
    state_machine_run_event(State_Event_VoiceChat_AIEnd);
    break;
  case VOICECHAT_EVENT_ON_TERMINAL:
    lingxin_log_debug("-----VOICECHAT_EVENT_ON_TERMINAL-----");
    isAIResponseding = false;
    if (terminateCheckCallback)
    {
      terminateCheckCallback();
    }
    break;
  case VOICECHAT_EVENT_ON_ERROR:
    lingxin_log_error("-----VOICECHAT_EVENT_ON_ERROR-----");
    // isAIResponseding = false;

    lingxin_log_error("%zu", len);
    if (data)
    {
      lingxin_log_error("%s", data);
    }
    if (errorCallback != NULL)
    {
      errorCallback(data); // 传递复制后的数据
    }
    else
    {
      lingxin_log_error("产生错误，但是无 errorCallback");
    }

    break;
  case VOICECHAT_EVENT_ON_DESTROY:
    lingxin_log_debug("------VOICECHAT_EVENT_ON_DESTROY-----");
    isAudioSending = false;
    isAIResponseding = false;
    // TODO: sdk断网等错误，就会释放实例
    lingxin_log_debug("销毁globalHandler");
    globalHandler = NULL;

    // recorderModeExit(NULL);
    state_machine_run_event(State_Event_VoiceChat_ExitEnd);
    
    break;
  default:
    break;
  }
}

bool isVoiceChatInited() { return globalHandler != NULL; }
bool isVoiceChatVadExit() { return isVadExit; }

bool isVoiceChatResponding() { return isAIResponseding; }
bool voiceChatTerminateCheck(TerminateCheckCallback callback,
                             ErrorCallback errorCallback_tem)
{
  lingxin_log_debug("%s ", __func__);

  if (errorCallback == NULL)
  {
    lingxin_log_error("%s 方法中设置的errorCallback为空", __func__);
  }
  else
  {
    lingxin_log_debug("%s 方法中设置的errorCallback_tem成功", __func__);
  }
  if (!isAIResponseding)
  {
    lingxin_log_warn("not isAIResponseding");
    return false;
  }
  if (waitTerminateOrEndSuccess)
  {
    lingxin_log_warn("before  end_task or terminate success,cannot terminate");
    return false;
  }
  terminateCheckCallback = callback;
  errorCallback = errorCallback_tem;
  bool result = voiceChatTerminal(globalHandler);
  if (result)
  {
    waitTerminateOrEndSuccess = true;
    isAudioSending = false;
  }
  return true;
}

void module_terminateCallback()
{
  state_machine_run_event(State_Event_VoiceChat_TerminateEnd);
}

void module_voiceChat_terminate()
{

  if (isAIResponseding)
  {
    lingxin_log_debug("zzz: 准备打断-1");
    // lingxin_lock_write_websocket_control();
    bool b = voiceChatTerminateCheck(module_terminateCallback, NULL);
    // lingxin_unlock_write_websocket_controle();

    if (b == false)
    {
      lingxin_log_warn("zzz: 准备打断-3");
      module_terminateCallback();
    }
  }
  else
  {
    lingxin_log_warn("zzz: 准备打断-2");
    module_terminateCallback();
  }
}

void module_voiceChat_exit()
{
  voiceChatDestroy(globalHandler);
}

bool voiceChatContinueCheck(ChatContinueParams *params, ContinueCheckCallback callback, ErrorCallback errorCallback_temp)
{
  if (!params)
  {
    lingxin_log_error("continue params null");
    return false;
  }
  if (params->isCreateVoiceTask)
  {
    // 如果是拉起一轮新的voice循环，更新start_task的payload
    initVoiceChatConfig();
  }
  else
  {
    // 如果是拉起新一轮novoice循环,允许continue
    isAIResponseding = false;
  }

  continueCheckCallback = callback;
  errorCallback = errorCallback_temp;
  lingxin_log_debug("%s ", __func__);

  if (errorCallback == NULL)
  {
    lingxin_log_error("%s 方法中设置的errorCallback为空", __func__);
  }
  else
  {
    lingxin_log_debug("%s 方法中设置的errorCallback成功", __func__);
  }

  // 不允许连续多次continue
  if (isAIResponseding)
  {
    lingxin_log_warn("isAIResponseding");
    return false;
  }
  // 之前没有建联或者建联后断联了
  if (globalHandler == NULL)
  {
    lingxin_log_warn("%s 重新初始化 SDK", __func__);
    getConfig(params);
    char *instanceId = voiceChatCreate(&globalHandler, config, onVoiceChatEvent);
    if (!instanceId)
    {
      lingxin_log_error("Failed to initialize VoiceChat handler");
      free(config);
      config = NULL;
      
      state_machine_receive_error(EXIT_REASON_WEBSOCKET_CONNECTION_FAILED);
      return false;
    }
    isAIResponseding = false;
    isAudioSending = false;
    lingxin_log_debug("-----Create viocechat finish-----");

    isFirstContinueAfterCreate = false;

    isAIResponseding = true;
  }
  else
  {
    if (isFirstContinueAfterCreate)
    {
      // 建联成功后，第一次调用continue，因为建联的时候已经发送过start了，这里直接回调callback
      isFirstContinueAfterCreate = false;
      lingxin_log_debug("%s 直接回调了 callback", __func__);
      if (continueCheckCallback)
      {
        continueCheckCallback();
      }
    }
    else
    {
      // 非一次调用continue，需要调用voiceChatContinue重新发送start
      // voiceChatContinue(globalHandler);
      getConfig(params);
      voiceChatContinueWithConfig(globalHandler, config);
    }
    isAIResponseding = true;
  }
  return true;
}

void voiceChatSendAudio(void *buf, int len)
{
  lingxin_log_debug("voiceChatSendAudio: %d", len);
  int result = voiceChatSend(globalHandler, (char *)buf, (size_t)len);
  if (result > 0)
  {
    isAudioSending = true;
  }
}

void voiceChatStopSendAudio()
{
  lingxin_log_debug("voiceChatStopSendAudio:");
  if (!isAudioSending)
  {
    lingxin_log_error("not Sending Audio:");
    return;
  }
  bool result = voiceChatSendStop(globalHandler);
  if (result)
  {
    isAudioSending = false;
  }
}
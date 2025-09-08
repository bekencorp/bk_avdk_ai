#include "audio_buffer_play.h"
#include "local_audio_play.h"
#include "audio_recorder.h"
#include "voice_chat.h"
#include "voice_chat_manager.h"
#include "chat_state_machine.h"
#include "lingxin_auth_config.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "lingxin_semaphore.h"

extern char *generateUUID(int length);
extern char *snprintfWithMalloc(const char *format, ...);

VoiceChatConfig *config = NULL;
// 语音进语音出
static const char *inputMode = "voice"; // no_voice
// 开场白
static const char *playPrologue = "false"; // false

static const char *userInput = ""; //"\"user_input\":\"今天星期几\"";

static const char *flowControl = "";
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

static void getConfig()
{
  if (config == NULL)
  {
    config = (VoiceChatConfig *)calloc(1, sizeof(VoiceChatConfig));
  }
  char *appKey = lingxin_auth_appKey_get();
  if (!appKey || strlen(appKey) == 0)
  {
    printf("0.0.6 appKey is null, please check lingxin_auth_appKey_get() implementation\n");
    return;
  }
  char *sn = lingxin_auth_sn_get();
  if (!sn || strlen(sn) == 0)
  {
    printf("0.0.6 sn is null, please check lingxin_auth_sn_get() implementation\n");
    return;
  }
  char *appId = lingxin_auth_appId_get();
  if (!appId || strlen(appId) == 0)
  {
    printf("0.0.6 appId is null, please check lingxin_auth_appId_get() implementation\n");
    return;
  }
  char *agentCode = lingxin_auth_agentCode_get();
  if (!agentCode || strlen(agentCode) == 0)
  {
    printf("0.0.6 agentCode is null, please check lingxin_auth_agentCode_get() implementation\n");
    return;
  }

  config->serverPath = "gw/ws/open/api/v2/agentChat";
  config->appKey = appKey;
  config->sn = sn;
  config->appId = appId;
  config->showLog = true;
  config->taskId = generateUUID(32);
  payload = snprintfWithMalloc(
      "{\"input_mode\":\"%s\",\"agent_code\":\"%s\",\"agent_basic_inputs\":{%s}"
      ",\"agent_ext_inputs\":{\"play_prologue\":%s},\"user_id\":\"111\",\"sn\":"
      "\"111\"%s}",
      inputMode, agentCode, userInput, playPrologue, flowControl);
  config->payload = payload;
}

static void vadEndRecorderExitCallback()
{
  printf("0.0.6 vadEndRecorderExitCallback");

  // state_machine_run(STATE_STOP_RECORDING);
  voiceChatStopSendAudio();
  printf("%s 调用了audio init", __func__);
  audioInit();
}

static void onVoiceChatEvent(VoiceChatEventType event,
                             const char *data, const size_t len, VoiceChatExtraInfo *extraInfo)
{
  switch (event)
  {
  case VOICECHAT_EVENT_ON_VOIC_SEND_READY:
    printf("0.0.6 -----VOICECHAT_EVENT_ON_VOIC_SEND_READY-----\n");
    isVadExit = false;
    waitTerminateOrEndSuccess = false;
    if (continueCheckCallback)
    {
      continueCheckCallback();
    }
    break;
  case VOICECHAT_EVENT_ON_VAD_EXIT:
    printf("0.0.6 -----VOICECHAT_EVENT_ON_VAD_EXIT-----\n");
    isVadExit = true;
    // recorderModeExit(vadEndRecorderExitCallback);
    state_machine_run_event(State_Event_Vad_Exit);
    break;
  case VOICECHAT_EVENT_ON_VAD_END:
    printf("0.0.6 -----VOICECHAT_EVENT_ON_VAD_END-----\n");
    // recorderModeExit(vadEndRecorderExitCallback);
    state_machine_run_event(State_Event_Vad_Stop);
    break;
  case VOICECHAT_EVENT_ON_RESULT_AI_VOICE_START:
    printf("0.0.6 -----VOICECHAT_EVENT_ON_RESULT_AI_VOICE_START-----\n");
    state_machine_run_event(State_Event_VoiceChat_AIStart);
    break;
  case VOICECHAT_EVENT_ON_TEXTOUT:
    printf("0.0.6 -----VOICECHAT_EVENT_ON_TEXTOUT-----%s\n", data);

    break;
  case VOICECHAT_EVENT_ON_RESULT_AI_VOICE:
    printf("0.0.6 -----VOICECHAT_EVENT_ON_RESULT_AI_VOICE-----%d\n", (int)len);
    // printf("0.0.6 mp3数据开始，长度:%d\n",len);

    // // 打印太长，会导致CPU过高
    // int newLen = len > 16 ? 16 : len;

    // for (int i = 0; i < newLen; i++) {
    //     unsigned char byte = data[i]; // 强制转换为无符号类型
    //     printf("0.0.6 %02X ", byte);        // 打印 16 进制值
    //     if (isprint(byte)) {          // 判断是否为可打印字符
    //         printf("0.0.6 ('%c') ", byte);
    //     } else {
    //         printf("0.0.6 (.) ");           // 非可打印字符用 '.' 表示
    //     }
    // }
    // printf("0.0.6 mp3数据结束\n");

    // audioBufferPlay(data, (int)len);
    state_machine_receive_mp3_data((void *)data, (int)len);
    break;
  case VOICECHAT_EVENT_ON_RESULT_AI_VOICE_READY_TO_END:
    printf("0.0.6 -----VOICECHAT_EVENT_ON_RESULT_AI_VOICE_READY_TO_END-----\n");
    waitTerminateOrEndSuccess = true;
    break;
  case VOICECHAT_EVENT_ON_RESULT_AI_VOICE_END:
    printf("0.0.6 -----VOICECHAT_EVENT_ON_RESULT_AI_VOICE_END-----\n");
    printf("0.0.6 zzz: 播放结束啦，需要停止播放啦");
    isAIResponseding = false;
    waitTerminateOrEndSuccess = false;
    // audioBufferEnd();
    state_machine_run_event(State_Event_VoiceChat_AIEnd);
    break;
  case VOICECHAT_EVENT_ON_TERMINAL:
    printf("0.0.6 -----VOICECHAT_EVENT_ON_TERMINAL-----\n");
    isAIResponseding = false;
    if (terminateCheckCallback)
    {
      terminateCheckCallback();
    }
    break;
  case VOICECHAT_EVENT_ON_ERROR:
    printf("0.0.6 -----VOICECHAT_EVENT_ON_ERROR-----\n");
    // isAIResponseding = false;

    printf("0.0.6 %zu\n", len);
    if (data)
    {
      printf("0.0.6 %s\n", data);
    }
    if (errorCallback != NULL)
    {
      errorCallback(data); // 传递复制后的数据
    }
    else
    {
      printf("0.0.6 产生错误，但是无 errorCallback");
    }

    break;
  case VOICECHAT_EVENT_ON_DESTROY:
    printf("0.0.6 ------VOICECHAT_EVENT_ON_DESTROY-----\n");
    isAudioSending = false;
    isAIResponseding = false;
    // TODO: sdk断网等错误，就会释放实例
    printf("0.0.6 销毁globalHandler");
    globalHandler = NULL;

    // recorderModeExit(NULL);

    state_machine_run_event(State_Event_VoiceChat_ExitEnd);
    break;
  default:
    break;
  }
}

static void doCreate()
{
  getConfig();
  char *instanceId = voiceChatCreate(&globalHandler, config, onVoiceChatEvent);
  if (!instanceId)
  {
    printf("0.0.6 Failed to initialize VoiceChat handler\n");
    free(config);
    config = NULL;
    return;
  }
  //modified by beken
  //isAIResponseding = false;
  isAudioSending = false;
  printf("0.0.6 -----Create viocechat finish-----\n");
}

bool isVoiceChatInited() { return globalHandler != NULL; }
bool isVoiceChatVadExit() { return isVadExit; }

bool isVoiceChatResponding() { return isAIResponseding; }

bool voiceChatTerminateCheck(TerminateCheckCallback callback,
                             ErrorCallback errorCallback_tem)
{
  printf("0.0.6 %s \n", __func__);

  if (errorCallback == NULL)
  {
    printf("0.0.6 %s 方法中设置的errorCallback为空", __func__);
  }
  else
  {
    printf("0.0.6 %s 方法中设置的errorCallback_tem成功", __func__);
  }
  if (!isAIResponseding)
  {
    printf("0.0.6 not isAIResponseding\n");
    return false;
  }
  if (waitTerminateOrEndSuccess)
  {
    printf("0.0.6 before  end_task or terminate success,cannot terminate\n");
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
      printf("0.0.6 zzz: 准备打断-1");
      // lingxin_pend_write_semaphore();
      bool b = voiceChatTerminateCheck(module_terminateCallback, NULL);
      // lingxin_post_write_semaphore();

      if (b == false) {
        printf("0.0.6 zzz: 准备打断-3");
         module_terminateCallback();
      }
      
    }
    else
    {
      printf("0.0.6 zzz: 准备打断-2");
        module_terminateCallback();
    }
}

void module_voiceChat_exit()
{
  voiceChatDestroy(globalHandler);
}

bool voiceChatContinueCheck(ContinueCheckCallback callback,
                            ErrorCallback errorCallback_tem)
{

  continueCheckCallback = callback;
  errorCallback = errorCallback_tem;
  printf("0.0.6 %s \n", __func__);

  if (errorCallback == NULL)
  {
    printf("0.0.6 %s 方法中设置的errorCallback为空", __func__);
  }
  else
  {
    printf("0.0.6 %s 方法中设置的errorCallback成功", __func__);
  }

  // 不允许连续多次continue
  if (isAIResponseding)
  {
    printf("0.0.6 isAIResponseding\n");
    return false;
  }
  // 之前没有建联或者建联后断联了
  if (globalHandler == NULL)
  {
    printf("0.0.6 %s 重新初始化 SDK", __func__);
    //modified by beken
    isAIResponseding = true;
    doCreate();

    isFirstContinueAfterCreate = false;
  }
  else
  {
    if (isFirstContinueAfterCreate)
    {
      // 建联成功后，第一次调用continue，因为建联的时候已经发送过start了，这里直接回调callback
      isFirstContinueAfterCreate = false;
      printf("0.0.6 %s 直接回调了 callback", __func__);
      if (continueCheckCallback)
      {
        continueCheckCallback();
      }
    }
    else
    {
      // 非一次调用continue，需要调用voiceChatContinue重新发送start
      voiceChatContinue(globalHandler);
    }
    isAIResponseding = true;
  }
  return true;
}

int voiceChatSendAudio(void *buf, int len)
{
  printf("0.0.6 voiceChatSendAudio: %d\n", len);
  int result = voiceChatSend(globalHandler, (char *)buf, (size_t)len);
  if (result > 0)
  {
    isAudioSending = true;
  }
  return result;
}

void voiceChatStopSendAudio()
{
  printf("0.0.6 voiceChatStopSendAudio:\n");
  if (!isAudioSending)
  {
    printf("0.0.6 not Sending Audio:\n");
    return;
  }
  bool result = voiceChatSendStop(globalHandler);
  if (result)
  {
    isAudioSending = false;
  }
}

void initVoiceChat()
{
  doCreate();
}
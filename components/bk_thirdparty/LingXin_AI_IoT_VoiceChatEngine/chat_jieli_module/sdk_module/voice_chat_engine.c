// #include "app_config.h"
#include "audio_buffer_play.h"
#include "client_diy.h"
#include "local_audio_play.h"
// #include "os/os_api.h"
#include <stdlib.h>
#include "voice_chat.h"
#include "voice_chat_engine.h"
#include "voice_chat_machine.h"

extern char *snprintfWithMalloc(const char *format, ...);
extern char *generateUUID(int length);

static VoiceChatHandler *handler;
VoiceChatConfig *config = NULL;
// 语音进语音出
static const char *inputMode = "voice"; // no_voice
// 开场白
static const char *playPrologue = "false"; // false

static const char *userInput = ""; //"\"user_input\":\"今天星期几\"";

static const char *flowControl = "";
// static const char *flowControl =
// ",\"flow_control_parameters\":{\"flow_control_strategy\":\"fixed_time_interval\",\"space_time\":300}";
// static const char *flowControl =
// ",\"flow_control_parameters\":{\"flow_control_strategy\":\"dynamic\",\"buffer_pool_size\":10240}";
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

static void onVoiceChatEvent(VoiceChatEventType event, const char *data, const size_t len, VoiceChatExtraInfo *extraInfo)
{
  printf("------------onVoiceChatEvent--------------taskId: %s. requestId: %s\n", extraInfo->taskId, extraInfo->requestId);

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
#ifdef PAYLOAD_DEMAND
    voiceChatGetNextFlow(handler);
#endif
    break;
  case VOICECHAT_EVENT_ON_TEXTOUT:
    printf("0.0.6 -----VOICECHAT_EVENT_ON_TEXTOUT-----%s\n", data);

    break;
  case VOICECHAT_EVENT_ON_RESULT_AI_VOICE:
    printf("0.0.6 -----VOICECHAT_EVENT_ON_RESULT_AI_VOICE-----%d\n", len);
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
    isAIResponseding = false;

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
    //printf("0.0.6 %s 销毁globalHandler");
    globalHandler = NULL;

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
  printf("0.0.6 %s \n", __func__);

  if (errorCallback == NULL)
  {
    printf("0.0.6 %s 方法中设置的errorCallback为空", __func__);
  }
  else
  {
    printf("0.0.6 %s 方法中设置成功 %s", __func__, errorCallback_tem);
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

void module_termiateCallback()
{
  state_machine_run_event(State_Event_VoiceChat_TerminateEnd);
}

void module_voiceChat_terminate()
{
  if (isAIResponseding)
  {
    terminateCheckCallback = module_termiateCallback;
    bool result = voiceChatTerminal(globalHandler);
    if (result)
    {
      isAudioSending = false;
    }
  }
  else
  {
    module_termiateCallback();
  }
}

static void doCreate();

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
    printf("0.0.6 %s 方法中设置成功 %s", __func__, errorCallback_tem);
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
    doCreate();
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

void voiceChatSendAudio(void *buf, int len)
{
  printf("0.0.6 voiceChatSendAudio: %d\n", len);
  int result = voiceChatSend(globalHandler, (char *)buf, (size_t)len);
  if (result > 0)
  {
    isAudioSending = true;
  }
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

static void getConfig()
{
  if (config == NULL)
  {
    config = (VoiceChatConfig *)calloc(1, sizeof(VoiceChatConfig));
  }
  config->appKey = APP_KEY;
  config->sn = SN;
  config->showLog = true;
  config->taskId = generateUUID(32);
  payload = snprintfWithMalloc(
      "{\"input_mode\":\"%s\",\"agent_code\":\"%s\",\"agent_basic_inputs\":{%s}"
      ",\"agent_ext_inputs\":{\"play_prologue\":%s},\"user_id\":\"111\",\"sn\":"
      "\"111\"%s}",
      inputMode, AGENT_CODE, userInput, playPrologue, flowControl);
  config->payload = payload;
}
static void doCreate()
{
  getConfig();
  char *instanceId = voiceChatCreate(&handler, config, onVoiceChatEvent);

  if (!instanceId)
  {
    printf("0.0.6 Failed to initialize VoiceChat handler\n");
    free(config);
    config = NULL;
    return;
  }
  isAIResponseding = false;
  isAudioSending = false;
  globalHandler = handler;
  printf("0.0.6 -----Create viocechat finish-----\n");
}

static void vadEndRecorderExitCallback()
{
  printf("0.0.6 vadEndRecorderExitCallback");

  // state_machine_run(STATE_STOP_RECORDING);
  voiceChatStopSendAudio();
  printf("%s 调用了audio init", __func__);
  audioInit();
}
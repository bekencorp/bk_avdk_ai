// #include "app_config.h"
#include "audio_buffer_play.h"
#include "client_diy.h"
#include "local_audio_play.h"
// #include "os/os_api.h"
#include <stdlib.h>
#include "voice_chat.h"
#include "voice_chat_engine.h"
#include "voice_chat_machine.h"
#include "components/log.h"

#define TAG "chat_engine"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
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
  LOGD("------------onVoiceChatEvent--------------taskId: %s. requestId: %s\n", extraInfo->taskId, extraInfo->requestId);

  switch (event)
  {
  case VOICECHAT_EVENT_ON_VOIC_SEND_READY:
    LOGI("0.0.6 -----VOICECHAT_EVENT_ON_VOIC_SEND_READY-----\n");
    isVadExit = false;
    waitTerminateOrEndSuccess = false;
    if (continueCheckCallback)
    {
      continueCheckCallback();
    }
    break;
  case VOICECHAT_EVENT_ON_VAD_EXIT:
    LOGI("0.0.6 -----VOICECHAT_EVENT_ON_VAD_EXIT-----\n");
    isVadExit = true;
    // recorderModeExit(vadEndRecorderExitCallback);
    state_machine_run_event(State_Event_Vad_Exit);
    break;
  case VOICECHAT_EVENT_ON_VAD_END:
    LOGI("0.0.6 -----VOICECHAT_EVENT_ON_VAD_END-----\n");
    // recorderModeExit(vadEndRecorderExitCallback);
    state_machine_run_event(State_Event_Vad_Stop);
    break;
  case VOICECHAT_EVENT_ON_RESULT_AI_VOICE_START:
    LOGI("0.0.6 -----VOICECHAT_EVENT_ON_RESULT_AI_VOICE_START-----\n");
    state_machine_run_event(State_Event_VoiceChat_AIStart);
#ifdef PAYLOAD_DEMAND
    voiceChatGetNextFlow(handler);
#endif
    break;
  case VOICECHAT_EVENT_ON_TEXTOUT:
    LOGI("0.0.6 -----VOICECHAT_EVENT_ON_TEXTOUT-----%s\n", data);

    break;
  case VOICECHAT_EVENT_ON_RESULT_AI_VOICE:
    LOGI("0.0.6 -----VOICECHAT_EVENT_ON_RESULT_AI_VOICE-----%d\n", len);
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
    LOGI("0.0.6 -----VOICECHAT_EVENT_ON_RESULT_AI_VOICE_READY_TO_END-----\n");
    waitTerminateOrEndSuccess = true;
    break;
  case VOICECHAT_EVENT_ON_RESULT_AI_VOICE_END:
    LOGI("0.0.6 -----VOICECHAT_EVENT_ON_RESULT_AI_VOICE_END-----\n");
    LOGI("0.0.6 zzz: 播放结束啦，需要停止播放啦\r\n");
    isAIResponseding = false;
    waitTerminateOrEndSuccess = false;
    // audioBufferEnd();
    state_machine_run_event(State_Event_VoiceChat_AIEnd);
    break;
  case VOICECHAT_EVENT_ON_TERMINAL:
    LOGI("0.0.6 -----VOICECHAT_EVENT_ON_TERMINAL-----\n");
    isAIResponseding = false;
    if (terminateCheckCallback)
    {
      terminateCheckCallback();
    }
    break;
  case VOICECHAT_EVENT_ON_ERROR:
    LOGI("0.0.6 -----VOICECHAT_EVENT_ON_ERROR-----\n");
    // isAIResponseding = false;

    LOGI("0.0.6 %zu\n", len);
    if (data)
    {
      LOGI("0.0.6 %s\n", data);
    }
    if (errorCallback != NULL)
    {
      errorCallback(data); // 传递复制后的数据
    }
    else
    {
      LOGI("0.0.6 产生错误，但是无 errorCallback\r\n");
    }

    break;
  case VOICECHAT_EVENT_ON_DESTROY:
    LOGI("0.0.6 ------VOICECHAT_EVENT_ON_DESTROY-----\n");
    isAudioSending = false;
    isAIResponseding = false;
    // TODO: sdk断网等错误，就会释放实例
    LOGI("0.0.6 销毁globalHandler\n");
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
  LOGI("0.0.6 %s \n", __func__);

  if (errorCallback == NULL)
  {
    LOGI("0.0.6 %s 方法中设置的errorCallback为空\r\n", __func__);
  }
  else
  {
    LOGI("0.0.6 %s 方法中设置成功 %s\n", __func__, errorCallback_tem);
  }
  if (!isAIResponseding)
  {
    LOGI("0.0.6 not isAIResponseding\n");
    return false;
  }
  if (waitTerminateOrEndSuccess)
  {
    LOGI("0.0.6 before  end_task or terminate success,cannot terminate\n");
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
  LOGI("0.0.6 %s \n", __func__);

  if (errorCallback == NULL)
  {
    LOGI("0.0.6 %s 方法中设置的errorCallback为空\r\n", __func__);
  }
  else
  {
    LOGI("0.0.6 %s 方法中设置成功 %s\r\n", __func__, errorCallback_tem);
  }

  // 不允许连续多次continue
  if (isAIResponseding)
  {
    LOGI("0.0.6 isAIResponseding\n");
    return false;
  }
  // 之前没有建联或者建联后断联了
  if (globalHandler == NULL)
  {
    LOGI("0.0.6 %s 重新初始化 SDK\r\n", __func__);
    doCreate();

    isFirstContinueAfterCreate = false;

    isAIResponseding = true;
  }
  else
  {
    if (isFirstContinueAfterCreate)
    {
      // 建联成功后，第一次调用continue，因为建联的时候已经发送过start了，这里直接回调callback
      isFirstContinueAfterCreate = false;
      LOGI("0.0.6 %s 直接回调了 callback\r\n", __func__);
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
  LOGD("0.0.6 voiceChatSendAudio: %d\n", len);
  int result = voiceChatSend(globalHandler, (char *)buf, (size_t)len);
  if (result > 0)
  {
    isAudioSending = true;
  }
  return result;
}

void voiceChatStopSendAudio()
{
  LOGI("0.0.6 voiceChatStopSendAudio:\n");
  if (!isAudioSending)
  {
    LOGI("0.0.6 not Sending Audio:\n");
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
  config->serverPath = "gw/ws/open/api/v2/agentChat";
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
    LOGI("0.0.6 Failed to initialize VoiceChat handler\n");
    free(config);
    config = NULL;
    return;
  }
  isAIResponseding = false;
  isAudioSending = false;
  globalHandler = handler;
  LOGI("0.0.6 -----Create viocechat finish-----\n");
}

static void vadEndRecorderExitCallback()
{
  LOGI("0.0.6 vadEndRecorderExitCallback\r\n");

  // state_machine_run(STATE_STOP_RECORDING);
  voiceChatStopSendAudio();
  LOGI("%s 调用了audio init\r\n", __func__);
  audioInit();
}
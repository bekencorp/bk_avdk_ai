#ifndef VOICE_CHAT_ENGINE_H
#define VOICE_CHAT_ENGINE_H

#include <stdbool.h>

#define current_demo_version "0.0.6"

typedef struct
{
    bool isCreateVoiceTask;
    bool useServerVad;
    char *taskId;
} ChatContinueParams;

typedef void (*ContinueCheckCallback)();
typedef void (*TerminateCheckCallback)();
typedef void (*ErrorCallback)(const char *data);
void setWaitTerminateOrEndSuccess(bool target);
void initScheduleConfig(char *taskId);
bool voiceChatContinueCheck(ChatContinueParams *params, ContinueCheckCallback callback, ErrorCallback errorCallback);
bool voiceChatTerminateCheck(TerminateCheckCallback callback, ErrorCallback errorCallback);

void voiceChatSendAudio(void *buf, int rlen);
void voiceChatStopSendAudio();
bool isVoiceChatResponding();
bool isVoiceChatInited();
bool isVoiceChatVadExit();

void module_voiceChat_terminate();

// 被退出对话流程
void module_voiceChat_exit();

#endif // VOICE_CHAT_ENGINE_H
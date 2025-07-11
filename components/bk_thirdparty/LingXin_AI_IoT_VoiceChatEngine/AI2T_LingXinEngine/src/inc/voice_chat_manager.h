#ifndef VOICE_CHAT_ENGINE_H
#define VOICE_CHAT_ENGINE_H

#include <stdbool.h>

#define current_demo_version "0.0.6"

typedef void (*ContinueCheckCallback)();
typedef void (*TerminateCheckCallback)();
typedef void (*ErrorCallback)(const char *data);

bool voiceChatContinueCheck(ContinueCheckCallback callback,
                            ErrorCallback errorCallback);
bool voiceChatTerminateCheck(TerminateCheckCallback callback,
                             ErrorCallback errorCallback);

int voiceChatSendAudio(void *buf, int rlen);
void voiceChatStopSendAudio();
bool isVoiceChatResponding();
bool isVoiceChatInited();
bool isVoiceChatVadExit();

void module_voiceChat_terminate();

void initVoiceChat();

// 被退出对话流程
void module_voiceChat_exit();

#endif // VOICE_CHAT_ENGINE_H
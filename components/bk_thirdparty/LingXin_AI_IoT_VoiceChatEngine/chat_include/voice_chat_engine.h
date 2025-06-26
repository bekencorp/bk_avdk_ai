#ifndef VOICE_CHAT_ENGINE_H
#define VOICE_CHAT_ENGINE_H

#include "chat_system_include.h"

#define current_demo_version "0.0.6"

typedef void (*ContinueCheckCallback)();
typedef void (*TerminateCheckCallback)();
typedef void (*ErrorCallback)(const char *data);

void extract_filename_core(const char *path, char *result, size_t result_size);

bool voiceChatContinueCheck(ContinueCheckCallback callback, ErrorCallback errorCallback);
bool voiceChatTerminateCheck(TerminateCheckCallback callback, ErrorCallback errorCallback);

int voiceChatSendAudio(void *buf, int rlen);
void voiceChatStopSendAudio();
bool isVoiceChatResponding();
bool isVoiceChatInited();
bool isVoiceChatVadExit();

void module_voiceChat_terminate();

#endif // VOICE_CHAT_ENGINE_H
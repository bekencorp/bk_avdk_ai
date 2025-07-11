#ifndef LOCAL_AUDIO_PLAY_H
#define LOCAL_AUDIO_PLAY_H

typedef void (*PlayFinishCallback)();
// void closeLocalAudioPlay();
int playFlashAudio(const char *path, PlayFinishCallback callback);
int playDingDongAudio();
// bool isLocalAudioPlaying();

#endif // SDK_LOCAL_AUDIO_PLAY_H
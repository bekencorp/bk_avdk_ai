#ifndef SDK_AUDIO_RECORDER_H
#define SDK_AUDIO_RECORDER_H

typedef void (*RecorderInitedCallback)();

typedef void (*RecorderExitCallback)();

int recorderModeInit(RecorderInitedCallback callback);
void recorderModeExit(RecorderExitCallback callback);



// 新接口
int module_record_init();
void module_record_stop();
void module_record_terminate();

#endif // SDK_AUDIO_RECORDER_H
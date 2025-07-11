
#ifndef AUDIO_BUFFER_PLAY_H
#define AUDIO_BUFFER_PLAY_H

// 功能：在方法里面实现模块的初始化逻辑。并且Chat套件内核会多次回调这个方法，如果当前模块已经初始化成功，可直接回调初始化成功的回调事件。
// 调用时机：由chat套件内核发起调用，客户实现
void module_bufferPlay_audioInit();

// buf为mp3数据 rlen为当前数据长度
void module_bufferPlay_data(void *buf, int rlen);

// 功能：指Chat套件内核调用流式播放模块告诉他已经没有流式播放数据了，并非要立刻停止播放。
// 调用时机：由chat套件内核发起调用，客户实现
void module_bufferPlay_audioEnd();

// 功能：停止当前的播放逻辑
// 调用时机：由chat套件内核发起调用，客户实现
void module_bufferPlay_terminate();

/**
 * 老接口
 */
typedef void (*NoParamsCallback)();

void audioInit();
void audioBufferPlay(void *buf, int rlen);
void audioBufferEnd();
int audio_buff_get_dec_status(void);


void audioBufferContinuePause(NoParamsCallback callback);

void postSemphore();


#endif // AUDIO_BUFFER_PLAY_H


#ifndef Voice_QUEUE_H
#define Voice_QUEUE_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef LINGXI_USE_VOICE_QUEUE

#include <stdbool.h>
#include <pthread.h>
#include "lingxin_common.h"

  // 定义队列结构
  typedef struct
  {
    void *buffer;    // 数据缓冲区
    size_t capacity; // 队列总容量（字节数）
    size_t count;    // 当前已使用的缓冲区大小
    size_t front;    // 队列头部索引
    size_t rear;     // 队列尾部索引
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
  } VoiceQueue;

  typedef struct
  {
    void *data;
    size_t length;
  } VoiceChunk;

  typedef bool (*ContinueWaitCheck)(void *userContext, size_t space);

  // 初始化队列
  VoiceQueue *voiceQueueCreate(size_t capacity);

  // 销毁队列
  void destroyVoiceQueue(VoiceQueue *queue);

  // 入队操作
  bool voiceEnqueue(VoiceQueue *queue, const char *data, size_t size);

  // 出队操作
  bool voiceDequeue(VoiceQueue *queue, VoiceChunk *chunk,
                    ContinueWaitCheck checkFunc, void *userContext);

  size_t getRemainSpaceOfVoiceQueue(VoiceQueue *queue);

  bool isVoiceQueueSpaceEnough(VoiceQueue *queue);

  void clearVoiceQueue(VoiceQueue *queue);

#endif // LINGXI_USE_VOICE_QUEUE

#ifdef __cplusplus
}
#endif
#endif // Voice_QUEUE_H

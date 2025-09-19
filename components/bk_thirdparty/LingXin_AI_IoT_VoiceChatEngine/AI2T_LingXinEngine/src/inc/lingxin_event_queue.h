#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef LINGXI_USE_VOICE_QUEUE

#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>
#include "lingxin_common.h"

  extern int EVENT_QUEUE_FINISH_FLAG;

  typedef struct
  {
    int eventType;
    const char *data;
    size_t dataSize;
  } EventChunk;

  // 队列节点结构
  typedef struct EventQueueNode
  {
    EventChunk *event;
    struct EventQueueNode *next;
  } EventQueueNode;

  typedef void (*EventQueueCallback)(void *userContext, int event,
                                     const char *data, const size_t len);

  // 队列结构
  typedef struct
  {
    EventQueueNode *front;
    EventQueueNode *rear;
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    bool isDestroyed;
    pthread_t notifyThread;
    EventQueueCallback callback;
    void *userContext;
  } EventQueue;

  // 状态码
  typedef enum
  {
    QUEUE_SUCCESS,
    QUEUE_MEMORY_ERROR,
    QUEUE_MUTEX_ERROR,
  } EventQueueStatus;

  // 初始化队列
  EventQueue *eventQueueCreate(void *userContext, EventQueueCallback callback);

  // 销毁队列
  void eventQueueDestroy(EventQueue *queue);

  // 入队操作
  EventQueueStatus eventQueueEnqueue(EventQueue *queue, int dataType,
                                     const char *message, size_t messageSize);

  // 出队操作
  EventChunk *eventQueueDequeue(EventQueue *queue);

#endif // LINGXI_USE_VOICE_QUEUE

#ifdef __cplusplus
}
#endif
#endif // EVENT_QUEUE_H

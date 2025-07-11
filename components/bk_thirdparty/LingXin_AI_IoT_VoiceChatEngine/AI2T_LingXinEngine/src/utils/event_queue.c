#ifdef LINGXI_USE_VOICE_QUEUE

#include <stdbool.h>
#include <stddef.h>
#include "lingxin_event_queue.h"

int EVENT_QUEUE_FINISH_FLAG = 10001;

static void *eventNotifyThread(void *arg)
{
  EventQueue *queue = (EventQueue *)arg;
  if (!queue)
  {
    return NULL;
  }
  // logPrintf("eventNotifyThread before");
  while (!queue->isDestroyed)
  {
    // logPrintf("eventNotifyThread");
    EventChunk *event = eventQueueDequeue(queue);
    if (event && queue->callback)
    {
      if (queue && queue->callback)
      {
        queue->callback(queue->userContext, event->eventType, event->data,
                        event->dataSize);
      }
      free(event); // 释放 eventChunk 内存
      logPrintf("eventNotifyThread callback finish");
    }
  }
  // logPrintf("eventNotifyThread after");
  return NULL;
}

// 初始化队列
EventQueue *eventQueueCreate(void *userContext, EventQueueCallback callback)
{
  EventQueue *queue = (EventQueue *)calloc(1, sizeof(EventQueue));
  if (!queue)
  {
    return NULL;
  }
  queue->front = NULL;
  queue->rear = NULL;
  queue->isDestroyed = false;
  queue->callback = callback;
  queue->userContext = userContext;
  queue->mutex = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));
  queue->cond = (pthread_cond_t *)calloc(1, sizeof(pthread_cond_t));
  pthread_mutex_init(queue->mutex, NULL);
  pthread_cond_init(queue->cond, NULL);

  pthread_create(&queue->notifyThread, NULL, eventNotifyThread, (void *)queue);

  return queue;
}

// 销毁队列
void eventQueueDestroy(EventQueue *queue)
{
  if (!queue)
  {
    return;
  }
  logPrintf("eventQueueDestroy begin");
  pthread_mutex_lock(queue->mutex);
  EventQueueNode *current = queue->front;
  while (current)
  {
    EventQueueNode *next = current->next;
    free(current);
    current = next;
  }
  queue->isDestroyed = true;
  // 唤醒等待的线程
  pthread_cond_signal(queue->cond);
  pthread_mutex_unlock(queue->mutex);

  // 等待通知线程结束
  pthread_join(queue->notifyThread, NULL);

  pthread_mutex_destroy(queue->mutex);
  pthread_cond_destroy(queue->cond);

  free(queue->mutex);
  free(queue->cond);
  free(queue);
  logPrintf("eventQueueDestroy finish");
}

// 入队操作
EventQueueStatus eventQueueEnqueue(EventQueue *queue, int dataType,
                                   const char *message, size_t messageSize)
{
  logPrintf("eventQueueEnqueue begin %d: %d: %s", dataType, messageSize,
            !message ? "NULL" : message);

  EventQueueNode *new_node =
      (EventQueueNode *)calloc(1, sizeof(EventQueueNode));
  if (!new_node)
  {
    return QUEUE_MEMORY_ERROR;
  }
  EventChunk *chunk = (EventChunk *)calloc(1, sizeof(EventChunk));
  if (!chunk)
  {
    logPrintf("event malloc fail");
  }
  chunk->eventType = dataType;
  chunk->data = message;
  chunk->dataSize = messageSize;

  new_node->event = chunk;
  new_node->next = NULL;
  int lockResult = pthread_mutex_lock(queue->mutex);
  if (lockResult != 0)
  {
    logPrintf("queueEnqueue: Mutex lock failed with error code %d", lockResult);
    free(new_node);
    return QUEUE_MUTEX_ERROR;
  }

  if (!queue->rear)
  {
    queue->front = new_node;
    queue->rear = new_node;
  }
  else
  {
    queue->rear->next = new_node;
    queue->rear = new_node;
  }
  pthread_cond_signal(queue->cond);
  pthread_mutex_unlock(queue->mutex);
  logPrintf("eventQueueEnqueue finish");
  return QUEUE_SUCCESS;
}

// 出队操作
EventChunk *eventQueueDequeue(EventQueue *queue)
{
  logPrintf("eventQueueDequeue: begin");
  pthread_mutex_lock(queue->mutex);
  while (!queue->front && !queue->isDestroyed)
  {
    logPrintf("eventQueueDequeue: wait");
    pthread_cond_wait(queue->cond, queue->mutex);
    logPrintf("eventQueueDequeue:after wait");
  }

  if (queue->isDestroyed)
  {
    pthread_mutex_unlock(queue->mutex);
    return NULL;
  }

  EventQueueNode *front_node = queue->front;
  EventChunk *data = front_node->event;
  queue->front = front_node->next;
  if (!queue->front)
  {
    queue->rear = NULL;
  }
  pthread_mutex_unlock(queue->mutex);
  free(front_node);
  logPrintf("eventQueueDequeue: finish: %d, %zu", data->eventType,
            data->dataSize);

  return data;
}
#endif // LINGXI_USE_VOICE_QUEUE
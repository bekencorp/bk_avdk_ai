#ifdef LINGXI_USE_VOICE_QUEUE

#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include "lingxin_voice_queue.h"

#define HEADER_SIZE sizeof(size_t)

static bool isSpaceAvailableInner(VoiceQueue *queue)
{
  return queue && (queue->capacity - queue->count) >= (queue->capacity * 0.3);
}

bool isVoiceQueueSpaceEnough(VoiceQueue *queue)
{
  pthread_mutex_lock(queue->mutex);
  bool result = isSpaceAvailableInner(queue);
  pthread_mutex_unlock(queue->mutex);
  return result;
}
// 初始化队列
VoiceQueue *voiceQueueCreate(size_t capacity)
{
  VoiceQueue *queue = (VoiceQueue *)calloc(1, sizeof(VoiceQueue));
  if (!queue)
  {
    logPrintf("voicequeue malloc failed");
    return NULL;
  }
  queue->buffer = calloc(1, capacity);
  if (!queue->buffer)
  {
    logPrintf("voicequeue buffer malloc failed");
    free(queue);
    return NULL;
  }
  queue->capacity = capacity;
  queue->count = 0;
  queue->front = 0;
  queue->rear = 0;
  queue->mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  if (!queue->mutex)
  {
    free(queue->buffer);
    free(queue);
    return NULL;
  }
  if (pthread_mutex_init(queue->mutex, NULL) != 0)
  {
    free(queue->buffer);
    free(queue->mutex);
    free(queue);
    return NULL;
  }
  queue->cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
  if (!queue->cond)
  {
    free(queue->buffer);
    free(queue->mutex);
    free(queue);
    return NULL;
  }
  if (pthread_cond_init(queue->cond, NULL) != 0)
  {
    free(queue->buffer);
    free(queue->mutex);
    free(queue->cond);
    free(queue);
    return NULL;
  }
  return queue;
}

// 销毁队列
void destroyVoiceQueue(VoiceQueue *queue)
{
  if (!queue)
  {
    return;
  }
  logPrintf("destroyVoiceQueue: begin");

  pthread_mutex_destroy(queue->mutex); // 销毁互斥锁
  pthread_cond_destroy(queue->cond);
  free(queue->mutex);
  free(queue->cond);
  free(queue->buffer);
  free(queue);
  logPrintf("destroyVoiceQueue: finish");
}

// 入队操作
bool voiceEnqueue(VoiceQueue *queue, const char *data, size_t length)
{
  if (!queue)
  {
    return false;
  }
  logPrintf("voiceEnqueue: begin: %d", length);

  pthread_mutex_lock(queue->mutex);

  size_t totalSize = HEADER_SIZE + length;
  if (queue->count == queue->capacity ||
      totalSize > (queue->capacity - queue->count))
  {
    pthread_mutex_unlock(queue->mutex);
    return false;
  }
  size_t spaceAtEnd = queue->capacity - queue->rear;
  if (spaceAtEnd >= totalSize)
  {
    memcpy((char *)queue->buffer + queue->rear, &length, HEADER_SIZE);
    queue->rear = (queue->rear + HEADER_SIZE) % queue->capacity;
    memcpy((char *)queue->buffer + queue->rear, data, length);
    queue->rear = (queue->rear + length) % queue->capacity;
  }
  else
  {
    if (spaceAtEnd >= HEADER_SIZE)
    {
      memcpy((char *)queue->buffer + queue->rear, &length, HEADER_SIZE);
      queue->rear = (queue->rear + HEADER_SIZE) % queue->capacity;
      size_t remainSpace = spaceAtEnd - HEADER_SIZE;
      memcpy((char *)queue->buffer + queue->rear, data, remainSpace);
      queue->rear = (queue->rear + remainSpace) % queue->capacity;
      memcpy(queue->buffer, (char *)data + remainSpace, length - remainSpace);
      queue->rear = (length - remainSpace) % queue->capacity;
    }
    else
    {
      size_t part1Size = spaceAtEnd;
      size_t part2Size = HEADER_SIZE - part1Size;
      memcpy((char *)queue->buffer + queue->rear, &length, part1Size);
      memcpy(queue->buffer, ((char *)&length) + part1Size, part2Size);
      queue->rear = part2Size;
      memcpy((char *)queue->buffer + queue->rear, data, length);
      queue->rear = (queue->rear + length) % queue->capacity;
    }
  }
  queue->count += totalSize;
  pthread_cond_signal(queue->cond);

  pthread_mutex_unlock(queue->mutex);
  logPrintf("voiceEnqueue: after");

  return true;
}

// 出队操作
bool voiceDequeue(VoiceQueue *queue, VoiceChunk *chunk,
                  ContinueWaitCheck continueWaitCheckFunc, void *userContext)
{
  if (!queue)
  {
    return false;
  }
  logPrintf("voiceDequeue: begin");

  pthread_mutex_lock(queue->mutex);
  while (queue && queue->count == 0)
  {
    logPrintf("voiceDequeue: wait");
    if (isSpaceAvailableInner(queue))
    {
      size_t space = queue->capacity - queue->count;
      if (!continueWaitCheckFunc(userContext, space))
      {
        pthread_mutex_unlock(queue->mutex);
        return false;
      }
    }

    // 设置超时时间为当前时间加上500毫秒
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_nsec += 500 * 1000 * 1000; // 500毫秒
    if (timeout.tv_nsec >= 1000 * 1000 * 1000)
    {
      timeout.tv_nsec -= 1000 * 1000 * 1000;
      timeout.tv_sec++;
    }

    int ret = pthread_cond_timedwait(queue->cond, queue->mutex, &timeout);
    if (ret == ETIMEDOUT)
    {
      logPrintf("voiceDequeue: timeout");
      continue;
    }
    else if (ret != 0)
    {
      logPrintf("voiceDequeue: pthread_cond_timedwait failed");
      pthread_mutex_unlock(queue->mutex);
      return false;
    }
    logPrintf("voiceDequeue: after wait");
  }

  size_t spaceAtEnd = queue->capacity - queue->front;
  if (spaceAtEnd >= HEADER_SIZE)
  {
    memcpy(&chunk->length, (char *)queue->buffer + queue->front, HEADER_SIZE);
    queue->front = (queue->front + HEADER_SIZE) % queue->capacity;
  }
  else
  {
    size_t part1Size = spaceAtEnd;
    size_t part2Size = HEADER_SIZE - part1Size;
    memcpy(&chunk->length, (char *)queue->buffer + queue->front, part1Size);
    memcpy(((char *)&chunk->length) + part1Size, queue->buffer, part2Size);
    queue->front = part2Size;
  }
  chunk->data = malloc(chunk->length);
  if (!chunk->data)
  {
    logPrintf("voiceDequeue: chunk->data fail malloc");
    pthread_mutex_unlock(queue->mutex);
    return false;
  }
  spaceAtEnd = queue->capacity - queue->front;
  if (spaceAtEnd >= chunk->length)
  {
    memcpy(chunk->data, (char *)queue->buffer + queue->front, chunk->length);
    queue->front = (queue->front + chunk->length) % queue->capacity;
  }
  else
  {
    memcpy(chunk->data, (char *)queue->buffer + queue->front, spaceAtEnd);
    memcpy((char *)chunk->data + spaceAtEnd, queue->buffer,
           chunk->length - spaceAtEnd);
    queue->front = (chunk->length - spaceAtEnd) % queue->capacity;
  }
  queue->count -= (HEADER_SIZE + chunk->length);
  pthread_mutex_unlock(queue->mutex);
  logPrintf("voiceDequeue: finish %d", queue->count);

  return true;
}

size_t getRemainSpaceOfVoiceQueue(VoiceQueue *queue)
{
  if (!queue)
  {
    return 0;
  }
  pthread_mutex_lock(queue->mutex);
  size_t result = queue->capacity - queue->count;
  pthread_mutex_unlock(queue->mutex);
  return result;
}

void clearVoiceQueue(VoiceQueue *queue)
{
  if (!queue)
  {
    return;
  }
  logPrintf("clearVoiceQueue begin");

  pthread_mutex_lock(queue->mutex);
  queue->count = 0;
  queue->front = 0;
  queue->rear = 0;
  pthread_mutex_unlock(queue->mutex);
  logPrintf("clearVoiceQueue finish");
}
#endif // LINGXI_USE_VOICE_QUEUE
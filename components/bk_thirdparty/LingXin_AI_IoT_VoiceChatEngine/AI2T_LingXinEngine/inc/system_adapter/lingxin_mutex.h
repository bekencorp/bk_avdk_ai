#ifndef __LINGXIN_MUTEX_H__
#define __LINGXIN_MUTEX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * 互斥锁句柄
 */
typedef void *lingxin_mutex_t;

/**
 * 创建互斥锁
 */
lingxin_mutex_t lingxin_mutex_create();

/**
 * 互斥锁 Lock
 * @param mutex 互斥锁句柄
 */
void lingxin_mutex_lock(lingxin_mutex_t mutex);

/**
 * 互斥锁 Unlock
 * @param mutex 互斥锁句柄
 */
void lingxin_mutex_unlock(lingxin_mutex_t mutex);

/**
 * 销毁互斥锁
 * @param mutex 互斥锁句柄
 */
void lingxin_mutex_destroy(lingxin_mutex_t mutex);


#ifdef __cplusplus
}
#endif

#endif /* __LINGXIN_MUTEX_H__ */
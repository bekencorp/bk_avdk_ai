#include <stdlib.h>
#include <string.h>
#include "os/os.h"
#include "os/mem.h"
#include "lingxin_mutex.h"

/**
 * 创建互斥锁
 */
lingxin_mutex_t lingxin_mutex_create()
{
    beken_mutex_t *mutex = os_malloc(sizeof(beken_mutex_t));
    if (mutex == NULL) {
        return NULL;
    }
    rtos_init_mutex(mutex);
    return (lingxin_mutex_t)mutex;
}

/**
 * 互斥锁 Lock
 * @param mutex 互斥锁句柄
 */
void lingxin_mutex_lock(lingxin_mutex_t mutex)
{
    rtos_lock_mutex((beken_mutex_t *)mutex);
}

/**
 * 互斥锁 Unlock
 * @param mutex 互斥锁句柄
 */
void lingxin_mutex_unlock(lingxin_mutex_t mutex)
{
    rtos_unlock_mutex((beken_mutex_t *)mutex);
}

/**
 * 销毁互斥锁
 * @param mutex 互斥锁句柄
 */
void lingxin_mutex_destroy(lingxin_mutex_t mutex)
{
    rtos_deinit_mutex((beken_mutex_t *)mutex);
    os_free((void *)mutex);
}
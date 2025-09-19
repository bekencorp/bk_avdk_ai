#include "os/os.h"
#include "os/mem.h"
#include "lingxin_semaphore.h"
#define TAG "bk_sema"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
/**
 * 创建信号量
 * @param cnt 信号量初始值
 */
lingxin_semaphore_t lingxin_semaphore_create(uint32_t cnt)
{
    int err;
    beken_semaphore_t *semaphore = (beken_semaphore_t *)os_malloc(sizeof(beken_semaphore_t));
    if (semaphore == NULL) {
		LOGE("%s, line:%d sema null\r\n", __func__, __LINE__);
        return NULL;
    }

    if ((err = rtos_init_semaphore(semaphore, 1)) != kNoErr) { // rtos_init_semaphore defaults uxInitialCount to 0, only need to configure the maximum count value
		LOGE("%s, line:%d init_sema fail %d\r\n", __func__, __LINE__, err);
        os_free(semaphore);
        return NULL;
    }
    
    return (lingxin_semaphore_t)semaphore;
}

/**
 * 等待信号量
 * @param sem 信号量句柄
 * @param timeout_ms 等待超时时间，单位为毫秒
 */
void lingxin_semaphore_pend(lingxin_semaphore_t sem, uint32_t timeout_ms)
{
    int err;
    if (sem == NULL) {
		LOGE("%s, line:%d sema null\r\n", __func__, __LINE__);
        return;
    }
    if ((err = rtos_get_semaphore((beken_semaphore_t *)sem, timeout_ms)) != kNoErr) {
		if (timeout_ms)
			LOGE("%s, line:%d get_sema fail %d timeout_ms:%d\r\n", __func__, __LINE__, err, timeout_ms);
    }
}

/**
 * 发送信号量
 * @param sem 信号量句柄
 */
void lingxin_semaphore_post(lingxin_semaphore_t sem)
{
    int err;
    if (sem == NULL) {
		LOGE("%s, line:%d sema null\r\n", __func__, __LINE__);
        return;
    }
    if ((err = rtos_set_semaphore((beken_semaphore_t *)sem)) != kNoErr) {
		LOGE("%s, line:%d post_sema fail %d\r\n", __func__, __LINE__, err);
    }
}

/**
 * 给信号量设值
 * @param sem 信号量句柄
 */
void lingxin_semaphore_set_value(lingxin_semaphore_t sem, uint32_t cnt)
{
    if (sem == NULL) {
		LOGE("%s, line:%d sema null\r\n", __func__, __LINE__);
        return;
    }
}

/**
 * 销毁信号量
 * @param sem 信号量句柄
 */
void lingxin_semaphore_destroy(lingxin_semaphore_t sem)
{
    int err;
    if (sem == NULL) {
		LOGE("%s, line:%d sema null\r\n", __func__, __LINE__);
        return;
    }
    if ((err = rtos_deinit_semaphore((beken_semaphore_t *)sem)) != kNoErr) {
		LOGE("%s, line:%d deinit_sema fail %d\r\n", __func__, __LINE__, err);
    }
    os_free((void *)sem);
}

static beken_semaphore_t ws_sem;

/**
 * websocket控制删除
 */
void lingxin_websocket_control_del(void)
{
    int ret = BK_OK;
    LOGE("%s, line:%d\r\n", __func__, __LINE__);
    ret = rtos_deinit_semaphore(&ws_sem);
    if (ret != BK_OK) {
        LOGE("%s, deinit ws_sem failed %d\r\n", __func__, ret);
    }
}

/**
 * websocket控制创建
 */
void lingxin_websocket_control_create(void)
{
    int ret = BK_OK;
    LOGI("%s, line:%d\r\n", __func__, __LINE__);
    ret = rtos_init_semaphore(&ws_sem, 1);
    if (ret != BK_OK) {
        LOGE("%s, init ws_sem failed %d\r\n", __func__, ret);
    }
}

/**
 * 解锁websocket控制
 */
void lingxin_unlock_write_websocket_controle(void)
{
    int ret = BK_OK;
    LOGE("%s, line:%d\r\n", __func__, __LINE__);
    ret = rtos_set_semaphore(&ws_sem);
    if (ret != BK_OK) {
        LOGE("%s, set ws_sem failed %d\r\n", __func__, ret);
    }
}

/**
 * 加锁websocket控制
 */
void lingxin_lock_write_websocket_control(void)
{
    int ret = BK_OK;
    LOGE("%s, line:%d\r\n", __func__, __LINE__);
    ret = rtos_get_semaphore(&ws_sem, BEKEN_NEVER_TIMEOUT);
    if (ret != BK_OK) {
        LOGE("%s, get ws_sem failed %d\r\n", __func__, ret);
    }
}
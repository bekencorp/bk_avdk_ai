#include "os/os.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <lingxin_thread.h>
#define TAG "lx_thread"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

/**
 * 创建线程
 * @param thread 指向存储新线程 ID 的变量的指针
 * @param param 指向线程参数结构体的指针
 * @param start_routine 线程启动时执行的函数指针
 * @param args 传递给线程启动函数的参数
 * @return 操作结果的状态码
 */
int lingxin_thread_create(lingxin_tid_t *thread, const lingxin_thread_param_t *param, void (*start_routine)(void *), void *args)
{
    beken_thread_t bk_thread;
    bk_err_t ret;

    if (thread == NULL || param == NULL || start_routine == NULL) {
        return BK_FAIL;
    }

    if (param->priority < 0 || param->stack_size <= 0) {
        return BK_FAIL;
    }

    char safe_name[LINXIN_THREAD_NAME_MAX_LENGTH];
    strncpy(safe_name, param->name, LINXIN_THREAD_NAME_MAX_LENGTH - 1);
    safe_name[LINXIN_THREAD_NAME_MAX_LENGTH - 1] = '\0';

    beken_thread_function_t bk_start_routine = (beken_thread_function_t)start_routine;

    ret = rtos_create_thread(&bk_thread, (uint8_t)param->priority, safe_name, bk_start_routine, (uint32_t)param->stack_size, args);

    if (ret != kNoErr) {
		LOGE("%s (line:%d) fail\r\n", __func__, __LINE__);
        return BK_FAIL;
    }

    *thread = (lingxin_tid_t)bk_thread;
    return BK_OK;
}

/**
 * 销毁线程
 * @param thread 线程 ID
 * @param mode 销毁模式
 * @return 操作结果的状态码
 */
void lingxin_thread_destroy(lingxin_tid_t thread, lingxin_thread_destroy_mode_t mode)
{
    beken_thread_t bk_thread;
    bk_err_t ret;
    bool is_current_task;
    bk_thread = (beken_thread_t)thread;
    if (bk_thread == NULL) {
        return;
    }
    if (rtos_get_current_thread() == bk_thread) {
        is_current_task = 1;
     } else {
        is_current_task = 0;
    }
    LOGI("%s (line:%d) thread:%p mode:%d is_current_task:%d\r\n", __func__, __LINE__, thread, mode, is_current_task);
    switch (mode) {
        case LINGXIN_THREAD_DESTROY_WAIT:
            // 唤醒等待
            ret = rtos_thread_force_awake(&bk_thread);
            if (ret != kNoErr) {
                LOGE("%s (line:%d) force_awake fail, ret=%d\r\n", __func__, __LINE__, ret);
            }
            // 等待任务退出
            ret = rtos_thread_join(&bk_thread);
            if (ret != kNoErr) {
                LOGE("%s (line:%d) join fail, ret=%d\r\n", __func__, __LINE__, ret);
            }

            if (is_current_task) {
                ret = rtos_delete_thread(NULL);
            } else {
                ret = rtos_delete_thread(&bk_thread);
            }
            if (ret != kNoErr) {
                LOGE("%s (line:%d) delete fail, ret=%d\r\n", __func__, __LINE__, ret);
            }
            break;
        case LINGXIN_THREAD_DESTROY_DETACH:
        case LINGXIN_THREAD_DESTROY_CANCEL:
        default:
            if (rtos_get_current_thread() == bk_thread) {
                ret = rtos_delete_thread(NULL);
            } else {
                ret = rtos_delete_thread(&bk_thread);
            }
            if (ret != kNoErr) {
                LOGE("%s (line:%d) delete fail, ret=%d\r\n", __func__, __LINE__, ret);
            }
            break;
    }
    return;
}

/**
 * 线程休眠
 * @param time 休眠时间，单位为毫秒
 * @return 操作结果的状态码
 */
void lingxin_thread_sleep(int time)
{
    if (time < 0) {
        return;
    }

    if (time == 0) {
        return;
    }

    rtos_delay_milliseconds((uint32_t)time);
}


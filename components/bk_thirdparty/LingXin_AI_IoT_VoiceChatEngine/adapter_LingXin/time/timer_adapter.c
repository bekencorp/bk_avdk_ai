#include <os/os.h>
#include <os/mem.h>
#include <stdlib.h>
#include <stdint.h>
#include <time/time.h>
#include "lingxin_time_task.h"
#include "lingxin_timer.h"
#include "lingxin_system_time.h"
#include <driver/aon_rtc.h>
#include <components/system.h>
#define TAG "bk_timer"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
//获取设备当前时间
void get_current_time(LINGXIN_TIME *lingxin_time)
{
    struct tm current_time = {0};
    datetime_get(&current_time);
    struct timeval tv;
    bk_rtc_gettimeofday(&tv, 0);
    long ms_time = (tv.tv_usec / 1000);
    lingxin_time->mill_sec = ms_time;
    lingxin_time->sec = current_time.tm_sec;
    lingxin_time->min = current_time.tm_min;
    lingxin_time->hour = current_time.tm_hour;
    lingxin_time->day = current_time.tm_mday;
    lingxin_time->mon = current_time.tm_mon + 1;
    lingxin_time->year = current_time.tm_year + 1900; // tm_year 是从1900年开始的年数
}

//定时器：后台只存在一个定时器，如果已经有，则不允许重复创建
/** 
 * 创建定时器
 * 如果已经存在定时器，则返回 false
 * 如果不存在定时器，则创建一个10s定时器，返回 true
*/
beken_timer_t lingxin_timer;
bool init_lingxin_chat_timer(void *priv, void (*callback)(void *)) //实际传NULL
{
    if (priv != NULL) {
        return false;
    }

    bk_err_t err;
	LOGE("%s\r\n", __func__);
    err = rtos_init_timer(&lingxin_timer, 10000, (timer_handler_t)callback, NULL);
    BK_ASSERT(kNoErr == err);
    err = rtos_start_timer(&lingxin_timer);
    BK_ASSERT(kNoErr == err);    
    return true;
}

/** 
 * 删除定时器
 * 如果定时器存在，则删除定时器，返回 true
 * 如果定时器不存在，则返回 false
*/
bool delete_lingxin_chat_timer(void *priv) //实际传NULL，先不判断priv值
{
    bk_err_t err;

    LOGE("%s\r\n", __func__);
    if (rtos_is_timer_running(&lingxin_timer))
    {
        err = rtos_stop_timer(&lingxin_timer);
    }
    err = rtos_deinit_timer(&lingxin_timer);
    BK_ASSERT(kNoErr == err);
    return true;
}

/** 
 * 重置定时器时间
 * 如果定时器存在，则重置定时器时间，返回 true
 * 如果定时器不存在，则返回 false
*/
bool reset_lingxin_chat_timer_run(void *priv) //实际传NULL，先不判断priv值
{
    bk_err_t err;

    LOGE("%s\r\n", __func__);
	if (lingxin_timer.handle) {
       err = rtos_reload_timer(&lingxin_timer);
       LOGE("%s err:%d\r\n", __func__, err);
	   return true;
	}
    return false;
}

// 功能：创建一个一次性软件定时器, 并将定时器指定到特定线程
// 调用时机：在同步最新的定时任务列表时调用
u16 create_schedule_timer(void *priv, void (*func)(void *), u32 countdown)
{
    // 将定时器指定到已存在线程 or 创建一个新线程进行定时器创建
    // 其中 priv 是 func 的回调参数
    // countdown 为定时器的倒计时时长，单位为 ms
    // 该方法返回一个唯一的 timerId，作为当前定时器的唯一标识符，用于后续清除定时器

    beken2_timer_t *timer = (beken2_timer_t *)os_malloc(sizeof(beken2_timer_t));
    if (timer == NULL) {
        return 0;
    }

    bk_err_t err = rtos_init_oneshot_timer(timer, countdown, (timer_2handler_t)func, priv, NULL);
    if (err != kNoErr) {
        os_free(timer);
        return 0;
    }

    err = rtos_start_oneshot_timer(timer);
    if (err != kNoErr) {
        rtos_deinit_oneshot_timer(timer);
        os_free(timer);
        return 0;
    }

    return (u16)(uintptr_t)timer;
}

// 功能：与定时器的创建成对使用，用于删除指定定时器
// 调用时机：在同步最新定时任务列表时调用
void delete_schedule_timer(u16 timerId)
{
    if (timerId == 0) {
        return;
    }

    beken2_timer_t *timer = (beken2_timer_t *)(uintptr_t)timerId;

    if (rtos_is_oneshot_timer_running(timer)) {
        rtos_stop_oneshot_timer(timer);
    }

    rtos_deinit_oneshot_timer(timer);
    os_free(timer);
}

/**
*
* @brief 获取系统的滴答秒（系统启动以来经过的秒）
*/
u32 get_sys_time_sec(void)
{
    return bk_get_second();
}


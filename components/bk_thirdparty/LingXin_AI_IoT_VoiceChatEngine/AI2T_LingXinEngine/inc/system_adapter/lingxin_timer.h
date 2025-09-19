#ifndef LINGXIN_TIMER_H
#define LINGXIN_TIMER_H
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>
//typedef uint16_t u16;
//typedef uint32_t u32;
#include <common/bk_typedef.h>
#define INVALID_TIMER_ID ((u16) 0)

    /**
    * @brief 在指定线程创建一个一次性定时器（不能创建在当前线程）
    * @param priv 定时器回调函数func的私有参数
    * @param func 定时器回调函数
    * @param countdown 超时时间， 单位：毫秒
    * @return 定时器分配的id号
    * @note 创建一个新线程或者指定非当前线程，执行定时任务
    */
    u16 create_schedule_timer(void *priv, void (*func)(void *priv), u32 countdown);

    /**
    * @brief 删除指定的一次性定时器
    * @param timerId 定时器ID
    */
    void delete_schedule_timer(u16 timerId);

    /**
    *
    * @brief 获取系统的滴答秒（系统启动以来经过的秒）
    */
    u32 get_sys_time_sec(void);

#ifdef __cplusplus
}
#endif
#endif
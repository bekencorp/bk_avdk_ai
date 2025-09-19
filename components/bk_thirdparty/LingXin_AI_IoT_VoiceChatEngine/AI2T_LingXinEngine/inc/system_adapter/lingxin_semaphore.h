#ifndef __LINGXIN_SEMAPHORE_H__
#define __LINGXIN_SEMAPHORE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * 信号量句柄
 */
typedef void *lingxin_semaphore_t;

/**
 * 创建信号量
 * @param cnt 信号量初始值
 */
lingxin_semaphore_t lingxin_semaphore_create(uint32_t cnt);

/**
 * 等待信号量
 * @param sem 信号量句柄
 * @param timeout_ms 等待超时时间，单位为毫秒
 */
void lingxin_semaphore_pend(lingxin_semaphore_t sem, uint32_t timeout_ms);

/**
 * 发送信号量
 * @param sem 信号量句柄
 */
void lingxin_semaphore_post(lingxin_semaphore_t sem);

/**
 * 给信号量设值
 * @param sem 信号量句柄
 */
void lingxin_semaphore_set_value(lingxin_semaphore_t sem, uint32_t cnt);

/**
 * 销毁信号量
 * @param sem 信号量句柄
 */
void lingxin_semaphore_destroy(lingxin_semaphore_t sem);



// 单例模式

/**
 * websocket控制:由内核调用，适配方只需要按照功能实现即可
 * 使用方法：
 *          用于控制服务端数据推送，当执行 lock 之后，服务端无法再发送webSocket指令回来
 *          执行 unlock 之后服务端即可发送webSocket指令
 * 使用场景：
 *          控制数据，在首次初始化流式播放时候，由于初始化线程可能稍微比较耗时，需要在流式播放模块初始化结束之后，才可以让服务端发送mp3数据
 */

// websocket控制删除
void lingxin_websocket_control_del();

// websocket控制创建
void lingxin_websocket_control_create();

// 解锁
void lingxin_unlock_write_websocket_controle();

// 加锁
void lingxin_lock_write_websocket_control();


#ifdef __cplusplus
}
#endif

#endif /* __LINGXIN_SEMAPHORE_H__ */
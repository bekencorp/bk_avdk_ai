#ifndef __LINGXIN_THREAD_H__
#define __LINGXIN_THREAD_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 线程名称最大长度
 */
#define LINXIN_THREAD_NAME_MAX_LENGTH 16

/**
 * 线程ID
 */
typedef int lingxin_tid_t;

/**
 * 定义线程参数的结构体
 */
typedef struct {
    // 线程的名称，最大长度由 LINXIN_THREAD_NAME_MAX_LENGTH 定义
    char name[LINXIN_THREAD_NAME_MAX_LENGTH];
    // 线程的优先级
    int  priority;
    // 线程的栈大小
    int  stack_size;
} lingxin_thread_param_t;

/**
 * 创建线程
 * @param thread 指向存储新线程 ID 的变量的指针
 * @param param 指向线程参数结构体的指针
 * @param start_routine 线程启动时执行的函数指针
 * @param args 传递给线程启动函数的参数
 * @return 操作结果的状态码
 */
int lingxin_thread_create(lingxin_tid_t *thread, const lingxin_thread_param_t *param, void (*start_routine)(void *), void *args);

/**
 * 线程销毁模式枚举
 */
typedef enum {
    LINGXIN_THREAD_DESTROY_WAIT = 0,    // 等待线程结束
    LINGXIN_THREAD_DESTROY_DETACH = 1,  // 分离线程，线程运行结束时自动销毁
    LINGXIN_THREAD_DESTROY_CANCEL = 2,  // 立即取消线程
} lingxin_thread_destroy_mode_t;

/**
 * 销毁线程
 * @param thread 线程 ID
 * @param mode 销毁模式
 */
void lingxin_thread_destroy(lingxin_tid_t thread, lingxin_thread_destroy_mode_t mode);

/**
 * 线程休眠
 * @param time 休眠时间，单位为毫秒
 */
void lingxin_thread_sleep(int time);

#ifdef __cplusplus
}
#endif

#endif /* __LINGXIN_THREAD_H__ */
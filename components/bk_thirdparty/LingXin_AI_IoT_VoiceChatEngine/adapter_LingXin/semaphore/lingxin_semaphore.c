#include "os/os.h"
#include "lingxin_semaphore.h"
#include "chat_module_config.h"

#ifdef CONFIG_SEMAPHORE_DEFAULT_ENABLE


/**
 * 信号量控制
 * 使用方法：
 *          用于控制服务端数据推送，当执行 pend 之后，服务端无法再发送webSocket指令回来
 *          执行 post 之后服务端即可发送webSocket指令
 * 使用场景：
 *          控制数据，在首次初始化流式播放时候，由于初始化线程可能稍微比较耗时，需要在流式播放模块初始化结束之后，才可以让服务端发送mp3数据
 */

beken_semaphore_t play_sem;    
void lingxin_semaphore_create()
{
    rtos_init_semaphore(&play_sem, 1);
// 在次创建信号量，由内核调用，会在内核第一次唤醒的时候调用
}


void lingxin_pend_write_semaphore()
{
// 在服务端发送 AI_Statr指令的时候，由内核调用，加锁，控制webSocket数据传
//输 
	rtos_get_semaphore(&play_sem, BEKEN_WAIT_FOREVER);
}

void lingxin_post_write_semaphore()
{
// 在流式播放模块初始化结束之后，由内核调用，解锁，开始webSocket数据传输
	rtos_set_semaphore(&play_sem);
}

void lingxin_semaphore_del()
{
// 在对话完全结束之后，由内核调用，删除当前信号量
	rtos_deinit_semaphore(&play_sem);
}
#endif
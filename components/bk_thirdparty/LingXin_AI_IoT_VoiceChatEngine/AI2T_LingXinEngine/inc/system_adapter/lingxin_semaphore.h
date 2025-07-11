/**
 * 信号量控制:由内核调用，适配方只需要按照功能实现即可
 * 使用方法：
 *          用于控制服务端数据推送，当执行 pend 之后，服务端无法再发送webSocket指令回来
 *          执行 post 之后服务端即可发送webSocket指令
 * 使用场景：
 *          控制数据，在首次初始化流式播放时候，由于初始化线程可能稍微比较耗时，需要在流式播放模块初始化结束之后，才可以让服务端发送mp3数据
 */

// 信号量删除
void lingxin_semaphore_del();

// 信号量创建
void lingxin_semaphore_create();

// 解锁
void lingxin_post_write_semaphore();

// 加锁
void lingxin_pend_write_semaphore();
#ifndef __LINGXIN_RECORDER_MANAGER_H__
#define __LINGXIN_RECORDER_MANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 初始化录音模块
 * @param custom_send_uni_size 录音单次发送的大小
 * @param custom_send_cbuf_scale 录音缓冲区相对于录音单次发送的大小比例
 */
int module_record_manager_init(int custom_send_uni_size, int custom_send_cbuf_scale);
/**
 * 开启录音
 */
int module_record_init();
/**
 * 录音可以开始发送
 */
void module_record_start_send();
/**
 * 录音结束
 * @param wait_send_left 是否等待发送剩余数据
 */
void module_record_stop(int wait_send_left);
/**
 * 录音被打断
 */
void module_record_terminate();

#ifdef __cplusplus
}
#endif

#endif /* __LINGXIN_RECORDER_MANAGER_H__ */
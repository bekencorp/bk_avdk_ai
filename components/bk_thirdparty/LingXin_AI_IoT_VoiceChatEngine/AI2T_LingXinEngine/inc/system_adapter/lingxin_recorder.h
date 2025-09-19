#ifndef __LINGXIN_RECORDER_H__
#define __LINGXIN_RECORDER_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 录音器句柄
 */
typedef void *lingxin_recorder_t;

/**
 * 录音器开启/关闭回调函数
 * @param result 开启/关闭结果。0为开启/关闭成功；-1为开启/关闭失败
 */
typedef void (*lingxin_recorder_callback_t)(int result);


/******************** 由chat套件内核发起调用，客户实现 ********************/
/**
 * 创建录音
 */
lingxin_recorder_t lingxin_recorder_create();

/**
 * 开启录音参数的结构体
 */
typedef struct {
  // 录音单次发送的大小
  int frame_size; 
} lingxin_recorder_open_param_t;

/**
 * 开启录音
 * @param recorder 录音句柄
 * @param param 开启录音参数的结构体的指针
 * @param callback 录音开启的回调(参数result: 0-成功/-1-失败)
 */
void lingxin_recorder_open(lingxin_recorder_t recorder, lingxin_recorder_open_param_t *param, lingxin_recorder_callback_t callback);

/**
 * 关闭录音
 * @param recorder 录音句柄
 * @param callback 录音关闭的回调(参数result: 0-成功/-1-失败)
 */
void lingxin_recorder_close(lingxin_recorder_t recorder, lingxin_recorder_callback_t callback);

/**
 * 销毁录音
 * @param recorder 录音句柄
 */
void lingxin_recorder_destroy(lingxin_recorder_t recorder);

/**
 * 获取默认每毫秒的录音大小
 * @return 每毫秒的录音大小
 */
int lingxin_recorder_get_size_per_ms();


/******************** 由chat套件内核实现，客户调用 ********************/
/**
 * 发送录音数据
 * @param data 录音数据指针
 * @param len 录音数据长度
 */
void lingxin_process_record_data(void *data, int len);

#ifdef __cplusplus
}
#endif

#endif // __LINGXIN_RECORDER_H__
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <common/bk_typedef.h>
#include <lingxin_recorder.h>
#include <audio_transfer.h>

#define TAG "lx_record"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef struct {
    int is_open;
    lingxin_recorder_callback_t open_callback;
    lingxin_recorder_callback_t close_callback;
} lingxin_recorder_handle_t;

void *recorder_handle = NULL;

int bk_recorder_data_send(unsigned char *data_ptr, unsigned int data_len)
{
    lingxin_recorder_handle_t *handle = recorder_handle;
    if (handle && handle->is_open) {
        lingxin_process_record_data(data_ptr, data_len);
        LOGD("lingxin TX audio, data_ptr=%p, data_len=%d\r\n", data_ptr, (int)data_len);
        return data_len;
    } else {
        return 0;
    }
}

/******************** 由chat套件内核发起调用，客户实现 ********************/

/**
 * 创建录音
 */
lingxin_recorder_t lingxin_recorder_create()
{
    LOGI("%s\n", __func__);

    lingxin_recorder_handle_t *handle = (lingxin_recorder_handle_t *)os_zalloc(sizeof(lingxin_recorder_handle_t));
    if (handle == NULL) {
        LOGE("Failed to allocate memory for recorder handle\n");
        return NULL;
    }

    handle->is_open = 0;
    handle->open_callback = NULL;
    handle->close_callback = NULL;

    audio_tras_register_tx_data_func(bk_recorder_data_send);
    recorder_handle = handle;
    return (lingxin_recorder_t)handle;
}

/**
 * 开启录音
 * @param recorder 录音句柄
 * @param param 开启录音参数的结构体的指针
 * @param callback 录音开启的回调(参数result: 0-成功/-1-失败)
 */
void lingxin_recorder_open(lingxin_recorder_t recorder, lingxin_recorder_open_param_t *param, lingxin_recorder_callback_t callback)
{
    LOGI("%s\n", __func__);

    lingxin_recorder_handle_t *handle = (lingxin_recorder_handle_t *)recorder;
    if (handle == NULL) {
        LOGE("Invalid recorder handle\n");
        if (callback) {
            callback(-1);
        }
        return;
    }

    handle->open_callback = callback;
    if (handle->open_callback) {
        handle->open_callback(0);
    }
    handle->is_open = 1;
}

/**
 * 关闭录音
 * @param recorder 录音句柄
 * @param callback 录音关闭的回调(参数result: 0-成功/-1-失败)
 */
void lingxin_recorder_close(lingxin_recorder_t recorder, lingxin_recorder_callback_t callback)
{
    LOGI("%s\n", __func__);

    lingxin_recorder_handle_t *handle = (lingxin_recorder_handle_t *)recorder;
    if (handle == NULL) {
        LOGE("Invalid recorder handle\n");
        if (callback) {
            callback(-1);
        }
        return;
    }
    handle->is_open = 0;
    handle->close_callback = callback;
    if (handle->close_callback) {
        handle->close_callback(0);
    }
}

/**
 * 销毁录音
 * @param recorder 录音句柄
 */
void lingxin_recorder_destroy(lingxin_recorder_t recorder)
{
    LOGI("%s\n", __func__);

    lingxin_recorder_handle_t *handle = (lingxin_recorder_handle_t *)recorder;
    if (handle == NULL) {
        LOGE("Invalid recorder handle\n");
        return;
    }
    os_free(handle);
    recorder_handle = NULL;
}

/**
 * 获取默认每毫秒的录音大小
 * @return 每毫秒的录音大小
 */
int lingxin_recorder_get_size_per_ms()
{
    return (640/20);
}



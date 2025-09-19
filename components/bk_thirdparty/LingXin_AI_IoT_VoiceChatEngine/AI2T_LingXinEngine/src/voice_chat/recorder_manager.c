#include <stdlib.h>
#include "lingxin_cbuffer.h"
#include "lingxin_mutex.h"
#include "lingxin_semaphore.h"
#include "lingxin_thread.h"
#include "chat_state_machine.h"
#include "lingxin_log.h"
#include "os/mem.h"
static lingxin_mutex_t send_thread_mutex = NULL;

static lingxin_recorder_t *lingxin_recorder = NULL;
static int send_flag = 0;
static LingxinCircularBuffer* send_cbuf = NULL;
static lingxin_semaphore_t send_r_sem = NULL;
static int send_thread_running = 0;
static lingxin_tid_t send_thread_pid = 0;
static int send_thread_stop_flag = 0; // 0: 不退出 1: 立刻退出 2: 等待发完再退出

static int send_uni_size = 0;
static int send_cbuf_scale = 200;

typedef void (*SendThreadStopCallback)();
//static SendThreadStopCallback send_finish_callback = NULL; 注释掉未使用的定义

static int inner_record_init();
static void record_open_callback(int result);
static void record_close_callback_for_start(int result);
static void record_close_callback_for_terminate(int result);
static void record_close_callback_for_stop(int result);
static void record_close_callback_for_stop_wait_send_left(int result);

static void send_record(void *arg);
static int stop_send_thread(int wait_send);
static int start_send_thread();
static int get_frame_size();
/****************** 录音模块初始化 ******************/
int module_record_manager_init(int custom_send_uni_size, int custom_send_cbuf_scale) {
    send_thread_mutex = lingxin_mutex_create();
    if (custom_send_uni_size) {
        send_uni_size = custom_send_uni_size;
    }
    if (custom_send_cbuf_scale) {
        send_cbuf_scale = custom_send_cbuf_scale;
    }
	return 0;
}

/****************** 录音模块开启 ******************/
int module_record_init() {
    lingxin_log_debug("%s 来自于状态机的调用", __func__);

    // [开启录音-1]关闭可能存在的录音
    if (lingxin_recorder == NULL) {
        record_close_callback_for_start(0);
    } else {
        lingxin_recorder_close(lingxin_recorder, record_close_callback_for_start);
    }
    return 0;
}
static void record_close_callback_for_start(int result) {
    if (result == 0) {
        if (lingxin_recorder != NULL) {
            lingxin_recorder_destroy(lingxin_recorder);
            lingxin_recorder = NULL;
            lingxin_log_debug("destroy recorder success");
        }
        // [开启录音-2]退出可能存在的录音发送线程
        int res = stop_send_thread(0);
        if (res == 0) {
            // [开启录音-3]初始化新的录音
            inner_record_init();
        } else {
            lingxin_log_error("Fail to stop the existing send thread.");
        }
    } else {
        lingxin_log_error("Fail to close the existing recorder.");
    }
}
static int inner_record_init() {
    // [开启录音-3.1]确定本次录音单次发送的大小
    send_uni_size = get_frame_size();
    lingxin_log_debug("The size of each send is %d bytes.", send_uni_size);

    // [开启录音-3.2]开启新的录音发送线程
    int res = start_send_thread();
    if (res == 0) {
        // [开启录音-3.3]开启录音
        lingxin_recorder = lingxin_recorder_create();
        lingxin_recorder_open_param_t props = {
            .frame_size = send_uni_size,
        };
        lingxin_recorder_open(lingxin_recorder, &props, record_open_callback);
    } else {
        lingxin_log_error("Fail to start the new send thread.");
    }
	return 0;
}
static void record_open_callback(int result) { 
    if (result == 0) {
        // [开启录音-4]通知状态机录音模块启动已完成
        state_machine_run_event(State_Event_Record_Ready);
    } else {
        lingxin_log_error("Fail to open recorder.");
    }
}


/****************** 录音开始发送 ******************/
void module_record_start_send() {
    lingxin_log_debug("%s", __func__);
    if (send_thread_running) {
        send_flag = 1; 
        if (send_thread_stop_flag == 2) {
            // 录音已经停止，需要通知发送线程发送剩余内容
            lingxin_semaphore_set_value(send_r_sem, 0);
            lingxin_semaphore_post(send_r_sem);
        }
    } else {
        lingxin_log_warn("Send thread is not running.");
    }
    
}

/****************** 录音模块打断 ******************/
void module_record_terminate() {
    lingxin_log_debug("%s 来自于状态机的调用", __func__);
    // [打断录音-1]关闭可能存在的录音
    if (lingxin_recorder == NULL) {
        record_close_callback_for_terminate(0);
    } else {
        lingxin_recorder_close(lingxin_recorder, record_close_callback_for_terminate);
    }
}
static void record_close_callback_for_terminate(int result) {
    if (result == 0) {
        if (lingxin_recorder != NULL) {
            lingxin_recorder_destroy(lingxin_recorder);
            lingxin_recorder = NULL;
            lingxin_log_debug("destroy recorder success");
        }
        // [打断录音-2]退出可能存在的录音发送线程
        int res = stop_send_thread(0);
        if (res == 0) {
            // [打断录音-3]通知状态机录音模块打断已完成
            state_machine_run_event(State_Event_Record_TerminateEnd);
        } else {
            lingxin_log_error("Fail to stop the existing send thread.");
        }
    } else {
        lingxin_log_error("Fail to close the existing recorder.");
    }
}

/****************** 录音模块正常结束 ******************/
void module_record_stop(int wait_send_left) {
    lingxin_log_debug("%s 来自于状态机的调用", __func__);
    // [正常结束录音-1]关闭可能存在的录音
    lingxin_recorder_callback_t callback = wait_send_left 
        ? record_close_callback_for_stop_wait_send_left 
        : record_close_callback_for_stop;
    if (lingxin_recorder == NULL) {
        callback(0);
    } else {
        lingxin_recorder_close(lingxin_recorder, callback);
    }
}
static void record_close_callback_for_stop(int result) {
    if (result == 0) {
        if (lingxin_recorder != NULL) {
            lingxin_recorder_destroy(lingxin_recorder);
            lingxin_recorder = NULL;
            lingxin_log_debug("destroy recorder success");
        }
        // [正常结束录音-2]退出可能存在的录音发送线程
        int res = stop_send_thread(0);
        if (res == 0) {
            // [正常结束录音-3]通知状态机录音模块正常结束已完成
            state_machine_run_event(State_Event_Record_Stop);
        } else {
            lingxin_log_error("Fail to stop the existing send thread.");
        }
    } else {
        lingxin_log_error("Fail to close the existing recorder.");
    }
}
static void record_close_callback_for_stop_wait_send_left(int result) {
    if (result == 0) {
        if (lingxin_recorder != NULL) {
            lingxin_recorder_destroy(lingxin_recorder);
            lingxin_recorder = NULL;
            lingxin_log_debug("destroy recorder success");
        }
        // [正常结束录音-2]退出可能存在的录音发送线程，但等待发完当前内容
        int res = stop_send_thread(1);
        if (res == 0) {
            // [正常结束录音-3]通知状态机录音模块正常结束已完成
            // state_machine_run_event(State_Event_Record_Stop);
        } else {
            lingxin_log_error("Fail to stop the existing send thread.");
        }
    } else {
        lingxin_log_error("Fail to close the existing recorder.");
    }
}


/****************** 发送线程（读缓存） ******************/
static int start_send_thread() {
    lingxin_mutex_lock(send_thread_mutex);

    // 0. 检查发送线程是否已经运行
    if (send_thread_running) {
        lingxin_log_error("Send thread is already running.");
        goto start_send_thread_err;
    }
    // 1. 初始化录音发送缓冲区
    send_cbuf = lingxin_cbuffer_init(send_cbuf_scale, send_uni_size);
    if (send_cbuf == NULL) {
        lingxin_log_error("Error: [start_send_thread]录音发送缓冲区初始化失败");
        goto start_send_thread_err;
    }
    // 2. 初始化信号量
    send_r_sem = lingxin_semaphore_create(0);
    if (send_r_sem == NULL) {
        lingxin_log_error("Error: [start_send_thread]录音发送信号量初始化失败");
        goto start_send_thread_err;
    }
    // 3. 重置flag
    send_flag = 0;
    send_thread_stop_flag = 0;
    // 4. 创建录音发送线程
    lingxin_thread_param_t thread_param = {
        .priority = 16,
        .stack_size = 4096*2,
        .name = "send_record",
    };
    int ret = lingxin_thread_create(&send_thread_pid, &thread_param, send_record, NULL);
    if (ret == 0) {  // 检查线程创建是否成功
        lingxin_log_debug("线程send_record创建成功，PID: %d", send_thread_pid);
        send_thread_running = 1;
    } else {
        lingxin_log_error("Error: 线程send_record创建失败，错误码: %d", ret);
        send_thread_pid = 0;
        send_thread_running = 0;
        goto start_send_thread_err;
    }
    lingxin_mutex_unlock(send_thread_mutex);
    return 0;

start_send_thread_err:
    lingxin_mutex_unlock(send_thread_mutex);
    return -1;
}
static int stop_send_thread(int wait_send) {
    lingxin_mutex_lock(send_thread_mutex);
    if (send_thread_running) {
        lingxin_log_debug("recorderModeExit send_thread running");
        if (wait_send) {
            send_thread_stop_flag = 2;
            if (send_flag) {
                lingxin_semaphore_set_value(send_r_sem, 0);
                lingxin_semaphore_post(send_r_sem);
            }
            goto finish_stop_send_thread;
        } else {
            send_thread_stop_flag = 1;
            lingxin_semaphore_set_value(send_r_sem, 0);
            lingxin_semaphore_post(send_r_sem);
            int retry = 0;
            while (send_thread_running && retry < 500) {
                lingxin_thread_sleep(10);
                retry++;
            }
            if (retry >= 500) {
                lingxin_log_error("Stop send_thread timeout!");
                goto stop_send_thread_err;
            }
            lingxin_log_debug("[send_record]线程正常退出");
            if (send_thread_pid) {
                lingxin_thread_destroy(send_thread_pid, LINGXIN_THREAD_DESTROY_WAIT);
                send_thread_pid = 0;
            }
            lingxin_log_debug("recorderModeExit send_thread not running");
        }
    }
    // 如果当前发送线程已经没有在运行，则释放可能存在的录音缓存
    if (send_cbuf) {
        lingxin_cbuffer_free(send_cbuf);
        send_cbuf = NULL;
    }
    if (send_r_sem) {
        lingxin_semaphore_destroy(send_r_sem);
        send_r_sem = NULL;
    }

finish_stop_send_thread:
    lingxin_mutex_unlock(send_thread_mutex);
    return 0;

stop_send_thread_err:    
    lingxin_mutex_unlock(send_thread_mutex);
    return -1;
}
static void send_record(void *arg) {
    lingxin_log_debug("%s", __func__);
    int ret;
    char *buf = psram_malloc(send_uni_size);
    if (buf == NULL) {
        lingxin_log_error("Error: [send_record]buf malloc failed");
        return;
    }

	while(1) {
		lingxin_semaphore_pend(send_r_sem, 0);
        if (send_thread_stop_flag == 1) {
            lingxin_log_debug("[send_record]收到立即停止信号，线程退出");
            break;
        }
        // 读取buffer的大小和写入一致，不存在写入数据不到current_frame_size的情况
        while(lingxin_cbuffer_size(send_cbuf) >= 1) {
            if (send_thread_stop_flag == 1) {
                lingxin_log_debug("[send_record]收到立即停止信号，线程退出");
                goto send_record_thread_exit;
            }
            ret = lingxin_cbuffer_get(send_cbuf, buf);
            if(ret < 0){
                lingxin_log_error("Warning: [send_record]录音缓存读取出错，ret=%d", ret);
                continue;
            }
            lingxin_log_debug("[send_record]发送长度为%d", send_uni_size);
            state_machine_post_record_data(buf, send_uni_size);
        }
        if (send_thread_stop_flag == 1) {
            lingxin_log_debug("[send_record]收到立即停止信号，线程退出");
            break;
        } else if (send_thread_stop_flag == 2) {
            lingxin_log_debug("[send_record]收到发送完当前内容后停止的信号，线程退出");
            break;
        }
	}
send_record_thread_exit:
    if (buf) {
        free(buf);
        buf = NULL;
    }
    if (send_cbuf) {
        lingxin_cbuffer_free(send_cbuf);
        send_cbuf = NULL;
    }
    if (send_r_sem) {
        lingxin_semaphore_destroy(send_r_sem);
        send_r_sem = NULL;
    }
    lingxin_tid_t pid = send_thread_pid; // 在修改线程运行状态前先保存pid，不要放后面的if语句里，因为一旦send_thread_running设置为0，send_thread_pid就可能会被置为NULL
    send_thread_running = 0;

    // 如果此时是等待发完当前内容，则需要通知状态机录音模块正常结束已完成
    //if (send_thread_stop_flag == 2) {
        send_thread_pid = 0; // 将send_thread_pid置为NULL，后续用pid销毁自己，防止外部回调重复新建线程而冲突
        lingxin_log_debug("[send_record]send_thread_pid: %d, pid: %d", send_thread_pid, pid);
        state_machine_run_event(State_Event_Record_Stop);
        if (pid) {
            lingxin_thread_destroy(pid, LINGXIN_THREAD_DESTROY_WAIT);
        }
    //}
    //return;
}

/****************** 接收录音数据写入缓存（写缓存） ******************/
void lingxin_process_record_data(void *data, int len) {
    if (!send_thread_running) {
        lingxin_log_debug("send_thread_running is not running");
        return;
    }
    if (send_r_sem == NULL) {
        lingxin_log_debug("send_r_sem is NULL");
        return;
    }
    if (data == NULL) {
        lingxin_log_debug("data is NULL");
        return;
    }
    if (send_cbuf == NULL) {
        lingxin_log_debug("send_cbuf is NULL");
        return;
    }
    lingxin_cbuffer_put(send_cbuf, data);
    lingxin_log_debug("lingxin_process_record_data: %d, send_flag: %d", len, send_flag);
    if (send_flag) {
        lingxin_semaphore_set_value(send_r_sem, 0);
        lingxin_semaphore_post(send_r_sem);
    }
}

/****************** 相关参数获取 ******************/
// 获取帧大小
static int get_frame_size() {
    if (send_uni_size <= 0) {
        return lingxin_recorder_get_size_per_ms() * 20; // 默认20ms写一次
    } else {
        return send_uni_size;
    }
}
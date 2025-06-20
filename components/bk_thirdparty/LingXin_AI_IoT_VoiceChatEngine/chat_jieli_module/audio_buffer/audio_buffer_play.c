#include "../../chat_include/chat_module_config.h"
#include "server/audio_server.h"
#include "app_config.h"
#include "../../chat_include/audio_buffer_play.h"
#include "../../voice_chat_machine.h"

#ifdef CONFIG_AUDIO_BUFFER_DEFAULT_ENABLE

enum
{
    AUDIO_SUCC = 0,
    AUDIO_EFOPEN,
    AUDIO_EREQ_OPEN,
    AUDIO_EREQ_START,
    AUDIO_EMEM,
    AUDIO_ESER_OPEN,
};

struct audio_buffer_hdl
{
    char volume;
    int play_time;
    FILE *file;
    u8 read_file_end_flag;
    int audio_data_offset;
    struct server *dec_server;
    volatile u8 run_flag;
    OS_SEM r_sem;
    OS_SEM w_sem;
    u8 *cache_buf;
    cbuffer_t save_cbuf;
    int sample_rate;
    int channel;
};

static bool hasInit = false;

static struct audio_buffer_hdl audio_buffer_handler;

#define __this (&audio_buffer_handler)

#define FIRST_INFO_SECTOR 512
#define READ_DATA_SIZE 512

static NoParamsCallback pauseMusicCallback;

static NoParamsCallback continueTerminateCallback;

static u32 total_read_len = 0;

static int audio_buffer_dec_stop(void)
{
    int err = 0;
    union audio_req r = {0};
    r.dec.cmd = AUDIO_DEC_PAUSE;
    server_request(__this->dec_server, AUDIO_REQ_DEC, &r);

    union audio_req req = {0};
    req.dec.cmd = AUDIO_DEC_STOP;
    server_request(__this->dec_server, AUDIO_REQ_DEC, &req);

    int argv[2];
    argv[0] = AUDIO_SERVER_EVENT_END;
    server_event_handler_del(__this->dec_server, 2, argv);

    return 0;
}
extern void audio_uninit();
static void dec_server_event_handler(void *priv, int argc, int *argv)
{
    printf("0.0.6 audio_buffe_play %s %d\n", __func__, __LINE__);
    switch (argv[0])
    {
    case AUDIO_SERVER_EVENT_ERR:
        printf("0.0.6 audio_buffer: AUDIO_SERVER_EVENT_ERR\n");
    case AUDIO_SERVER_EVENT_END:
        printf("0.0.6 zzz: 最终都结束啦 audio_buffer: AUDIO_SERVER_EVENT_END");
        audio_buffer_dec_stop();
        audio_uninit();

        

        if (continueTerminateCallback) {
            printf("%s 消费了continueTerminateCallback",__func__);
            continueTerminateCallback();
            continueTerminateCallback = NULL;
        }
        else {

            state_machine_run_event(State_Event_BufferPlay_PlayEnd);
            // printf("调用连续对话");
            // 调用连续对话
            // continue_chat();
        }

        break;
    case AUDIO_SERVER_EVENT_CURR_TIME:
        printf("0.0.6 play_time: %d\n", argv[1]);
        __this->play_time = argv[1];
        break;
    }
}
extern u8 audio_get_read_file_is_end();
extern void audio_pend_read_semaphore();
extern void audio_post_write_semaphore();

/*
    file 需要读取的数据，刚启动时为0
    data 给到播放器的数据
    len 每次读取的长度
*/
static int audio_vfs_fread(void *file, void *data, u32 len)
{
    printf("0.0.6 audio_buffe_play %s %d\n", __func__, __LINE__);

    cbuffer_t *cbuf = (cbuffer_t *)file;
    u32 rlen;

    while (__this->run_flag)
    {
        rlen = cbuf_get_data_size(cbuf);
        if (rlen < len)
        {
            if (audio_get_read_file_is_end())
            {
                printf("0.0.6 zzz: 最后一次读取啦");
                printf("0.0.6 %s 重置 run_flag 值, 从 %d 到 %d", __func__, __this->run_flag, 0);

                __this->run_flag = 0;
                return cbuf_read(cbuf, data, rlen);
            }
            audio_post_write_semaphore();
            audio_pend_read_semaphore();

            continue;
        }
        if (cbuf_read(cbuf, data, len) > 0)
        {
            total_read_len = total_read_len + len;
            // 标记解码器已初始化完成
            if (total_read_len > 2048 && pauseMusicCallback)
            {

                pauseMusicCallback();
                pauseMusicCallback = NULL;
                printf("%s pauseMusicCallback 使用了",__func__);
            }

            printf("0.0.6 zzz: 解码器初始化完成, 当前read的长度%d，总长度：%d", len, total_read_len);
            return len;
        }
    }
    printf("0.0.6 __this->read_file_end_flag:%d, __this->run_flag:%d", __this->read_file_end_flag, __this->run_flag);
    // 此处返回0， 会导致mp3解码crash问题

    total_read_len = 0;
    return 0;
}

static int audio_vfs_fclose(void *file)
{
    printf("0.0.6 audio_buffe_play %s %d\n", __func__, __LINE__);

    printf("0.0.6 %s %d\n", __func__, __LINE__);
    return 0;
}

static int audio_vfs_flen(void *file)
{
    printf("0.0.6 audio_buffe_play %s %d\n", __func__, __LINE__);

    printf("0.0.6 %s %d\n", __func__, __LINE__);
    // return flen(__this->file);
    return -1;
}

static int audio_vfs_fseek(void *file, u32 offset, int orig)
{
    // printf("0.0.6 audio_buffe_play %s %d\n", __func__, __LINE__);

    printf("0.0.6 audio_buffe_play %s %d offset:%d orig:%d\n", __func__, __LINE__, offset, orig);
    __this->audio_data_offset = offset;
    // mp3_current_r = __this->audio_data_offset;
    return 0;
}

static const struct audio_vfs_ops audio_vfs_ops = {
    .fread = audio_vfs_fread,
    .fclose = audio_vfs_fclose,
    .flen = audio_vfs_flen,
    .fseek = audio_vfs_fseek,
};

static void init_audio_buffer_hdl()
{
    memset(__this, 0, sizeof(__this));
}

static u8 audio_is_info_read(int read_offset)
{
    return read_offset ? 1 : 0;
}
static void audio_file_data_offset()
{
#if 0
    fseek(__this->file, __this->audio_data_offset, SEEK_SET);
#else
    // mp3_current_r = __this->audio_data_offset;
    // printf("0.0.6 %s %d\n", __func__, mp3_current_r);
#endif
}

static int audio_check_cbuf_is_write_able(int write_len)
{
    return cbuf_is_write_able(&__this->save_cbuf, write_len);
}

static void audio_semaphore_del()
{
    os_sem_del(&__this->r_sem, 0);
    os_sem_del(&__this->w_sem, 0);
}

static void audio_semaphore_create()
{
    os_sem_create(&__this->r_sem, 0);
    os_sem_create(&__this->w_sem, 0);
}

static void audio_post_write_semaphore()
{
    printf("0.0.6 zzz: 打开写");
    os_sem_set(&__this->w_sem, 0);
    os_sem_post(&__this->w_sem);
}

static void audio_pend_write_semaphore()
{
    printf("0.0.6 zzz: 阻止写");
    os_sem_pend(&__this->w_sem, 0);
}

static void audio_post_read_semaphore()
{
    printf("0.0.6 zzz: 打开读");
    os_sem_set(&__this->r_sem, 0);
    os_sem_post(&__this->r_sem);
}

static void audio_pend_read_semaphore()
{
    printf("0.0.6 zzz: 阻止读");
    os_sem_pend(&__this->r_sem, 0);
}

static void audio_set_read_file_is_end()
{
    printf("0.0.6 zzz: 设置 audio 读取结束状态");
    __this->read_file_end_flag = 1;
}
static u8 audio_get_read_file_is_end()
{
    return __this->read_file_end_flag;
}

static u8 is_audio_file_data_end(int rlen)
{
    return rlen != READ_DATA_SIZE ? 1 : 0;
}

static void audio_free_init_cbuf()
{
    if (__this->cache_buf)
    {
        free(__this->cache_buf);
        __this->cache_buf = NULL;
    }
}

static void dec_server_uninit()
{
    server_close(__this->dec_server);
    __this->dec_server = NULL;
}

static int audio_malloc_init_cbuf()
{
    printf("0.0.6 audio_buffe_play %s %d\n", __func__, __LINE__);
    __this->cache_buf = malloc(__this->sample_rate * __this->channel * 10);
    if (!__this->cache_buf)
    {
        return AUDIO_EMEM;
    }
    cbuf_init(&__this->save_cbuf, __this->cache_buf, __this->sample_rate * __this->channel * 10);

    return AUDIO_SUCC;
}

static void set_audio_parameter()
{
    __this->volume = 80;
    __this->read_file_end_flag = 0;
    printf("%s, 设置 read_file_end_flag 为 %d", __func__, __this->read_file_end_flag);
    __this->sample_rate = 16000;
    __this->channel = 1;
    __this->run_flag = 1;

    printf("0.0.6 %s 设置 run_flag 值 %d", __func__, __this->run_flag);
}

static void audio_uninit()
{
    audio_post_read_semaphore();
    audio_post_write_semaphore();
    audio_free_init_cbuf();
    dec_server_uninit();
    audio_semaphore_del();
    hasInit = false;
}
//真正初始化的audio的地方
static void audio_init()
{
    if (hasInit)
    {
        printf("0.0.6 zzz: audio_init 已经完成");
        return;
    }

    printf("0.0.6 zzz: audio_init 开始初始化");
    int ret = AUDIO_SUCC;

    init_audio_buffer_hdl();
    set_audio_parameter();
    audio_semaphore_create();
    //ASSERT(ret = (dec_server_init_and_register() == AUDIO_SUCC), "ret = %d\n", ret);
    //ASSERT(ret = (audio_malloc_init_cbuf() == AUDIO_SUCC), "ret = %d\n", ret);
    hasInit = true;
}

#define RTOS_STACK_CHECK_ENABLE // 是否启用定时检查任务栈

int get_mep_dec_status(void) 
{
	union audio_req req = {0};

	if(!__this->dec_server) {
		return -1;
	}

	req.dec.cmd = AUDIO_DEC_GET_STATUS;
	int ret = server_request(__this->dec_server, AUDIO_REQ_DEC, &req);
	printf("ret : %d\n", ret);
	
	return req.dec.status;
}

static int audio_buffer_data_out()
{
    printf("0.0.6 zzz: audio_buffer_data_out");
    printf("0.0.6 audio_buffe_play %s %d\n", __func__, __LINE__);
    union audio_req req = {0};

    req.dec.volume = __this->volume;
    req.dec.output_buf_len = 4 * 1024;
    req.dec.channel = __this->channel;
    req.dec.vfs_ops = &audio_vfs_ops;
    req.dec.dec_type = "mp3";
    req.dec.sample_source = "dac";
    req.dec.file = (FILE *)&__this->save_cbuf;
// for aec work
#ifdef CONFIG_AEC_USE_PLAY_MUSIC_ENABLE
    req.dec.orig_sr = __this->sample_rate;
    req.dec.force_sr = 48000;
#else
    req.dec.sample_rate = __this->sample_rate;
#endif

    printf("0.0.6 zzz: 开始解码");
    req.dec.cmd = AUDIO_DEC_OPEN;
    int err = server_request(__this->dec_server, AUDIO_REQ_DEC, &req);
    printf("0.0.6 %s %d err:%d\n", __func__, __LINE__, err);
    if (err)
    {
        if (__this->cache_buf)
        {
            free(__this->cache_buf);
            __this->cache_buf = NULL;
        }

        printf("0.0.6 %s 重置 run_flag 值, 从 %d 到 %d", __func__, __this->run_flag, 0);

        __this->run_flag = 0;
        return AUDIO_EREQ_OPEN;
    }

    req.dec.cmd = AUDIO_DEC_START;
    server_request(__this->dec_server, AUDIO_REQ_DEC, &req);
    printf("0.0.6 AUDIO_DEC_START %s %d\n", __func__, __LINE__);
    if (err)
    {
        if (__this->cache_buf)
        {
            free(__this->cache_buf);
            __this->cache_buf = NULL;
        }
        printf("0.0.6 %s 重置 run_flag 值, 从 %d 到 %d", __func__, __this->run_flag, 0);

        __this->run_flag = 0;
        return AUDIO_EREQ_START;
    }

    printf("0.0.6 AUDIO_DEC_START 执行成功 %s %d\n", __func__, __LINE__);
    return AUDIO_SUCC;
}

static int dec_server_init_and_register()
{
    printf("0.0.6 audio_buffe_play %s %d\n", __func__, __LINE__);
    __this->dec_server = server_open("audio_server", "dec");
    //printf("0.0.6 %s %d __this->dec_server:0x%x\n", __func__, __LINE__, __this->dec_server);

    if (!__this->dec_server)
    {
        return AUDIO_ESER_OPEN;
    }
    server_register_event_handler_to_task(__this->dec_server, NULL, dec_server_event_handler, "app_core");
    return AUDIO_SUCC;
}

static int audio_buffer_play()
{
    printf("0.0.6 zzz: audio_buffer_play");

    audio_init();

    int ret = audio_buffer_data_out();

    printf("0.0.6 audio_buffer_data_out 执行成功 %s", __func__);

    if (ret != AUDIO_SUCC)
    {
        printf("audio buffer play 打断 assert, ret: %d", ret);

        audio_uninit();

        hasInit = false;
        audio_init();
        ASSERT(ret == audio_buffer_data_out(), "ret = %d\n", ret);
    }

    state_machine_run_event(State_Event_BufferPlay_AudioInitEnd);

    printf("0.0.6 State_Event_BufferPlay_AudioInitEnd 执行成功 %s", __func__);

    return 0;
}

/**
 * 播放 buffer
 */
void audioBufferPlay(void *buf, int rlen)
{
    if (!__this->run_flag)
    {
        printf("0.0.6 zzz: 已经结束了，但是还在往里 post buffer");
        audio_post_write_semaphore();
        audio_post_read_semaphore();
        return;
    }
    printf("0.0.6 zzz:  audioBufferPlay 开始写");
    audio_pend_write_semaphore();
    cbuf_write(&__this->save_cbuf, buf, rlen);
    audio_file_data_offset();
    audio_post_read_semaphore();
}

/**
 * 播放结束
 */
void audioBufferEnd()
{
    audio_set_read_file_is_end();

    audio_post_read_semaphore();
}

void mp3_user_buf_main()
{
    printf("0.0.6 %s %d\n", __func__, __LINE__);
    thread_fork("audio_buffer", 10, 1024, 0, 0, audio_buffer_play, 0);
}

void audioInit()
{
    printf("0.0.6 zzz: audioInit");

    total_read_len = 0;

    if (pauseMusicCallback) {
        printf("%s pauseMusicCallback 未被执行",__func__);
        pauseMusicCallback = NULL;
    }

    if (continueTerminateCallback) {
        printf("%s continueTerminateCallback 未被执行",__func__);
        continueTerminateCallback = NULL;
    }

    if (hasInit)
    {
        if (__this->cache_buf)
        {
            cbuf_clear(&__this->save_cbuf);
        }
        if (__this->read_file_end_flag)
        {
            printf("0.0.6 %s 重置 read_file_end_flag 值, 从 %d 到 %d， __this->run_flag :%d", __func__, __this->read_file_end_flag, 0,__this->run_flag);
            __this->read_file_end_flag = 0;
        }

        int mp3_sts =  get_mep_dec_status();
        printf("0.0.6 %s  mp3_status : %d\n",__func__, mp3_sts);

        if (mp3_sts == AUDIO_DEC_STOP) {

            printf("0.0.6 %s  重启音频服务",__func__);
            audio_uninit();

            hasInit = false;
            audioInit();
        }

        return;
    }
    mp3_user_buf_main();
}

static void pauseCallbackMethod()
{
    printf("0.0.6 zzz:  audioBufferContinuePause 停止往里推");
    audioBufferEnd();
}

/**
 * 触发时机： 接收到你好小图的时候
 */
void audioBufferContinuePause(NoParamsCallback callback)
{
    printf("0.0.6 zzz:  audioBufferContinuePause begin");

    if (__this->read_file_end_flag == 1 || __this->run_flag == 0)
    {
        printf("0.0.6 __this->run_flag %d; 非播放状态，就停止播放, 直接回调 continueTerminateCallback",__this->run_flag);
        if (callback) {
            callback();
        }
        return;
    }
    if (total_read_len > 2048) {
        printf("0.0.6 已经解析了2048字节长度直接停止, 直接回调 continueTerminateCallback");
        pauseCallbackMethod();
        // if (callback) {
        //     callback();
        // }
        continueTerminateCallback = callback;
    }
    else {
        printf("%s pauseMusicCallback 被赋值了，延迟执行 continueTerminateCallback",__func__);
        pauseMusicCallback = pauseCallbackMethod;
        continueTerminateCallback = callback;
    }
}

void postSemphore()
{
    printf("0.0.6 write、read post，解锁之前的锁定 1 %s %d\n", __func__, __LINE__);
    audio_post_write_semaphore();
    audio_post_read_semaphore();

}


/**
 * 新接口实现
 */

// 功能：在方法里面实现模块的初始化逻辑。并且Chat套件内核会多次回调这个方法，如果当前模块已经初始化成功，可直接回调初始化成功的回调事件。
// 调用时机：由chat套件内核发起调用，客户实现
void module_bufferPlay_audioInit()
{
    audioInit();
}

// buf为mp3数据 rlen为当前数据长度
void module_bufferPlay_data(void *buf, int rlen)
{
    audioBufferPlay(buf, rlen);
}

// 功能：指Chat套件内核调用流式播放模块告诉他已经没有流式播放数据了，并非要立刻停止播放。
// 调用时机：由chat套件内核发起调用，客户实现
void module_bufferPlay_audioEnd()
{
    audioBufferEnd();
}

static void terminCallback() {
    state_machine_run_event(State_Event_BufferPlay_TerminateEnd);
}

// 功能：停止当前的播放逻辑
// 调用时机：由chat套件内核发起调用，客户实现
void module_bufferPlay_terminate()
{
    audioBufferContinuePause(terminCallback);
}

#endif
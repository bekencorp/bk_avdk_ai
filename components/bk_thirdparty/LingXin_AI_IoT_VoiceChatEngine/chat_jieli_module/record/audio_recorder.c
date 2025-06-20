#include "../../chat_include/chat_module_config.h"
#include "server/audio_server.h"
#include "server/server_core.h"
#include "system/app_core.h"
#include "generic/circular_buf.h"
#include "os/os_api.h"
#include "app_config.h"
#include "syscfg/syscfg_id.h"
#include "event/key_event.h"
#include "fs/fs.h"
#include <time.h>
#include "../../chat_include/audio_recorder.h"

#include "../../voice_chat_machine.h"
#include "storage_device.h"

#ifdef CONFIG_RECORD_DEFAULT_ENABLE

#define CONFIG_STORE_VOLUME
#define VOLUME_STEP 5
#define GAIN_STEP 5
#define MIN_VOLUME_VALUE 5
#define MAX_VOLUME_VALUE 100
#define INIT_VOLUME_VALUE 50

struct recorder_hdl
{
    FILE *fp;
    struct server *enc_server;
    void *cache_buf;
    cbuffer_t save_cbuf;
    OS_SEM w_sem;
    OS_SEM r_sem;
    volatile u8 run_flag;
    u8 volume;
    u8 gain;
    u8 channel;
    u8 direct;
    const char *sample_source;
    int sample_rate;
    RecorderInitedCallback initedCallback;
};

static struct recorder_hdl recorder_handler;

#define __this (&recorder_handler)

// 编码器输出PCM数据
static int recorder_vfs_fwrite(void *file, void *data, u32 len)
{
    // NOTE: 这个方法没有啊
    // voiceChatSendAudio(data, (int)len);
    state_machine_post_record_data(data, (int)len);
    return len;
}

static int recorder_vfs_fclose(void *file)
{
    printf("recorder_vfs_fclose");
    return 0;
}

static int recorder_vfs_flen(void *file)
{
    printf("recorder_vfs_flen");
    return 0;
}

static const struct audio_vfs_ops recorder_vfs_ops = {
    .fwrite = recorder_vfs_fwrite,
    .fclose = recorder_vfs_fclose,
    .flen = recorder_vfs_flen,
};

static int recorder_close(void)
{
    union audio_req req = {0};
    printf("----------recorder_close----------\n");

    if (!__this->run_flag)
    {
        printf("----------recorder_close run_flag----------\n");

        return 0;
    }

    __this->run_flag = 0;

    os_sem_post(&__this->w_sem);
    os_sem_post(&__this->r_sem);

    if (__this->enc_server)
    {
        req.enc.cmd = AUDIO_ENC_CLOSE;
        server_request(__this->enc_server, AUDIO_REQ_ENC, &req);
    }

    if (__this->cache_buf)
    {
        free(__this->cache_buf);
        __this->cache_buf = NULL;
    }

    if (__this->fp)
    {
        fclose(__this->fp);
        __this->fp = NULL;
    }

    return 0;
}

/**
 * 把 server 注册到 event_handler 中的回调
 * NOTE: 这个从来没有调用过。永远在编码，即使 server_close 也不会走到这个回调
 */
static void enc_server_event_handler(void *priv, int argc, int *argv)
{

    switch (argv[0])
    {
    case AUDIO_SERVER_EVENT_ERR:
    case AUDIO_SERVER_EVENT_END:
        recorder_close();
        break;

        break;
    default:
        break;
    }
}

/**
 * 执行 enc_server
 */
// 将MIC的数字信号采集后推到DAC播放
// 注意：如果需要播放两路MIC，DAC分别对应的是DACL和DACR，要留意芯片封装是否有DACR引脚出来，
//       而且要使能DAC的双通道输出，DAC如果采用差分输出方式也只会听到第一路MIC的声音
static int recorder_play_to_dac(int sample_rate, u8 channel)
{
    int err;
    union audio_req req = {0};

    printf("----------recorder_play_to_dac----------\n");

    if (channel > 2)
    {
        channel = 2;
    }
    __this->cache_buf = malloc(sample_rate * channel); // 上层缓冲buf缓冲0.5秒的数据，缓冲太大听感上会有延迟
    if (__this->cache_buf == NULL)
    {
        return -1;
    }
    cbuf_init(&__this->save_cbuf, __this->cache_buf, sample_rate * channel);

    os_sem_create(&__this->w_sem, 0);
    os_sem_create(&__this->r_sem, 0);

    __this->run_flag = 1;

    /****************打开编码器*******************/
    memset(&req, 0, sizeof(union audio_req));

    // BIT(x)用来区分上层需要获取哪个通道的数据
    #ifdef CONFIG_FenTengDa_ENABLE
        req.enc.channel_bit_map	= BIT(0);//CONFIG_AUDIO_ADC_CHANNEL_L;
    #else
        if (channel == 2)
        {
            req.enc.channel_bit_map = BIT(CONFIG_AUDIO_ADC_CHANNEL_L) | BIT(CONFIG_AUDIO_ADC_CHANNEL_R);
        }
        else
        {
            req.enc.channel_bit_map = BIT(CONFIG_AUDIO_ADC_CHANNEL_L);
        }
    #endif 

    req.enc.frame_size = sample_rate / 100 * 4 * channel; // 收集够多少字节PCM数据就回调一次fwrite  相当于 40ms 一个 frame
    req.enc.output_buf_len = req.enc.frame_size * 3;      // 底层缓冲buf至少设成3倍frame_size
    req.enc.cmd = AUDIO_ENC_OPEN;
    req.enc.channel = channel;
    req.enc.volume = __this->gain;
    req.enc.sample_rate = sample_rate;
    req.enc.format = "pcm";
    req.enc.sample_source = __this->sample_source;
    req.enc.vfs_ops = &recorder_vfs_ops;
    req.enc.file = (FILE *)&__this->save_cbuf;
    req.enc.use_vad = 0;

    err = server_request(__this->enc_server, AUDIO_REQ_ENC, &req);
    if (err)
    {
        goto __err;
    }
    if (__this->initedCallback)
    {
        __this->initedCallback();
    }

    return 0;

__err:
    if (__this->cache_buf)
    {
        free(__this->cache_buf);
        __this->cache_buf = NULL;
    }

    __this->run_flag = 0;

    return -1;
}

/**
 * 创建 server，注册 server 初始化 handler
 */
int recorderModeInit(RecorderInitedCallback callback)
{
    printf("%s", __func__);
    memset(__this, 0, sizeof(struct recorder_hdl));
    __this->initedCallback = callback;
    __this->sample_source = "mic";
    __this->channel = CONFIG_AUDIO_RECORDER_CHANNEL;
    __this->gain = CONFIG_AUDIO_ADC_GAIN;
    __this->sample_rate = CONFIG_AUDIO_RECORDER_SAMPLERATE;

    __this->enc_server = server_open("audio_server", "enc");
    server_register_event_handler_to_task(__this->enc_server, NULL, enc_server_event_handler, "app_core");
    return recorder_play_to_dac(__this->sample_rate, __this->channel);
}

/**
 *  主动触发，退出 server
 */
void recorderModeExit(RecorderExitCallback callback)
{
    printf("module_record_stop\n");

    if (!__this->enc_server)
    {
        printf("recorderMode has Exited\n");
        if (callback)
        {
            callback();
        }
        return;
    }
    
    recorder_close();
    printf("module_record_stop recorder_close after\n");
    server_close(__this->enc_server);
    __this->enc_server = NULL;
    __this->initedCallback = NULL;
    if (callback)
    {
        callback();
    }
}

static void initCallback() {
    state_machine_run_event(State_Event_Record_Ready);
}
int module_record_init()
{
    printf("%s 来自于状态机的调用",__func__);
    return recorderModeInit(initCallback);
}

static void stopCallback() {
    state_machine_run_event(State_Event_Record_Stop);
}
void module_record_stop()
{
    printf("%s 来自于状态机的调用",__func__);
    return recorderModeExit(stopCallback);
}

static void terminalCallback() {
    state_machine_run_event(State_Event_Record_TerminateEnd);
}

void module_record_terminate()
{
    printf("%s 来自于状态机的调用",__func__);
    return recorderModeExit(terminalCallback);
}

#endif

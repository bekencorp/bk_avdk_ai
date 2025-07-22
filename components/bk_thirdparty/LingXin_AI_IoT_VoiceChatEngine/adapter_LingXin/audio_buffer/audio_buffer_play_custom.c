#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <common/bk_err.h>
#include "chat_module_config.h"
#include "audio_buffer_play.h"
#include "chat_state_machine.h"
#include <aud_intf.h>
#include "audio_engine.h"

#define TAG "lx_play"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#ifdef CONFIG_AUDIO_BUFFER_CUSTOM_ENABLE

typedef struct {
    uint8_t *buffer;
    size_t size;
    size_t read_index;
    size_t write_index;
    size_t *length_buffer;
    size_t length_read_index;
    size_t length_write_index;
    size_t buffer_count;
} data_buffer_t;

typedef struct {
    data_buffer_t *ring_buffer;
    beken_timer_t data_read_tmr;
    audio_info_t info;
    int playing_flags; //0:running 1:end
    beken_mutex_t lock; //TODO if needed
} play_config_t;

#if CONFIG_DEBUG_DUMP
#include "debug_dump.h"
extern bool rx_spk_data_flag;
#endif
play_config_t *play_info = NULL;
#define WSS_AUDIO_BUFFER_SIZE (1500*1024)

// 实现自定义的逻辑
int play_user_audio_rx_data_handle(unsigned char *data, unsigned int size)
{
    bk_err_t ret = BK_OK;

    #if CONFIG_DEBUG_DUMP
    if(rx_spk_data_flag)
    {
        uint32_t decoder_type = bk_aud_get_decoder_type();
        uint8_t dump_file_type = dbg_dump_get_dump_file_type((uint8_t)decoder_type);
        DEBUG_DATA_DUMP_UPDATE_HEADER_DUMP_FILE_TYPE(DUMP_TYPE_RX_SPK,0,dump_file_type);
        DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW_LEN(DUMP_TYPE_RX_SPK,0,size);
        DEBUG_DATA_DUMP_UPDATE_HEADER_TIMESTAMP(DUMP_TYPE_RX_SPK);
        #if CONFIG_DEBUG_DUMP_DATA_TYPE_EXTENSION
        DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW_SAMP_RATE(DUMP_TYPE_RX_SPK,0,bk_aud_get_dac_sample_rate());
        DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW_FRAME_IN_MS(DUMP_TYPE_RX_SPK,0,bk_aud_get_dec_frame_len_in_ms());
        #endif
        DEBUG_DATA_DUMP_BY_UART_HEADER(DUMP_TYPE_RX_SPK);
        DEBUG_DATA_DUMP_UPDATE_HEADER_SEQ_NUM(DUMP_TYPE_RX_SPK);
        DEBUG_DATA_DUMP_BY_UART_DATA(data, size);
    }
    #endif//CONFIG_DEBUG_DUMP
    ret = bk_aud_intf_write_spk_data((uint8_t *)data, (uint32_t)size);
    if (ret != BK_OK)
    {
        LOGE("write spk data fail \r\n");
    }

    return ret;
}

data_buffer_t *play_data_buffer_init (size_t buffer_size, size_t length_buffer_size) {
#if CONFIG_PSRAM_AS_SYS_MEMORY
    data_buffer_t *rb = (data_buffer_t *)psram_malloc(sizeof(data_buffer_t));
#else
    data_buffer_t *rb = (data_buffer_t *)os_malloc(sizeof(data_buffer_t));
#endif
    if (rb == NULL)
    {
        LOGE("malloc rb fail\n");
        return NULL;
    }
    memset(rb, 0, sizeof(data_buffer_t));
#if CONFIG_PSRAM_AS_SYS_MEMORY
    rb->buffer = (uint8_t*) psram_malloc (buffer_size);
#else
    rb->buffer = (uint8_t*) os_malloc (buffer_size);
#endif
    if (rb->buffer == NULL)
    {
        LOGE("malloc buffer_size fail\n");
        os_free(rb);
        return NULL;
    }

    memset(rb->buffer, 0, buffer_size);
    rb->size = buffer_size;
    rb->read_index = 0;
    rb->write_index = 0;
#if CONFIG_PSRAM_AS_SYS_MEMORY
    rb->length_buffer = (size_t*) psram_malloc (length_buffer_size * sizeof (size_t));
#else
    rb->length_buffer = (size_t*) os_malloc (length_buffer_size * sizeof (size_t));
#endif
    if (rb->length_buffer == NULL)
    {
        LOGE("malloc length_buffer fail\n");
        os_free(rb);
        os_free(rb->buffer);
        return NULL;
    }
    memset((size_t*)rb->length_buffer, 0, length_buffer_size * sizeof (size_t));
    rb->length_read_index = 0;
    rb->length_write_index = 0;
    rb->buffer_count = length_buffer_size;
    LOGE("ringbuffer size:%d ringbuffer max count:%d\n", buffer_size, length_buffer_size);
    return rb;
}

void play_data_buffer_deinit(data_buffer_t *rb) {

    if (rb == NULL)
    {
        LOGE("buffer deinit already\n");
        return;
    }

    if (rb->buffer)
    {
        os_free(rb->buffer);
    }

    if (rb->length_buffer)
    {
        os_free (rb->length_buffer);
    }

    memset(rb, 0, sizeof(data_buffer_t));
    if (rb)
    {
        os_free(rb);
    }
}

int play_data_buffer_write (data_buffer_t* rb, const uint8_t *data, size_t data_len) {

    size_t remaining = rb->size - rb->write_index;
    if ((rb->length_write_index + 1) % rb->buffer_count == rb->length_read_index) {
        return -1;
    }

    if (data_len <= remaining) {
        memcpy (&rb->buffer [rb->write_index], data, data_len);
        rb->write_index += data_len;
    } else {
        size_t first_part = remaining;
        memcpy (&rb->buffer [rb->write_index], data, first_part);
        size_t second_part = data_len - first_part;
        memcpy (rb->buffer, &data [first_part], second_part);
        rb->write_index = second_part;
    }
    rb->length_buffer [rb->length_write_index] = data_len;
    rb->length_write_index = (rb->length_write_index + 1) % rb->buffer_count;

    return 0;
}

size_t play_data_buffer_read(data_buffer_t *rb, uint8_t *output) {

    if ((rb->length_read_index == rb->length_write_index) && (rb->read_index == rb->write_index)) {
        return 0;
    }
    size_t data_len = rb->length_buffer [rb->length_read_index];
    size_t available = (rb->write_index >= rb->read_index)? (rb->write_index - rb->read_index) : (rb->size - rb->read_index + rb->write_index);
    if (available < data_len) {
        return 0;
    }
    if (rb->write_index >= rb->read_index) {
        memcpy (output, &rb->buffer [rb->read_index], data_len);
        rb->read_index += data_len;
    } else {
        size_t first_part = rb->size - rb->read_index;
        if (first_part >= data_len) {
            memcpy (output, &rb->buffer [rb->read_index], data_len);
            rb->read_index += data_len;
        } else {
            memcpy (output, &rb->buffer [rb->read_index], first_part);
            size_t second_part = data_len - first_part;
            memcpy (&output [first_part], rb->buffer, second_part);
            rb->read_index = second_part;
        }
    }
    rb->length_read_index = (rb->length_read_index + 1) % rb->buffer_count;
    LOGD("%s length_read_index:%d length_write_index:%d write_index:%d read_index:%d available:%d data_len:%d size:%d\r\n",
        __func__, rb->length_read_index, rb->length_write_index, rb->write_index, rb->read_index, available, data_len, rb->size);
    return data_len;
}

void play_data_buffer_clean(data_buffer_t *rb)
{
    size_t available = (rb->write_index >= rb->read_index)? (rb->write_index - rb->read_index) : (rb->size - rb->read_index + rb->write_index);
    LOGI("%s available:%d\r\n", __func__, available);
    memset(rb->buffer, 0, rb->size);
    memset(rb->length_buffer, 0, (rb->buffer_count * sizeof (size_t)));
    rb->read_index = 0;
    rb->write_index = 0;
    rb->length_read_index = 0;
    rb->length_write_index = 0;
    return;
}

void play_data_check(void *param)
{
    play_config_t *play = (play_config_t *) param;
    if(play == NULL) {
        LOGE("play config null...\n");
        return;
    }
    uint8_t *packet = NULL;
    packet = os_zalloc(play->info.dec_node_size);
    int size = 0;
    if (packet != NULL && (play->info.dec_node_size <= bk_aud_intf_get_dec_rb_free_size()))
    {
        if (play->ring_buffer && (size = play_data_buffer_read(play->ring_buffer, packet))) {
            play_user_audio_rx_data_handle(packet, size);
            LOGD("data coming...\n");
        } else {
            LOGD("Buffer empty, waiting for data...\n");
        }
    }
    os_free(packet);
}

int play_data_start_timeout_check(uint32_t timeout, void *param)
{
    bk_err_t err = kNoErr;
    play_config_t *play = (play_config_t *) param;
    if(play == NULL) {
        LOGE("play config null...\n");
        return BK_FAIL;
    }

    LOGI("ring_data status timer start!!! dectype:%s\n", play->info.decoding_type);
    err = rtos_init_timer(&(play->data_read_tmr), timeout, (timer_handler_t)play_data_check, param);

    BK_ASSERT(kNoErr == err);
    err = rtos_start_timer(&(play->data_read_tmr));
    BK_ASSERT(kNoErr == err);
    LOGI("ring_data status timer:%d\n", timeout);

    return BK_OK;
}

void play_data_stop_timeout_check(beken_timer_t *data_read_tmr)
{
    if(!data_read_tmr->handle) {
        LOGE("data_read_tmr deinit already...\n");
        return;
    }

    if (rtos_is_timer_init(data_read_tmr)) {
        if (rtos_is_timer_running(data_read_tmr))
        {
            rtos_stop_timer(data_read_tmr);
        }
    rtos_deinit_timer(data_read_tmr);
    }
}

int play_data_end(void *param)
{
    LOGI("%s\r\n", __func__);
    if (play_info && play_info->playing_flags == 0)
        state_machine_run_event(State_Event_BufferPlay_PlayEnd);
    return BK_OK;
}

void Play_audioClean(void)
{
    play_config_t *play = play_info;
    if (play && play->ring_buffer) {
        play_data_buffer_clean(play->ring_buffer);
    }
}

play_config_t *Play_audioInit()
{
    int max_count;
    play_config_t *play = os_zalloc(sizeof(play_config_t));
    if (play == NULL) {
        LOGI("malloc play_config_t fail\r\n");
        return NULL;
    }
    os_memcpy(&play->info, &general_audio, sizeof(audio_info_t));
    if (play->info.dec_node_size) {
        play->info.dec_node_size = play->info.dec_node_size * 4;  //TODO, max size set to 1232 * 4
        max_count = WSS_AUDIO_BUFFER_SIZE / (play->info.dec_node_size);
        LOGI("module_bufferPlay_audioInit. enctype:%s dectype:%s adc_rate:%d dac_rate:%d enc:%d dec:%d encsize:%d decsize:%d max_count:%d\n",
            play->info.encoding_type, play->info.decoding_type, play->info.adc_samp_rate, play->info.dac_samp_rate,
            play->info.enc_samp_interval, play->info.dec_samp_interval, play->info.enc_node_size, play->info.dec_node_size, max_count);
    } else {
        LOGE("audio info error\n");
        goto exit;
    }
    audio_register_play_finish_func(play_data_end);

    if (play->info.decoding_type) {
        play->ring_buffer = play_data_buffer_init(((play->info.dec_node_size) * max_count), max_count);
        if (play->ring_buffer == NULL)
        {
            LOGE("%s, %d, data_buffer_init fail\n", __func__, __LINE__);
            goto exit;
        }
    }
    if (play_data_start_timeout_check(play->info.dec_samp_interval, (void *)play))
    {
        LOGE("%s, %d, data_start_timeout_check fail\n", __func__, __LINE__);
        goto exit;
    }
    return play;

exit:
    if (play->ring_buffer)
        play_data_buffer_deinit(play->ring_buffer);
    if (play)
        os_free(play);
    return NULL;
}

void Play_audioDeinit(play_config_t *play)
{
    if(play == NULL) {
        LOGE("play_config_t aleady null\r\n");
        return;
    }
    play_data_stop_timeout_check(&play->data_read_tmr);
    play_data_buffer_deinit(play->ring_buffer);
    play->ring_buffer = NULL;

    if (play) {
        os_free(play);
    }
}

// 功能：在方法里面实现模块的初始化逻辑。并且Chat套件内核会多次回调这个方法，如果当前模块已经初始化成功，可直接回调初始化成功的回调事件。
// 调用时机：由chat套件内核发起调用，客户实现
void module_bufferPlay_audioInit()
{
     LOGI("module_bufferPlay_audioInit\n");
     if (play_info == NULL) {
        play_info = Play_audioInit();
     }
     play_info->playing_flags = 0;
     bk_aud_intf_voc_write_spk_data_ctrl(1);
     state_machine_run_event(State_Event_BufferPlay_AudioInitEnd);
}

// buf为mp3数据 rlen为当前数据长度
void module_bufferPlay_data(void *buf, int rlen)
{
    int i = 0;

    if (rlen > (play_info->info.dec_node_size)) {
        LOGE("data too large, len:%d limit:%d\n", rlen, general_audio.dec_node_size);
        return;
    }

    LOGD("%s rlen:%d\n", __func__, rlen);

retry:
    if (play_info->ring_buffer) {
        if (play_data_buffer_write(play_info->ring_buffer, buf, rlen) != 0) {
            rtos_delay_milliseconds(20);
            i++;
            if (i > 50) {
                LOGE("%s, write buffer fail length_write_index:%d write_index:%d read_index:%d data_len:%d\r\n",
                        __func__, play_info->ring_buffer->length_write_index, play_info->ring_buffer->write_index, play_info->ring_buffer->read_index, rlen);
                return;
            }
            goto retry;
        }
    }
}

// 功能：指Chat套件内核调用流式播放模块告诉他已经没有流式播放数据了，并非要立刻停止播放。
// 调用时机：由chat套件内核发起调用，客户实现
void module_bufferPlay_audioEnd()
{
    LOGI("module_bufferPlay_audioEnd\n");
    bk_aud_intf_voc_write_spk_data_ctrl(0);
    //Play_audioDeinit(play_info);
    //play_info = NULL;
}

// 功能：停止当前的播放逻辑
// 调用时机：由chat套件内核发起调用，客户实现
void module_bufferPlay_terminate()
{
    LOGI("module_bufferPlay_terminate\n");
    Play_audioClean();
    if (play_info)
        play_info->playing_flags = 1;
    state_machine_run_event(State_Event_BufferPlay_TerminateEnd);
}

#endif

// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "cli.h"

#include <os/mem.h>
#include <os/os.h>
#include <stdio.h>
#include <stdlib.h>
#include <modules/opus.h>
#include <driver/audio_ring_buff.h>
#include "opus_data.h"

#define RUNTIME_MEAS 1
#define DEC_OUTPUT_DUMP 0
#define MEM_USAGE 1
#define TASK_SRAM 0

uint16 enc_len[50] = {0}; //1s,50 frames,20ms frame length
uint16 enc_cnt = 0;
int32_t encoder_total_len = 0;

//need manually modify dec_len/dec_cnt accord to input data if it is not loopback test
#if !OPUS_LOOPBACK_TEST
#if OPUS_DEC_ONLY_DAC_24K_SAMPLING_RATE_VBR_EN_CASE
uint16 dec_len[17] = 
{
    75,86,138,118,111,123,126,107,85,102,114,109,97,81,71,54,66,
};
uint16 dec_cnt = 10;
#else
#endif
#endif

static beken_thread_t opus_enc_thread_handle = NULL;
static beken_thread_t opus_dec_thread_handle = NULL;

char enc_app_str[4][20] = 
{
    "voip",
    "audio",
    "restricted-lowdelay",
    "unknow",
};

typedef struct 
{
    uint32_t adc_samp_rate;//enc
    uint32_t dac_samp_rate;//dec
    uint32_t enc_bitrate;
    uint32_t dec_bitrate;
    uint32_t enc_input_size_in_byte;
    uint32_t dec_input_size_in_byte;
    uint16_t enc_application;
    uint16_t enc_bandwidth;
    uint16_t enc_signal;
    uint16_t enc_frame_size;
    uint16_t dec_frame_size;
    uint16_t max_payload_size;
    uint8_t  enc_ch_num;
    uint8_t  dec_ch_num;
    uint8_t  enc_vbr_en;
    uint8_t  dec_vbr_en;
    uint8_t  enc_vbr_constraint;
    uint8_t  enc_complexity;
    uint8_t  enc_inband_fec;
    uint8_t  enc_packet_loss_perc;
    uint8_t  enc_lsb_depth;
    uint8_t  enc_prediction_disable;
    uint8_t  enc_dtx;
    uint8_t  enc_force_chs;
}opus_codec_para_t;

opus_codec_para_t opus_codec_setup = 
{
    .adc_samp_rate = 16000,//enc
    .dac_samp_rate = 16000,//dec
    .enc_bitrate = 16000,
    .dec_bitrate = 64000,
    .enc_application = OPUS_APPLICATION_AUDIO,
    .enc_bandwidth = OPUS_BANDWIDTH_MEDIUMBAND,
    .enc_signal = OPUS_SIGNAL_VOICE,
    .enc_frame_size = OPUS_FRAMESIZE_60_MS,
    .dec_frame_size = OPUS_FRAMESIZE_20_MS,
    .max_payload_size = 4000,
    .enc_ch_num = 1,
    .dec_ch_num = 1,
    .enc_vbr_en = 1,
    .dec_vbr_en = 1,
    .enc_vbr_constraint = 0,
    .enc_complexity = 1,
    .enc_inband_fec = 0,
    .enc_packet_loss_perc = 0,
    .enc_lsb_depth = 16,
    .enc_prediction_disable = 1,
    .enc_dtx = 0,
    .enc_force_chs = 1,
};

#if RUNTIME_MEAS
#include <soc/soc.h>

#define TIMER0_REG_SET(reg_id, l, h, v) REG_SET((SOC_TIMER0_REG_BASE + ((reg_id) << 2)), (l), (h), (v))
#define TIMER0_PERIOD 0xFFFFFFFF

uint32_t timer_hal_get_timer0_cnt(void)
{
    TIMER0_REG_SET(8, 2, 3, 0);
    TIMER0_REG_SET(8, 0, 0, 1);
    while (REG_READ((SOC_TIMER0_REG_BASE + (8 << 2))) & BIT(0));

    return REG_READ(SOC_TIMER0_REG_BASE + (9 << 2));
}

uint32_t get_duration_clk_cycles(uint32_t start, uint32_t end)
{
    return start < end?(end-start):(TIMER0_PERIOD - (start-end) + 1);
}
#endif

char* app_mapping(uint16_t enc_app)
{
    uint8_t idx;
    
    switch(enc_app)
    {
        case OPUS_APPLICATION_VOIP:
        {
            idx = 0;
            break;
        }
        case OPUS_APPLICATION_AUDIO:
        {
            idx = 1;
            break;
        }
        case OPUS_APPLICATION_RESTRICTED_LOWDELAY:
        {
            idx = 2;
            break;
        }
        default:
        {
            idx = 3;
            os_printf("invalid application type:%d,use 20ms frame!\n",enc_app);
            break;
        }
    }

    return (char *)&enc_app_str[idx];
}


int8_t frame_len_mapping(uint16_t frame_len)
{
    int8_t frame_len_in_ms;
    
    switch(frame_len)
    {
        case OPUS_FRAMESIZE_2_5_MS:
        {
            frame_len_in_ms = 20;
            os_printf("don't support 2.5ms frame length,use 20ms frame!\n");
            break;
        }
        case OPUS_FRAMESIZE_5_MS:
        {
            frame_len_in_ms = 5;
            break;
        }
        case OPUS_FRAMESIZE_10_MS:
        {
            frame_len_in_ms = 10;
            break;
        }
        case OPUS_FRAMESIZE_20_MS:
        {
            frame_len_in_ms = 20;
            break;
        }
        case OPUS_FRAMESIZE_40_MS:
        {
            frame_len_in_ms = 40;
            break;
        }
        case OPUS_FRAMESIZE_60_MS:
        {
            frame_len_in_ms = 60;
            break;
        }
        case OPUS_FRAMESIZE_80_MS:
        {
            frame_len_in_ms = 80;
            break;
        }
        case OPUS_FRAMESIZE_100_MS:
        {
            frame_len_in_ms = 100;
            break;
        }
        case OPUS_FRAMESIZE_120_MS:
        {
            frame_len_in_ms = 120;
            break;
        }
        default:
        {
            os_printf("invalid frame len:%d,use 20ms frame!\n",frame_len);
            frame_len_in_ms = 20;
            break;
        }
    }

    return frame_len_in_ms;
}


void cal_codec_input_size(opus_codec_para_t * opus_codec_para, uint8_t encode)
{
    uint8_t frame_len_in_ms;
    
    if(encode)
    {
        frame_len_in_ms = frame_len_mapping(opus_codec_para->enc_frame_size);
        opus_codec_para->enc_input_size_in_byte = opus_codec_para->adc_samp_rate*frame_len_in_ms/1000*2;
    }
    else
    {
        frame_len_in_ms = frame_len_mapping(opus_codec_para->dec_frame_size);
        opus_codec_para->dec_input_size_in_byte = opus_codec_para->dec_bitrate*frame_len_in_ms/1000/8;

        if(opus_codec_para->dec_vbr_en)
        {
            opus_codec_para->dec_input_size_in_byte = opus_codec_para->dec_input_size_in_byte*2;//double size in case of VBR
        }
    }
}

void print_opus_enc_config(opus_codec_para_t * opus_codec_para)
{
    os_printf("opus enc cfg:samp rate:%d,bitrate:%d,app:%s,frame(ms):%d,ch:%d\n",
             opus_codec_para->adc_samp_rate,
             opus_codec_para->enc_bitrate,
             app_mapping(opus_codec_para->enc_application),
             frame_len_mapping(opus_codec_para->enc_frame_size),
             opus_codec_para->enc_ch_num);
    os_printf("vbr:%d,vbr constraint:%d,complexity:%d,inband fec:%d,loss_perc:%d\n",
             opus_codec_para->enc_vbr_en,
             opus_codec_para->enc_vbr_constraint,
             opus_codec_para->enc_complexity,
             opus_codec_para->enc_inband_fec,
             opus_codec_para->enc_packet_loss_perc);
    os_printf("lsb_depth:%d,prediction_disable:%d,dtx:%d,force_chs:%d,input_size:%d\n\n",
             opus_codec_para->enc_lsb_depth,
             opus_codec_para->enc_prediction_disable,
             opus_codec_para->enc_dtx,
             opus_codec_para->enc_force_chs,
             opus_codec_para->enc_input_size_in_byte);
}

void print_opus_dec_config(opus_codec_para_t * opus_codec_para)
{
    os_printf("opus dec cfg:samp rate:%d,bitrate:%d,frame(ms):%d,ch:%d,vbr:%d,input_size:%d\n",
             opus_codec_para->dac_samp_rate,
             opus_codec_para->dec_bitrate,
             frame_len_mapping(opus_codec_para->dec_frame_size),
             opus_codec_para->dec_ch_num,
             opus_codec_para->dec_vbr_en,
             opus_codec_para->dec_input_size_in_byte);
}

void print_opus_codec_config(opus_codec_para_t * opus_codec_para, uint8_t encode)
{
    #if OPUS_LOOPBACK_TEST
    if(encode)
    {
        print_opus_enc_config(opus_codec_para);
    }
    else
    {
        print_opus_enc_config(opus_codec_para);
        print_opus_dec_config(opus_codec_para);
    }
    #else
    if(encode)
    {
        print_opus_enc_config(opus_codec_para);
    }
    else
    { 
        print_opus_dec_config(opus_codec_para);        
    }
    #endif
}

void print_mem_usage(void)
{
    #if CONFIG_FREERTOS
    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();
    void rtos_dump_stack_memory_usage(void);
    rtos_dump_stack_memory_usage();
    GLOBAL_INT_RESTORE();
    #endif

    #if CONFIG_FREERTOS && CONFIG_MEM_DEBUG
    os_dump_memory_stats(0, 0, NULL);
    #endif

    #if CONFIG_FREERTOS_V10
    extern void port_check_isr_stack(void);
    port_check_isr_stack();
    #endif
}

void opus_encoder_main(beken_thread_arg_t *thread_param)
{
    uint32 i;
    int32 pcm_data_size = OPUS_ENC_16K_INPUT_SIZE;
    uint8_t *pcm_addr = NULL;
    uint8_t *opus_out = NULL;
    int32_t encoder_len = 0;
    int error;
    OpusEncoder *enc = NULL;
    #if RUNTIME_MEAS
    uint32 start,end,int_mask;
    #endif
    int8 *p_enc_out;
    opus_codec_para_t *opus_codec_para = (opus_codec_para_t *)thread_param;

    cal_codec_input_size(opus_codec_para,1);

    pcm_addr = (uint8_t *)os_malloc(opus_codec_para->enc_input_size_in_byte);
    if (pcm_addr == NULL) {
        os_printf("malloc pcm_addr fail \r\n");
        return;
    }
    os_memset(pcm_addr, 0, opus_codec_para->enc_input_size_in_byte);

    opus_out = (uint8_t *)os_malloc(opus_codec_para->max_payload_size);
    if (opus_out == NULL) {
        os_printf("malloc opus_out fail \r\n");
        return;
    }
    os_memset(opus_out, 0, opus_codec_para->max_payload_size);


    enc = opus_encoder_create(opus_codec_para->adc_samp_rate, 
                              opus_codec_para->enc_ch_num, 
                              opus_codec_para->enc_application, 
                              &error);
    if (enc == NULL) {
        os_printf("creat fail \r\n");
        return;
    }

    opus_encoder_ctl(enc, OPUS_SET_BITRATE(opus_codec_para->enc_bitrate));
    opus_encoder_ctl(enc, OPUS_SET_FORCE_CHANNELS(opus_codec_para->enc_force_chs));
    opus_encoder_ctl(enc, OPUS_SET_VBR(opus_codec_para->enc_vbr_en));
    opus_encoder_ctl(enc, OPUS_SET_VBR_CONSTRAINT(opus_codec_para->enc_vbr_constraint));
    opus_encoder_ctl(enc, OPUS_SET_COMPLEXITY(opus_codec_para->enc_complexity));
    opus_encoder_ctl(enc, OPUS_SET_MAX_BANDWIDTH(opus_codec_para->enc_bandwidth));
    opus_encoder_ctl(enc, OPUS_SET_SIGNAL(opus_codec_para->enc_signal));
    opus_encoder_ctl(enc, OPUS_SET_INBAND_FEC(opus_codec_para->enc_inband_fec));
    opus_encoder_ctl(enc, OPUS_SET_PACKET_LOSS_PERC(opus_codec_para->enc_packet_loss_perc));
    opus_encoder_ctl(enc, OPUS_SET_LSB_DEPTH(opus_codec_para->enc_lsb_depth));
    opus_encoder_ctl(enc, OPUS_SET_PREDICTION_DISABLED(opus_codec_para->enc_prediction_disable));
    opus_encoder_ctl(enc, OPUS_SET_DTX(opus_codec_para->enc_dtx));
    opus_encoder_ctl(enc, OPUS_SET_EXPERT_FRAME_DURATION(opus_codec_para->enc_frame_size));
    os_printf("7\n");

    i = 0;    
    enc_cnt = 0;
    encoder_total_len = 0;
    p_enc_out = &opus_dec_input[0];

    print_opus_codec_config(opus_codec_para,1);
    os_printf("opus encode start!\r\n");
    
    while (pcm_data_size >= opus_codec_para->enc_input_size_in_byte)
    {
        os_memcpy(pcm_addr,&opus_enc_input[(opus_codec_para->enc_input_size_in_byte*i)],opus_codec_para->enc_input_size_in_byte);
        i++;

        #if RUNTIME_MEAS
        int_mask = rtos_disable_int();
        start = timer_hal_get_timer0_cnt();
        #endif
        encoder_len = opus_encode(enc, 
                                  (int16_t *)pcm_addr, 
                                  opus_codec_para->enc_input_size_in_byte/2, 
                                  opus_out, opus_codec_para->max_payload_size);
        #if RUNTIME_MEAS
        end = timer_hal_get_timer0_cnt();
        rtos_enable_int(int_mask);
        os_printf("opus enc %d,s:%u e:%u, dur:%d us,enc_len:%d\r\n", i-1,start,end,get_duration_clk_cycles(start,end)/26,encoder_len);
        #endif
        enc_len[i-1] = encoder_len;
        encoder_total_len += encoder_len;
        enc_cnt += 1;
        os_memcpy(p_enc_out,opus_out,encoder_len);
        p_enc_out += encoder_len;

        pcm_data_size -= opus_codec_para->enc_input_size_in_byte;
    }

    os_printf("opus enc input_s:%d,output_s:%d \r\n", opus_codec_para->enc_input_size_in_byte*i,encoder_total_len);

    #if MEM_USAGE
    print_mem_usage();
    #endif

    opus_encoder_destroy(enc);

    os_free(pcm_addr);
    os_free(opus_out);
    os_printf("encoder test complete \r\n");

    /* delete task */
    opus_enc_thread_handle = NULL;
    rtos_delete_thread(NULL);
}

void opus_decoder_main(beken_thread_arg_t *thread_param)
{
    uint32 i;

    uint8_t *opus_addr = NULL;
    uint8_t *pcm_out = NULL;
    int32_t decoder_len = 0;
    int32_t decoder_total_len = 0;
    int error;
    OpusDecoder *dec = NULL;
    #if OPUS_LOOPBACK_TEST
    int8 *p_dec_out = &opus_dec_output[0];
    #else
    int8 *p_dec_out = &opus_enc_input[0];
    #endif
    uint32 start,end,dec_input_total;
    #if RUNTIME_MEAS
    uint32 int_mask,start_time,end_time;
    #endif
    #if DEC_OUTPUT_DUMP
    int8 *p_dec_out_start;
    p_dec_out_start = p_dec_out;
    #endif

    opus_codec_para_t *opus_codec_para = (opus_codec_para_t *)thread_param;

    #if OPUS_LOOPBACK_TEST
    opus_codec_para->dac_samp_rate = opus_codec_para->adc_samp_rate;
    opus_codec_para->dec_bitrate = opus_codec_para->enc_bitrate;
    opus_codec_para->dec_frame_size = opus_codec_para->enc_frame_size;
    opus_codec_para->dec_ch_num = opus_codec_para->enc_ch_num;
    opus_codec_para->dec_vbr_en = opus_codec_para->enc_vbr_en;
    #endif
    
    cal_codec_input_size(opus_codec_para,0);

    opus_addr = (uint8_t *)os_malloc(opus_codec_para->dec_input_size_in_byte);

    if (opus_addr == NULL) {
        os_printf("malloc pcm_addr fail \r\n");
        return;
    }
    os_memset(opus_addr, 0, opus_codec_para->dec_input_size_in_byte);

    pcm_out = (uint8_t *)os_malloc(opus_codec_para->max_payload_size);
    if (pcm_out == NULL) {
        os_printf("malloc opus_out fail \r\n");
        return;
    }
    os_memset(pcm_out, 0, opus_codec_para->max_payload_size);

    dec = opus_decoder_create(opus_codec_para->dac_samp_rate, opus_codec_para->dec_ch_num, &error);
    if (dec == NULL) {
        os_printf("creat fail \r\n");
        return;
    }
    
    start = 0;
    end = 0;
    dec_input_total = 0;
    
    print_opus_codec_config(opus_codec_para,0);
    os_printf("opus decode start!\r\n");
   
#if OPUS_LOOPBACK_TEST
    for(i = 0; i < enc_cnt; i++)
    {
        end += enc_len[i];
        os_memcpy(opus_addr,&opus_dec_input[start],enc_len[i]);
        start = end;

        #if RUNTIME_MEAS
        int_mask = rtos_disable_int();
        start_time = timer_hal_get_timer0_cnt();
        #endif
        decoder_len = opus_decode(dec, opus_addr, enc_len[i], (int16_t *)pcm_out, opus_codec_para->max_payload_size, 0);
        #if RUNTIME_MEAS
        end_time = timer_hal_get_timer0_cnt();
        rtos_enable_int(int_mask);
        os_printf("opus dec %d,s:%u e:%u, dur:%d us\r\n", i,start_time,end_time,get_duration_clk_cycles(start_time,end_time)/26);
        #endif
        decoder_total_len += decoder_len;
        os_memcpy(p_dec_out,pcm_out,(decoder_len<<1));
        p_dec_out += (decoder_len<<1);
        dec_input_total += enc_len[i];
    }
#else //DECODE only
    for(i = 0; i < dec_cnt; i++)
    {
        end += dec_len[i];
        os_memcpy(opus_addr,&opus_dec_input[start],dec_len[i]);
        start = end;

        #if RUNTIME_MEAS
        int_mask = rtos_disable_int();
        start_time = timer_hal_get_timer0_cnt();
        #endif
        decoder_len = opus_decode(dec, opus_addr, dec_len[i], (int16_t *)pcm_out, opus_codec_para->max_payload_size, 0);
        #if RUNTIME_MEAS
        end_time = timer_hal_get_timer0_cnt();
        rtos_enable_int(int_mask);
        os_printf("opus dec %d,s:%d e:%d, dur:%d us\r\n", i,start_time,end_time,get_duration_clk_cycles(start_time,end_time)/26);
        #endif
        decoder_total_len += decoder_len;
        os_memcpy(p_dec_out,pcm_out,(decoder_len<<1));
        p_dec_out += (decoder_len<<1);
        dec_input_total += dec_len[i];
    }
#endif
    os_printf("opus dec input_s:%d,output_s:%d \r\n", dec_input_total,(decoder_total_len<<1));

    #if DEC_OUTPUT_DUMP
    os_printf("dec output dump start!\r\n");
    BK_DUMP_RAW_OUT((char *)p_dec_out_start,(decoder_total_len<<1));
    os_printf("dec output dump end!\r\n");
    #endif

    #if MEM_USAGE
    print_mem_usage();
    #endif

    opus_decoder_destroy(dec);

    os_free(opus_addr);
    os_free(pcm_out);
    os_printf("decoder test complete \r\n");

    /* delete task */
    opus_dec_thread_handle = NULL;
    rtos_delete_thread(NULL);
}

static void cli_audio_opus_help(void)
{
    os_printf("Usage: opus_encoder_test [options]\n");
    os_printf("       opus_decoder_test [options] \n\n");

    os_printf("options:\n" );
    os_printf("-app                 : <voip|audio|restricted-lowdelay>\n" );
    os_printf("-sampling-rate       : <8000|12000|16000|24000|48000>\n" );
    os_printf("-ch                  : <1|2>\n" );
    os_printf("-bitrate             : <>1000>\n" );
    os_printf("-cbr                 : enable constant bitrate; default: variable bitrate\n" );
    os_printf("-cvbr                : enable constrained variable bitrate; default: unconstrained\n" );
    os_printf("-delayed-decision    : use look-ahead for speech/music detection (experts only); default: disabled\n" );
    os_printf("-bandwidth <NB|MB|WB|SWB|FB> : audio bandwidth (from narrowband to fullband); default: sampling rate\n" );
    os_printf("-framesize <2.5|5|10|20|40|60|80|100|120> : frame size in ms; default: 20 \n" );
    os_printf("-max_payload <bytes> : maximum payload size in bytes,max:4000, default: 1024\n" );
    os_printf("-complexity <comp>   : complexity, 0 (lowest) ... 10 (highest); default: 10\n" );
    os_printf("-inbandfec           : enable SILK inband FEC\n" );
    os_printf("-forcemono           : force mono encoding, even for stereo input\n" );
    os_printf("-dtx                 : enable SILK DTX\n" );
    os_printf("-loss <perc>         : simulate packet loss, in percent (0-100); default: 0\n" );
}


void cli_opus_encoder_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = BK_OK;
    int args = 1;

    while( args < argc) {
        /* process command line options */
        if( os_strcasecmp( argv[ args ], "-app" ) == 0 ) {
            if (os_strcasecmp(argv[args+1], "voip")==0)
                opus_codec_setup.enc_application = OPUS_APPLICATION_VOIP;
            else if (os_strcasecmp(argv[args+1], "restricted-lowdelay")==0)
                opus_codec_setup.enc_application = OPUS_APPLICATION_RESTRICTED_LOWDELAY;
            else if (os_strcasecmp(argv[args+1], "audio")!=0) {
                cli_audio_opus_help();
              goto failure;
            }
            args += 2;
        }else if( os_strcasecmp( argv[ args ], "-sampling_rate" ) == 0 ) {
            opus_codec_setup.adc_samp_rate = os_strtoul(argv[args+1], NULL, 10);
            if (opus_codec_setup.adc_samp_rate != 8000 && opus_codec_setup.adc_samp_rate != 12000
             && opus_codec_setup.adc_samp_rate != 16000 && opus_codec_setup.adc_samp_rate != 24000
             && opus_codec_setup.adc_samp_rate != 48000)
            {
                os_printf("Supported sampling rates are 8000, 12000, 16000, 24000 and 48000.\n");
                goto failure;
            }
            args += 2;
        }else if( os_strcasecmp( argv[ args ], "-ch" ) == 0 ) {
            opus_codec_setup.enc_ch_num = os_strtoul(argv[args+1], NULL, 10);
            if (opus_codec_setup.enc_ch_num < 1 || opus_codec_setup.enc_ch_num > 2)
            {
                os_printf("Opus_demo supports only 1 or 2 channels.\n");
                goto failure;
            }
            args += 2;
        }else if( os_strcasecmp( argv[ args ], "-bitrate" ) == 0 ) {
            opus_codec_setup.enc_bitrate = os_strtoul(argv[args+1], NULL, 10);
            args += 2;
        }else if( os_strcasecmp( argv[ args ], "-cbr" ) == 0 ) {
            opus_codec_setup.enc_vbr_en = 0;
            args++;
        }else if( os_strcasecmp( argv[ args ], "-bandwidth" ) == 0 ) {
            if (os_strcasecmp(argv[ args + 1 ], "NB")==0)
                opus_codec_setup.enc_bandwidth = OPUS_BANDWIDTH_NARROWBAND;
            else if (os_strcasecmp(argv[ args + 1 ], "MB")==0)
                opus_codec_setup.enc_bandwidth = OPUS_BANDWIDTH_MEDIUMBAND;
            else if (os_strcasecmp(argv[ args + 1 ], "WB")==0)
                opus_codec_setup.enc_bandwidth = OPUS_BANDWIDTH_WIDEBAND;
            else if (os_strcasecmp(argv[ args + 1 ], "SWB")==0)
                opus_codec_setup.enc_bandwidth = OPUS_BANDWIDTH_SUPERWIDEBAND;
            else if (os_strcasecmp(argv[ args + 1 ], "FB")==0)
                opus_codec_setup.enc_bandwidth = OPUS_BANDWIDTH_FULLBAND;
            else {
                os_printf("Unknown bandwidth %s. "
                                "Supported are NB, MB, WB, SWB, FB.\n",
                                argv[ args + 1 ]);
                goto failure;
            }
            args += 2;
        } else if( os_strcasecmp( argv[ args ], "-framesize" ) == 0 ) {
            if (os_strcasecmp(argv[ args + 1 ], "2.5")==0)
                opus_codec_setup.enc_frame_size = OPUS_FRAMESIZE_2_5_MS;
            else if (os_strcasecmp(argv[ args + 1 ], "5")==0)
                opus_codec_setup.enc_frame_size = OPUS_FRAMESIZE_5_MS;
            else if (os_strcasecmp(argv[ args + 1 ], "10")==0)
                opus_codec_setup.enc_frame_size = OPUS_FRAMESIZE_10_MS;
            else if (os_strcasecmp(argv[ args + 1 ], "20")==0)
                opus_codec_setup.enc_frame_size = OPUS_FRAMESIZE_20_MS;
            else if (os_strcasecmp(argv[ args + 1 ], "40")==0)
                opus_codec_setup.enc_frame_size = OPUS_FRAMESIZE_40_MS;
            else if (os_strcasecmp(argv[ args + 1 ], "60")==0)
                opus_codec_setup.enc_frame_size = OPUS_FRAMESIZE_60_MS;
            else if (os_strcasecmp(argv[ args + 1 ], "80")==0)
                opus_codec_setup.enc_frame_size = OPUS_FRAMESIZE_80_MS;
            else if (os_strcasecmp(argv[ args + 1 ], "100")==0)
                opus_codec_setup.enc_frame_size = OPUS_FRAMESIZE_100_MS;
            else if (os_strcasecmp(argv[ args + 1 ], "120")==0)
                opus_codec_setup.enc_frame_size = OPUS_FRAMESIZE_120_MS;
            else {
                os_printf("Unsupported frame size: %s ms. Supported are 2.5, 5, 10, 20, 40, 60, 80, 100, 120.\n",
                                argv[ args + 1 ]);
                goto failure;
            }
            args += 2;
        } else if( os_strcasecmp( argv[ args ], "-max_payload" ) == 0 ) {
            opus_codec_setup.max_payload_size = os_strtoul(argv[args+1], NULL, 10);
            args += 2;
        } else if( os_strcasecmp( argv[ args ], "-complexity" ) == 0 ) {
            opus_codec_setup.enc_complexity = os_strtoul(argv[args+1], NULL, 10);
            args += 2;
        } else if( os_strcasecmp( argv[ args ], "-inbandfec" ) == 0 ) {
            opus_codec_setup.enc_inband_fec = 1;
            args++;
        } else if( os_strcasecmp( argv[ args ], "-forcemono" ) == 0 ) {
            opus_codec_setup.enc_force_chs = 1;
            args++;
        } else if( os_strcasecmp( argv[ args ], "-cvbr" ) == 0 ) {
            opus_codec_setup.enc_vbr_constraint = 1;
            args++;
        } else if( os_strcasecmp( argv[ args ], "-dtx") == 0 ) {
            opus_codec_setup.enc_dtx = 1;
            args++;
        } else if( os_strcasecmp( argv[ args ], "-loss" ) == 0 ) {
            opus_codec_setup.enc_packet_loss_perc = os_strtoul(argv[args+1], NULL, 10);
            args += 2;
        } else {
            os_printf( "Error: unrecognized setting: %s\n\n", argv[ args ] );
            cli_audio_opus_help();
            goto failure;
        }
    }

    #if TASK_SRAM
    ret = rtos_create_sram_thread(&opus_enc_thread_handle,
                                   BEKEN_DEFAULT_WORKER_PRIORITY,
                                   "opus_enc",
                                   (beken_thread_function_t)opus_encoder_main,
                                   1024*30,
                                   &opus_codec_setup);
    #else
    ret = rtos_create_psram_thread(&opus_enc_thread_handle,
                                   BEKEN_DEFAULT_WORKER_PRIORITY,
                                   "opus_enc",
                                   (beken_thread_function_t)opus_encoder_main,
                                   1024*30,
                                   &opus_codec_setup);
    #endif
    
    if (ret != kNoErr) {
        os_printf("Error: Failed to create opus encoder thread: %d\r\n",
                  ret);
    }

    os_printf("test finish \r\n");

failure:
    return;
}

void cli_opus_decoder_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = BK_OK;
    int args = 1;

    while( args < argc) {
        /* process command line options */
        if( os_strcasecmp( argv[ args ], "-sampling_rate" ) == 0 ) {
            opus_codec_setup.dac_samp_rate = os_strtoul(argv[args+1], NULL, 10);
            if (opus_codec_setup.dac_samp_rate != 8000 && opus_codec_setup.dac_samp_rate != 12000
             && opus_codec_setup.dac_samp_rate != 16000 && opus_codec_setup.dac_samp_rate != 24000
             && opus_codec_setup.dac_samp_rate != 48000)
            {
                os_printf("Supported sampling rates are 8000, 12000, 16000, 24000 and 48000.\n");
                goto failure;
            }
            args += 2;
        }else if( os_strcasecmp( argv[ args ], "-ch" ) == 0 ) {
            opus_codec_setup.dec_ch_num = os_strtoul(argv[args+1], NULL, 10);
            if (opus_codec_setup.dec_ch_num < 1 || opus_codec_setup.dec_ch_num > 2)
            {
                os_printf("Opus_demo supports only 1 or 2 channels.\n");
                goto failure;
            }
            args += 2;
        }else if( os_strcasecmp( argv[ args ], "-bitrate" ) == 0 ) {
            opus_codec_setup.dec_bitrate = os_strtoul(argv[args+1], NULL, 10);
            args += 2;
        }else if( os_strcasecmp( argv[ args ], "-framesize" ) == 0 ) {
            if (os_strcasecmp(argv[ args + 1 ], "2.5")==0)
                opus_codec_setup.dec_frame_size = OPUS_FRAMESIZE_2_5_MS;
            else if (os_strcasecmp(argv[ args + 1 ], "5")==0)
                opus_codec_setup.dec_frame_size = OPUS_FRAMESIZE_5_MS;
            else if (os_strcasecmp(argv[ args + 1 ], "10")==0)
                opus_codec_setup.dec_frame_size = OPUS_FRAMESIZE_10_MS;
            else if (os_strcasecmp(argv[ args + 1 ], "20")==0)
                opus_codec_setup.dec_frame_size = OPUS_FRAMESIZE_20_MS;
            else if (os_strcasecmp(argv[ args + 1 ], "40")==0)
                opus_codec_setup.dec_frame_size = OPUS_FRAMESIZE_40_MS;
            else if (os_strcasecmp(argv[ args + 1 ], "60")==0)
                opus_codec_setup.dec_frame_size = OPUS_FRAMESIZE_60_MS;
            else if (os_strcasecmp(argv[ args + 1 ], "80")==0)
                opus_codec_setup.dec_frame_size = OPUS_FRAMESIZE_80_MS;
            else if (os_strcasecmp(argv[ args + 1 ], "100")==0)
                opus_codec_setup.dec_frame_size = OPUS_FRAMESIZE_100_MS;
            else if (os_strcasecmp(argv[ args + 1 ], "120")==0)
                opus_codec_setup.dec_frame_size = OPUS_FRAMESIZE_120_MS;
            else {
                os_printf("Unsupported frame size: %s ms. Supported are 2.5, 5, 10, 20, 40, 60, 80, 100, 120.\n",
                                argv[ args + 1 ]);
                goto failure;
            }
            args += 2;
        }else if( os_strcasecmp( argv[ args ], "-cbr" ) == 0 ) {
            opus_codec_setup.dec_vbr_en = 0;
            args++;
        }else {
            os_printf( "Error: unrecognized setting: %s\n\n", argv[ args ] );
            cli_audio_opus_help();
            goto failure;
        }
    }

    #if TASK_SRAM
    ret = rtos_create_sram_thread(&opus_dec_thread_handle,
                                   BEKEN_DEFAULT_WORKER_PRIORITY,
                                   "opus_dec",
                                   (beken_thread_function_t)opus_decoder_main,
                                   1024*10,
                                   &opus_codec_setup);
    #else
    ret = rtos_create_psram_thread(&opus_dec_thread_handle,
                                   BEKEN_DEFAULT_WORKER_PRIORITY,
                                   "opus_dec",
                                   (beken_thread_function_t)opus_decoder_main,
                                   1024*10,
                                   &opus_codec_setup);
    #endif
    if (ret != kNoErr) {
        os_printf("Error: Failed to create opus decoder thread: %d\r\n",
                  ret);
    }

    os_printf("test finish \r\n");

failure:
        return;

}





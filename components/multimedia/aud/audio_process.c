#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <modules/audio_process.h>
#include <components/log.h>
#include "audio_default_para.h"
#include "audio_debug.h"


static vad_delay_ring_buf_t vad_delay_ring_buf = {0};
static int cur_vad_flag = 0x0;

void vad_delay_ring_buf_init(uint32_t samp_rate_points,uint32_t delay_num) {
	uint32_t vad_delay_num = 0;
	vad_delay_num = delay_num >= VAD_DELAY_BUF_NUM ? (VAD_DELAY_BUF_NUM-1): delay_num ;

    vad_delay_ring_buf.read_ptr = 0;
    vad_delay_ring_buf.write_ptr = vad_delay_num;
    vad_delay_ring_buf.vad_read_ptr = 0;
    vad_delay_ring_buf.vad_write_ptr = 0;
	os_printf("vad_delay_ring_buf_init: vad_delay_num =%d\r\n",vad_delay_num);
}

void vad_delay_ring_buf_deinit(void) {

}
void vad_delay_ring_buf_write( int16_t *data, uint32_t samp_rate_points, int16_t vad_flag) {

	vad_delay_ring_buf.write_ptr  = (vad_delay_ring_buf.write_ptr + 1) % VAD_DELAY_BUF_NUM;
	vad_delay_ring_buf.vad_write_ptr  = (vad_delay_ring_buf.vad_write_ptr + 1) % VAD_DELAY_BUF_NUM;
	vad_delay_ring_buf.vad_flag[vad_delay_ring_buf.vad_write_ptr] = vad_flag;
	os_memcpy(&vad_delay_ring_buf.vad_delay_buf[samp_rate_points* vad_delay_ring_buf.write_ptr],data,samp_rate_points*2);

}

void vad_delay_ring_buf_read(int16_t *data, uint32_t samp_rate_points) {

	vad_delay_ring_buf.read_ptr  = (vad_delay_ring_buf.read_ptr + 1) % VAD_DELAY_BUF_NUM;
	vad_delay_ring_buf.vad_read_ptr  = (vad_delay_ring_buf.vad_read_ptr + 1) % VAD_DELAY_BUF_NUM;
	if(vad_delay_ring_buf.vad_flag[vad_delay_ring_buf.vad_read_ptr] == 2) {
		os_memset(data,0,samp_rate_points*2);
	}
	else
	{
	    os_memcpy(data,&vad_delay_ring_buf.vad_delay_buf[samp_rate_points* vad_delay_ring_buf.read_ptr],samp_rate_points*2);
	}
}
app_aud_para_t aud_para = {
    .sys_config_voice = DEFAULT_SYS_CONFIG_VOICE(),
    .eq_dl_voice = DEFAULT_EQ_PARA_DL_VOICE(),
    .eq_ul_voice = DEFAULT_EQ_PARA_UL_VOICE(),
    .aec_config_voice = DEFAULT_AEC_CONFIG_VOICE(),
};

void voice_dl_process_init(void)
{
    app_aud_eq_init(&aud_para.eq_dl_voice,EQ_ID_DL_VOICE);
}
void voice_ul_process_init(void)
{

    app_aud_eq_init(&aud_para.eq_ul_voice,EQ_ID_UL_VOICE);

}

void voice_process_init()
{
    voice_dl_process_init();
    voice_ul_process_init();
    dump_aud_para();
}

void voice_dl_process(int16 *buf, uint32 sample_points)
{
    if(aud_para.eq_dl_voice.eq_en)
    {
         app_aud_eq_process(buf, sample_points, EQ_ID_DL_VOICE);
    }
}

void voice_ul_pre_process(int16 *buf, uint32 sample_points)
{
	//todo
}

void voice_ul_post_process(int16 *buf, uint32 sample_points,int16 vad_flag,int16 *debug_buf)
{
		if(vad_flag)
		{
			cur_vad_flag = vad_flag;
		}
		if(debug_buf)
		{
			if(cur_vad_flag == 1)
			{
				os_memset(debug_buf,0x11,640);
			}else if(cur_vad_flag == 2)
			{
				os_memset(debug_buf,0x0,640);
			}else if(cur_vad_flag == 3)
			{
				os_memset(debug_buf,0x33,640);
			}
		}

		vad_delay_ring_buf_write(buf, sample_points,cur_vad_flag);
		vad_delay_ring_buf_read(buf, sample_points);
}
bk_err_t audio_para_init(app_aud_para_t *aud_para_ptr)
{
    os_memcpy(&aud_para, aud_para_ptr, sizeof(app_aud_para_t));
    bk_printf("audio_para_init ok\r\n");
    return BK_OK;
}

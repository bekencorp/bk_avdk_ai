#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <modules/audio_process.h>
#include <components/log.h>
#include "audio_para.h"
#include "audio_debug.h"


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

void voice_ul_post_process(int16 *buf, uint32 sample_points)
{
    if(aud_para.eq_ul_voice.eq_en)
    {
		//todo
        //app_aud_eq_process(buf, sample_points, EQ_ID_UL_VOICE);
    }

}
bk_err_t audio_para_init(app_aud_para_t *aud_para_ptr)
{
    os_memcpy(&aud_para, aud_para_ptr, sizeof(app_aud_para_t));
    bk_printf("audio_para_init ok\r\n");
    return BK_OK;
}

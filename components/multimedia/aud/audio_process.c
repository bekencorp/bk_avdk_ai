#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <modules/audio_process.h>
#include "audio_para.h"



app_aud_para_t aud_para = {
    .eq_dl_voice = DEFAULT_EQ_PARA_DL_VOICE(),
};

void voice_dl_process_init(void)
{
    //app_aud_eq_init(&aud_para.eq_dl_voice,EQ_ID_DL_VOICE);
    bk_printf("voice_dl_process_init\r\n" );

}

void voice_dl_process_deinit(void)
{
    
}

void voice_dl_process(int16 *buf, uint32 sample_points)
{
    app_aud_eq_process(buf, sample_points, EQ_ID_DL_VOICE);
}

bk_err_t audio_para_init(app_aud_para_t *aud_para_ptr)
{
    os_memcpy(&aud_para, aud_para_ptr, sizeof(app_aud_para_t));
    return BK_OK;
}

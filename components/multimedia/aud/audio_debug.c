#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <modules/audio_process.h>
#include <components/log.h>
#include "audio_debug.h"

#define TAG "aud_dbg"
#define AUD_DBG_TOOL_PRT(...) BK_LOGD(TAG, ##__VA_ARGS__)



extern void bk_aud_debug_register_update_dl_eq_para_cb(bk_aud_intf_update_dl_eq_para_cb_t dl_eq_para_cb);
extern void bk_aud_debug_register_update_aec_config_cb(bk_aud_intf_update_aec_config_cb_t aec_config_cb);
extern void bk_aud_debug_register_update_sys_config_cb(bk_aud_intf_update_sys_config_cb_t sys_config_cb);
extern void aud_tras_update_dl_eq_para(app_eq_t *dl_eq_para_ptr);
extern void aud_tras_update_aec_para(app_aud_aec_config_t *aec_para_ptr);
extern void aud_tras_update_sys_config_para(app_aud_sys_config_t *sys_config_para_ptr);
extern bk_err_t bk_aud_intf_update_dl_eq_para(app_eq_t *dl_eq_para_ptr);
extern bk_err_t bk_aud_intf_update_aec_para(app_aud_aec_config_t *aec_para_ptr);
extern bk_err_t bk_aud_intf_update_sys_config_para(app_aud_sys_config_t *aud_sys_config_ptr);


static  app_eq_t eq_dbg_eq;


void audio_debug_init()
{
 
    #if CONFIG_SYS_CPU0
    
    bk_aud_debug_register_update_dl_eq_para_cb(bk_aud_intf_update_dl_eq_para);
    bk_aud_debug_register_update_aec_config_cb(bk_aud_intf_update_aec_para);
    bk_aud_debug_register_update_sys_config_cb(bk_aud_intf_update_sys_config_para);
    AUD_DBG_TOOL_PRT("audio_debug_init ok\r\n");
    #endif
}


void dump_aud_sys_config_voice()
{
    AUD_DBG_TOOL_PRT("dump sys_config_voice:\r\n mic0_digital_gain:0x%x\r\n mic0_ana_gain:0x%x\r\n mic1_ana_gain:0x%x\r\n dac_ana_gain:0x%x\r\n main_mic_sel:0x%x\r\n",\
               aud_para.sys_config_voice.mic0_digital_gain,\
               aud_para.sys_config_voice.mic0_analog_gain,\
               aud_para.sys_config_voice.mic1_analog_gain,\
               aud_para.sys_config_voice.speaker_chan0_analog_gain,\
               aud_para.sys_config_voice.main_mic_select);

}
void dump_aec_config_voice()
{
    AUD_DBG_TOOL_PRT("\r\n dump aec_config_voice:\r\n aec_en:0x%x\r\n init_flags:0x%x\r\n ec_filter:0x%x\r\n ns_filter:0x%x\r\n ref_scale:0x%x\r\n \
ec_depth:0x%x\r\n mic_delay:%d\r\n drc_gain:%d\r\n voice_vol:%d\r\n ns_level:%d\r\n ns_para:%d\r\n ns_type0x%x\r\n \
vad_en:0x%x\r\n vad_start_thr:%d\r\n vad_stop_thr:%d\r\n vad_silence_thr:%d\r\n vad_eng_threshold:%d\r\n",\
                aud_para.aec_config_voice.aec_enable,aud_para.aec_config_voice.init_flags,aud_para.aec_config_voice.ec_filter,\
                aud_para.aec_config_voice.ns_filter,aud_para.aec_config_voice.ref_scale,aud_para.aec_config_voice.ec_depth,\
                aud_para.aec_config_voice.mic_delay,aud_para.aec_config_voice.drc_gain,aud_para.aec_config_voice.voice_vol,\
                aud_para.aec_config_voice.ns_level,aud_para.aec_config_voice.ns_para,aud_para.aec_config_voice.ns_type,\
                aud_para.aec_config_voice.vad_enable,aud_para.aec_config_voice.vad_start_threshold,\
                aud_para.aec_config_voice.vad_stop_threshold,aud_para.aec_config_voice.vad_silence_threshold,\
                aud_para.aec_config_voice.vad_eng_threshold);
}

void dump_aud_eq_dl_voice()
{
    uint32 i;
    AUD_DBG_TOOL_PRT("dump eq_dl_voice:\r\n eq_en:0x%x\r\n globle_gain:0x%x\r\n filters:%d\r\n",\
                aud_para.eq_dl_voice.eq_en,aud_para.eq_dl_voice.globle_gain,aud_para.eq_dl_voice.filters);
    for(i=0;i < aud_para.eq_dl_voice.filters;i++)
    {
        AUD_DBG_TOOL_PRT("eq_para[%d].a[0]=%d\r\n eq_para[%d].a[1]=%d\r\n eq_para[%d].b[0]=%d\r\n eq_para[%d].b[1]=%d\r\n eq_para[%d].b[2]=%d\r\n",\
                i,aud_para.eq_dl_voice.eq_para[i].a[0],i,aud_para.eq_dl_voice.eq_para[i].a[1],\
                i,aud_para.eq_dl_voice.eq_para[i].b[0],i,aud_para.eq_dl_voice.eq_para[i].b[1],i,aud_para.eq_dl_voice.eq_para[i].b[2]);
    }
}
void dump_aud_para()
{
    dump_aud_sys_config_voice();
    dump_aec_config_voice();
    dump_aud_eq_dl_voice();    
}

void app_eq_dbg(uint8_t* params)
{
    uint16_t enable;
    uint16_t total_gain;
    uint8_t eqType;
    uint8_t index;
    uint32  i;

    app_eq_t *eq_dbg_eq_para = &eq_dbg_eq;


    AUD_DBG_TOOL_PRT("app_eq_dbg 0x%x\r\n", params[0]);
	switch(params[0])
	{
	case 0xFA:

		break;
	case 0xFB:
        index = params[1];
        enable =params[2];  
        eq_dbg_eq_para->eq_para[index].a[0] = (params[3] | params[4]<<8 | params[5]<<8 | params[6]<<8);
        eq_dbg_eq_para->eq_para[index].a[1] = (params[7] | params[8]<<8 | params[9]<<8 | params[10]<<8);
        eq_dbg_eq_para->eq_para[index].b[0] = (params[11] | params[12]<<8 | params[13]<<8 | params[14]<<8);
        eq_dbg_eq_para->eq_para[index].b[1] = (params[15] | params[16]<<8 | params[17]<<8 | params[18]<<8);
        eq_dbg_eq_para->eq_para[index].b[2] = (params[19] | params[20]<<8 | params[21]<<8 | params[22]<<8);
        AUD_DBG_TOOL_PRT("eq_dbg index=%d, enable=%d,a[0]=%d,a[1]=%d,b[0]=%d,b[1]=%d,b[2]=%d\r\n",index,enable,\
                    eq_dbg_eq_para->eq_para[index].a[0],eq_dbg_eq_para->eq_para[index].a[1],\
                    eq_dbg_eq_para->eq_para[index].b[0],eq_dbg_eq_para->eq_para[index].b[1],eq_dbg_eq_para->eq_para[index].b[2]);
        if(enable)
        {
            eq_dbg_eq_para->filters++;
            eq_dbg_eq_para->eq_en = 1;
        }
		break;
	case 0xFC:

		break;
	case 0xFD:
		if(params[1] == 0x01)
        {
            eq_dbg_eq_para->filters = 0;
            AUD_DBG_TOOL_PRT("eq_dbg start\r\n");
        }
        else if(params[1] == 0x00)
        {
            AUD_DBG_TOOL_PRT("eq_dbg enable=%d filters=%d\r\n",eq_dbg_eq_para->eq_en,eq_dbg_eq_para->filters);
            for(i = 0; i < eq_dbg_eq_para->filters; i++)
            {
                AUD_DBG_TOOL_PRT("eq_dbg a[0]=%d,a[1]=%d,b[0]=%d,b[1]=%d,b[2]=%d\r\n",eq_dbg_eq_para->eq_para[i].a[0],\
                        eq_dbg_eq_para->eq_para[i].a[1],eq_dbg_eq_para->eq_para[i].b[0],eq_dbg_eq_para->eq_para[i].b[1],\
                        eq_dbg_eq_para->eq_para[i].b[2]);

            }
            
        }
		break;
	case 0xFE:
        enable = (params[2] << 8 | params[1]);
        total_gain = (params[4] << 8 | params[3]);
        eqType = params[5];
        AUD_DBG_TOOL_PRT("eq_dbg enable:%d, total_gain:%d, eqType:%d\r\n", enable, total_gain, eqType);
		break;
	default: break;
	}
}


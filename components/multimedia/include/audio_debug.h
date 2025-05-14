#pragma once

#ifdef __cplusplus
extern "C" {
#endif



#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <modules/audio_process.h>

#ifdef CLI_DEBUG
#define CLI_PRT       os_printf
#define CLI_WPRT      warning_prf
#else
#define CLI_PRT       os_null_printf
#define CLI_WPRT      os_null_printf
#endif


void bk_aud_debug_get_audpara(app_aud_para_t * aud_para_ptr);
void bk_aud_debug_register_update_dl_eq_para_cb(bk_aud_intf_update_dl_eq_para_cb_t dl_eq_para_cb);
void bk_aud_debug_register_update_aec_config_cb(bk_aud_intf_update_aec_config_cb_t aec_config_cb);
void bk_aud_debug_register_update_sys_config_cb(bk_aud_intf_update_sys_config_cb_t sys_config_cb);
void audio_intf_debug_init();
void dump_aud_sys_config_voice();
void dump_aec_config_voice();
void dump_aud_eq_dl_voice();
void dump_aud_para();

#ifdef __cplusplus
}
#endif
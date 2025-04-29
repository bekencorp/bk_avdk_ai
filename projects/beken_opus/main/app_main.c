#include <common/sys_config.h>
#include <components/log.h>
#include <components/event.h>
#include <string.h>

#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "cli.h"
//#include "media_service.h"
#include <driver/pwr_clk.h>
#include <driver/pwr_clk.h>
#include <modules/pm.h>

#include "sys_driver.h"
#include "sys_hal.h"
#include <driver/gpio.h>
#include "gpio_driver.h"

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);
bk_err_t bk_audio_osi_funcs_init(void);


void user_app_main(void)
{
#if (CONFIG_SYS_CPU0)
    bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_240M);
#endif

}

int main(void)
{
    //if (bk_misc_get_reset_reason() != RESET_SOURCE_FORCE_DEEPSLEEP)
    {
#if (CONFIG_SYS_CPU0)
        rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
#endif
        bk_init();

#if (CONFIG_SYS_CPU0)   
        bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_AUDP_AUDIO, PM_POWER_MODULE_STATE_ON);
#endif

#if (CONFIG_SYS_CPU1) 
        bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_480M);
#endif

        //media_service_init();
        bk_audio_osi_funcs_init();
    }

    return 0;
}

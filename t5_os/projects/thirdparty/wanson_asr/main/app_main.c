#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "cli.h"
#include "wanson_asr.h"
#include <driver/pwr_clk.h>


extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);

#ifdef CONFIG_CACHE_CUSTOM_SRAM_MAPPING
const unsigned int g_sram_addr_map[4] =
{
	0x38000000,
	0x30020000,
	0x38020000,
	0x30000000
};
#endif


#if (CONFIG_SYS_CPU0)
void user_app_main(void)
{
	//bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_AUDP_AUDIO, PM_POWER_MODULE_STATE_ON);
}
#endif


int main(void)
{
#if (CONFIG_SYS_CPU0)
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
	// bk_set_printf_sync(true);
	// shell_set_log_level(BK_LOG_WARN);
#endif
	bk_init();
	extern int media_service_init(void);
	media_service_init();

#if (CONFIG_SYS_CPU0)
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_AUDP_AUDIO, PM_POWER_MODULE_STATE_ON);
#endif

#if ((CONFIG_ASDF_WORK_CPU1 && CONFIG_SYS_CPU1) || (CONFIG_ASDF_WORK_CPU0 && CONFIG_SYS_CPU0))
	//rtos_delay_milliseconds(5000);
	wanson_asr_init();
	wanson_asr_start();
#endif
	return 0;
}

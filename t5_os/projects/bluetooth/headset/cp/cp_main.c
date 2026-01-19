#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <modules/pm.h>
#include <driver/pwr_clk.h>
#include <components/ate.h>

extern void rtos_set_user_app_entry(beken_thread_function_t entry);

void user_app_main(void) {
	if (!ate_is_enabled())
    {
    	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_APP,PM_POWER_MODULE_STATE_ON);
	}
}



int main(void)
{
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
	bk_init();

	return 0;
}
#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <modules/pm.h>
#include <driver/pwr_clk.h>
#include <components/ate.h>

#include "FreeRTOS.h"
#include "task.h"
#include "tuya_cloud_types.h"

extern OPERATE_RET tuya_ipc_init(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);

void user_app_main(void)
{
    if (!ate_is_enabled()) {
        bk_printf_raw(BK_LOG_INFO, NULL, "-------- co-procresser startup, left heap: %d, reset reason: %x\r\n",
                xPortGetFreeHeapSize(), bk_misc_get_reset_reason() & 0xFF);

        // cifd_cust_msg_init();
        // doorbell_core_init();
        bk_printf_raw(BK_LOG_INFO, NULL, "start cp1\r\n");
        bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_APP,PM_POWER_MODULE_STATE_ON);

        // tuya_ipc_init();

#if (CONFIG_TUYA_TEST_CLI)
        extern int cp_cli_tuya_test_init(void);
        cp_cli_tuya_test_init();
#endif
    } else {
        bk_printf_raw(BK_LOG_INFO, NULL, "ate enable\r\n");
    }
}

int main(void)
{
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
	bk_init();

	return 0;
}

#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <modules/pm.h>
#include <driver/pwr_clk.h>
#include <media_service.h>

#include "FreeRTOS.h"
#include "task.h"
#include "tuya_cloud_types.h"

extern OPERATE_RET tuya_ipc_init(void);
extern int tuya_upgrade_main(void);
extern void tuya_app_main(void);
extern TUYA_OTA_PATH_E tkl_ota_is_under_seg_upgrade(void);
extern void test_core_mark(void);

static void entry_app_main(void)
{
    bk_printf("-------- app startup, left heap: %d, reset reason: %x\r\n",
            xPortGetFreeHeapSize(), bk_misc_get_reset_reason() & 0xFF);

    tuya_ipc_init();
    
    bk_printf("go to tuya\r\n");
    tuya_app_main();
    // test_core_mark();

#if (CONFIG_TUYA_TEST_CLI)
        extern int ap_cli_tuya_test_init(void);
        ap_cli_tuya_test_init();
#endif
}

int main(void)
{
    bk_init();
    media_service_init();

    entry_app_main();

    return 0;
}

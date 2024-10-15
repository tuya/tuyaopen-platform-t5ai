#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>

#include "cli.h"
#if CONFIG_SOC_BK7256XX
#else
#include "media_service.h"
#endif

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


void user_app_main(void)
{
}

int main(void)
{
#if (CONFIG_SYS_CPU0)
    rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
    // bk_set_printf_sync(true);
    // shell_set_log_level(BK_LOG_WARN);
#endif
    bk_init();

#if CONFIG_SOC_BK7256XX
#else
    media_service_init();
#endif

    rtos_delay_milliseconds(500);

#if CONFIG_SYS_CPU0

#if CONFIG_BT
    extern int cli_a2dp_source_demo_init(void);
    cli_a2dp_source_demo_init();
#endif

#if CONFIG_BLE
    extern int cli_ble_gatt_demo_init(void);
    cli_ble_gatt_demo_init();

#endif
#endif
    return 0;
}

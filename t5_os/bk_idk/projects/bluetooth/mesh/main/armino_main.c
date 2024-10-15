#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>

#include "cli.h"

#include "mesh_demo_cli.h"

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

uint8_t host_init_finish(int32_t reason)
{
    os_printf("%s reason %d\n", __func__, reason);
    return 0;
}

int main(void)
{
#if (CONFIG_SYS_CPU0)
    rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
    // bk_set_printf_sync(true);
    // shell_set_log_level(BK_LOG_WARN);
#endif
    bk_init();

    hci_driver_init();

    freertos_zephyr_port_init();
    init_mem_slab_module();

    zephyr_ble_mesh_init(host_init_finish);

    cli_ble_mesh_demo_init();

    return 0;
}

#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "bk_private/bk_wifi.h"
#include "rlk_control_client.h"
#include "rlk_cli_client.h"
#include "media_service.h"

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);

void user_app_main(void)
{
    rlk_client_init();
    rlk_cli_client_init();
}

int main(void)
{
#if (CONFIG_SYS_CPU0)
    rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
    //bk_set_printf_sync(true);
    // shell_set_log_level(BK_LOG_WARN);
#endif
    bk_init();
    media_service_init();

#if (CONFIG_SYS_CPU0)
    rtos_set_semaphore(&s_rlk_cntrl_client_sem);
#endif

    return 0;
}

#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>

#include <components/shell_task.h>
#include "bk_private/bk_wifi.h"
#include "rlk_demo_cli.h"
#include "rlk_common.h"

RLK_DEMO rlk_demo_env = {0};

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);

void rlk_demo_callback_init(void)
{
    rlk_demo_env.is_inited = true;
    BK_LOG_ON_ERR(bk_rlk_register_recv_cb(rlk_demo_dispatch));
}

void user_app_main(void){

    BK_LOG_ON_ERR(bk_rlk_init());
    rlk_demo_callback_init();
    rlk_demo_env.chnl = 11;
    // set channel
    BK_LOG_ON_ERR(bk_rlk_set_channel(rlk_demo_env.chnl));
    cli_rlk_init();
}

int main(void)
{
#if (CONFIG_SYS_CPU0)
    rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
    // bk_set_printf_sync(true);
    // shell_set_log_level(BK_LOG_WARN);
#endif
    bk_init();

    return 0;
}

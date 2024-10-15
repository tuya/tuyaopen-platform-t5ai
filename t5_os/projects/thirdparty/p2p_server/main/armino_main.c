#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>

#include "media_service.h"
extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);

extern void cli_wifi_sta_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern bk_err_t demo_doorbell_cs2_p2p_server_init(int argc, char **argv);

extern int media_service_init(void);


void user_app_main(void)
{
#if 1

#endif
}

int main(void)
{
#if (CONFIG_SYS_CPU0)
    rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
//    bk_set_printf_sync(false);
    // shell_set_log_level(BK_LOG_WARN);
#endif
    bk_init();
    media_service_init();

	return 0;
}

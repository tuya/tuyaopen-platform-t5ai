#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "media_service.h"
#include "av_server_udp_service.h"
#include "av_server_tcp_service.h"
#include <modules/wifi.h>
#include "bk_private/bk_wifi.h"

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);

void user_app_main(void)
{

	rtos_delay_milliseconds(100);
	os_printf("start av server \r\n");
	demo_softap_app_init("av_demo", "", "13");
	os_printf("start ap: av_demo \r\n");
#if CONFIG_INTEGRATION_DOORBELL
#if CONFIG_AV_DEMO_MODE_TCP
	av_server_tcp_service_init();
#else
	av_server_udp_service_init();
#endif
#endif
}

int main(void)
{
#if (CONFIG_SYS_CPU0)
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
	// bk_set_printf_sync(true);
	// shell_set_log_level(BK_LOG_WARN);
#endif
	bk_init();
    media_service_init();

#if 0//(!CONFIG_AV_DEMO_MODE_TCP)
#if (CONFIG_SYS_CPU0)
    rtos_set_semaphore(&s_av_server_service_sem);
#endif
#endif

	return 0;
}
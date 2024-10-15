#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include "media_service.h"
#if (CONFIG_SYS_CPU0)
#include <components/shell_task.h>
#include "bk_private/bk_wifi.h"
#include "dual_device_voice_transmission.h"
#include <modules/wifi.h>
#include "cli.h"

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);


#if ((CONFIG_SYS_CPU0 && CONFIG_SOC_BK7256XX) || (CONFIG_SYS_CPU0 && CONFIG_SOC_BK7236XX))
static void cli_dual_device_voice_call_help(void)
{
	os_printf("voice_call {start|stop udp|tcp client|server} \r\n");
}

void cli_dual_device_voice_call_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc != 4) {
		cli_dual_device_voice_call_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		if (os_strcmp(argv[2], "udp") == 0) {
			if (os_strcmp(argv[3], "server") == 0) {
				os_printf("start voice call server \r\n");
				demo_softap_app_init("voice_call", "", "13");
				os_printf("start ap: voice_call \r\n");
				dual_device_voice_transmission_udp_init(VOICE_DEVICE_ROLE_SERVER);
				os_printf("voice call server run \r\n");
			} else {
				os_printf("start voice call client \r\n");
				demo_sta_app_init("voice_call", "");
				os_printf("connect ap: voice_call \r\n");
				dual_device_voice_transmission_udp_init(VOICE_DEVICE_ROLE_CLIENT);
				//bk_wifi_capa_config(WIFI_CAPA_ID_TX_AMPDU_EN, 0);
				os_printf("voice call client run \r\n");
			}
		} else {
			/* TODO */
			if (os_strcmp(argv[3], "server") == 0) {
				//dual_device_voice_transmission_tcp_init(VOICE_DEVICE_ROLE_SERVER);
			} else {
				//dual_device_voice_transmission_tcp_init(VOICE_DEVICE_ROLE_CLIENT);
			}
		}
	}else if (os_strcmp(argv[1], "stop") == 0) {
		if (os_strcmp(argv[2], "udp") == 0) {
			dual_device_voice_transmission_udp_deinit();
		} else {
			//dual_device_voice_transmission_tcp_deinit();
		}
	} else {
		cli_dual_device_voice_call_help();
	}

}


#define DUAL_DEVICE_VOICE_CALL_CMD_CNT  (sizeof(s_dual_device_voice_call_commands) / sizeof(struct cli_command))
static const struct cli_command s_dual_device_voice_call_commands[] =
{
	{"voice_call", "voice_call {start|stop udp|tcp client|server}", cli_dual_device_voice_call_cmd},
};

int cli_dual_device_voice_call_init(void)
{
	os_printf("cli_dual_device_voice_call_init \n");

	return cli_register_commands(s_dual_device_voice_call_commands, DUAL_DEVICE_VOICE_CALL_CMD_CNT);
}
#endif

void user_app_main(void){
	rtos_delay_milliseconds(100);
	cli_dual_device_voice_call_init();
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
	media_service_init();

#if (CONFIG_SYS_CPU0)
//	cli_dual_device_voice_call_init();
#endif

	return 0;
}

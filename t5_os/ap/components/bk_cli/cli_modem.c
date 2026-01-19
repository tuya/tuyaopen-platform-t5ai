#include <common/bk_include.h>
#include "sdkconfig.h"
#include "stdarg.h"
#include <os/mem.h>
#include "sys_rtos.h"
#include <os/os.h>
#include <common/bk_kernel_err.h>
#include "bk_sys_ctrl.h"
#include "bk_cli.h"
#include "bk_uart.h"
#include <os/str.h>
#include <components/log.h>
#include <driver/media_types.h>
#include "drv_model_pub.h"
#include "cli.h"
#include <components/usb.h>
#include <components/usb_types.h>
#include <driver/audio_ring_buff.h>
#if (CONFIG_BK_MODEM)
#include "bk_modem_dce.h"
#include "components/modem_driver.h"
#endif

#if (CONFIG_BK_MODEM)

void cli_modem_config_help(void)
{
	CLI_RAW_LOGI("\r\nmodem_config [init|deinit] \n");
	CLI_RAW_LOGI("  Modem config. \n");
	CLI_RAW_LOGI("  -init: init modem. \n");
	CLI_RAW_LOGI("  -deinit: deinit modem. \n");
	CLI_RAW_LOGI("  example1: modem_config init \n");
	CLI_RAW_LOGI("  example2: modem_config deinit \n");
}


void cli_modem_config(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;

	if (argc < 2) {
		CLI_LOGW("invalid argc number\n");
		cli_modem_config_help();
		goto error;
	}

	if (os_strcmp(argv[1], "help") == 0) {
		cli_modem_config_help();
		goto succeed;
	}

	bool is_init = true;
	if (os_strcmp(argv[1], "init") == 0) {
		is_init = true;
	}
	else if (os_strcmp(argv[1], "deinit") == 0) {
		is_init = false;
	}
	else{
		CLI_LOGW("bad parameters\r\n");
		cli_modem_config_help();
		goto error;
	}

	if(is_init)
	{
		if (argc == 4)
		{
			CLI_LOGI("bk modem config init %d, %d, %d.\r\n", argc, (uint8_t)os_strtoul(argv[2], NULL, 10), (uint8_t)os_strtoul(argv[3], NULL, 10));
			bk_modem_init((uint8_t)os_strtoul(argv[2], NULL, 10), (uint8_t)os_strtoul(argv[3], NULL, 10));
		}
		else
		{
			CLI_LOGI("bk modem config argc error.\r\n");
		}
	}
	else
	{
		CLI_LOGI("bk modem config deinit.\r\n");
		bk_modem_deinit();
	}

succeed:
	msg = WIFI_CMD_RSP_SUCCEED;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;

error:
	msg = WIFI_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;
}

const struct cli_command bk_modem_clis[] = {
	{"bk_modem_config", "bk_modem_config {init|deinit} {comm protocal} {comm interface}", cli_modem_config},
};

int cli_modem_init(void)
{
	int ret;
	ret = cli_register_commands(bk_modem_clis, sizeof(bk_modem_clis) / sizeof(struct cli_command));
	if (ret)
		CLI_LOGD("register usb host commands fail.\r\n");

	return ret;
}
#endif //CONFIG_BK_MODEM


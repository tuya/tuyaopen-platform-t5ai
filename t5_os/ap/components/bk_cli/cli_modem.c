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
#include "components/modem_driver.h"
#if (CONFIG_BK_MODEM)
void cli_modem_config(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		CLI_LOGI("%s ,line:%d.\r\n",__FILE__,__LINE__);
		return;
	}

	bool is_init = true;
	if (os_strcmp(argv[1], "init") == 0) {
		is_init = true;
	}
	else if (os_strcmp(argv[1], "deinit") == 0) {
		is_init = false;
	}
	else{
		CLI_LOGI("%s ,line:%d.\r\n",__FILE__,__LINE__);
		return;
	}

	if(is_init)
	{
		CLI_LOGI("bk modem config init.\r\n");
		bk_modem_init();
	}
	else
	{
		CLI_LOGI("bk modem config deinit.\r\n");
		bk_modem_deinit();
	}
}

const struct cli_command bk_modem_clis[] = {
	{"bk_modem_config", "bk_modem_config [init|deinit]", cli_modem_config},
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


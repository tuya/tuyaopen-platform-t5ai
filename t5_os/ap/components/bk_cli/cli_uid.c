#include <stdlib.h>
#include "cli.h"
#include <components/bk_uid.h>

static void cli_uid_help(void)
{
	CLI_LOGD("uid [init/get] \r\n");
}

static void cli_uid_ops_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2)
	{
		cli_uid_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		BK_LOG_ON_ERR(bk_uid_driver_init());
	} else if(os_strcmp(argv[1], "get") == 0) {
		unsigned char data[32];
		BK_LOG_ON_ERR(bk_uid_get_data(data));
		for(int j = 0;j < 32; j++)
		{
			CLI_LOGD("%x index:%d\r\n", data[j], j);
		}
	} else {
		cli_uid_help();
		return;
	}
}

#define UID_CMD_CNT (sizeof(s_uid_commands) / sizeof(struct cli_command))
static const struct cli_command s_uid_commands[] = {
	{"uid", "uid [init/get]/", cli_uid_ops_cmd},
};

int cli_uid_init(void)
{
	return cli_register_commands(s_uid_commands, UID_CMD_CNT);
}

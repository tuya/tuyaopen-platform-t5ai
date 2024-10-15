#include <common/bk_include.h>
#include "cli.h"
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <driver/int.h>
#include <common/bk_err.h>

#include "doorbell.h"

void cli_doorbell_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;
	int ret = BK_FAIL;

	if (argc == 1)
	{
		ret = demo_doorbell_udp_init("MJPEG");
		goto output;
	}

	if (os_strcmp(argv[1], "tcp") == 0)
	{
		ret = demo_doorbell_tcp_init(argv[2]);
	}
#if (!CONFIG_INTEGRATION_DOORBELL_CS2)
#if CONFIG_CS2_P2P_SERVER
	else if (os_strcmp(argv[1], "cs2_p2p_server") == 0)
	{
		ret = demo_doorbell_cs2_p2p_server_init(argc - 2, &argv[2]);
	}
#endif

#if CONFIG_CS2_P2P_CLIENT
	else if (os_strcmp(argv[1], "cs2_p2p_client") == 0)
	{
		ret = demo_doorbell_cs2_p2p_client_init(argc - 2, &argv[2]);
	}
#endif
#endif // !CONFIG_INTEGRATION_DOORBELL_CS2
	else
	{
		ret = demo_doorbell_udp_init(argv[2]);
	}

output:

	if (ret != BK_OK)
	{
		msg = CLI_CMD_RSP_ERROR;
	}
	else
	{
		msg = CLI_CMD_RSP_SUCCEED;
	}

	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

#if CONFIG_AV_DEMO
extern void cli_av_audio_udp_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void cli_av_audio_tcp_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
#endif

#define DOORBELL_CMD_CNT	(sizeof(s_doorbell_commands) / sizeof(struct cli_command))

static const struct cli_command s_doorbell_commands[] = {
	{"doorbell", "doorbell...", cli_doorbell_test_cmd},
#if CONFIG_AV_DEMO
	{"av_audio_udp_test", "av_audio_udp_test {server|client}", cli_av_audio_udp_test_cmd},
	{"av_audio_tcp_test", "av_audio_tcp_test {server|client}", cli_av_audio_tcp_test_cmd},
#endif
};

int cli_doorbell_init(void)
{
	return cli_register_commands(s_doorbell_commands, DOORBELL_CMD_CNT);
}


#include "conv_utf8_pub.h"
#include "rlk_cli_client.h"
#include "rlk_control_client.h"

#define RLK_CLIENT_CMD_CNT (sizeof(s_rlk_client_commands) / sizeof(struct cli_command))
static void rlk_cli_client_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    int ret = 0;
    char *msg = NULL;

    if(os_strcmp(argv[1], "ps") == 0) {
        rlk_client_ps_cmd_handler();
    }
    else {
        CLI_LOGI("invalid RLK paramter\n");
        goto error;
    }

    if (!ret) {
        msg = RLK_CMD_RSP_SUCCEED;
        os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
        return;
    }
error:
    msg = RLK_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    return;
}
static const struct cli_command s_rlk_client_commands[] = {
    {"rlkc", "rlkc ps", rlk_cli_client_cmd},
};
int rlk_cli_client_init(void)
{
    return cli_register_commands(s_rlk_client_commands, RLK_CLIENT_CMD_CNT);
}
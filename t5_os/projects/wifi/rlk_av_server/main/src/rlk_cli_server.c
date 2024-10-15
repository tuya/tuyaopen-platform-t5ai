#include "conv_utf8_pub.h"
#include "rlk_cli_server.h"
#include "rlk_control_server.h"

#define RLK_SERVER_CMD_CNT (sizeof(s_rlk_server_commands) / sizeof(struct cli_command))
static void rlk_cli_server_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    int ret = 0;
    char *msg = NULL;
    //os_printf("argc:%d argv[0]:%s argv[1]:%s\n", argc, argv[0], argv[1]);
    
    if(os_strcmp(argv[1], "wakeup_peer") == 0) {
        rlk_cntrl_server_wakeup_peer();
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
static const struct cli_command s_rlk_server_commands[] = {
    {"rlks", "rlks [wakeup_peer]", rlk_cli_server_cmd},
};
int rlk_cli_server_init(void)
{
    return cli_register_commands(s_rlk_server_commands, RLK_SERVER_CMD_CNT);
}
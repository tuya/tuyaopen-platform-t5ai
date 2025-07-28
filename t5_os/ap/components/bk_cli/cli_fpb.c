#include "cli.h"
#include "sdkconfig.h"

#if CONFIG_SOC_BK7236XX
#include "fpb.h"

static void fpb_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t instruction_addr;
    if (2 != argc) {
        BK_LOGD(NULL, "Usage: fpb instruction_addr\r\n");
        return;
    }

    instruction_addr = strtoll(argv[1], NULL, 16);
    fpb_set_program_breakpoint(instruction_addr);
}


#define FPB_CMD_CNT (sizeof(s_fpb_commands) / sizeof(struct cli_command))
static const struct cli_command s_fpb_commands[] = {
    {"fpb", "fpb instruction_addr", fpb_Command},
};

int cli_fpb_init(void)
{
    return cli_register_commands(s_fpb_commands, FPB_CMD_CNT);
}
#endif // CONFIG_SOC_BK7236XX


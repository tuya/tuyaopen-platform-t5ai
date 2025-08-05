#include "cli.h"
#include "sdkconfig.h"

#if CONFIG_SOC_BK7236XX
#include "dwt.h"

static void dwt_command_usage(void)
{
    BK_LOGD(NULL,"dwti instruction_addr\r\n");
    BK_LOGD(NULL,"     instruction_addr: hex format, and it is a valid instruction address\r\n");

    BK_LOGD(NULL,"dwtd r/w/b data_address\r\n");
    BK_LOGD(NULL,"     r: read this data address\r\n");
    BK_LOGD(NULL,"     w: write this data address\r\n");
    BK_LOGD(NULL,"     b: access this data address\r\n");
    BK_LOGD(NULL,"     data_address: watch this data address, hex format\r\n");

    BK_LOGD(NULL,"dwtdr data_address data_address_limit\r\n");
    BK_LOGD(NULL,"     data_address: watch this data start address, hex format\r\n");
    BK_LOGD(NULL,"     data_address_limit: watch this data end address, hex format\r\n");

    BK_LOGD(NULL,"dwtdd data_address data_value\r\n");
    BK_LOGD(NULL,"     data_address: watch this data address, hex format\r\n");
    BK_LOGD(NULL,"     data_value: match the data value with watching this data address, hex format\r\n");
}

static void dwti_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t instruction_addr;

    if (2 != argc) {
        dwt_command_usage();
        return;
    }

    instruction_addr = strtoll(argv[1], NULL, 16);
    dwt_set_instruction_address(instruction_addr);
}

static void dwtd_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char watch_type;
    uint32_t data_addr;

    if (3 != argc) {
        dwt_command_usage();
        return;
    }

    watch_type = argv[1][0];
    data_addr = strtoll(argv[2], NULL, 16);
    switch (watch_type){
        case 'r':
            dwt_set_data_address_read(data_addr);
            break;

        case 'w':
            dwt_set_data_address_write(data_addr);
            break;

        case 'b':
            dwt_set_data_address_access(data_addr);
            break;
        default:
            dwt_command_usage();
            break;
    }
}

static void dwtdd_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t addr, value;

    if (3 != argc) {
        dwt_command_usage();
        return;
    }

    addr = strtoll(argv[1], NULL, 16);
    value = strtoll(argv[2], NULL, 16);

    dwt_conditional_data_watchpoint(addr, value);
}

static void dwtdr_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t addr, limit;

    if (3 != argc) {
        dwt_command_usage();
        return;
    }

    addr = strtoll(argv[1], NULL, 16);
    limit = strtoll(argv[2], NULL, 16);

    dwt_set_data_address_range(addr, limit, ACCESS_TYPE_WRITE);
}


#define DWT_CMD_CNT (sizeof(s_dwt_commands) / sizeof(struct cli_command))
static const struct cli_command s_dwt_commands[] = {
    {"dwti", "dwt instruction_addr", dwti_Command},
    {"dwtd", "dwtd r/w/b data_address", dwtd_Command},
    {"dwtdr", "dwtdr data_address data_address_limit", dwtdr_Command},
    {"dwtdd", "dwtdd data_address data_value", dwtdd_Command},
};

int cli_dwt_init(void)
{
    return cli_register_commands(s_dwt_commands, DWT_CMD_CNT);
}
#endif // CONFIG_SOC_BK7236XX


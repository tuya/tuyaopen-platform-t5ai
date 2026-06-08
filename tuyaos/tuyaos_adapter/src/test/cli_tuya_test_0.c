#include "cli.h"
#include "modules/pm.h"
#include "modules/wifi.h"
#include "tuya_cloud_types.h"
#include "tkl_wifi.h"
#include "cli_tuya_test.h"
#include "tkl_display.h"
#include "lwip_netif_address.h"
#include "lwip/inet.h"
#include "driver/hal/hal_efuse_types.h"
#include "driver/otp_types.h"

#define __PRINT_MACRO(x) #x
#define PRINT_MACRO(x) #x"="__PRINT_MACRO(x)
//#pragma message(PRINT_MACRO(AON_RTC_DEFAULT_CLOCK_FREQ))


extern VOID_T tkl_system_sleep(CONST UINT_T num_ms);

static void cli_rf_set_cali_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_printf("set rf calibration flag begin\r\n");

    char *arg[5];
    arg[0] = "txevm";
    arg[1] = "-e";
    arg[2] = "2\r\n";
    tx_evm_cmd_test(pcWriteBuffer, xWriteBufferLen, 3, arg);
    tkl_system_sleep(200);

    arg[0] = "txevm";
    arg[1] = "-g";
    arg[2] = "8\r\n";
    tx_evm_cmd_test(pcWriteBuffer, xWriteBufferLen, 3, arg);
    tkl_system_sleep(200);

    arg[0] = "txevm";
    arg[1] = "-g";
    arg[2] = "0\r\n";
    tx_evm_cmd_test(pcWriteBuffer, xWriteBufferLen, 3, arg);
    tkl_system_sleep(200);

    arg[0] = "txevm";
    arg[1] = "-e";
    arg[2] = "1\r\n";
    tx_evm_cmd_test(pcWriteBuffer, xWriteBufferLen, 3, arg);
    tkl_system_sleep(200);

    arg[0] = "txevm";
    arg[1] = "-s";
    arg[2] = "11";
    arg[3] = "1";
    arg[4] = "20\r\n";
    tx_evm_cmd_test(pcWriteBuffer, xWriteBufferLen, 5, arg);
    tkl_system_sleep(200);

    arg[0] = "txevm";
    arg[1] = "-e";
    arg[2] = "2\r\n";
    tx_evm_cmd_test(pcWriteBuffer, xWriteBufferLen, 3, arg);
    tkl_system_sleep(200);

    arg[0] = "txevm";
    arg[1] = "-e";
    arg[2] = "4";
    arg[3] = "1\r\n";
    tx_evm_cmd_test(pcWriteBuffer, xWriteBufferLen, 4, arg);
    tkl_system_sleep(200);

    arg[0] = "txevm";
    arg[1] = "-g";
    arg[2] = "8\r\n";
    tx_evm_cmd_test(pcWriteBuffer, xWriteBufferLen, 3, arg);
    tkl_system_sleep(200);

    bk_printf("set rf calibration flag end\r\n");
}

static void __get_flash_id(void)
{
    uint32_t flash_size;
    uint32_t flash_id = bk_flash_get_id();

    flash_size = 2 << ((flash_id & 0xff) - 1);

    bk_printf("flash id: 0x%08x, flash size: %x / %dM\r\n", flash_id, flash_size, flash_size / 1048576);
}

extern void port_check_isr_stack(void);
#include "tuya_cloud_types.h"

uint32_t  __attribute__((weak)) ty_app_memory_occupied(void)
{
    return 0;
}

static void cli_system_info_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc > 1) {
        for (int i = 0; i < argc; i++) {
            switch (argv[i][0]) {
                case 't':
                    rtos_dump_backtrace();
                    break;
                case 's':
                    port_check_isr_stack();
                    break;
#if CONFIG_FREERTOS && CONFIG_MEM_DEBUG
                case 'm':
                    os_dump_memory_stats(0, 0, NULL);
                    break;
#endif // CONFIG_FREERTOS && CONFIG_MEM_DEBUG
                default:
                    bk_printf("unknown param: %s\r\n", argv[i]);
                    break;
            }
        }

    }

    rtos_dump_task_list();

#if 0
    WF_WK_MD_E mode = WWM_UNKNOWN;
    NW_IP_S ip;
    NW_MAC_S mac;

    os_memset(&ip, 0, sizeof(NW_IP_S));
    os_memset(&mac, 0, sizeof(NW_MAC_S));

    tkl_wifi_get_work_mode(&mode);
    if (mode == WWM_STATION) {
        tkl_wifi_get_ip(WF_STATION, &ip);
        tkl_wifi_get_mac(WF_STATION, &mac);
        bk_printf("sta ");
    } else if (mode == WWM_SOFTAP) {
        tkl_wifi_get_ip(WF_AP, &ip);
        tkl_wifi_get_mac(WF_AP, &mac);
        bk_printf("ap ");
    }

#if defined(ENABLE_IPv6) && (ENABLE_IPv6 == 1)
    if (ip.type == TY_AF_INET6) {
        bk_printf("ip: %s\r\n", ip.addr.ip6.ip);
    } else
#endif
    {
        bk_printf("ip: %s mask: %s gw: %s\r\n",
                ip.nwipstr, ip.nwmaskstr, ip.nwgwstr);
    }
    bk_printf("mac: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
            mac.mac[0], mac.mac[1], mac.mac[2], mac.mac[3], mac.mac[4], mac.mac[5]);
#endif

    extern int shell_assert_out(bool bContinue, char * format, ...);
    shell_assert_out(1, "sram left heap: %d, min: %d\r\n", xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
    shell_assert_out(1, "psram left: %d, total: %d, use count: %d\r\n",
            xPortGetPsramFreeHeapSize(),
            xPortGetPsramTotalHeapSize(),
            bk_psram_heap_get_used_count());
    shell_assert_out(1, "runtime: %d\r\n", xTaskGetTickCount());

    __get_flash_id();

    uint8_t dev_id[EFUSE_DEVICE_ID_BYTE_NUM];
    bk_otp_apb_read(OTP_DEVICE_ID, dev_id, EFUSE_DEVICE_ID_BYTE_NUM);
    shell_assert_out(1, "chip id: 0x%02x%02x%02x%02x%02x, %d\r\n",
            dev_id[0], dev_id[1], dev_id[2], dev_id[3], dev_id[4], OTP_DEVICE_ID);

    return;
}

#define TUYA_TEST_CMD_CNT (sizeof(s_sinfo_commands) / sizeof(struct cli_command))
static const struct cli_command s_sinfo_commands[] = {
    {"info", "system info", cli_system_info_cmd},
    {"rf_cali", "set rf calibration flag, just for test", cli_rf_set_cali_cmd},
};

int cp_cli_tuya_test_init(void)
{
    cli_register_commands(s_sinfo_commands, TUYA_TEST_CMD_CNT);
    return 0;
}


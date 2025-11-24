#include "cli.h"
#include "modules/pm.h"
#include "modules/wifi.h"
#include "tuya_cloud_types.h"
#include "tkl_wifi.h"
#include "cli_tuya_test.h"
#include "lwip_netif_address.h"
#include "lwip/inet.h"
#include "driver/hal/hal_efuse_types.h"
#include "driver/otp_types.h"

#include <components/netif.h>
#include <components/netif_types.h>

extern void port_check_isr_stack(void);
#include "tuya_cloud_types.h"

#include "sdkconfig.h"

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
    rtos_dump_task_runtime_stats();

    NW_MAC_S mac;
    tkl_wifi_get_mac(WF_STATION, &mac);

    bk_printf("mac: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
            mac.mac[0], mac.mac[1], mac.mac[2], mac.mac[3], mac.mac[4], mac.mac[5]);

    netif_ip4_config_t ip4_config;
    bk_netif_get_ip4_config(0, &ip4_config);

    bk_printf("ip: %s, mask: %s, gw: %s, dns: %s\r\n",
            ip4_config.ip, ip4_config.mask,
            ip4_config.gateway, ip4_config.dns);


    bk_printf("sram left heap: %d, min: %d\r\n", xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
    bk_printf("psram left: %d, total: %d, used count: %d\r\n",
            xPortGetPsramFreeHeapSize(),
            xPortGetPsramTotalHeapSize(),
            bk_psram_heap_get_used_count());
    bk_printf("runtime: %d\r\n", xTaskGetTickCount());
    bk_printf("bk_pm_current_max_cpu_freq_get: %d\r\n", bk_pm_current_max_cpu_freq_get());

    return;
}

extern void smp_arch_dwt_trap_write(uint32_t addr, uint32_t data);
static uint32_t g_dwt_test = 1234;

static void cli_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_printf("argc: %d\r\n cmd: ", argc);
    for (int i = 0; i < argc; i++) {
        bk_printf_raw(0, NULL, " %s", argv[i]);
    }
    bk_printf_raw(0, NULL, "\r\n");

    if (!os_strcmp(argv[1], "init")) {
        smp_arch_dwt_trap_write(&g_dwt_test, 0);
        bk_printf_raw(0, NULL, "init\r\n");
    } else if (!os_strcmp(argv[1], "read")) {
        bk_printf_raw(0, NULL, "g_dwt_test: %d\r\n", g_dwt_test);
    } else if (!os_strcmp(argv[1], "write")) {
        bk_printf_raw(0, NULL, "set g_dwt_test 0, will dump\r\n");
        tkl_system_sleep(500);
        g_dwt_test = 0;
    }
}


static const struct cli_command tuya_cli_commands[] = {
    // {"audio_test", "mic to speaker test", cli_audio_test_cmd},
    {"info",    "system info",      cli_system_info_cmd},
    {"xt",      "test",             cli_test_cmd},
    {"lfs",     "little fs test",   cli_littlefs_cmd},
    {"xsd",     "sd card test",     cli_sdcard_test_cmd},
#if 0
    {"xid",     "set mac",          cli_tmp_cmd},
#if 1
    {"xadc",    "adc test",         cli_adc_cmd},
#endif
    {"xwifi",   "wifi test",        cli_wifi_cmd},
    {"xgpio",   "gpio test",        cli_gpio_cmd},
    {"xlcd",    "lcd test",         cli_xlcd_cmd},
    {"xpwm",    "pwm test",         cli_pwm_cmd},
    // {"xmt",     "media test",       cli_tuya_media_cmd},
    {"xqspi",   "qspi test",        cli_xqspi_cmd},

    {"xqspilcd",   "qspi test",        cli_xqspi_lcd_cmd},
    // {"xmtd",   "mtd test",        cli_xmtd_cmd},
    {"xusb",    "usb device check", cli_usb_cmd},
    {"xspi",    "spi test",         cli_spi_cmd},
    {"xspi2",    "spi test",        cli_spi_2_3_cmd},


    // {"xeth",    "eth test",         cli_eth_cmd},
    // {"xiperf",  "iperf test",       cli_iperf_cmd},
    {"xmic",    "mic test",         cli_mic_cmd},
    {"xspk",    "spk test",         cli_speaker_cmd},
    // {"xi2s",    "i2s test",         cli_tuya_i2s_cmd},
#endif
};

#define TUYA_TEST_CMD_CNT (sizeof(tuya_cli_commands) / sizeof(struct cli_command))

int ap_cli_tuya_test_init(void)
{
    cli_register_commands(tuya_cli_commands, TUYA_TEST_CMD_CNT);

#if (CONFIG_AUD_INTF_TEST)
	extern int cli_aud_intf_init(void);
	cli_aud_intf_init();
#endif

    return 0;
}



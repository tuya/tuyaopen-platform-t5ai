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


extern void port_check_isr_stack(void);
#include "tuya_cloud_types.h"

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

    NW_MAC_S mac;
    tkl_wifi_get_mac(WF_STATION, &mac);

    bk_printf("mac: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
            mac.mac[0], mac.mac[1], mac.mac[2], mac.mac[3], mac.mac[4], mac.mac[5]);

    bk_printf("sram left heap: %d, min: %d\r\n", xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
    bk_printf("psram left: %d, total: %d, used count: %d\r\n",
            xPortGetPsramFreeHeapSize(),
            xPortGetPsramTotalHeapSize(),
            bk_psram_heap_get_used_count());
    bk_printf("runtime: %d\r\n", xTaskGetTickCount());

    return;
}

extern OPERATE_RET tkl_wifi_set_mac(CONST WF_IF_E wf, CONST NW_MAC_S *mac);
static void cli_set_mac(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2) {
        bk_printf("Usage: ap xmac set|get\r\n");
        return;
    }

    if (!os_strcmp(argv[1], "set")) {
        uint8_t tmp[6] = {0x3C, 0x0B, 0x59, 0xE9, 0x9D, 0x59};
        NW_MAC_S mac;
        memcpy(mac.mac, tmp, 6);
        bk_printf("set mac: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                mac.mac[0], mac.mac[1], mac.mac[2], mac.mac[3], mac.mac[4], mac.mac[5]);
        tkl_wifi_set_mac(WF_STATION, &mac);
    } else if (!os_strcmp(argv[1], "get")) {
        NW_MAC_S m;
        memset(&m, 0, sizeof(NW_MAC_S));
        tkl_wifi_get_mac(WF_STATION, &m);
        bk_printf("get mac: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                m.mac[0], m.mac[1], m.mac[2], m.mac[3], m.mac[4], m.mac[5]);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
#include "FreeRTOS.h"
#include "task.h"
static TaskHandle_t __tmp_thread_handle = NULL;
static void __test_tmp_func(void *arg)
{
    TUYA_CPU_INFO_T *dev = NULL;
    INT_T cnt = 0;
    tkl_system_get_cpu_info(&dev, &cnt);
    bk_printf("chip id: 0x%02x%02x%02x%02x%02x, cnt: %d\r\n",
            dev->chipid[0], dev->chipid[1], dev->chipid[2],
            dev->chipid[3], dev->chipid[4], cnt);

    __tmp_thread_handle = NULL;
    vTaskDelete(__tmp_thread_handle);
}

static void cli_tmp_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    xTaskCreate(__test_tmp_func, "test_func", 1024, NULL, 6, (TaskHandle_t * const )&__tmp_thread_handle);
}
///////////////////////////////////////////////////////////////////////////////////////////////

static uint32_t tttttttttt = 0;
static void cli_uuuuuu_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    tttttttttt++;
    bk_printf("tttttttttt: %p, %d\r\n", &tttttttttt, tttttttttt);
}


static const struct cli_command tuya_cli_commands[] = {
    // {"audio_test", "mic to speaker test", cli_audio_test_cmd},
    {"info",    "system info",      cli_system_info_cmd},
    {"xmac",    "set mac",          cli_set_mac},
    {"xid",     "set mac",          cli_tmp_cmd},
#if 1
    {"xadc",    "adc test",         cli_adc_cmd},
#endif
    {"xwifi",   "wifi test",        cli_wifi_cmd},
    {"xgpio",   "gpio test",        cli_gpio_cmd},
    {"xlcd",    "lcd test",         cli_xlcd_cmd},
    {"xpwm",    "pwm test",         cli_pwm_cmd},
    {"xmt",     "media test",       cli_tuya_media_cmd},
    {"xqspi",   "qspi test",        cli_xqspi_cmd},
    {"xmtd",   "mtd test",        cli_xmtd_cmd},
    {"lfs",     "little fs test",   cli_littlefs_cmd},
    {"xusb",    "usb device check", cli_usb_cmd},
    {"xsd",     "sd card test",     cli_sdcard_test_cmd},
    {"xspi",    "spi test",         cli_spi_cmd},
    // {"xeth",    "eth test",         cli_eth_cmd},
    // {"xiperf",  "iperf test",       cli_iperf_cmd},
    {"xmic",    "mic test",         cli_mic_cmd},
    {"xspk",    "spk test",         cli_speaker_cmd},
    {"xt",      "spk test",         cli_uuuuuu_cmd},
    // {"xi2s",    "i2s test",         cli_tuya_i2s_cmd},

};

#define TUYA_TEST_CMD_CNT (sizeof(tuya_cli_commands) / sizeof(struct cli_command))

int ap_cli_tuya_test_init(void)
{
    cli_register_commands(tuya_cli_commands, TUYA_TEST_CMD_CNT);
    return 0;
}



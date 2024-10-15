#include "cli.h"
#include "flash.h"
#include "modules/pm.h"
#include "modules/wifi.h"
#include "tuya_cloud_types.h"
#include "tkl_wifi.h"
#include "cli_tuya_test.h"

#define __PRINT_MACRO(x) #x
#define PRINT_MACRO(x) #x"="__PRINT_MACRO(x)
//#pragma message(PRINT_MACRO(AON_RTC_DEFAULT_CLOCK_FREQ))


extern void tkl_system_sleep(const uint32_t num_ms);

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

static void cli_wifi_set_interval_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint8_t interval = 0;
    int ret = 0;
    char *msg = NULL;

    if (argc < 2) {
        bk_printf("invalid argc num\r\n");
        goto error;
    }

    if (!os_strcmp(argv[1], "deepsleep")) {
        TUYA_WAKEUP_SOURCE_BASE_CFG_T  cfg;
        if (!os_strcmp(argv[2], "rtc")) {
            cfg.source = TUYA_WAKEUP_SOURCE_TIMER;
            uint32_t s = os_strtoul(argv[3], NULL, 10);
            cfg.wakeup_para.timer_param.ms = s * 1000;

        }else if (!os_strcmp(argv[2], "gpio")) {
            cfg.source = TUYA_WAKEUP_SOURCE_GPIO;
            uint32_t num = os_strtoul(argv[3], NULL, 10);
            cfg.wakeup_para.gpio_param.gpio_num = num;
            cfg.wakeup_para.gpio_param.level = TUYA_GPIO_LEVEL_LOW;
        } else {
            bk_printf("Usage: lp deepsleep [rtc|gpio] [rtc time<seconds>|gpio num]");
            goto error;
        }
        tkl_wakeup_source_set(&cfg);

        tkl_cpu_sleep_mode_set(1, TUYA_CPU_DEEP_SLEEP);
        return;
    }

    interval = (uint8_t)os_strtoul(argv[1], NULL, 10);

    bk_printf("\r\nset wifi dtim %d\r\n", interval);
    ret = bk_wifi_send_listen_interval_req(interval);

    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 1, 0);

    if (!ret) {
        bk_printf("set_interval ok");
        msg = WIFI_CMD_RSP_SUCCEED;
        os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
        return;
    }
    else {
        bk_printf("set_interval failed");
        goto error;
    }

error:
    msg = WIFI_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    return;
}

static void __get_flash_id(void)
{
    uint32_t flash_size;
    uint32_t flash_id = bk_flash_get_id();

    flash_size = 2 << ((flash_id & 0xff) - 1);

    bk_printf("flash id: 0x%08x, flash size: %x / %dM\r\n", flash_id, flash_size, flash_size / 1048576);
}

#if CONFIG_FREERTOS_V10
extern void port_check_isr_stack(void);
#endif // CONFIG_FREERTOS_V10
static void cli_task_cpuload_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc > 1) {
        for (int i = 0; i < argc; i++) {
            switch (argv[i][0]) {
                case 't':
                    rtos_dump_backtrace();
                    break;
#if CONFIG_FREERTOS_V10
                case 's':
                    port_check_isr_stack();
                    break;
#endif // CONFIG_FREERTOS_V10
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

#if CONFIG_FREERTOS
    rtos_dump_task_runtime_stats();
#endif // CONFIG_FREERTOS

    bk_printf("left heap: %d\r\n", xPortGetFreeHeapSize());
    bk_printf("runtime: %d\r\n", xTaskGetTickCount());

    __get_flash_id();

    return;
}

#if (CONFIG_CPU_INDEX == 0) && CONFIG_TUYA_LCD
#include "media_app.h"
#include "media_evt.h"
#include "tkl_display.h"
#include "tkl_thread.h"
#include "tkl_audio.h"
//extern OPERATE_RET tkl_disp_init(TKL_DISP_DEVICE_S *display_device, TKL_DISP_EVENT_HANDLER_S *event_handler);
static void cli_tuya_media_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    //tuya_media_wifi_transfer();
    if (argc < 1) {
        bk_printf("Usage: xmt open|close [uvc|lcd|h264|audio]\r\n");
        return;
    }
    bk_printf("argc: %d\r\n", argc);
    for (int i = 0; i < argc; i++) {
        bk_printf("argv[%d]: %s\r\n", i, argv[i]);
    }

    if (!os_strcmp(argv[1], "open")) {
        if (argc == 2) {
            // init uvc
            tkl_vi_init(NULL, 0);
            tkl_system_sleep(100);
            // init lcd
            tkl_disp_init(NULL, NULL);
            // init h264
            tkl_venc_init(0, NULL, 0);
        } else if (!os_strcmp(argv[2], "uvc")) {
            tkl_vi_init(NULL, 0);
        } else if (!os_strcmp(argv[2], "lcd")) {
            tkl_disp_init(NULL, NULL);
        } else if (!os_strcmp(argv[2], "h264")) {
            tkl_venc_init(0, NULL, 0);
        } else if (!os_strcmp(argv[2], "audio")) {
            TKL_AUDIO_CONFIG_T config;
            config.put_cb = NULL;
            tkl_ai_init(&config, 0);
        } else if (!os_strcmp(argv[2], "lp")) {
            // init uvc
            tkl_vi_init(NULL, 0);
            tkl_system_sleep(100);
            // init lcd
            tkl_disp_init(NULL, NULL);
            // init h264
            tkl_venc_init(0, NULL, 0);

            tkl_wifi_set_lp_mode(1, 10);
        }
    } else if (!os_strcmp(argv[1], "close")) {
        if (argc == 2) {
            // deinit h264
            tkl_venc_uninit(0);
            // close lcd
            tkl_disp_deinit(NULL);
            // disable uvc
            tkl_vi_uninit();
        } else if (!os_strcmp(argv[2], "uvc")) {
            tkl_vi_uninit();
        } else if (!os_strcmp(argv[2], "lcd")) {
            tkl_disp_deinit(NULL);
        } else if (!os_strcmp(argv[2], "h264")) {
            tkl_venc_uninit(0);
        }
    } else {
        bk_printf("Usage: xmt open|close [uvc|lcd|h264|audio]\r\n");
    }
    return;
}

#endif // CONFIG_CPU_INDEX == 0

#if CONFIG_USB
#if CONFIG_USB_HOST

//#include "bk_usb.h"
#include "usbh_core.h"
#include "usbh_hub.h"
extern void tuya_get_usb_dev(uint32_t *vid, uint32_t *pid);
void cli_usb_detect_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t idVendor = 0, idProduct = 0;

    tuya_get_usb_dev(&idVendor, &idProduct);
    os_printf("usb device idVendor:  0x%04x\r\n", idVendor);
    os_printf("usb device idProduct: 0x%04x\r\n", idProduct);
}
#endif // CONFIG_USB_HOST
#endif // CONFIG_USB

#include "tkl_wifi.h"
static void cli_scan_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2 || argv[1] == NULL) {
        bk_printf("xscan [ssid]");
        return;
    }
    bk_printf("ssid: %s\r\n", argv[1]);

    tkl_wifi_set_work_mode(WWM_STATION);   //设置为station开始扫描

    uint32_t num = 0;
    AP_IF_S *ap = NULL;
    tkl_wifi_scan_ap(argv[1], &ap, &num);
    if (ap) {
        bk_printf("ap rssi %d, bssid: %02x:%02x:%02x:%02x:%02x:%02x, ssid %s\r\n",
                ap->rssi, ap->bssid[0], ap->bssid[1], ap->bssid[2],
                ap->bssid[3], ap->bssid[4], ap->bssid[5], ap->ssid);
    }

    if (ap) {
        tkl_wifi_release_ap(ap);
        ap = NULL;
    }
}

#include <modules/pm.h>
#include <driver/aon_rtc_types.h>
static void cli_xxxt_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    float t1 = AON_RTC_MS_TICK_CNT;
    float t2 = (float)AON_RTC_MS_TICK_CNT;
    uint32_t t3 = (uint32_t)AON_RTC_MS_TICK_CNT;

    bk_printf("t1: %f %d\r\n", t1, t1);
    bk_printf("t2: %f %d\r\n", t2, t2);
    bk_printf("t3: %d\r\n", t3);

    bk_printf("rtc clk: %d %f %d\r\n",
            bk_rtc_get_clock_freq(),
            bk_rtc_get_ms_tick_count(),
            bk_rtc_get_ms_tick_count());
}

static void cli_uart_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_printf("---- %s\r\n", argv[1]);
    if (argv[1] == NULL) {
        bk_printf("not set parameter\r\n");
        return;
    }

    if (!strcmp("lp", argv[1])) {
        tkl_cpu_sleep_mode_set(1, TUYA_CPU_SLEEP);
    }
}


#define TUYA_TEST_CMD_CNT (sizeof(s_sinfo_commands) / sizeof(struct cli_command))
static const struct cli_command s_sinfo_commands[] = {
    {"info", "system info", cli_task_cpuload_cmd },
    {"lp", "set wifi dtim", cli_wifi_set_interval_cmd},
    {"rf_cali", "set rf calibration flag, just for test", cli_rf_set_cali_cmd},
    {"audio_test", "mic to speaker test", cli_audio_test_cmd},
    {"xadc", "adc test", cli_adc_cmd},
    {"xgpio", "gpio test", cli_gpio_cmd},
    {"xscan", "scan", cli_scan_cmd},
    {"xlcd", "lcd test", cli_xlcd_cmd},
    {"xwifi", "lcd test", cli_wifi_cmd},

#if CONFIG_BK7258_PWM_ENABLE
    {"xpwm", "pwm test", cli_pwm_cmd},
#endif // CONFIG_BK7258_PWM_ENABLE

#if CONFIG_USB && CONFIG_USB_HOST
    {"xusb", "usb device detect", cli_usb_detect_cmd},
#endif // CONFIG_USB

#if (CONFIG_CPU_INDEX == 0) && CONFIG_TUYA_LCD
    {"xmt", "tuya media test", cli_tuya_media_cmd},
#endif // CONFIG_CPU_INDEX == 0

    {"xt", "test", cli_xxxt_cmd},
    {"xu", "uart test", cli_uart_test_cmd},

};

int cli_tuya_test_init(void)
{
    return cli_register_commands(s_sinfo_commands, TUYA_TEST_CMD_CNT);
}


/*
 * test_wifi.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "cli.h"
#include "cli_tuya_test.h"
#include "tkl_wifi.h"

static void __cmd_usage(void)
{
    bk_printf("xwifi connect ssid password\r\n");
    bk_printf("xwifi scan [ssid]\r\n");
}

static TaskHandle_t __test_wifi_scan_thread = NULL;
static void __test_wifi_scan(void *args)
{
    CONST SCHAR_T *ssid = NULL;
    AP_IF_S *ap_ary = NULL;
    UINT_T num = 0;
    bk_printf("--- trace %s %d\r\n", __func__, __LINE__);
    OPERATE_RET ret = tkl_wifi_scan_ap(ssid, &ap_ary, &num);
    if (ret != 0) {
        bk_printf("scan error\r\n");
    }

    if (num != 0) {
        bk_printf("scan count: %d\r\n", num);
        for (int i = 0; i < num; i++) {
            bk_printf("ssid %16s, channel %02d, rssi %8d, bssid %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                    (ap_ary[i].ssid == NULL? "null": ap_ary[i].ssid), ap_ary[i].channel, ap_ary[i].rssi,
                    ap_ary[i].bssid[0], ap_ary[i].bssid[1], ap_ary[i].bssid[2],
                    ap_ary[i].bssid[3], ap_ary[i].bssid[4], ap_ary[i].bssid[5]);
        }
    } else {
        bk_printf("no ap scan\r\n");
    }

    bk_printf("--- trace %s %d\r\n", __func__, __LINE__);
    tkl_wifi_release_ap(ap_ary);

    tkl_system_sleep(1000);

    tkl_thread_release(NULL);
    __test_wifi_scan_thread = NULL;
}

void cli_wifi_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2) {
        bk_printf("parameter error\r\n");
        __cmd_usage(); return;
    }
    for (int i = 0; i < argc; i++) {
        bk_printf("argv[%d]: %s\r\n", i, argv[i]);
    }

    if (!os_strcmp(argv[1], "connect")) {
        if (argv[2] != NULL) {
            bk_printf("station connect ssid: %s pwd: %s\r\n", argv[2], argv[3]);
            tkl_wifi_set_work_mode(WWM_STATION);
            tkl_system_sleep(1000);
            tkl_wifi_station_connect(argv[2], argv[3]);
        } else {
            __cmd_usage();
        }
    } else if (!os_strcmp(argv[1], "scan")) {
        tkl_thread_create(&__test_wifi_scan_thread, "tscan", 4096, 5,  __test_wifi_scan, NULL);
    } else if (!os_strcmp(argv[1], "ap")) {
        bk_printf("not support ap test\r\n");
    }
}



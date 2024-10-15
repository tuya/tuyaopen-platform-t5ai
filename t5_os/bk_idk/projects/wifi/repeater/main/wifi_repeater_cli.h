#pragma once
#include <stdlib.h>
#include <string.h>
#include <common/bk_include.h>
#include "cli_config.h"
#include "bk_wifi_types.h"
#include "bk_wifi.h"
#include <os/str.h>
#include <os/mem.h>
#include <os/os.h>
#include <common/bk_err.h>
#include <components/log.h>
#include <components/event.h>
#include <common/sys_config.h>
#include <driver/uart.h>
#include "bk_uart.h"
#include <driver/uart.h>
#include <components/netif.h>
#include "bk_cli.h"
#include "cli.h"
#include "bk_private/bk_wifi.h"
#include "bk_wifi_private.h"


#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_REPEATER_TAG "RPT"

#define IPERF_LOGI(...) BK_LOGI(WIFI_REPEATER_TAG, ##__VA_ARGS__)
#define IPERF_LOGW(...) BK_LOGW(WIFI_REPEATER_TAG, ##__VA_ARGS__)
#define IPERF_LOGE(...) BK_LOGE(WIFI_REPEATER_TAG, ##__VA_ARGS__)
#define IPERF_LOGD(...) BK_LOGD(WIFI_REPEATER_TAG, ##__VA_ARGS__)


#define WIFI_REPEATER_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define WIFI_REPEATER_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

#define WIFI_REPEATER_EVT_GOT_IP                    "EVT:GOT-IP\r\n"


#ifdef __cplusplus
}
#endif



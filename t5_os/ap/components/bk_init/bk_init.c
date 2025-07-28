// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <components/log.h>
#include <modules/wifi.h>
#include <components/netif.h>
#include <components/event.h>
#include <driver/uart.h>
#include <string.h>
#include "boot.h"
#include <modules/pm.h>
#include "aon_pmu_driver.h"
#include <driver/pwr_clk.h>
#include "bk_rtos_debug.h"
#if CONFIG_ROSC_CALIB_SW
#include <driver/rosc_32k.h>
#endif
#if CONFIG_BLUETOOTH_AP
#include "components/bluetooth/bk_dm_bluetooth.h"
#include "components/bluetooth/bk_ble.h"
//#include "ble_api_5_x.h"

#endif
//#include <bk_wifi_adapter.h>
// #include <bk_phy_adapter.h>
// #include <bk_rf_adapter.h>
#if (CONFIG_PSRAM)
#include <driver/psram.h>
#endif

#if CONFIG_USB
#include <components/usb.h>
#endif

#if CONFIG_CAN
#include <driver/can.h>
#endif

#if (CONFIG_OTA_UPDATE_DEFAULT_PARTITION && CONFIG_HTTP_AB_PARTITION)
#include <modules/ota.h>
extern void bk_ota_confirm_update_partition(ota_confirm_flag ota_confirm_val);
#endif

#if (CONFIG_CLI)
#include "bk_api_cli.h"
#else
#if CONFIG_SHELL_ASYNCLOG
#include "bk_api_cli.h"
#endif
#endif

#if defined(CONFIG_WIFI_AT_ENABLE) && defined(CONFIG_WIFI_ENABLE)
#include "wifi_at.h"
#endif
#if defined(CONFIG_BT_AT_ENABLE) && defined(CONFIG_BT)
#include "bk_at_bluetooth.h"
#endif
#if defined(CONFIG_NETIF_AT_ENABLE) && defined(CONFIG_WIFI_ENABLE)
#include "bk_at_netif.h"


#endif
#if (CONFIG_NTP_SYNC_RTC)
#include <components/ate.h>
#include <components/app_time_intf.h>
#endif

#define TAG "bk_init"

#ifdef CONFIG_VND_CAL
#include "vnd_cal.h"
#endif

#if CONFIG_BUTTON
#include "key_main.h"
#endif

#if (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)
#include "sys_ll.h"
// #include "bk_saradc.h"
// #include "temp_detect_pub.h"
#endif

#if CONFIG_WIFI_PS_DISABLE
#include "modules/wifi.h"
#endif

#if (CONFIG_SDIO_V2P0 && CONFIG_SDIO_SLAVE)
#include "sdio_slave_driver.h"
#if CONFIG_SDIO_TEST_EN
#include "sdio_test.h"
#endif
#endif

#if CONFIG_AT
#include "atsvr_unite.h"
#endif

#if (CONFIG_DEBUG_VERSION)
extern bk_err_t bk_dbg_init(void);
#endif

// #ifdef CONFIG_WIFI_VNET_CONTROLLER
// #include "wdrv_main.h"
// #endif
#include "wifi_api.h"

void rtos_user_app_launch_over(void);

extern const uint8_t build_version[]; // = __DATE__ " " __TIME__;

int app_phy_init(void)
{
    return BK_OK;
}

static int app_wifi_init(void)
{
#if 1//(CONFIG_WIFI_ENABLE)
	//wifi_init_config_t wifi_config = WIFI_DEFAULT_INIT_CONFIG();
	BK_LOG_ON_ERR(bk_event_init());
	BK_LOG_ON_ERR(bk_netif_init());
	BK_LOG_ON_ERR(bk_wifi_init());
#endif

#if (CONFIG_WIFI_ENABLE)
#if (CONFIG_DEBUG_VERSION)
	BK_LOG_ON_ERR(bk_dbg_init());
#endif

#if CONFIG_WIFI_PS_DISABLE
	//disable ps if needed
	bk_wifi_sta_pm_disable();
#endif
#if CONFIG_WIFI_AT_ENABLE
	wifi_at_cmd_init();
#endif
#if CONFIG_NETIF_AT_ENABLE
	netif_at_cmd_init();
#endif

#endif
	return BK_OK;
}

static int app_ble_init(void)
{
#if CONFIG_BLUETOOTH_AP
    BK_LOG_ON_ERR(bk_bluetooth_init());
#endif
	return BK_OK;
}

static int app_bt_init(void)
{
#if (CONFIG_BT)
	BK_LOGD(TAG, "BT active\r\n");
#if 0//TODO
	if (!ate_is_enabled())
		bt_activate(NULL);
#endif
#if (CONFIG_BT_AT_ENABLE)
	bt_at_cmd_init();
#endif

#endif
	return BK_OK;
}

static int app_key_init(void)
{
#if CONFIG_BUTTON
	key_initialization();
#endif
	return BK_OK;
}

static int app_mp3_player_init(void)
{
#if (CONFIG_MP3PLAYER)
	key_init();
	media_thread_init();
#endif
	return BK_OK;
}

int app_sdio_init(void)
{
#if (CONFIG_SDIO_V2P0 && CONFIG_SDIO_SLAVE)
	bk_sdio_slave_driver_init();
#if CONFIG_SDIO_TEST_EN
	bk_sdio_test_init();
#endif
#endif
	return BK_OK;
}

int app_usb_init(void)
{
#if CONFIG_USB
	BK_LOGD(TAG, "fusb init\r\n");
#if 0//TODO
	if (!ate_is_enabled())
		fusb_init();
#endif
#endif
	return BK_OK;
}

static int app_cli_init(void)
{
#if (CONFIG_CLI)
#if !CONFIG_FULLY_HOSTED
	bk_cli_init();
#endif
#else
#if CONFIG_SHELL_ASYNCLOG
	bk_cli_init();
#endif
#endif
	return BK_OK;
}

static int app_usb_charge_init(void)
{
#if CONFIG_USB_CHARGE
	extern void usb_plug_func_open(void);
	usb_plug_func_open();
#endif
	return BK_OK;
}

static int app_uart_debug_init_todo(void)
{
#if CONFIG_UART_DEBUG
#ifndef KEIL_SIMULATOR
	BK_LOGD(TAG, "uart debug init\r\n");
	uart_debug_init();
#endif
#endif
	return BK_OK;
}

#if CONFIG_ETH
extern int net_eth_start();
static int app_eth_init(void)
{
	static bool app_eth_inited = false;

	if (!app_eth_inited) {
		BK_LOGD(TAG, "ETH init\n");
		net_eth_start();
		app_eth_inited = true;
	}

	return BK_OK;
}

static void app_eth_init_thread(void *arg)
{
	app_eth_init();

	rtos_delete_thread(NULL);
}

void start_eth_init_thread(void)
{
	rtos_create_thread(NULL, CONFIG_APP_MAIN_TASK_PRIO,
		"eth_init",
		(beken_thread_function_t)app_eth_init_thread,
		CONFIG_APP_MAIN_TASK_STACK_SIZE,
		(beken_thread_arg_t)0);
}
#endif

#if CONFIG_ENABLE_WIFI_DEFAULT_CONNECT
extern void demo_wifi_fast_connect(void);
#endif

int components_init(void);

int bk_init(void)
{
    set_ap_startup_index(AP_ENTER_BK_INIT);

	components_init();

	BK_LOGD(TAG, "armino app init: %s\n", build_version);
	BK_LOGD(TAG, "verify id: %s\n", BK_VERIFY_ID);

#ifdef APP_VERSION
	BK_LOGD(TAG, "APP Version: %s\n", APP_VERSION);
#endif


#ifdef CONFIG_VND_CAL
	vnd_cal_overlay();
#endif

	app_cli_init();

	bk_event_init();

#if 0   // CONFIG_AT
    set_ap_startup_index(AP_ENTER_AT_SERVER_INIT);
	at_server_init();

	extern int atsvr_app_init(void);
	if(0 != atsvr_app_init())
		return -1;
	extern void wifi_at_cmd_init(void);
	wifi_at_cmd_init();
    set_ap_startup_index(AP_EXIT_AT_SERVER);
#endif


#if (CONFIG_NTP_SYNC_RTC)
    // if(ate_is_enabled() == 0)
    // {
	  //   app_time_rtc_ntp_sync_init();
    // }
    app_time_rtc_ntp_sync_init();
#endif


#if (CONFIG_FREERTOS)
#if CONFIG_SEMI_HOSTED
	semi_host_init();
#endif
#endif

#if CONFIG_UDISK_MP3
	um_init();
#endif

#if CONFIG_ENABLE_WIFI_DEFAULT_CONNECT
	demo_wifi_fast_connect();
#endif

#if (CONFIG_OTA_UPDATE_DEFAULT_PARTITION&& CONFIG_HTTP_AB_PARTITION)
#if (CONFIG_OTA_POSITION_INDEPENDENT_AB)
	bk_ota_double_check_for_execution();
#endif
#endif

#if CONFIG_MICROPYTHON
extern int mp_do_startup(int heap_len);
	mp_do_startup(0);
#endif

#if CONFIG_PSRAM
	REG_READ(SOC_PSRAM_DATA_BASE);//check psram whether valid
#endif
	bk_pm_cp1_boot_ok_response_set();

#if CONFIG_USB //&& CONFIG_MENTOR_USB
	bk_usb_driver_init();
#endif

#if CONFIG_CAN
	bk_can_driver_init();
#endif

#if (CONFIG_PSRAM)
	bk_psram_id_auto_detect();
#endif

#if 1//CONFIG_WIFI_ENABLE
    set_ap_startup_index(AP_EXIT_AT_SERVER);
    app_wifi_init();
#endif

#ifdef CONFIG_BLUETOOTH_AP
    set_ap_startup_index(AP_ENTER_APP_BLE_INIT);
    app_ble_init();
#endif

#if CONFIG_ETH
	start_eth_init_thread();
#endif

    set_ap_startup_index(AP_EXIT_BK_INIT);
	return 0;
}

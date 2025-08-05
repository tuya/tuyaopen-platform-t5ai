/*
 * Copyright 2020-2025 Beken

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *     http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "os/os.h"
#include "wdrv_co_list.h"
#include "wdrv_cntrl.h"
#include "wifi_log.h"
//#include "wifi_types.h"

#define wifi_lock() \
    do{\
        GLOBAL_INT_DECLARATION();\
        GLOBAL_INT_DISABLE();
#define wifi_unlock() \
        GLOBAL_INT_RESTORE();\
    }while(0);

/* bk_wifi_init() set the bit, bk_wifi_deinit() clear the bit */
#define WIFI_INIT_BIT               (1)

/* bk_wifi_sta_start() set the bit, bk_wifi_sta_stop() clear the bit */
#define WIFI_STA_STARTED_BIT        (1<<1)
#define WIFI_STA_CONFIGURED_BIT     (1<<2)

/* bk_wifi_sta_connect() set the bit, bk_wifi_sta_disconnect() clear the bit */
/* It doesn't indicate the STA is connected, it only means the API is called */
/* or NOT. */
#define WIFI_STA_CONNECTED_BIT      (1<<3)

/* bk_wifi_ap_start() set the bit, bk_wifi_ap_stop() clear the bit */
#define WIFI_AP_STARTED_BIT         (1<<4)
#define WIFI_AP_CONFIGURED_BIT      (1<<5)
#define WIFI_PURE_SCAN_STARTED_BIT  (1<<7)

#define WIFI_RESERVED_BYTE_VALUE    0

#define CONFIG_ROLE_NULL        0
#define CONFIG_ROLE_AP          1
#define CONFIG_ROLE_STA         2

typedef struct {
	uint8_t is_sta_up;
	uint8_t is_ap_up;
	wifi_link_status_t link_status;
	netif_ip4_config_t sta_ip4_info;
	wifi_ap_config_t ap_info;
	netif_ip4_config_t ap_ip4_info;
} wifi_status_t;

bk_err_t bk_wifi_api_test(void);
bk_err_t bk_wifi_get_status(wifi_status_t *status);


/**************************WLAN API**************************/
/**
 * @brief     Start the BK STA
 *
 * This API init the resoure specific to BK STA, e.g. init STA specific globals, init
 * STA specific WiFi driver etc.
 *
 */
bk_err_t bk_wifi_init(void);

/**
 * @brief     Start the BK STA
 *
 * This API init the resoure specific to BK STA, e.g. init STA specific globals, init
 * the supplicant and STA specific WiFi driver etc.
 *
 * @attention 1. Make sure the bk_wifi_sta_set_config() are succeedful before calling this API.
 * @attention 2. This API connect the BK STA to the configured AP automatically.
 * @attention 3. If the BK STA is already started, this API ignores the request and returns BK_OK.
 *               if you want to restart the BK STA, call bk_wifi_sta_stop() first and then call
 *               this API.
 * @attention 4. TODO description about fast connection
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_WIFI_STA_NOT_CONFIG: the BK STA is not configured.
 *    - BK_ERR_WIFI_MONITOR_IP: the BK STA can't be started because monitor is started.
 *    - others: other errors.
 */
bk_err_t bk_wifi_sta_start(void);

/**
 * @brief     Stop the BK STA
 *
 * @attention This API causes the BK STA disconnect from the AP, it similar as
 *            bk_wifi_sta_disconnect(), but this API may free resource.
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_FAIL: fail
 */
bk_err_t bk_wifi_sta_stop(void);

/**
 * @brief     Config the BK STA
 *
 * This API configures the basic configurations of the BK STA, it should be called
 * before we call bk_wifi_sta_start() to start the BK STA. It can also be used to
 * update the configuration after the BK STA is started.
 *
 * Usage example:
 *
 *     wifi_sta_config_t sta_config = WIFI_DEFAULT_STA_CONFIG();
 *
 *     os_strncpy(sta_config.ssid, "ssid", WIFI_SSID_STR_LEN);
 *     os_strncpy(sta_config.password, "password", WIFI_PASSWORD_LEN);
 *     //more initialization here
 *     BK_LOG_ON_ERR(bk_wifi_sta_set_config(&sta_config));
 *
 * @attention 1. Don't call this API when the monitor is in-progress
 * @attention 2. If STA is already connected to AP, this API cases BK STA reconnects the AP.
 * @attention 3. Make sure the reserved fields in sta_config is zero, otherwise you may
 *               encounter compatibility issue in future if more config fields are added.
 * @attention 4. Auto reconnect max count and timeout can be set. When user app receives
 *               EVENT_WIFI_STA_DISCONNECTED event, it's user app's responsibility to
 *               reconnect to AP manually. Max count and timeout set to zero to let
 *               supplicant automatically handles connection without user app's interaction.
 *
 * @param sta_config the STA configuration
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_WIFI_NOT_INIT: the STA is not initialized, call bk_wifi_init() first.
 *    - BK_ERR_NULL_PARAM: parameter config is NULL.
 *    - BK_ERR_WIFI_RESERVED_FIELD: the reserved field of config is not 0.
 *    - others: other errors
 */
bk_err_t bk_wifi_sta_set_config(const wifi_sta_config_t *sta_config);

/**
 * @brief     Get the configuration of the BK STA.
 *
 * @param sta_config store the BK STA configuration
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_NULL_PARAM: the parameter config is NULL.
 *    - BK_ERR_WIFI_STA_NOT_CONFIG: STA is not configured yet.
 *    - others: other errors
 */
bk_err_t bk_wifi_sta_get_config(wifi_sta_config_t *sta_config);

/**
 * @brief     Get the MAC of BK STA
 *
 * @attention 1. The AP's MAC is derived from the base MAC of the system.
 * @attention 2. If you want to change the MAC of AP, call bk_set_mac() to set
 *               the base MAC of the system.
 *
 * @param mac store the MAC of BK STA
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors
 */

bk_err_t bk_wifi_sta_get_mac(uint8_t *mac);

/**
 * @brief     Get the MAC of BK AP
 *
 * @attention 1. The AP's MAC is derived from the base MAC of the system.
 * @attention 2. If you want to change the MAC of AP, call bk_set_mac() to set
 *               the base MAC of the system.
 *
 * @param mac store the MAC of BK AP
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors
 */
bk_err_t bk_wifi_ap_get_mac(uint8_t *mac);

/**
 * @brief     Connect the BK STA to the AP.
 *
 * @attention 1. Make sure STA is started by bk_wifi_sta_start() before calling this API.
 * @attention 2. Don't call this API when the monitor is in-progress
 * @attention 3. If STA is already connected to AP, this API reconnects the BK STA.
 * @attention 4. TODO - multiple same SSID connect???
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_WIFI_STA_NOT_STARTED: the STA is not started, call bk_wifi_sta_start() first.
 *    - BK_ERR_WIFI_MONITOR_IP: the API is not allowed because monitor is in-progress.
 *    - others: other failures.
 */
bk_err_t bk_wifi_sta_connect(void);

/**
 * @brief     Disconnect the WiFi connection of the BK STA
 *
 * @attention TODO - add description about disconnect event!
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other failures.
 */
bk_err_t bk_wifi_sta_disconnect(void);

/**
 * @brief     Start a scan
 *
 * This API notifies the WiFi driver to start a scan, the event EVENT_WIFI_SCAN_DONE will
 * be raised if the scan is completed. General steps to use this API:
 *  - prepare the scan done event callback, the callback can call bk_wifi_scan_get_result()
 *    to get the scan result and then call bk_wifi_scan_free_result() to free the resource.
 *  - call bk_event_register_cb(EVENT_MOD_WIFI, EVENT_WIFI_SCAN_DONE, ...) to register
 *    scan done event callback.
 *  - call this API to trigger this the scan.
 *
 * Usage example:
 *
 *     //Define your scan done handler
 *     int scan_done_handler(void *arg, event_module_t event_module, int event_id, void *event_data)
 *     {
 *         wifi_scan_result_t scan_result = {0};
 *
 *         BK_LOG_ON_ERR(bk_wifi_scan_get_result(&scan_result));
 *         bk_wifi_scan_dump_result(&scan_result);
 *         bk_wifi_scan_free_result(&scan_result);
 *
 *         return BK_OK;
 *     }
 *
 *     //Start the scan
 *     wifi_scan_config_t config = {0};
 *
 *     BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_WIFI, EVENT_WIFI_SCAN_DONE, scan_done_handler, NULL));
 *     BK_LOG_ON_ERR(bk_wifi_scan_start(&scan_config));
 *
 * @attention 1. This API triggers an active scan on all channels (TODO double check)
 * @attention 2. Pass NULL scan_config to scan all APs, otherwise scan the SSID specified in scan_config.ssid
 * @attention 3. Make sure the reserved fields in scan_config is zero, otherwise you may
 *               encounter compatibility issue in future if more config fields are added.
 * @attention 4. TODO scan result limits???
 *
 * @param scan_config the configuration of scan
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_WIFI_NOT_INIT: the STA is not initialized, call bk_wifi_init() first.
 *    - BK_ERR_WIFI_MONITOR_IP: the API is not allowed because monitor is in-progress.
 *    - others: other failures.
 */
bk_err_t bk_wifi_scan_start(const wifi_scan_config_t *scan_config);

/**
 * @brief     Stop the pure scan operation
 *
 * @attention This API causes the BK STA disconnect from the AP, it similar as
 *            bk_wifi_sta_disconnect(), but this API may free resource.
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_FAIL: fail
 */
bk_err_t bk_wifi_scan_stop(void);

/**
 * @brief     Start the BK AP
 *
 * This API init the resoure specific to BK AP, e.g. init BK AP specific globals, init
 * the hostapd and AP specific WiFi driver etc.
 *
 * If the BK AP is already started, this API ignores the request and returns BK_OK, we can
 * call bk_wifi_ap_stop() to stop BK AP first and then call bk_wifi_ap_start() to restart it.
 *
 * **Restart** AP Usage example:
 *
 *     BK_LOG_ON_ERR(bk_wifi_ap_stop());
 *     BK_LOG_ON_ERR(bk_wifi_ap_start());
 *
 * @attention 1. Don't call this API when the monitor is in-progress
 * @attention 2. If bk_wifi_ap_set_config() is not called, this API start the AP with
 *               default value, normally you should configure the AP first before calling
 *               this API.
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_WIFI_AP_NOT_CONFIG: the WiFi AP is not configured, call bk_wifi_ap_set_config() first.
 *    - others: other errors
 */
bk_err_t bk_wifi_ap_start(void);

/**
 * @brief     Stop the BK AP
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_WIFI_NOT_INIT: the WiFi is not initialized, call bk_wifi_init() first.
 *    - others: other errors
 */
bk_err_t bk_wifi_ap_stop(void);

/**
 * @brief     Get the WiFi link info of the BK STA.
 *
 * Get the actual WiFi link status of the BK STA.
 *
 * @attention the difference between this API and bk_wifi_sta_get_config() is
 *     - This API gets the actual STA link info while the later gets the configured value
 *     - This API can get more info of the link, such as RSSI, WiFi link status, AID etc.
 *     - The AID is only valid if @link_status->state is WIFI_LINK_CONNECTED.
 *
 * @param link_status store the WiFi link status of the BK STA
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_NULL_PARAM: the parameter link_info is NULL.
 *    - BK_ERR_WIFI_DRIVER: WiFi low level driver has failure.
 *    - BK_ERR_WIFI_NO_MEM: Not enough memory
 *    - others: other errors
 */
bk_err_t bk_wifi_sta_get_link_status(wifi_link_status_t *link_status);

/**
 * @brief     Configure wifi multimedia mode when the multimedia is running.
 *
 * This API is used to configure multimedia mode of Wi-Fi. Multimedia
 * mode can enchance video fluency and make a better experience of video.
 *
 * @attention 1. If you want to running multimedia mode,please configure
 *               this flag when open and close multimedia mode.
 * @attention 2. Please set this flag before running multimedia and clear this
 *               flag after closing multimedia.
 * @attention 3. If the flag is set when multimedia is turned on and it is not
 *               cleared after multimedia is turned off,the Wi-Fi will not be
 *               able to enter ps mode.
 *
 * @param flag set true if you want to enable multimedia mode.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors
 */
bk_err_t bk_wifi_set_wifi_media_mode(bool flag);

/**
 * @brief     Configure video quality when the video is running.
 *
 * This API is used to configure video quality.There are three mode to choose:
 *
 * Fluent definition(FD): The video transmission is relatively stable and has strong
 * anti-interference ability,which can ensure the stable video transmission in a
 * complex enviroment;
 *
 * Standard definition(SD):The video transmission is stable in a clean environment and
 * is slightly stuck in a complex environment,which transmission bandwidth is improved;
 *
 * High definition(HD):The video transmission gives priority to high speed which can
 * transmit high-bandwidth data in a clean environment, but the video transmission is
 * relatively stuck in a complex environment;
 *
 * The default value is HD mode when the video is running.
 *
 * @attention 1. If you want to running video mode,please configure this parameter
 *               when open video mode.
 * @attention 2. Please configure this parameter before running video mode.
 * @attention 3. Please configure bk_wifi_set_wifi_media_mode(flag) before useing this
 *               API.
 *
 * @param quality set the video quality when the video is running. Three mode
 *                  can be choose: 0-FD,1-SD,2-HD,default is HD.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors
 */
bk_err_t bk_wifi_set_video_quality(uint8_t quality);

/**
 * @brief     Config the BK AP
 *
 * Usage example:
 *
 *     wifi_ap_config_t ap_config = WIFI_DEFAULT_AP_CONFIG();
 *
 *     os_strncpy(ap_config.ssid, "ssid", WIFI_SSID_STR_LEN);
 *     os_strncpy(ap_config.password, "password", WIFI_PASSWORD_LEN);
 *     BK_LOG_ON_ERR(bk_wifi_ap_set_config(&ap_config));
 *
 * @attention 1. Make sure the reserved field in config is zero, otherwise you may
 *               encounter compatibility issue in future if more config fields are added.
 *
 * @param ap_config the AP configuration
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_NULL_PARAM: the ap_config is NULL.
 *    - others: other errors
*/
bk_err_t bk_wifi_ap_set_config(const wifi_ap_config_t *ap_config);

/**
 * @brief     Start the BK AP
 *
 * This API init the resoure specific to BK AP, e.g. init BK AP specific globals, init
 * the hostapd and AP specific WiFi driver etc.
 *
 * If the BK AP is already started, this API ignores the request and returns BK_OK, we can
 * call bk_wifi_ap_stop() to stop BK AP first and then call bk_wifi_ap_start() to restart it.
 *
 * **Restart** AP Usage example:
 *
 *     BK_LOG_ON_ERR(bk_wifi_ap_stop());
 *     BK_LOG_ON_ERR(bk_wifi_ap_start());
 *
 * @attention 1. Don't call this API when the monitor is in-progress
 * @attention 2. If bk_wifi_ap_set_config() is not called, this API start the AP with
 *               default value, normally you should configure the AP first before calling
 *               this API.
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_WIFI_AP_NOT_CONFIG: the WiFi AP is not configured, call bk_wifi_ap_set_config() first.
 *    - others: other errors
 */

bk_err_t bk_wifi_ap_start(void);

/**
 * @brief     Stop the BK AP
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_WIFI_NOT_INIT: the WiFi is not initialized, call bk_wifi_init() first.
 *    - others: other errors
 */
bk_err_t bk_wifi_ap_stop(void);


#ifdef __cplusplus
}
#endif


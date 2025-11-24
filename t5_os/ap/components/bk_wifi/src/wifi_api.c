/*
 * Copyright 2020-2025 Beken
 *
 * @file wdrv_api.c 
 * 
 * @brief Beken Wi-Fi Driver Command Control Entry
 *
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

#include "wifi_api.h"
#include "wdrv_main.h"
#include <common/bk_include.h>
#include <common/bk_err.h>
#include <stdint.h>
#include <stdbool.h>
#include <os/str.h>
#include <os/mem.h>
#include <os/os.h>
#include "net.h"
#include "wdrv_cntrl.h"
#include "wdrv_co_list.h"
#include "wdrv_tx.h"
#include "wifi_api_ipc.h"
#include "wdrv_cntrl.h"
#if CONFIG_NETIF_LWIP
#include "lwip/inet.h"
#include "net.h"
#include "lwip/netif.h"
#endif

general_param_t *g_wlan_general_param = NULL;
ap_param_t *g_ap_param_ptr = NULL;
sta_param_t *g_sta_param_ptr = NULL;
struct scan_cfg_scan_param_tag scan_param_env = {0};

static wifi_monitor_cb_t s_monitor_ap_cb = NULL;
static wifi_filter_cb_t s_filter_ap_cb = NULL;
/* State Indication */
static uint16_t s_wifi_state_bits = 0;
static inline void wifi_set_state_bit(uint16_t state_bit)
{
    wifi_lock();
    s_wifi_state_bits |= state_bit;
    wifi_unlock();
}

static inline void wifi_clear_state_bit(uint16_t state_bit)
{
    wifi_lock();
    s_wifi_state_bits &= ~state_bit;
    wifi_unlock();
}

static inline bool wifi_is_inited(void)
{
    return (s_wifi_state_bits & WIFI_INIT_BIT);
}

bool wifi_sta_is_started(void)
{
    return (s_wifi_state_bits & WIFI_STA_STARTED_BIT);
}

static inline bool wifi_sta_is_connected(void)
{
    return (s_wifi_state_bits & WIFI_STA_CONNECTED_BIT);
}

bool wifi_ap_is_started(void)
{
    return (s_wifi_state_bits & WIFI_AP_STARTED_BIT);
}

static inline bool wifi_sta_is_configured(void)
{
    return (s_wifi_state_bits & WIFI_STA_CONFIGURED_BIT);
}

static inline bool wifi_ap_is_configured(void)
{
    return (s_wifi_state_bits & WIFI_AP_CONFIGURED_BIT);
}

/* API Reference */
static int wifi_sta_validate_config(const wifi_sta_config_t *config)
{
    if (!config) {
        WDRV_LOGV("sta config fail, null config\n");
        return BK_ERR_NULL_PARAM;
    }

    //TODO more check
    return BK_OK;
}


//TODO optimize param_config.c
//Init global STA configurations
static int wifi_sta_init_global_config(void)
{
    BK_ASSERT(g_sta_param_ptr); /* ASSERT VERIFIED */
    BK_ASSERT(g_wlan_general_param); /* ASSERT VERIFIED */

    bk_wifi_sta_get_mac((uint8_t *)(&g_sta_param_ptr->own_mac));
    g_wlan_general_param->role = CONFIG_ROLE_STA;
    WDRV_LOGD("wdrv mac addr:"BK_MAC_FORMAT"\r\n", BK_MAC_STR(g_sta_param_ptr->own_mac) );

    return BK_OK;
}

static int wifi_scan_init_global_config(void)
{
    BK_ASSERT(g_sta_param_ptr); /* ASSERT VERIFIED */
    BK_ASSERT(g_wlan_general_param); /* ASSERT VERIFIED */

    bk_wifi_sta_get_mac((uint8_t *)(&g_sta_param_ptr->own_mac));

    return BK_OK;
}

//Set STA configuration to global configuration
static int wifi_sta_set_global_config(const wifi_sta_config_t *config)
{
    g_sta_param_ptr->ssid.length = MIN(SSID_MAX_LEN, os_strlen(config->ssid));
    memcpy(g_sta_param_ptr->ssid.array, config->ssid, g_sta_param_ptr->ssid.length);

    g_sta_param_ptr->cipher_suite = config->security;

    g_sta_param_ptr->key_len = os_strlen(config->password);
    os_memcpy(g_sta_param_ptr->key, config->password, g_sta_param_ptr->key_len);
    g_sta_param_ptr->key[g_sta_param_ptr->key_len] = 0;

    WDRV_LOGD("sta config, ssid=%s password=%s security=%d\n",
                      config->ssid, config->password, config->security);
    return BK_OK;
}

static int wifi_sta_get_global_config(wifi_sta_config_t *sta_config)
{
    if (!sta_config)
        return BK_ERR_NULL_PARAM;

    os_memset(sta_config, 0, sizeof(sta_config));
    os_memcpy(sta_config->ssid, g_sta_param_ptr->ssid.array, g_sta_param_ptr->ssid.length);

    os_memcpy(sta_config->password, g_sta_param_ptr->key, g_sta_param_ptr->key_len);

    WDRV_LOGV("sta get sta_config, ssid=%s password=%s security=%d\n",
                  sta_config->ssid, sta_config->password, sta_config->security);

    return BK_OK;
}

bk_err_t bk_wifi_init(void)
{
    WDRV_LOGD("%s, %d\r\n", __func__, __LINE__);
    uint8_t mac[ETH_ALEN];

#ifdef CONFIG_WIFI_VNET_CONTROLLER
    wdrv_init();
#endif

    if (wifi_is_inited())
    {
        WDRV_LOGD("wifi already init!\n");
        host_wlan_remove_netif();
    }

    bk_wifi_sta_get_mac((uint8_t *)mac);
    host_wlan_add_netif(mac);
    //ToDo: AP Netif should add separated
    //bk_wifi_ap_get_mac((uint8_t *)mac);
    //host_wlan_add_netif(mac);

    wifi_set_state_bit(WIFI_INIT_BIT);
    WDRV_LOGD("wifi inited(%x)\n", s_wifi_state_bits);

    return BK_OK;
}

bk_err_t bk_wifi_sta_stop(void)
{
    bk_err_t ret = BK_OK;

    WDRV_LOGD("sta stopping\n");

    if (!wifi_sta_is_started()) {
        WDRV_LOGD("sta stop, already stopped\n");
        return BK_OK;
    }

    bk_wifi_sta_disconnect();

    /* Post CMD to CIF */
    ret = wifi_send_com_api_cmd(STA_STOP, 0);
    if (ret != BK_OK)
    {
        WDRV_LOGD("%s failed, ret=%d\n",__func__, ret);
    }

#if CONFIG_LWIP
    host_wlan_remove_netif();
#endif

    wifi_clear_state_bit(WIFI_STA_STARTED_BIT);
    WDRV_LOGD("sta stopped(%x)\n", s_wifi_state_bits);

    return BK_OK;
}

bk_err_t bk_wifi_sta_disconnect(void)
{
    WDRV_LOGD("sta disconnecting\n");

    if (wifi_sta_is_connected()) {
#if CONFIG_LWIP
        sta_ip_down();
#endif

        //TODO do we need to post the disconnect event?
        wifi_clear_state_bit(WIFI_STA_CONNECTED_BIT);
    }

    WDRV_LOGD("sta disconnected(%x)\n", s_wifi_state_bits);
    return BK_OK;
}

bk_err_t  bk_wifi_sta_get_mac(uint8_t *mac)
{
    bk_err_t ret = 0;
    void *buffer_to_ipc = NULL;

    if (!mac)
        return BK_ERR_NULL_PARAM;

    buffer_to_ipc = os_malloc(WIFI_MAC_LEN);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    ret = wifi_send_com_api_cmd(STA_GET_MAC, 1, (uint32_t)buffer_to_ipc);
    os_memcpy(mac, buffer_to_ipc, WIFI_MAC_LEN);
    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_ap_get_mac(uint8_t *mac)
{
    bk_err_t ret = 0;
    void *buffer_to_ipc = NULL;

    if (!mac)
        return BK_ERR_NULL_PARAM;

    buffer_to_ipc = os_malloc(WIFI_MAC_LEN);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    ret = wifi_send_com_api_cmd(AP_GET_MAC, 1, (uint32_t)buffer_to_ipc);
    os_memcpy(mac, buffer_to_ipc, WIFI_MAC_LEN);
    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_set_wifi_media_mode(bool flag)
{
    return wifi_send_com_api_cmd(WIFI_SET_MEDIA_MODE, 1, flag);
}

bk_err_t bk_wifi_set_video_quality(uint8_t quality)
{
    return wifi_send_com_api_cmd(WIFI_SET_VIDEO_QUALITY, 1, quality);
}

bk_err_t bk_wifi_set_csa_coexist_mode_flag(bool is_close)
{
    if (s_wifi_state_bits & WIFI_STA_STARTED_BIT)
    {
        WDRV_LOGW("Set csa coxist mode before starting station! state_bit=0x%x\r\n",s_wifi_state_bits);
        return BK_ERR_STATE;
    }

    return wifi_send_com_api_cmd(WIFI_SET_CSA_COEXIST_MODE_FLAG, 1, is_close);
}

#if 1 //CONFIG_WIFI_SOFTAP

static inline int is_zero_ether_addr(const u8 *a)
{
    return !(a[0] | a[1] | a[2] | a[3] | a[4] | a[5]);
}

void bk_wifi_ap_init(void)
{
    WDRV_LOGD("%s, %d\r\n", __func__, __LINE__);
    uint8_t mac[ETH_ALEN];

    if(wifi_is_inited())
    {
        WDRV_LOGD("wifi already init, reinit anyway!\n");
        uap_ip_down();
        host_wlan_remove_sap_netif();
    }

    bk_wifi_ap_get_mac((uint8_t *)mac);
    host_wlan_add_netif(mac);

    wifi_set_state_bit(WIFI_INIT_BIT);
    WDRV_LOGD("wifi inited(%x)\n", s_wifi_state_bits);

}

bk_err_t bk_wifi_ap_start(void)
{
    bk_err_t ret = BK_OK;

    WDRV_LOGV("ap starting\n");

    if (!wifi_ap_is_configured()) {
        WDRV_LOGV("start ap failed, ap not configured\n");
        return BK_ERR_WIFI_AP_NOT_CONFIG;
    }

    if (wifi_ap_is_started()) {
        WDRV_LOGV("start ap, already started, ignored\n");
        return BK_OK;
    }

#if CONFIG_LWIP
    WDRV_LOGV("ap start, ip down\n");
    //TODO move to event handler
    uap_ip_down();
#endif

    WDRV_LOGV("ap start, ap start rf\n");
#if 0
    if(wifi_ap_init_rw_driver()) {
        WDRV_LOGE("ap start fail,ap init rw driver fail!\n");
        return BK_ERR_WIFI_AP_NOT_STARTED;
    }
    //TODO return value
    if(wlan_ap_enable()) {
        WDRV_LOGE("ap start fail,ap enable fail!\n");
        return BK_ERR_WIFI_AP_NOT_STARTED;
    }
    if(wlan_ap_reload()) {
        WDRV_LOGE("ap start fail,ap reload fail!\n");
        return BK_ERR_WIFI_AP_NOT_STARTED;
    }
#endif

    bk_wifi_ap_init();

    ret = wifi_send_com_api_cmd(AP_START, 0);
    if (ret != BK_OK)
    {
        WDRV_LOGW("ap start failed, ret=%d\n", ret);
    }

    WDRV_LOGD("ap started\n");

#if CONFIG_LWIP
    //TODO move to event handler
#if CONFIG_BRIDGE
	if (bk_wifi_get_bridge_state() == BRIDGE_STATE_DISABLED)
#endif
		uap_ip_start();
#endif

    wifi_set_state_bit(WIFI_AP_STARTED_BIT);
    return BK_OK;
}

bk_err_t wifi_ap_validate_config(const wifi_ap_config_t *ap_config)
{
    if (!ap_config)
        return BK_ERR_NULL_PARAM;
#if (CONFIG_SOC_BK7239XX) && CONFIG_WIFI_BAND_5G
    if (ap_config->channel >= 36 && ap_config->channel <=165) {

        //check if configured channel is avaliable channel and no need for radat detection
        int selected_channels_size = 0;
        extern int* rw_select_5g_non_radar_avaliable_channels(int *selected_channels_size);
        int *non_radar_avaliable_channels = rw_select_5g_non_radar_avaliable_channels(&selected_channels_size);

        for (int i = 0; i < selected_channels_size; i++) {
            if (non_radar_avaliable_channels[i] == ap_config->channel)
                return BK_OK;
        }

        //TODO more parameter checking
        WDRV_LOGE("[%s]configured unavaliable or dfs channel\r\n",__FUNCTION__);
        return BK_ERR_NOT_FOUND;
    }
#endif
    return BK_OK;
}

static bk_err_t wifi_ap_set_config(const wifi_ap_config_t *ap_config)
{
    BK_ASSERT(g_ap_param_ptr); /* ASSERT VERIFIED */
    BK_ASSERT(g_wlan_general_param); /* ASSERT VERIFIED */

    if (is_zero_ether_addr((u8 *)&g_ap_param_ptr->bssid))
        bk_wifi_ap_get_mac((uint8_t *)(&g_ap_param_ptr->bssid));

    //TODO
    if ((ap_config->channel >= 1 && ap_config->channel <=14)
#if CONFIG_SOC_BK7239XX
        || (ap_config->channel >= 36 && ap_config->channel <=165)
#endif
        ) {
        g_ap_param_ptr->chann = ap_config->channel;
    } else if (ap_config->channel == 0){
        g_ap_param_ptr->chann = 0;
    } else {
        WDRV_LOGE("error:invalid channel\r\n");
        return BK_FAIL;
    }

    if(ap_config->max_con == 0 || ap_config->max_con > 2) {
        if(ap_config->max_con == 0)
            WDRV_LOGW("the max conn num is zero, set it is default\n");
        if(ap_config->max_con > 2)
            WDRV_LOGW("the max conn num is more than SUPPORTED_MAX_STA_NUM, set it is default\n");

        g_ap_param_ptr->max_con = 2;
    } else {
        g_ap_param_ptr->max_con = ap_config->max_con;
    }
    g_wlan_general_param->role = CONFIG_ROLE_AP;
    //TODO why need this???
    //bk_wlan_set_coexist_at_init_phase(CONFIG_ROLE_AP);

    g_ap_param_ptr->ssid.length = MIN(SSID_MAX_LEN, os_strlen(ap_config->ssid));
    os_memcpy(g_ap_param_ptr->ssid.array, ap_config->ssid, g_ap_param_ptr->ssid.length);
    g_ap_param_ptr->key_len = os_strlen(ap_config->password);
    g_ap_param_ptr->hidden_ssid = ap_config->hidden;
    if (g_ap_param_ptr->key_len < 8) {
        g_ap_param_ptr->cipher_suite = WIFI_SECURITY_NONE;
    } else {
#if CONFIG_SOFTAP_WPA3
        g_ap_param_ptr->cipher_suite = WIFI_SECURITY_WPA3_WPA2_MIXED;
#else
        g_ap_param_ptr->cipher_suite = WIFI_SECURITY_WPA2_AES;
#endif
        os_memset(g_ap_param_ptr->key, 0, sizeof(g_ap_param_ptr->key));
        os_memcpy(g_ap_param_ptr->key, ap_config->password, g_ap_param_ptr->key_len);
    }
#if CONFIG_AP_VSIE
    g_ap_param_ptr->vsie_len = ap_config->vsie_len;
    if (ap_config->vsie_len)
        os_memcpy(g_ap_param_ptr->vsie, ap_config->vsie, g_ap_param_ptr->vsie_len);
#endif

    g_wlan_general_param->dhcp_enable = 1;
    g_ap_param_ptr->hidden_ssid = ap_config->hidden;

    return BK_OK;
}

bk_err_t bk_wifi_ap_set_config(const wifi_ap_config_t *ap_config)
{
    int ret = BK_OK;
    netif_ip4_config_t ip4_config = {0};
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_ap_config_t);
    uint32_t len_ip4_config = sizeof(netif_ip4_config_t);

    WDRV_LOGD("ap configuring\n");

    os_strcpy(ip4_config.ip, WLAN_DEFAULT_IP);
    os_strcpy(ip4_config.mask, WLAN_DEFAULT_MASK);
    os_strcpy(ip4_config.gateway, WLAN_DEFAULT_GW);
    os_strcpy(ip4_config.dns, WLAN_DEFAULT_GW);

    BK_RETURN_ON_ERR(bk_netif_set_ip4_config(NETIF_IF_AP, &ip4_config));

    buffer_to_ipc = os_malloc(len_ip4_config);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }
    os_memcpy(buffer_to_ipc, &ip4_config, len_ip4_config);
    ret = wifi_send_com_api_cmd(AP_NETIF_IP4_CONFIG, 1, (uint32_t)buffer_to_ipc);
    if (ret != BK_OK)
    {
        WDRV_LOGE("%s set ap netif ip4 config failed, ret=%d\n", __func__, ret);
        return ret;
    }
    os_free(buffer_to_ipc);

#if 0
    if (!wifi_is_inited()) {
        WDRV_LOGD("set ap config fail, wifi not init\n");
        return BK_ERR_WIFI_NOT_INIT;
    }
#endif

    ret = wifi_ap_validate_config(ap_config);
    if (ret != BK_OK)
        return ret;

    ret = wifi_ap_set_config(ap_config);
    if (ret != BK_OK)
        return ret;

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, ap_config, len);
    ret = wifi_send_com_api_cmd(AP_SET_CONFIG, 1, (uint32_t)buffer_to_ipc);
    if (ret != BK_OK)
    {
        WDRV_LOGE("%s set config failed, ret=%d\n", __func__, ret);
        return ret;
    }

    os_free(buffer_to_ipc);

    wifi_set_state_bit(WIFI_AP_CONFIGURED_BIT);
    WDRV_LOGD("ap configured\n");

    if (wifi_ap_is_started()) {
        BK_LOG_ON_ERR(bk_wifi_ap_stop());
        BK_LOG_ON_ERR(bk_wifi_ap_start());
    }
    return BK_OK;
}

bk_err_t bk_wifi_ap_stop(void)
{
    bk_err_t ret = BK_OK;

    if (!wifi_ap_is_started()) {
        WDRV_LOGD("ap stop: already stopped\n");
        return BK_OK;
    }

    ret = wifi_send_com_api_cmd(AP_STOP, 0);
    if (ret != BK_OK) {
        WDRV_LOGW("ap stop fail ret %d\n", ret);
    }

    WDRV_LOGD("ap stopped\n");
    uap_ip_down();
    host_wlan_remove_sap_netif();
    wifi_clear_state_bit(WIFI_AP_STARTED_BIT);

    return BK_OK;
}

int demo_softap_app_init(char *ap_ssid, char *ap_key, char *ap_channel)
{
    wifi_ap_config_t ap_config = {0};//WIFI_DEFAULT_AP_CONFIG();
#if 0
    netif_ip4_config_t ip4_config = {0};
#endif

    int len, key_len = 0;
    len = os_strlen(ap_ssid);

    if (ap_key)
        key_len = os_strlen(ap_key);
    if (SSID_MAX_LEN < len) {
        WDRV_LOGW("ssid name more than 32 Bytes\r\n");
        return BK_FAIL;
    }
    if (0 == len) {
        WDRV_LOGW("ssid name must not be null\r\n");
        return BK_FAIL;
    }

    if (8 > key_len)
        WDRV_LOGW("key less than 8 Bytes, the security will be set NONE\r\n");

    if (64 < key_len) {
        WDRV_LOGW("key more than 64 Bytes\r\n");
        return BK_FAIL;
    }

#if 0
    os_strcpy(ip4_config.ip, WLAN_DEFAULT_IP);
    os_strcpy(ip4_config.mask, WLAN_DEFAULT_MASK);
    os_strcpy(ip4_config.gateway, WLAN_DEFAULT_GW);
    os_strcpy(ip4_config.dns, WLAN_DEFAULT_GW);

    BK_RETURN_ON_ERR(bk_netif_set_ip4_config(NETIF_IF_AP, &ip4_config));
#endif

    os_strcpy(ap_config.ssid, ap_ssid);
    if (ap_key)
        os_strcpy(ap_config.password, ap_key);

    if (ap_channel) {
        int channel;
        char *end;

        channel = strtol(ap_channel, &end, 0);
        if (*end) {
            WDRV_LOGW("Invalid number '%s'", ap_channel);
            return BK_FAIL;
        }
        ap_config.channel = channel;
    }

    WDRV_LOGD("ssid:%s  key:%s\r\n", ap_config.ssid, ap_config.password);
    BK_RETURN_ON_ERR(bk_wifi_ap_set_config(&ap_config));
    BK_RETURN_ON_ERR(bk_wifi_ap_start());
    return BK_OK;
}


#endif

bk_err_t bk_wifi_ap_get_config(wifi_ap_config_t *ap_config)
{
    if (!ap_config)
        return BK_ERR_NULL_PARAM;

    os_memcpy(ap_config->ssid, g_ap_param_ptr->ssid.array, g_ap_param_ptr->ssid.length);
    os_memcpy(ap_config->password, g_ap_param_ptr->key, g_ap_param_ptr->key_len);
    ap_config->channel = g_ap_param_ptr->chann;
    ap_config->security = g_ap_param_ptr->cipher_suite;

    return BK_OK;
}

bk_err_t bk_wifi_sta_get_link_status(wifi_link_status_t *link_status)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_link_status_t);

    if (link_status == NULL) {
        WIFI_LOGE("%s failed, invalid pointer\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    ret = wifi_send_com_api_cmd(STA_GET_LINK_STATUS, 1, (uint32_t)buffer_to_ipc);

    os_memcpy(link_status, buffer_to_ipc, len);
    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_get_channel(void)
{
    uint8_t channel = 0;
    void *buffer_to_ipc = NULL;

    buffer_to_ipc = os_malloc(1);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    wifi_send_com_api_cmd(WIFI_GET_CHANNEL, 1, (uint32_t)buffer_to_ipc);

    channel = *(uint8_t *)buffer_to_ipc;
    os_free(buffer_to_ipc);

    WIFI_LOGD("%s: %d \n", __func__, channel);

    return channel;
}

bk_err_t bk_wifi_set_country(const wifi_country_t *country)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_country_t);

    if (country == NULL) {
        WIFI_LOGE("%s failed, invalid input param\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, country, len);
    ret = wifi_send_com_api_cmd(WIFI_SET_COUNTRY, 1, (uint32_t)buffer_to_ipc);

    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_get_country(wifi_country_t *country)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_country_t);

    if (country == NULL) {
        WIFI_LOGE("%s failed, invalid input param\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    ret = wifi_send_com_api_cmd(WIFI_GET_COUNTRY, 1, (uint32_t)buffer_to_ipc);

    os_memcpy(country, buffer_to_ipc, len);
    os_free(buffer_to_ipc);

    return ret;

}

bk_err_t bk_wifi_get_listen_interval(uint8_t *listen_interval)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;

    buffer_to_ipc = os_malloc(1);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    ret = wifi_send_com_api_cmd(STA_GET_LISTEN_INTERVAL, 1, (uint32_t)buffer_to_ipc);

    *listen_interval = *(uint8_t *)buffer_to_ipc;
    os_free(buffer_to_ipc);

    WIFI_LOGD("%s: %d \n", __func__, *listen_interval);

    return ret;
}

bk_err_t bk_wifi_send_listen_interval_req(uint8_t interval)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(STA_SET_LISTEN_INTERVAL, 1, interval);

    return ret;
}

bk_err_t bk_wifi_send_bcn_loss_int_req(uint8_t interval,uint8_t repeat_num)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(STA_SET_BCN_LOSS_INT, 2, interval, repeat_num);

    return ret;
}

bk_err_t bk_wifi_set_bcn_recv_win(uint8_t default_win, uint8_t max_win, uint8_t step)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(STA_SET_BCN_RECV_WIN, 3, default_win, max_win, step);

    return ret;
}

bk_err_t bk_wifi_set_bcn_loss_time(uint8_t wait_cnt, uint8_t wake_cnt)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(STA_SET_BCN_LOSS_TIME, 2, wait_cnt, wake_cnt);

    return ret;
}

bk_err_t bk_wifi_set_bcn_miss_time(uint8_t bcnmiss_time)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(STA_SET_BCN_MISS_TIME, 1, bcnmiss_time);

    return ret;
}

bk_err_t bk_wifi_sta_get_linkstate_with_reason(wifi_linkstate_reason_t *info)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_linkstate_reason_t);

    if (info == NULL) {
        WIFI_LOGE("%s failed, invalid input param\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    ret = wifi_send_com_api_cmd(STA_GET_LINK_STATE_WITH_REASON, 1, (uint32_t)buffer_to_ipc);

    os_memcpy(info, buffer_to_ipc, len);
    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_get_support_wifi_mode(uint8_t* support_mode)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;

    buffer_to_ipc = os_malloc(1);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    ret = wifi_send_com_api_cmd(WIFI_GET_SUPPORT_MODE, 1, (uint32_t)buffer_to_ipc);

    *support_mode = *(uint8_t *)buffer_to_ipc;
    os_free(buffer_to_ipc);

    WIFI_LOGD("%s: %d \n", __func__, *support_mode);

    return ret;
}

bk_err_t bk_scan_country_code(uint8_t *country_code, int *len)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc_1 = NULL;
    void *buffer_to_ipc_2 = NULL;
    uint8_t len_2 = sizeof(int);

    if (country_code == NULL) {
        WIFI_LOGE("%s failed, invalid input param\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc_1 = os_malloc(MAC_COUNTRY_STRING_LEN);
    if (!buffer_to_ipc_1)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc_2 = os_malloc(len_2);
    if (!buffer_to_ipc_2)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    *len = 0;
    ret = wifi_send_com_api_cmd(SCAN_CONTRY_CODE, 2, (uint32_t)buffer_to_ipc_1, (uint32_t)buffer_to_ipc_2);

    os_memcpy(len, buffer_to_ipc_2, len_2);
    os_free(buffer_to_ipc_2);

    WIFI_LOGD("%s: cc_len %d \n", __func__, *len);

    if (*len > 0)
        os_memcpy(country_code, buffer_to_ipc_1, *len);
    os_free(buffer_to_ipc_1);

    return ret;
}

static wifi_beacon_cc_rxed_t g_scan_cc_rxed_cb = NULL;
void *g_scan_cc_ctxt = NULL;
bk_err_t bk_wifi_bcn_cc_rxed_register_cb(const wifi_beacon_cc_rxed_t cc_cb, void *ctxt)
{
    bk_err_t ret = BK_OK;
    bool enable = (cc_cb == NULL)? false: true;

    g_scan_cc_rxed_cb = cc_cb;
    g_scan_cc_ctxt = ctxt;

    ret = wifi_send_com_api_cmd(WIFI_GET_BCN_CC, 1, enable);

    return ret;
}

bk_err_t bk_wifi_bcn_cc_rxed_cb(void *data, uint16_t len)
{
    uint8_t *cc;
    uint8_t cc_len = len;

    cc = os_malloc(cc_len);
    os_memcpy(cc, (uint8_t *)data, cc_len);

    if (g_scan_cc_rxed_cb)
        g_scan_cc_rxed_cb(g_scan_cc_ctxt, cc, cc_len);

    os_free(cc);

    return 0;
}

bk_err_t bk_wifi_sta_connect(void)
{
    wifi_sta_config_t sta_config = { 0 };

    WDRV_LOGD("sta connecting\n");

    wifi_sta_get_global_config(&sta_config);

    if (!wifi_sta_is_started()) {
        WDRV_LOGD("sta connect fail, sta not start\n");
        return BK_ERR_WIFI_STA_NOT_STARTED;
    }

    wifi_set_state_bit(WIFI_STA_CONNECTED_BIT);
    WDRV_LOGD("sta connected(%x)\n", s_wifi_state_bits);

    return BK_OK;
}

bk_err_t bk_wifi_sta_start(void)
{
    bk_err_t ret = BK_OK;

    WDRV_LOGD("sta starting\n");

    if (!wifi_sta_is_configured()) {
        WDRV_LOGD("sta start fail, sta not configured\n");
        return BK_ERR_WIFI_STA_NOT_CONFIG;
    }

    wifi_sta_init_global_config();

    if (wifi_sta_is_started()) {
        WDRV_LOGD("sta already started, ignored!\n");
        return BK_OK;
    }

    //bk_wifi_init();

    wifi_set_state_bit(WIFI_STA_STARTED_BIT);

    /* always connect the AP automatically */
    bk_wifi_sta_connect();

    ret = wifi_send_com_api_cmd(STA_START, 0);
    if (ret != BK_OK)
    {
        WDRV_LOGD("%s failed, ret=%d\n",__func__, ret);
    }

    return BK_OK;
}

bk_err_t bk_wifi_sta_set_config(const wifi_sta_config_t *config)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_sta_config_t);

    WDRV_LOGD("sta configuring\n");

//    if (!wifi_is_inited()) {
//        WDRV_LOGV("set sta config fail, wifi not init\n");
//        return BK_ERR_WIFI_NOT_INIT;
//    }

    if (config == NULL) {
        WIFI_LOGE("%s failed, invalid config\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    ret = wifi_sta_validate_config(config);
    if (ret != BK_OK) {
        WDRV_LOGD("set config fail, invalid param\n");
        return ret;
    }

    wifi_sta_set_global_config(config);

    wifi_set_state_bit(WIFI_STA_CONFIGURED_BIT);
    WDRV_LOGD("sta configured(%x)\n", s_wifi_state_bits);


    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, config, len);
    ret = wifi_send_com_api_cmd(STA_SET_CONFIG, 1, (uint32_t)buffer_to_ipc);

    os_free(buffer_to_ipc);

    return ret;
}
bk_err_t bk_wifi_sta_get_config(wifi_sta_config_t *config)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_sta_config_t);

    //WIFI_LOGE("%s config len:%d\r\n", __func__, len);
    if (config == NULL) {
        WIFI_LOGE("%s failed, invalid config\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(config, 0, sizeof(config));

    if (!wifi_sta_is_configured())
        return BK_ERR_WIFI_STA_NOT_CONFIG;

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(buffer_to_ipc, 0, len);   // Modified by TUYA
    ret = wifi_send_com_api_cmd(STA_GET_CONFIG, 1, (uint32_t)buffer_to_ipc);

    os_memcpy(config, buffer_to_ipc, len);
    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_sta_pm_enable(void)
{
    return wifi_send_com_api_cmd(STA_PM_ENABLE, 0);
}

bk_err_t bk_wifi_sta_pm_disable(void)
{
    return wifi_send_com_api_cmd(STA_PM_DISABLE, 0);
}

int demo_sta_app_init(char *oob_ssid, char *connect_key)
{
    wifi_sta_config_t sta_config = {0};
    int len;

    len = os_strlen(oob_ssid);
    if (SSID_MAX_LEN < len) {
        WIFI_LOGD("ssid name more than 32 Bytes\r\n");
        return BK_FAIL;
    }

    os_strcpy(sta_config.ssid, oob_ssid);
    if (connect_key)
        os_strcpy(sta_config.password, connect_key);

    WIFI_LOGD("ssid:%s key:%s\r\n", sta_config.ssid, sta_config.password);
    BK_LOG_ON_ERR(bk_wifi_sta_set_config(&sta_config));
    BK_LOG_ON_ERR(bk_wifi_sta_start());
    return BK_OK;
}

bk_err_t bk_wifi_monitor_start(void)
{
    return wifi_send_com_api_cmd(MONITOR_START, 0);
}

bk_err_t bk_wifi_monitor_stop(void)
{
    return wifi_send_com_api_cmd(MONITOR_STOP, 0);
}

bk_err_t bk_wifi_monitor_set_channel(const wifi_channel_t *chan)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_channel_t);

    if (chan == NULL) {
        WIFI_LOGE("%s failed, invalid config\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, chan, len);
    ret = wifi_send_com_api_cmd(MONITOR_SET_CHANNEL, 1, (uint32_t)buffer_to_ipc);

    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_send_raw(uint8_t *buffer, int len)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;

    if (buffer == NULL) {
        WIFI_LOGE("%s failed, invalid config\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, buffer, len);
    ret = wifi_send_com_api_cmd(SEND_RAW, 2, (uint32_t)buffer_to_ipc, len);

    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_manual_cal_rfcali_status(void)
{
    return wifi_send_com_api_cmd(PHY_CAL_RFCALI, 0);
}

bk_err_t bk_wifi_capa_config(wifi_capability_t capa_id, uint32_t capa_val)
{
    return wifi_send_com_api_cmd(WIFI_CAPA_CONFIG, 2, (uint32_t)capa_id, capa_val);
}

bk_err_t bk_wifi_set_mac_address(char *mac)
{
    bk_err_t ret = 0;
    void *buffer_to_ipc = NULL;

    if (!mac)
        return BK_ERR_NULL_PARAM;

    buffer_to_ipc = os_malloc(WIFI_MAC_LEN);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, mac, WIFI_MAC_LEN);
    ret = wifi_send_com_api_cmd(SET_MAC_ADDRESS, 1, (uint32_t)buffer_to_ipc);
    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_scan_start(const wifi_scan_config_t *scan_config)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_scan_config_t);
    uint8_t ssid_len = 0;

    WDRV_LOGD("scaning\n");

    wifi_scan_init_global_config();
    wifi_set_state_bit(WIFI_PURE_SCAN_STARTED_BIT);

    if (scan_config ) {
        ssid_len = MIN(SSID_MAX_LEN, os_strlen((char *)scan_config->ssid));

        if((0 != scan_config->scan_type) ||(0 != scan_config->chan_cnt) ||(0 != scan_config->duration)) {
            scan_param_env.set_param = 1;
            scan_param_env.scan_type = scan_config->scan_type;
            scan_param_env.chan_cnt = scan_config->chan_cnt;
            if(WIFI_MAX_SCAN_CHAN_DUR < scan_config->duration) {
                WDRV_LOGW("scan duration is too long %dus,need less than 200ms\r\n",scan_config->duration);
                scan_param_env.duration = WIFI_MAX_SCAN_CHAN_DUR * 1000;
            } else
                scan_param_env.duration = scan_config->duration * 1000;
            os_memcpy(scan_param_env.chan_nb, scan_config->chan_nb, scan_config->chan_cnt);
        }
    }

    WIFI_LOGD("%s ssid_len:%d\r\n", __func__, ssid_len);
    //WIFI_LOGE("%s config len:%d\r\n", __func__, len);
    if (scan_config == NULL) {
        return wifi_send_com_api_cmd(SCAN_START, 1, 0);
    } else {
        buffer_to_ipc = os_malloc(len);
        if (!buffer_to_ipc)
        {
            WIFI_LOGE("%s malloc failed\r\n", __func__);
            return BK_ERR_NO_MEM;
        }

        os_memcpy(buffer_to_ipc, scan_config, len);

        ret = wifi_send_com_api_cmd(SCAN_START, 1, (uint32_t)buffer_to_ipc);

        os_free(buffer_to_ipc);

        return ret;
    }
}

bk_err_t bk_wifi_scan_stop(void)
{
    return wifi_send_com_api_cmd(SCAN_STOP, 0);
}

bk_err_t bk_wifi_scan_get_result(wifi_scan_result_t *scan_result)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_scan_result_t);

    if (scan_result == NULL) {
        WIFI_LOGW("%s failed, invalid scan_result\r\n", __func__);
        return BK_ERR_PARAM;
    }
    os_memset(scan_result, 0, sizeof(*scan_result));

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGW("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    ret = wifi_send_com_api_cmd(SCAN_RESULT, 1, (uint32_t)buffer_to_ipc);
    if (ret != BK_OK) {
        WIFI_LOGW("%s wifi_send_com_api_cmd failed: %d\r\n", __func__, ret);
        goto _free_and_exit;
    }

    os_memcpy(scan_result, buffer_to_ipc, len);

    if (scan_result->ap_num > 0 && scan_result->aps == NULL) {
        WIFI_LOGW("%s failed, aps pointer is NULL but ap_num is %d\r\n", __func__, scan_result->ap_num);
        scan_result->ap_num = 0;
        ret = BK_ERR_PARAM; //update reason code
        goto _free_and_exit;
    }

_free_and_exit:
    os_free(buffer_to_ipc);
    return ret;
}

static const char *wifi_sec_type_string_api(wifi_security_t security)
{
    switch (security) {
    case WIFI_SECURITY_NONE:
        return "NONE";
    case WIFI_SECURITY_WEP:
        return "WEP";
    case WIFI_SECURITY_WPA_TKIP:
        return "WPA-TKIP";
    case WIFI_SECURITY_WPA_AES:
        return "WPA-AES";
    case WIFI_SECURITY_WPA_MIXED:
        return "WPA-MIX";
    case WIFI_SECURITY_WPA2_TKIP:
        return "WPA2-TKIP";
    case WIFI_SECURITY_WPA2_AES:
        return "WPA2-AES";
    case WIFI_SECURITY_WPA2_MIXED:
        return "WPA2-MIX";
    case WIFI_SECURITY_WPA3_SAE:
        return "WPA3-SAE";
    case WIFI_SECURITY_WPA3_WPA2_MIXED:
        return "WPA3-WPA2-MIX";
    case WIFI_SECURITY_EAP:
        return "EAP";
    case WIFI_SECURITY_OWE:
        return "OWE";
    case WIFI_SECURITY_AUTO:
        return "AUTO";
#ifdef CONFIG_WAPI_SUPPORT
    case WIFI_SECURITY_TYPE_WAPI_PSK:
        return "WAPI_PSK";
    case WIFI_SECURITY_TYPE_WAPI_CERT:
        return "WAPI_CERT";
#endif
    default:
        return "UNKNOWN";
    }
}

static void wifi_scan_dump_ap(const wifi_scan_ap_info_t *ap)
{
    const char *security_str = wifi_sec_type_string_api(ap->security);
#if (CONFIG_SHELL_ASYNCLOG)
    shell_cmd_ind_out("%-32s " BK_MAC_FORMAT "   %4d %2d %s\r\n",
               ap->ssid, BK_MAC_STR(ap->bssid), (int8_t)ap->rssi, ap->channel, security_str);
#else
    WIFI_LOG_RAW("%-32s " BK_MAC_FORMAT "   %4d %2d %s\n",
               ap->ssid, BK_MAC_STR(ap->bssid), (int8_t)ap->rssi, ap->channel, security_str);
#endif
}

bk_err_t bk_wifi_scan_dump_result(const wifi_scan_result_t *scan_result)
{
    int i;

    if (!scan_result) {
#if (CONFIG_SHELL_ASYNCLOG)
        shell_cmd_ind_out("scan doesn't found AP\n");
#else
        WIFI_LOGW("Invalid scan_result(NULL)\r\n");
#endif
        return BK_ERR_PARAM;
    }

    if (scan_result->ap_num < 0 || scan_result->ap_num > 100) {
        WIFI_LOGW("Invalid AP count: %d\r\n", scan_result->ap_num);
        return BK_ERR_PARAM;
    }

    if ((scan_result->ap_num > 0) && (!scan_result->aps)) {
        WIFI_LOGW("scan number is %d, but AP info is NULL\n", scan_result->ap_num);
        return BK_ERR_PARAM;
    }
#if (CONFIG_SHELL_ASYNCLOG)
    shell_cmd_ind_out("scan found %d AP\r\n", scan_result->ap_num);
    shell_cmd_ind_out("%32s %17s   %4s %4s %s\r\n", "              SSID              ",
               "      BSSID      ", "RSSI", "chan", "security");
    shell_cmd_ind_out("%32s %17s   %4s %4s %s\r\n", "--------------------------------",
               "-----------------", "----", "----", "---------\n");
#else
    WIFI_LOGD("scan found %d AP\n", scan_result->ap_num);
    WIFI_LOG_RAW("%32s %17s   %4s %4s %s\n", "              SSID              ",
               "      BSSID      ", "RSSI", "chan", "security");
    WIFI_LOG_RAW("%32s %17s   %4s %4s %s\n", "--------------------------------",
               "-----------------", "----", "----", "---------\n");
#endif
    for (i = 0; i < scan_result->ap_num; i++) {
        wifi_scan_dump_ap(&scan_result->aps[i]);
        rtos_delay_milliseconds(10);
    }

    //WIFI_LOG_RAW("\n");

    return BK_OK;
}

void bk_wifi_scan_free_result(wifi_scan_result_t *scan_result)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_scan_result_t);

    if (scan_result == NULL) {
        WIFI_LOGW("%s failed, invalid scan_result\r\n", __func__);
        return;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGW("%s malloc failed\r\n", __func__);
        return;
    }

    os_memcpy(buffer_to_ipc, scan_result, len);
    ret = wifi_send_com_api_cmd(SCAN_RESULT_FREE, 1, (uint32_t)buffer_to_ipc);

    if (ret != BK_OK) {
        WIFI_LOGW("%s wifi_send_com_api_cmd failed with error %d\r\n", __func__, ret);
    }

    os_free(buffer_to_ipc);
}

//MONITOR
bk_err_t bk_wifi_monitor_register_cb(const wifi_monitor_cb_t monitor_cb)
{
    s_monitor_ap_cb = monitor_cb;
    return wifi_send_com_api_cmd(MONITOR_REGISTER_CB, 0);
}

wifi_monitor_cb_t bk_wifi_monitor_get_cb(void)
{
    return s_monitor_ap_cb;
}

bk_err_t bk_wifi_monitor_register_ind(uint8_t * msg_payload)
{
    struct monitor_struct
    {
        cpdu_t cp;
        struct bk_rx_msg_hdr rx_msg_hdr;
        uint32_t para[3];
        uint32_t payload[1];
    };

    bk_err_t ret = BK_OK;
    const uint8_t *frame = NULL;
    uint32_t len;
    const wifi_frame_info_t *frame_info;
    wifi_filter_cb_t cb = bk_wifi_monitor_get_cb();
    //uint8_t *payload_temp =  NULL;
    struct pbuf* pbuf = NULL;
    cpdu_t * cpdu = NULL;
    uint8_t vif_idx = 0;

    pbuf = (struct pbuf*)((uint8_t*)msg_payload + sizeof(uint32_t) - sizeof(cpdu_t) - sizeof(wdrv_rx_msg) - sizeof(struct pbuf));
    struct monitor_struct* mon_hdr = (struct monitor_struct*)(pbuf + 1);
    
    len = mon_hdr->para[0];
    frame = (const uint8_t*)mon_hdr->para[2];
    frame_info = (const wifi_frame_info_t *)mon_hdr->para[1];

    if(cb)
    {
        cb(frame,len,frame_info);//This payload will free in next line.
    }

    cpdu = (cpdu_t*)(pbuf + 1);
    cpdu->co_hdr.need_free = 1;//RXC free
    vif_idx = cpdu->co_hdr.vif_idx;
    ret = wdrv_txdata_sender(pbuf,vif_idx);//vif null, just for free this RXC pbuf
    return ret;
}
//Filter
bk_err_t bk_wifi_filter_register_cb(const wifi_filter_cb_t filter_cb)
{
    s_filter_ap_cb = filter_cb;
    return wifi_send_com_api_cmd(FILTER_REGISTER_CB, 0);
}

wifi_filter_cb_t bk_wifi_filter_get_cb(void)
{
    return s_filter_ap_cb;
}
bk_err_t bk_wifi_filter_register_ind(uint8_t * msg_payload)
{
    struct filter_struct
    {
        cpdu_t cp;
        struct bk_rx_msg_hdr rx_msg_hdr;
        uint32_t para[3];
        uint32_t payload[1];
    };

    bk_err_t ret = BK_OK;
    const uint8_t *frame = NULL;
    uint32_t len;
    const wifi_frame_info_t *frame_info;
    wifi_filter_cb_t cb = bk_wifi_filter_get_cb();
    //uint8_t *payload_temp =  NULL;
    struct pbuf* pbuf = NULL;
    cpdu_t * cpdu = NULL;
    uint8_t vif_idx = 0;

    pbuf = (struct pbuf*)((uint8_t*)msg_payload + sizeof(uint32_t) - sizeof(cpdu_t) - sizeof(wdrv_rx_msg) - sizeof(struct pbuf));
    struct filter_struct* filter_hdr = (struct filter_struct*)(pbuf + 1);
    
    len = filter_hdr->para[0];
    frame = (const uint8_t*)filter_hdr->para[2];
    frame_info = (const wifi_frame_info_t *)filter_hdr->para[1];
    //bk_mem_dump("ap",(uint32_t)pbuf,200);
    if(cb)
    {
        cb(frame,len,frame_info);//This payload will free in next line.
    }

    cpdu = (cpdu_t*)(pbuf + 1);
    cpdu->co_hdr.need_free = 1;//RXC free
    vif_idx = cpdu->co_hdr.vif_idx;
    ret = wdrv_txdata_sender(pbuf,vif_idx);//vif null, just for free this RXC pbuf
    return ret;
}
bk_err_t bk_wifi_send_arp_set_rate_req(uint16_t arp_tx_rate)
{
    return wifi_send_com_api_cmd(SEND_ARP_SET_RATE_REQ, 1, arp_tx_rate);
}

bk_err_t bk_wifi_get_status(wifi_status_t *status)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_status_t);

    if (status == NULL) {
        WIFI_LOGE("%s failed, invalid pointer\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    ret = wifi_send_com_api_cmd(GET_STATUS, 1, (uint32_t)buffer_to_ipc);

    os_memcpy(status, buffer_to_ipc, len);
    os_free(buffer_to_ipc);

    return ret;
}

bk_err_t bk_wifi_set_block_bcmc_en(uint8_t config)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(WIFI_SET_BLOCK_BCMC_EN, 1, config);

    return ret;
}

bool bk_wifi_get_block_bcmc_en(void)
{
    void *buffer_to_ipc = NULL;
    bool bcmcm_en;

    buffer_to_ipc = os_malloc(1);

    wifi_send_com_api_cmd(WIFI_GET_BLOCK_BCMC_EN, 1, (uint32_t)buffer_to_ipc);

    bcmcm_en = *(bool *)buffer_to_ipc;
    os_free(buffer_to_ipc);

    return bcmcm_en;
}

bk_err_t bk_wifi_ftm_start(const wifi_ftm_config_t *config, wifi_ftm_results_t *ftm_results)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc_1 = NULL;
    void *buffer_to_ipc_2 = NULL;
    uint32_t len1 = sizeof(wifi_ftm_config_t);
    uint32_t len2 = sizeof(wifi_ftm_results_t);

    if ((config == NULL) || (ftm_results == NULL)) {
        WIFI_LOGE("%s failed, invalid pointer\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    buffer_to_ipc_1 = os_malloc(len1);
    buffer_to_ipc_2 = os_malloc(len2);
    if (!buffer_to_ipc_1 || !buffer_to_ipc_2)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    ret = wifi_send_com_api_cmd(FTM_START, 2, (uint32_t)buffer_to_ipc_1, (uint32_t)buffer_to_ipc_2);

    os_memcpy(ftm_results, buffer_to_ipc_2, len2);
    os_free(buffer_to_ipc_1);
    os_free(buffer_to_ipc_2);

    return ret;
}

bk_err_t bk_wifi_ftm_dump_result(const wifi_ftm_results_t *ftm_results)
{
    if (!ftm_results) {
        WIFI_LOGD("ftm doesn't found responser\n");
        return BK_OK;
    }

    if ((ftm_results->nb_ftm_rsp > 0) && (!ftm_results->rsp)) {
        WIFI_LOGE("ftm responser number is %d, but responser info is NULL\n", ftm_results->nb_ftm_rsp);
        return BK_ERR_PARAM;
    }

    WIFI_LOGD("ftm found %d responser\n", ftm_results->nb_ftm_rsp);

    for (int i = 0; i < ftm_results->nb_ftm_rsp; i++) {
        WIFI_LOGD("The distance to " WIFI_MAC_FORMAT " is %.2f meters, rtt is %d nSec \n",
            WIFI_MAC_STR(ftm_results->rsp[i].bssid), ftm_results->rsp[i].distance, ftm_results->rsp[i].rtt);
        rtos_delay_milliseconds(10);
    }

    WIFI_LOG_RAW("\n");

    return BK_OK;
}

bk_err_t bk_wifi_ftm_free_result(wifi_ftm_results_t *ftm_results)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint32_t len = sizeof(wifi_ftm_results_t);

    if (ftm_results == NULL) {
        WIFI_LOGE("%s failed, invalid pointer\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    if (ftm_results->rsp == NULL)
    {
        WIFI_LOGE("%s no need to free, num %d\r\n", __func__, ftm_results->nb_ftm_rsp);
        return BK_OK;
    }

    buffer_to_ipc = os_malloc(len);

    os_memcpy(buffer_to_ipc, ftm_results, len);
    ret = wifi_send_com_api_cmd(FTM_FREE_RESULT, 1, (uint32_t)buffer_to_ipc);

    os_free(buffer_to_ipc);

    return ret;
}


bk_err_t bk_wifi_csi_alg_config(double thres1)
{
    bk_err_t ret = BK_OK;
    void *buffer_to_ipc = NULL;
    uint8_t len = sizeof(double);

    buffer_to_ipc = os_malloc(len);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memcpy(buffer_to_ipc, &thres1, len);
    ret = wifi_send_com_api_cmd(CSI_ALG_CONFIG, 1, (uint32_t)buffer_to_ipc);

    os_free(buffer_to_ipc);

    return ret;

}

bk_err_t bk_wifi_csi_start_req(uint8_t csi_work_type,uint8_t csi_work_mode,uint8_t csi_work_identity,uint8_t csi_data_format,
                               uint32_t csi_data_interval,uint32_t delay)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(CSI_START, 6, csi_work_type, csi_work_mode, csi_work_identity,
                                csi_data_format, csi_data_interval, delay);

    return ret;
}

bk_err_t bk_wifi_csi_stop_req(void)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(CSI_STOP, 0);

    return ret;
}

bk_err_t bk_wifi_csi_static_param_reset_req(uint8_t update_cali_mode,uint32_t cali_cnt)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(CSI_STATIC_PARAM_RESET, 2, update_cali_mode, cali_cnt);

    return ret;
}

wifi_csi_cb_t g_wifi_csi_info_handler = NULL;
void bk_wifi_csi_info_cb_register(wifi_csi_cb_t cb)
{
    bool enable = (cb == NULL)? false: true;

    g_wifi_csi_info_handler = cb;

    wifi_send_com_api_cmd(CSI_INFO_GET, 1, enable);

    return;
}

void bk_wifi_csi_info_cb(void *data)
{
    struct wifi_csi_info_t *info;
    uint16_t len = sizeof(struct wifi_csi_info_t);

    info = os_malloc(len);
    os_memcpy(info, (uint8_t *)data, len);

    if(g_wifi_csi_info_handler)
        g_wifi_csi_info_handler(info);

    os_free(info);
}


bk_err_t bk_wifi_csi_demo_turn_on_light(uint8_t color, bool flicker)
{
    bk_err_t ret = BK_OK;

    ret = wifi_send_com_api_cmd(CSI_DEMO_LIGHT, 2, color, flicker);

    return ret;
}

#if CONFIG_BRIDGE
bk_err_t bk_wifi_check_client_mac_connected(uint8_t *mac)
{
    void *buffer_to_ipc = NULL;
    bk_err_t ret = BK_OK;

    buffer_to_ipc = os_malloc(WIFI_MAC_LEN);
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }
    os_memcpy(buffer_to_ipc, mac, WIFI_MAC_LEN);
    ret = wifi_send_com_api_cmd(CHECK_CLIENT_MAC_CONNECTED, 1, (uint32_t)buffer_to_ipc);
    os_free(buffer_to_ipc);
    return ret;
}

#include "netif/bridgeif.h"
#include "lwip/netifapi.h"
#include "lwip/inet.h"

static bk_bridge_state_t bridge_state = BRIDGE_STATE_DISABLED;
bk_bridge_config_t bridge_config = {0};

/*private apis for beken bridge start*/
bk_bridge_state_t bk_wifi_get_bridge_state(void)
{
    return bridge_state;
}

bk_err_t bk_wifi_sync_bridge_state(bk_bridge_state_t br_state)
{
    void *buffer_to_ipc = NULL;

    buffer_to_ipc = os_malloc(sizeof(bk_bridge_state_t));
    if (!buffer_to_ipc)
    {
        WIFI_LOGE("%s malloc failed\r\n", __func__);
        return BK_ERR_NO_MEM;
    }
    *((bk_bridge_state_t *)buffer_to_ipc) = br_state;
    wifi_send_com_api_cmd(SET_BRIDGE_SYNC_STATE, 1, (uint32_t)buffer_to_ipc);
    os_free(buffer_to_ipc);
    return BK_OK;
}

bk_err_t bk_wifi_save_bridge_config(bk_bridge_config_t *br_config)
{
    if (br_config == NULL) {
        WIFI_LOGE("%s failed, invalid pointer\r\n", __func__);
        return BK_FAIL;
    }
    if (bridge_config.bridge_ssid) {
        os_free(bridge_config.bridge_ssid);
    }
    bridge_config.bridge_ssid = os_strdup(br_config->bridge_ssid);
    if (bridge_config.ext_sta_ssid) {
        os_free(bridge_config.ext_sta_ssid);
    }
    bridge_config.ext_sta_ssid = os_strdup(br_config->ext_sta_ssid);
    if (bridge_config.key) {
        os_free(bridge_config.key);
    }
    bridge_config.key = os_strdup(br_config->key);
    if (bridge_config.hostname) {
        os_free(bridge_config.hostname);
    }
    bridge_config.hostname = os_strdup(br_config->hostname);
    bridge_config.channel = br_config->channel;
    return BK_OK;
}

static beken_thread_t br_start_thread_internal;
void bk_br_start_internal(void* data)
{
    int len, key_len = 0;
    uint8_t mac[6] = {0};
    bridgeif_initdata_t mybr_initdata = {0};
    netif_ip4_config_t ip4_config = {0};
    ip4_addr_t my_ip, my_gw, my_mask;
    wifi_link_status_t link_status = {0};
    wifi_ap_config_t ap_config = {0};//WIFI_DEFAULT_AP_CONFIG();
    wifi_link_status_t link_sta_status = {0};

    WIFI_LOGI("bk_wifi_start_softap_for_bridge, ssid: %s, key: %s, channel: %d, hostname: %s, state: %d\r\n",
            bridge_config.bridge_ssid, bridge_config.key, bridge_config.channel, bridge_config.hostname, bridge_state);
    if (bridge_state == BRIDGE_STATE_ENABLING) {
        /*confige bridgeif and add sta to bridgeif*/
        bk_wifi_sta_get_mac(mac);
        os_memcpy(((struct netif *)net_get_br_handle())->hwaddr, mac, 6);
        os_memcpy(&mybr_initdata.ethaddr, mac, 6);
        mybr_initdata.max_fdb_dynamic_entries = 64;
        mybr_initdata.max_fdb_static_entries = 4;
        mybr_initdata.max_ports = 16;
        bk_netif_get_ip4_config(NETIF_IF_STA, &ip4_config);
        inet_aton((char *)&ip4_config.ip, &my_ip);
        inet_aton((char *)&ip4_config.gateway, &my_gw);
        inet_aton((char *)&ip4_config.mask, &my_mask);
        // set STA interface IP to 0.0.0.0
        //netifapi_netif_set_addr((struct netif *)net_get_sta_handle(), NULL, NULL, NULL);
        netifapi_netif_add((struct netif *)net_get_br_handle(), &my_ip, &my_mask, &my_gw,
                            &mybr_initdata, bridgeif_init, netif_input);
        bridgeif_add_port((struct netif *)net_get_br_handle(), (struct netif *)net_get_sta_handle());
        if (bridge_config.hostname) {
            netif_set_hostname((struct netif *)net_get_sta_handle(), bridge_config.hostname);
        } else {
            netif_set_hostname((struct netif *)net_get_sta_handle(), "Beken_Bridge");
        }
        bk_wifi_sta_get_link_status(&link_status);
        //start softap
        len = os_strlen(bridge_config.bridge_ssid);
        if (bridge_config.key)
            key_len = os_strlen(bridge_config.key);
        if (SSID_MAX_LEN < len) {
            WIFI_LOGE("ssid name more than 32 Bytes\r\n");
            return;
        }
        if (0 == len) {
            WIFI_LOGE("ssid name must not be null\r\n");
            return;
        }
        if (8 > key_len)
            WIFI_LOGE("key less than 8 Bytes, the security will be set NONE\r\n");
        if (64 < key_len) {
            WIFI_LOGE("key more than 64 Bytes\r\n");
            return;
        }
        os_strcpy(ip4_config.ip, WLAN_ANY_IP);
        os_strcpy(ip4_config.mask, WLAN_ANY_IP);
        os_strcpy(ip4_config.gateway, WLAN_ANY_IP);
        os_strcpy(ip4_config.dns, WLAN_ANY_IP);
        bk_netif_set_ip4_config(NETIF_IF_AP, &ip4_config);
        os_strcpy(ap_config.ssid, bridge_config.bridge_ssid);
        os_memset(&link_sta_status, 0x0, sizeof(link_sta_status));
        bk_wifi_sta_get_link_status(&link_sta_status);
        if (link_sta_status.security == WIFI_SECURITY_NONE) { 
            // do not set the key for softap
        } else if (bridge_config.key) {
            os_strcpy(ap_config.password, bridge_config.key);
        }
        ap_config.security = link_sta_status.security;
        //bridge vendor IEs is optional
        //os_memcpy(ap_config.vsie, "\xdd\x07\xc8\x47\x8c\x01\x00\x00\x00", 9); //bridge vise set to 1
        //ap_config.vsie_len = 9;
        WIFI_LOGD("ssid:%s  key:%s\r\n", ap_config.ssid, ap_config.password);
        bk_wifi_ap_set_config(&ap_config);
        bk_wifi_ap_start();
        bridgeif_add_port((struct netif *)net_get_br_handle(), (struct netif *)net_get_uap_handle());
        netifapi_netif_set_default(net_get_br_handle());
        netifapi_netif_set_up((struct netif *)net_get_br_handle());
        bridge_set_ip_start_flag(true);
        bridge_state = BRIDGE_STATE_ENABLED;
        bk_wifi_sync_bridge_state(BRIDGE_STATE_ENABLED);
    }
    rtos_delete_thread(NULL);
}

bk_err_t bk_wifi_start_softap_for_bridge(void)
{
    bk_err_t ret = BK_OK;
    ret = rtos_create_thread(&br_start_thread_internal,
        BEKEN_APPLICATION_PRIORITY,
        "br_start_thread_internal",
        (beken_thread_function_t)bk_br_start_internal,
        4*1024,
        0);
    if (br_start_thread_internal == NULL) {
        WIFI_LOGE("create thread failed\r\n");
        ret = BK_FAIL;
    }
    return ret;
}

void bk_wifi_switch_bridge_to_sta(void)
{
    if (bridge_state == BRIDGE_STATE_ENABLED ||
        bridge_state == BRIDGE_STATE_ENABLING) {
        bridge_state = BRIDGE_STATE_DISABLING;
        bridge_ip_stop();
        /*just stop softap, station still work*/
        netifapi_netif_set_default(net_get_sta_handle());
        bridge_set_ip_start_flag(false);
        bk_wifi_ap_stop();
        bridge_state = BRIDGE_STATE_DISABLED;
        bk_wifi_sync_bridge_state(BRIDGE_STATE_DISABLED);
    }
}
/*private apis for beken bridge end*/

bk_err_t bk_bridge_stop(void)
{
    if (bridge_state > BRIDGE_STATE_DISABLING) {
        bridge_state = BRIDGE_STATE_DISABLING;
        bk_wifi_sync_bridge_state(BRIDGE_STATE_DISABLING);
        if(bk_wifi_sta_stop())
            BK_LOGD(NULL, "bridge stop sta fail\r\n");
        if(bk_wifi_ap_stop())
            BK_LOGD(NULL, "bridge stop ap fail\r\n");
        bridge_ip_stop();
        bridge_state = BRIDGE_STATE_DISABLED;
        bk_wifi_sync_bridge_state(BRIDGE_STATE_DISABLED);
    }
    return BK_OK;
}

bk_err_t bk_bridge_start(bk_bridge_config_t *br_config)
{
    wifi_sta_config_t sta_config = {0};
    int len = 0;

    bk_bridge_stop();
    bridge_state = BRIDGE_STATE_ENABLING;
    bk_wifi_sync_bridge_state(BRIDGE_STATE_ENABLING);
    bk_wifi_save_bridge_config(br_config);
    //start sta
    len = os_strlen(br_config->ext_sta_ssid);
    if (33 < len) {
        LWIP_LOGD("ssid name more than 32 Bytes\r\n");
        return BK_FAIL;
    }

    os_strcpy(sta_config.ssid, br_config->ext_sta_ssid);
    if (br_config->key)
        os_strcpy(sta_config.password, br_config->key);

    LWIP_LOGD("ssid:%s key:%s\r\n", sta_config.ssid, sta_config.password);
    BK_LOG_ON_ERR(bk_wifi_sta_set_config(&sta_config));
    BK_LOG_ON_ERR(bk_wifi_sta_start());
    //left process in wdrv_cntrl.c

    return BK_OK;
}
#endif
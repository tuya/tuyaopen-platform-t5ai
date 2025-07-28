#include "bk_feature.h"
#include "sdkconfig.h"


int bk_feature_bssid_connect_enable(void) {
#if CONFIG_BSSID_CONNECT
	return 1;
#else
	return 0;
#endif
}

int bk_feature_fast_connect_enable(void) {
#if CONFIG_WIFI_FAST_CONNECT
	return 1;
#else
	return 0;
#endif
}

int bk_feature_fast_dhcp_enable(void) {
#if CONFIG_WIFI_FAST_DHCP
	return 1;
#else
	return 0;
#endif
}

int bk_feature_sta_vsie_enable(void) {
#if CONFIG_COMPONENTS_STA_VSIE
	return 1;
#else
	return 0;
#endif
}

int bk_feature_ap_statype_limit_enable(void) {
#if CONFIG_AP_STATYPE_LIMIT
	return 1;
#else
	return 0;
#endif
}

int bk_feature_temp_detect_enable(void) {
#if CONFIG_TEMP_DETECT
    return 1;
#else
    return 0;
#endif
}




int bk_feature_get_cpu_cnt(void) {
#if CONFIG_CPU_CNT
    return CONFIG_CPU_CNT;
#else
    return 1;
#endif
}


int bk_feature_receive_bcmc_enable(void) {
#if CONFIG_RECEIVE_BCMC_IN_DTIM10
    return 1;
#else
    return 0;
#endif
}

int bk_feature_not_check_ssid_enable(void) {
#if CONFIG_NOT_CHECK_SSID_CHANGE
    return 1;
#else
    return 0;
#endif
}

int bk_feature_config_cache_enable(void) {
#if CONFIG_CACHE_ENABLE
	return 1;
#else
	return 0;
#endif
}

int bk_feature_ckmn_enable(void) {
#if CONFIG_CKMN
	return 1;
#else
	return 0;
#endif
}

int bk_feature_send_deauth_before_connect(void) {
#if CONFIG_DEAUTH_BEFORE_CONNECT
	return 1;
#else
	return 0;
#endif
}

int bk_feature_close_coexist_csa(void) {
#if CONFIG_CLOSE_COEXIST_CSA
	return 1;
#else
	return 0;
#endif
}

int bk_feature_get_scan_speed_level(void) {
	return CONFIG_SCAN_SPEED_LEVEL;
}

int bk_feature_get_mac_sup_sta_max_num(void) {
#if CONFIG_WIFI_MAC_SUPPORT_STAS_MAX_NUM
    return CONFIG_WIFI_MAC_SUPPORT_STAS_MAX_NUM;
#else
    return 2;
#endif
}

int bk_feature_network_found_event(void) {
#if CONFIG_NETWORK_FOUND_EVENT_ENABLE
	return 1;
#else
	return 0;
#endif
}

int bk_feature_save_rfcali_to_otp_enable(void) {
#if (CONFIG_OTP_V1 && CONFIG_PHY_RFCALI_TO_OTP)
    return 1;
#else
    return 0;
#endif
}

int bk_feature_phy_log_enable(void) {
#if CONFIG_PHY_LOG_ENABLE
	return 1;
#else
	return 0;
#endif
}

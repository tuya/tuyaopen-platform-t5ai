/**
* @file tkl_init_wifi.h
* @brief Common process - tkl init wifi description
* @version 0.1
* @date 2021-08-06
*
* @copyright Copyright 2021-2030 Tuya Inc. All Rights Reserved.
*
*/
#ifndef __TKL_INIT_WIFI_H__
#define __TKL_INIT_WIFI_H__

#include "tkl_wifi.h"

#ifdef ENABLE_HOSTAPD
#include "tkl_wifi_hostap.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * @brief the description of tuya kernel adapter layer wifi api
 *
 */
typedef struct {
    OPERATE_RET (*init)                         (WIFI_EVENT_CB cb);
    OPERATE_RET (*scan_ap)                      (CONST SCHAR_T *ssid, AP_IF_S **ap_ary, UINT_T *num);
    OPERATE_RET (*release_ap)                   (AP_IF_S *ap);
    OPERATE_RET (*start_ap)                     (CONST WF_AP_CFG_IF_S *cfg);
    OPERATE_RET (*stop_ap)                      (VOID_T);
    OPERATE_RET (*set_cur_channel)              (CONST UCHAR_T chan);
    OPERATE_RET (*get_cur_channel)              (UCHAR_T *chan);
    OPERATE_RET (*set_sniffer)                  (CONST BOOL_T en, CONST SNIFFER_CALLBACK cb);
    OPERATE_RET (*set_ip)                       (CONST WF_IF_E wf, NW_IP_S *ip);
    OPERATE_RET (*get_ip)                       (CONST WF_IF_E wf, NW_IP_S *ip);
    OPERATE_RET (*get_ipv6)                     (CONST WF_IF_E wf, NW_IP_TYPE type, NW_IP_S *ip);
    OPERATE_RET (*set_mac)                      (CONST WF_IF_E wf, CONST NW_MAC_S *mac);
    OPERATE_RET (*get_mac)                      (CONST WF_IF_E wf, NW_MAC_S *mac);
    OPERATE_RET (*set_work_mode)                (CONST WF_WK_MD_E mode);
    OPERATE_RET (*get_work_mode)                (WF_WK_MD_E *mode);
    OPERATE_RET (*get_connected_ap_info)        (FAST_WF_CONNECTED_AP_INFO_T **fast_ap_info);
    OPERATE_RET (*get_bssid)                    (UCHAR_T *mac);
    OPERATE_RET (*set_country_code)             (CONST COUNTRY_CODE_E ccode);
    OPERATE_RET (*set_country_code_v2)          (CONST UCHAR_T *ccode);
    OPERATE_RET (*get_country_code)             (UCHAR_T *ccode);
    OPERATE_RET (*set_lp_mode)                  (CONST BOOL_T en, CONST UCHAR_T dtim);
    BOOL_T      (*set_rf_calibrated)            (VOID_T);
    OPERATE_RET (*station_fast_connect)         (CONST FAST_WF_CONNECTED_AP_INFO_T *fast_ap_info);
    OPERATE_RET (*station_connect)              (CONST SCHAR_T *ssid, CONST SCHAR_T *passwd);
    OPERATE_RET (*station_disconnect)           (VOID_T);
    OPERATE_RET (*station_get_conn_ap_rssi)     (SCHAR_T *rssi);
    OPERATE_RET (*station_get_status)           (WF_STATION_STAT_E *stat);
    OPERATE_RET (*send_mgnt)                    (CONST UCHAR_T *buf, CONST UINT_T len);
    OPERATE_RET (*register_recv_mgnt_callback)  (CONST BOOL_T enable, CONST WIFI_REV_MGNT_CB recv_cb);
    OPERATE_RET (*ioctl)                        (WF_IOCTL_CMD_E cmd,  VOID *args);
} TKL_WIFI_DESC_T;

/**
 * @brief register wifi description to tuya object manage
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
TKL_WIFI_DESC_T* tkl_wifi_desc_get(VOID_T);

/**
 *
 * @brief the description of tuya kernel adapter layer wifi hostap api
 *
 */
typedef struct {
        OPERATE_RET (*ioctl)                    (INT_T dev, INT_T vif_index, UINT_T cmd, ULONG_T arg);
} TKL_WIFI_HOSTAP_DESC_T;

/**
 * @brief register wifi hostap description to tuya object manage
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
TKL_WIFI_HOSTAP_DESC_T* tkl_wifi_hostap_desc_get(VOID_T);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // __TKL_INIT_WIFI_H__


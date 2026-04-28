/**
* @file tkl_init_cellular.h
* @brief Common process - tkl init wired description
* @version 0.1
* @date 2021-08-06
*
* @copyright Copyright 2021-2030 Tuya Inc. All Rights Reserved.
*
*/
#ifndef __TKL_INIT_WIRED_H__
#define __TKL_INIT_WIRED_H__

#include "tkl_cellular.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * @brief the description of tuya kernel adapter layer wired
 *
 */
typedef struct {
    OPERATE_RET (*init)                     (TKL_CELLULAR_BASE_CFG_T *cfg);
    OPERATE_RET (*get_status)               (TKL_CELLULAR_STAT_E *is_up);
    OPERATE_RET (*set_status_cb)            (TKL_CELLULAR_STATUS_CHANGE_CB cb);
    OPERATE_RET (*set_ip)                   (NW_IP_S *ip);
    OPERATE_RET (*get_ip)                   (NW_IP_S *ip);
    OPERATE_RET (*get_ipv6)                 (NW_IP_TYPE type, NW_IP_S *ip);
    OPERATE_RET (*get_ccid)                 (char *ccid);
    OPERATE_RET (*get_rssi)                 (char *rssi);
    OPERATE_RET (*get_volt)                 (uint32_t *volt);
    OPERATE_RET (*get_imei)                 (char *imei);
    OPERATE_RET (*get_sn)                   (char *sn);
    OPERATE_RET (*get_sw_ver)               (char *ver);
    OPERATE_RET (*mf_test_start)            (TKL_CELLULAR_BASE_CFG_T *cfg);
    OPERATE_RET (*mf_test_stop)             (void);
} TKL_CELLULAR_DESC_T;

/**
 * @brief register cellular description to tuya object manage
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
TKL_CELLULAR_DESC_T* tkl_cellular_desc_get(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __TKL_INIT_WIRED_H__


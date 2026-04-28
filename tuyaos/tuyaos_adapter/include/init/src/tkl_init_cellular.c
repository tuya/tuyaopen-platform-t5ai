/**
* @file tkl_init_celluar.c
* @brief Common process - tkl init cellular description
* @version 0.1
* @date 2021-08-06
*
* @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
*
*/
#include "tkl_init_cellular.h"

const TKL_CELLULAR_DESC_T c_cellular_desc = {
    .init               = tkl_cellular_init,
    .get_status         = tkl_cellular_get_status,
    .set_status_cb      = tkl_cellular_set_status_cb,
    .get_ip             = tkl_cellular_get_ip,
    .get_ipv6           = tkl_cellular_get_ipv6,
    .get_ccid           = tkl_cellular_get_ccid,
    .get_rssi           = tkl_cellular_get_rssi,
    .get_volt           = tkl_cellular_get_volt,
    .get_imei           = tkl_cellular_get_imei,
    .get_sn             = tkl_cellular_get_sn,
    .get_sw_ver         = tkl_cellular_get_sw_ver,
};

/**
 * @brief register cellular description to tuya object manage
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
TUYA_WEAK_ATTRIBUTE TKL_CELLULAR_DESC_T* tkl_cellular_desc_get()
{
    return (TKL_CELLULAR_DESC_T*)&c_cellular_desc;
}


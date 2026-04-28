/**
* @file tkl_wired.h
* @brief Common process - adapter the wired api
* @version 0.1
* @date 2020-11-09
*
* @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
*
*/

#ifndef __TKL_WIRED_H__
#define __TKL_WIRED_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* tuyaos definition of wired network status */
typedef enum {
    TKL_WIRED_LINK_DOWN = 0,    ///< the network cable is unplugged
    TKL_WIRED_LINK_UP,          ///< the network cable is plugged and IP is got
}TKL_WIRED_STAT_E;

typedef enum {
    TKL_WIRED_SPI = 0,
    TKL_WIRED_RMII,
}TKL_WIRED_DEV_TYPE_E;

/**
 * @brief callback function: WIRED_STATUS_CHANGE_CB
 *        when wired connect status changed, notify tuyaos
 *        with this callback.
 *
 * @param[out]       is_up         the wired link status is up or not
 */
typedef void (*TKL_WIRED_STATUS_CHANGE_CB)(TKL_WIRED_STAT_E status);

typedef struct wired_rmii_s {
    TUYA_GPIO_NUM_E  int_gpio;
    TUYA_GPIO_NUM_E  mdc_gpio;
    TUYA_GPIO_NUM_E  mdio_gpio;
    TUYA_GPIO_NUM_E  rxd0_gpio;
    TUYA_GPIO_NUM_E  rxd1_gpio;
    TUYA_GPIO_NUM_E  rxdv_gpio;
    TUYA_GPIO_NUM_E  txd0_gpio;
    TUYA_GPIO_NUM_E  txd1_gpio;
    TUYA_GPIO_NUM_E  txen_gpio;
    TUYA_GPIO_NUM_E  ref_clk_gpio;
} TKL_WIRED_RMII_GPIO_CFG_T;

typedef struct wired_spi_s {
    TUYA_GPIO_NUM_E int_gpio;   ///< gpio number of SPI wired's interrupt
} TKL_WIRED_SPI_GPIO_CFG_T;

typedef struct {
    TUYA_GPIO_NUM_E int_gpio;   ///< gpio number of SPI wired's interrupt
}TKL_WIRED_BASE_CFG_T;

/* tuya sdk definition of wired ioctl cmd */
typedef enum {
    WIRED_IOCTL_GPIO_CFG,
} WIRED_IOCTL_CMD_E;

typedef struct {
    TKL_WIRED_DEV_TYPE_E dev_type;
    union {
        TKL_WIRED_RMII_GPIO_CFG_T rmii;
        TKL_WIRED_SPI_GPIO_CFG_T spi;
    };
} WIRED_IOCTL_GPIO_T;

/**
 * @brief  init create wired link
 *
 * @param[in]   cfg: the configure for wired link
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wired_init(TKL_WIRED_BASE_CFG_T *cfg);

/**
 * @brief  get the link status of wired link
 *
 * @param[out]  is_up: the wired link status is up or not
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wired_get_status(TKL_WIRED_STAT_E *status);

/**
 * @brief  set the status change callback
 *
 * @param[in]   cb: the callback when link status changed
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wired_set_status_cb(TKL_WIRED_STATUS_CHANGE_CB cb);

/**
 * @brief  set the ip address of the wired link, used for set the static ipaddress
 *
 * @param[in]   ip: the ip address
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wired_set_ip(NW_IP_S *ip);


/**
 * @brief  get the ip address of the wired link
 *
 * @param[in]   ip: the ip address
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wired_get_ip(NW_IP_S *ip);

/**
 * @brief  get the ip address of the wired link
 *
 * @param[in]   ip: the ip address
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wired_get_ipv6(NW_IP_TYPE type, NW_IP_S *ip);

/**
 * @brief  get the mac address of the wired link
 *
 * @param[in]   mac: the mac address
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wired_get_mac(NW_MAC_S *mac);

/**
 * @brief  set the mac address of the wired link
 *
 * @param[in]   mac: the mac address
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wired_set_mac(const NW_MAC_S *mac);

/**
 * @brief wired ioctl
 *
 * @param[in]       cmd     refer to WIRED_IOCTL_CMD_E
 * @param[in]       args    args associated with the command
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wired_ioctl(WIRED_IOCTL_CMD_E cmd,  void *args);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // __TKL_WIRED_H__


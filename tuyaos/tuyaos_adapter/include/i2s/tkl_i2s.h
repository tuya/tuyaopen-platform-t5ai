#ifndef __TKL_I2S_H__
#define __TKL_I2S_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief tuya i2s init
 * 
 * @param[in] i2s_num: i2s port number
 * @param[in] i2s_config: i2s config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */

OPERATE_RET tkl_i2s_init(TUYA_I2S_NUM_E i2s_num, const TUYA_I2S_BASE_CFG_T *i2s_config);

/**
 * @brief tuya i2s send
 * 
 * @param[in] i2s_num: i2s port number
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */

OPERATE_RET tkl_i2s_send(TUYA_I2S_NUM_E i2s_num, void *buff, uint32_t len);

/**
 * @brief tuya i2s async recv
 * 
 * @param[in] i2s_num: i2s port number
 *
 * @return return >= 0: number of data read; return < 0: read errror
 */

int32_t tkl_i2s_recv(TUYA_I2S_NUM_E i2s_num, void *buff, uint32_t len);

/**
 * @brief tuya i2s send stop
 * 
 * @param[in] i2s_num: i2s port number
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */

/**
 * @brief Start I2S hardware (DMA + codec)
 *
 * @param[in] i2s_num: i2s port number
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2s_start(TUYA_I2S_NUM_E i2s_num);

OPERATE_RET tkl_i2s_send_stop(TUYA_I2S_NUM_E i2s_num);

/**
 * @brief tuya i2s recv stop
 * 
 * @param[in] i2s_num: i2s port number
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */

OPERATE_RET tkl_i2s_recv_stop(TUYA_I2S_NUM_E i2s_num);


/**
 * @brief tuya i2s deinit
 * 
 * @param[in] i2s_num: i2s port number
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */

OPERATE_RET tkl_i2s_deinit(TUYA_I2S_NUM_E i2s_num);

/**
 * @brief Set I2S volume
 *
 * @param[in] i2s_num: i2s port number
 * @param[in] gain: volume gain
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2s_set_vol(TUYA_I2S_NUM_E i2s_num, uint32_t gain);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __TKL_I2S_H__


/**
* @file tkl_i2c.h
* @brief Common process - adapter the i2c api
* @version 0.1
* @date 2021-08-06
*
* @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
*
*/
#ifndef __TKL_I2C_H__
#define __TKL_I2C_H__

#include "tuya_cloud_types.h"

#include "tkl_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ioctl cmd define
 * 
 **/

#define I2C_IOCTL_SET_REGADDR_WIDTH    1

/**
 * @brief ioctl args define
 * 
 **/
typedef struct {
    UINT32_T dev_addr;
    UINT32_T reg_addr_width;
} REGADDR_WIDTH_T;

/**
 * @brief i2c init
 * 
 * @param[in] port: i2c port
 * @param[in] cfg: i2c config
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_init(TUYA_I2C_NUM_E port, CONST TUYA_IIC_BASE_CFG_T *cfg);

/**
 * @brief i2c deinit
 * 
 * @param[in] port: i2c port
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_deinit(TUYA_I2C_NUM_E port);

/**
 * @brief i2c irq init
 * NOTE: call this API will not enable interrupt
 * 
 * @param[in] port: i2c port, id index starts at 0
 * @param[in] cb:  i2c irq cb
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_irq_init(TUYA_I2C_NUM_E port, TUYA_I2C_IRQ_CB cb);

/**
 * @brief i2c irq enable
 * 
 * @param[in] port: i2c port id, id index starts at 0
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_irq_enable(TUYA_I2C_NUM_E port);

/**
 * @brief i2c irq disable
 * 
 * @param[in] port: i2c port id, id index starts at 0
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_irq_disable(TUYA_I2C_NUM_E port);

/**
 * @brief i2c master send
 * 
 * @param[in] port: i2c port
 * @param[in] dev_addr: iic addrress of slave device.
 * @param[in] data: i2c data to send
 * @param[in] size: Number of data items to send
 * @param[in] xfer_pending: xfer_pending: TRUE : not send stop condition, FALSE : send stop condition.
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_master_send(TUYA_I2C_NUM_E port, UINT16_T dev_addr, CONST VOID_T *data, UINT32_T size, BOOL_T xfer_pending);

/**
 * @brief i2c master recv
 * 
 * @param[in] port: i2c port
 * @param[in] dev_addr: iic addrress of slave device.
 * @param[in] data: i2c buf to recv
 * @param[in] size: Number of data items to receive
 * @param[in] xfer_pending: TRUE : not send stop condition, FALSE : send stop condition.
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_master_receive(TUYA_I2C_NUM_E port, UINT16_T dev_addr, VOID *data, UINT32_T size, BOOL_T xfer_pending);

/**
 * @brief i2c slave
 * 
 * @param[in] port: i2c port
 * @param[in] dev_addr: slave device addr
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_set_slave_addr(TUYA_I2C_NUM_E port, UINT16_T dev_addr);

/**
 * @brief i2c slave send
 * 
 * @param[in] port: i2c port
 * @param[in] data: i2c buf to send
 * @param[in] size: Number of data items to send
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */

OPERATE_RET tkl_i2c_slave_send(TUYA_I2C_NUM_E port, CONST VOID *data, UINT32_T size);

/**
 * @brief IIC slave receive, Start receiving data as IIC Slave.
 * 
 * @param[in] port: i2c port
 * @param[in] data: Pointer to buffer for data to receive from IIC Master
 * @param[in] size: Number of data items to receive
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */

OPERATE_RET tkl_i2c_slave_receive(TUYA_I2C_NUM_E port, VOID *data, UINT32_T size);

/**
 * @brief IIC get status.
 * 
 * @param[in] port: i2c port
 * @param[out]  TUYA_IIC_STATUS_T
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_get_status(TUYA_I2C_NUM_E port, TUYA_IIC_STATUS_T *status);

/**
 * @brief i2c's reset
 * 
 * @param[in] port: i2c port number
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET  tkl_i2c_reset(TUYA_I2C_NUM_E port);

/**
 * @brief i2c transferred data count.
 * 
 * @param[in] port: i2c port id, id index starts at 0
 *
 * @return >=0,number of currently transferred data items. <0,err. 
 * tkl_i2c_master_send:number of data bytes transmitted and acknowledged
 * tkl_i2c_master_receive:number of data bytes received
 * tkl_i2c_slave_send:number of data bytes transmitted
 * tkl_i2c_slave_receive:number of data bytes received and acknowledged
 */
INT32_T tkl_i2c_get_data_count(TUYA_I2C_NUM_E port);

/**
 * @brief i2c ioctl
 *
 * @param[in]       cmd     user def
 * @param[in]       args    args associated with the command
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_ioctl(TUYA_I2C_NUM_E port, UINT32_T cmd,  VOID *args);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif



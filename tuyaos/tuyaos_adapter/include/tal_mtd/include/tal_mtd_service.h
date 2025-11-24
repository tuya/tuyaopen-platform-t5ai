/**
 * @file tal_mtd_service.h
 * @brief Common process - driver mtd
 * @version 0.1
 * @date 2019-08-20
 *
 * @copyright Copyright 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TAL_MTD_SERVICE_H__
#define __TAL_MTD_SERVICE_H__

#include "tal_mtd_nand.h"
#include "tal_mtd_nor.h"
#include "tal_mtd_qspi.h"
#include "tal_mtd_spi.h"
#include "tuya_cloud_types.h"

typedef enum {
    MTD_NOR,  // Nor Flash
    MTD_NAND, // Nand Flash
} MTD_TYPE_E;

typedef struct {
    union {
        MTD_NOR_CFG_T nor_cfg;
        MTD_NAND_CFG_T nand_cfg;
    };
} MTD_CFG_T;

typedef struct {
    char *name;
    MTD_TYPE_E type;
    union {
        MTD_NAND_DEV_T nand_dev; /**< qspi device config */
        MTD_NOR_DEV_T nor_dev;   /**< spi device config */
    };
} MTD_DEVICE_T;

typedef struct {
    char *name;
    BOOL_T is_init;
    MTD_TYPE_E type;
    union {
        MTD_NAND_HANDLE_T *nand_handle; /**< qspi device handle */
        MTD_NOR_HANDLE_T *nor_handle;   /**< spi device handle */
    };
} MTD_HANDLE_T, *MTD_HANDLE;

/**
 * @brief mtd init
 *
 * @param[in] dev       Flash device config
 * @param[in] cfg      Flash device information
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
MTD_HANDLE tal_mtd_init(MTD_DEVICE_T *dev, MTD_CFG_T *cfg);

/**
 * @brief mtd read
 *
 * @param[in] handle       Flash device handle
 * @param[in] addr      address  of start
 * @param[in] buf       address of buffer
 * @param[in] len       read length
 *
 * @return Bytes on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_read(MTD_HANDLE handle, UINT_T addr, UINT8_T *buf,
                         UINT_T len);

/**
 * @brief mtd write
 *
 * @param[in] handle       Flash device handle
 * @param[in] addr      address  of start
 * @param[in] buf       address of buffer
 * @param[in] len       write length
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_write(MTD_HANDLE handle, UINT_T addr, const UINT8_T *buf,
                          UINT_T len);

/**
 * @brief mtd erase
 *
 * @param[in] handle       Flash device handle
 * @param[in] addr      address  of start
 * @param[in] len       write length
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_erase(MTD_HANDLE handle, UINT_T addr, UINT_T size);

/**
 * @brief mtd write
 *
 * @param[in] handle       Flash device handle
 * @param[out] id      id  of mtd
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_get_id(MTD_HANDLE handle, UINT_T *id);

/**
 * @brief mtd deinit
 *
 * @param[in] handle       Flash device handle
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_deinit(MTD_HANDLE handle);

#endif

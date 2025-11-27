/**
 * @file tal_mtd_nand_spi.h
 * @brief Common process - driver nand mtd
 * @version 0.1
 * @date 2019-08-20
 *
 * @copyright Copyright 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TAL_MTD_NAND_H__
#define __TAL_MTD_NAND_H__

#include "tal_mtd_qspi.h"
#include "tal_mtd_spi.h"
#include "tuya_cloud_types.h"

typedef struct {
    UINT_T page_size;
    UINT_T sector_size;
    UINT_T block_size;
    UINT_T total_size;
    UINT_T oob_size;
    MTD_INTERFACE_E interface;
    union {
        MTD_QSPI_DEV_T qspi_dev; /**< qspi device config */
        MTD_SPI_DEV_T spi_dev;   /**< spi device config */
    };
} MTD_NAND_DEV_T;

typedef struct {
    UINT_T page_size;
    UINT_T sector_size;
    UINT_T block_size;
    UINT_T total_size;
    UINT_T oob_size;
    MTD_INTERFACE_E interface;
    union {
        MTD_QSPI_HANDLE_T *qspi_handle; /**< qspi device config */
        MTD_SPI_HANDLE_T *spi_handle;   /**< spi device config */
    };
} MTD_NAND_HANDLE_T, *MTD_NAND_HANDLE;

typedef struct {
    union {
        MTD_QSPI_CFG_T cfg_qspi; /**< qspi device config */
        MTD_SPI_CFG_T cfg_spi;   /**< spi device config */
    };
} MTD_NAND_CFG_T;

/**
 * @brief nand mtd init
 *
 * @param[in] dev       nand Flash device infomation
 * @param[in] cfg       nand Flash device config
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
MTD_NAND_HANDLE tal_mtd_nand_init(MTD_NAND_DEV_T *dev, MTD_NAND_CFG_T *cfg);

/**
 * @brief nand mtd read
 *
 * @param[in] handle       Flash device handle
 * @param[in] addr      address  of start
 * @param[in] buf       address of buffer
 * @param[in] len       read length
 *
 * @return Bytes on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_nand_read(MTD_NAND_HANDLE handle, UINT_T addr, UINT8_T *buf,
                              UINT_T len);

/**
 * @brief nand mtd write
 *
 * @param[in] handle       Flash device handle
 * @param[in] addr      address  of start
 * @param[in] buf       address of buffer
 * @param[in] len       write length
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_nand_write(MTD_NAND_HANDLE handle, UINT_T addr,
                               const UINT8_T *buf, UINT_T len);

/**
 * @brief nand mtd write
 *
 * @param[in] handle       Flash device handle
 * @param[out] id      id  of nand mtd
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_nand_get_id(MTD_NAND_HANDLE handle, UINT_T *id);

/**
 * @brief nand mtd erase
 *
 * @param[in] handle       Flash device handle
 * @param[in] addr      address  of start
 * @param[in] buf       address of buffer
 * @param[in] len       write length
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_nand_erase(MTD_NAND_HANDLE handle, UINT_T addr,
                               UINT_T size);

/**
 * @brief nand mtd deinit
 *
 * @param[in] handle       Flash device handle
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_nand_deinit(MTD_NAND_HANDLE handle);

#endif

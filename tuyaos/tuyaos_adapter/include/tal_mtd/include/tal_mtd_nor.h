/**
 * @file tal_mtd_nor_qspi.h
 * @brief Common process - driver nor mtd
 * @version 0.1
 * @date 2019-08-20
 *
 * @copyright Copyright 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TAL_MTD_NOR_H__
#define __TAL_MTD_NOR_H__

#include "tal_mtd_qspi.h"
#include "tal_mtd_spi.h"
#include "tuya_cloud_types.h"

typedef struct {
    uint32_t page_size;
    uint32_t sector_size;
    uint32_t block_size;
    uint32_t total_size;
    MTD_INTERFACE_E interface;
    union {
        MTD_QSPI_DEV_T qspi_dev; /**< qspi device config */
        MTD_SPI_DEV_T spi_dev;   /**< spi device config */
    };
} MTD_NOR_DEV_T;

typedef struct {
    uint32_t page_size;
    uint32_t sector_size;
    uint32_t block_size;
    uint32_t total_size;
    MTD_INTERFACE_E interface;
    union {
        MTD_QSPI_HANDLE_T *qspi_handle; /**< qspi device config */
        MTD_SPI_HANDLE_T *spi_handle;   /**< spi device config */
    };
} MTD_NOR_HANDLE_T, *MTD_NOR_HANDLE;

typedef struct {
    union {
        MTD_QSPI_CFG_T cfg_qspi; /**< qspi device config */
        MTD_SPI_CFG_T cfg_spi;   /**< spi device config */
    };
} MTD_NOR_CFG_T;

/**
 * @brief nor mtd init
 *
 * @param[in] dev       nor Flash device infomation
 * @param[in] cfg       nor Flash device config
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
MTD_NOR_HANDLE tal_mtd_nor_init(MTD_NOR_DEV_T *dev, MTD_NOR_CFG_T *cfg);

/**
 * @brief nor mtd read
 *
 * @param[in] handle       Flash device handle
 * @param[in] addr      address  of start
 * @param[in] data       address of buffer
 * @param[in] size       read length
 *
 * @return Bytes on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_nor_read(MTD_NOR_HANDLE handle, uint32_t addr, uint8_t *data,
                             uint32_t size);

/**
 * @brief nor mtd write
 *
 * @param[in] handle       Flash device handle
 * @param[in] addr      address  of start
 * @param[in] data       address of buffer
 * @param[in] size       write length
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_nor_write(MTD_NOR_HANDLE handle, uint32_t addr,
                              const uint8_t *data, uint32_t size);

/**
 * @brief nor mtd erase
 *
 * @param[in] handle      address  of start
 * @param[in] addr       address of buffer
 * @param[in] len       write length
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_nor_erase(MTD_NOR_HANDLE handle, uint32_t addr, uint32_t size);

/**
 * @brief nor mtd write
 *
 * @param[in] handle       Flash device handle
 * @param[out] id      id  of nor mtd
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_nor_get_id(MTD_NOR_HANDLE handle, uint32_t *id);

/**
 * @brief nor mtd deinit
 *
 * @param[in] handle       Flash device handle
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_nor_deinit(MTD_NOR_HANDLE handle);

#endif

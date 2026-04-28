/**
 * @file tal_mtd_spi_spi.h
 * @brief Common process - driver mtd spi
 * @version 0.1
 * @date 2019-08-20
 *
 * @copyright Copyright 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TAL_MTD_SPI_H__
#define __TAL_MTD_SPI_H__

#include "tkl_spi.h"
#include "tuya_cloud_types.h"

typedef struct MTD_SPI_CFG_T {
    TUYA_SPI_NUM_E port; // SPI端口号
} MTD_SPI_CFG_T;

typedef int32_t (*SPI_MTD_INIT_T)(MTD_SPI_CFG_T *cfg);
typedef void (*SPI_MTD_WAIT_DONE_T)(MTD_SPI_CFG_T *cfg);
typedef int32_t (*SPI_MTD_DEINIT_T)(MTD_SPI_CFG_T *cfg);
typedef int32_t (*SPI_MTD_WRITE_UNLOCK_T)(MTD_SPI_CFG_T *cfg);

typedef struct {
    uint8_t read_id;           // 读ID指令 (e.g., 0x9F)
    uint8_t spi_read_data;     // 读数据指令 (e.g., 0x03)
    uint8_t spi_read_cache;    // 读cache数据指令 (e.g., 0x03)
    uint8_t spi_page_program;  // 页编程指令 (e.g., 0x02)
    uint8_t spi_program_cache; // 页cache编程指令 (e.g., 0x02)
    uint8_t sector_erase;      // 扇区擦除指令 (e.g., 0xC7)
    uint8_t block_erase;       // 块擦除指令 (e.g., 0xC7)
    uint8_t write_enable;      // 写使能指令 (e.g., 0xC7)
    uint8_t write_disable;     // 写禁止指令 (e.g., 0xC7)
} MTD_SPI_CMD_SET_T;

typedef struct {
    SPI_MTD_INIT_T init;
    SPI_MTD_WAIT_DONE_T wait;
    SPI_MTD_WRITE_UNLOCK_T unlock;
    SPI_MTD_DEINIT_T deinit;
} MTD_SPI_OPS_T;

typedef struct MTD_SPI_DEV_T {
    MTD_SPI_CMD_SET_T cmd_set;
    TUYA_SPI_BASE_CFG_T spi;
    MTD_SPI_OPS_T ops;
} MTD_SPI_DEV_T;

typedef struct MTD_SPI_HANDLE_T {
    TUYA_SPI_NUM_E port; // SPI端口号
    MTD_SPI_DEV_T dev;
} MTD_SPI_HANDLE_T, *MTD_SPI_HANDLE;
/**
 * @brief mtd spi init
 *
 * @param[in] dev       spi device
 * @param[in] cfg       spi config
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
MTD_SPI_HANDLE tal_mtd_spi_init(MTD_SPI_DEV_T *dev, MTD_SPI_CFG_T *cfg);

/**
 * @brief mtd spi read
 *
 * @param[in] handle       spi device handle
 * @param[in] addr      address  of start
 * @param[in] buf       address of buffer
 * @param[in] len       read length
 *
 * @return Bytes on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_spi_page_read(MTD_SPI_HANDLE handle, uint32_t addr,
                                  void *buf, uint32_t len, BOOL_T is_nand);

/**
 * @brief mtd spi write
 *
 * @param[in] handle       spi device handle
 * @param[in] addr      address  of start
 * @param[in] buf       address of buffer
 * @param[in] len       write length
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_spi_page_program(MTD_SPI_HANDLE handle, uint32_t addr,
                                     const void *buf, uint32_t len,
                                     BOOL_T is_nand);

/**
 * @brief mtd spi erase
 *
 * @param[in] handle       spi device handle
 * @param[in] addr      address  of start
 * @param[in] buf       address of buffer
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_spi_erase(MTD_SPI_HANDLE handle, uint32_t addr,
                              uint32_t erase_type);

/**
 * @brief mtd spi write
 *
 * @param[in] handle       spi device handle
 * @param[out] id      id  of flash
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_spi_read_id(MTD_SPI_HANDLE handle, uint32_t *id);

/**
 * @brief mtd spi deinit
 *
 * @param[in] handle       spi device handle
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_spi_deinit(MTD_SPI_HANDLE handle);

#endif

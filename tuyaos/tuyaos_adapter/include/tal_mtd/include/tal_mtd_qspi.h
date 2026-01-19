/**
 * @file tal_mtd_qspi_qspi.h
 * @brief Common process - driver mtd qspi
 * @version 0.1
 * @date 2019-08-20
 *
 * @copyright Copyright 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TAL_MTD_QSPI_H__
#define __TAL_MTD_QSPI_H__

#include "tkl_qspi.h"
#include "tuya_cloud_types.h"
#define QSPI_FIFO_SIZE (256)

typedef struct MTD_QSPI_CFG_T {
    TUYA_QSPI_NUM_E port; // QSPI端口号
} MTD_QSPI_CFG_T;

typedef INT32_T (*QSPI_MTD_INIT_T)(MTD_QSPI_CFG_T *cfg);
typedef VOID_T (*QSPI_MTD_WAIT_DONE_T)(MTD_QSPI_CFG_T *cfg);
typedef INT32_T (*QSPI_MTD_DEINIT_T)(MTD_QSPI_CFG_T *cfg);
typedef INT32_T (*QSPI_MTD_WRITE_UNLOCK_T)(MTD_QSPI_CFG_T *cfg);

typedef struct {
    UINT8_T command;
    UINT8_T addr_size;
    UINT8_T addr_lines;
    UINT8_T wire_lines;
    UINT8_T dummy;
} CMD_DSC_T;

typedef struct {
    CMD_DSC_T read_id;            // 读ID指令 (e.g., 0x9F)
    CMD_DSC_T quad_read_data;     // 读数据指令 (e.g., 0x03)
    CMD_DSC_T quad_read_cache;    // 读数据指令 (e.g., 0x03)
    CMD_DSC_T quad_program_cache; // 读缓存指令 (e.g., 0x03)
    CMD_DSC_T quad_page_program;  // 页编程指令 (e.g., 0x02)
    CMD_DSC_T sector_erase;       // 块擦除指令 (e.g., 0xC7)
    CMD_DSC_T block_erase;        // 块擦除指令 (e.g., 0xC7)
    CMD_DSC_T write_enable;       // 写使能指令 (e.g., 0xC7)
    CMD_DSC_T write_disable;      // 写禁止指令 (e.g., 0xC7)
} MTD_QSPI_CMD_SET_T;

typedef struct {
    QSPI_MTD_INIT_T init;
    QSPI_MTD_WAIT_DONE_T wait;
    QSPI_MTD_WRITE_UNLOCK_T unlock;
    QSPI_MTD_DEINIT_T deinit;
} MTD_QSPI_OPS_T;

typedef struct MTD_QSPI_DEV_T {
    MTD_QSPI_CMD_SET_T cmd_set; // 命令集合
    TUYA_QSPI_BASE_CFG_T qspi;  // qspi配置参数
    MTD_QSPI_OPS_T ops;         // qspi mtd qspi相关操作函数集合
} MTD_QSPI_DEV_T;

typedef struct MTD_QSPI_HANDLE_T {
    TUYA_SPI_NUM_E port; // SPI端口号
    MTD_QSPI_DEV_T dev;  // qspi mtd qspi相关操作函数集合
} MTD_QSPI_HANDLE_T, *MTD_QSPI_HANDLE;

/**
 * @brief mtd qspi init
 *
 * @param[in] dev       qspi device
 * @param[in] cfg       qspi configure
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
MTD_QSPI_HANDLE tal_mtd_qspi_init(MTD_QSPI_DEV_T *dev, MTD_QSPI_CFG_T *cfg);

/**
 * @brief mtd qspi read
 *
 * @param[in] handle       qspi configure
 * @param[in] addr      address  of start
 * @param[in] buf       address of buffer
 * @param[in] len       read length
 *
 * @return Bytes on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_qspi_page_read(MTD_QSPI_HANDLE handle, UINT_T addr,
                                   VOID_T *buf, UINT_T len, BOOL_T is_nand);

/**
 * @brief mtd qspi write
 *
 * @param[in] handle       qspi configure
 * @param[in] addr      address  of start
 * @param[in] buf       address of buffer
 * @param[in] len       write length
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_qspi_page_program(MTD_QSPI_HANDLE handle, UINT_T addr,
                                      const VOID_T *buf, UINT_T len,
                                      BOOL_T is_nand);

/**
 * @brief mtd qspi erase
 *
 * @param[in] handle       qspi configure
 * @param[in] addr      address  of start
 * @param[in] erase_type  type of erase
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_qspi_erase(MTD_QSPI_HANDLE handle, UINT_T addr,
                               UINT_T erase_type);

/**
 * @brief mtd qspi write
 *
 * @param[in] handle       Flash device handle
 * @param[out] id      id  of flash
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_qspi_read_id(MTD_QSPI_HANDLE handle, UINT_T *id);

/**
 * @brief mtd qspi deinit
 *
 * @param[in] handle       qspi configure
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_qspi_deinit(MTD_QSPI_HANDLE handle);

#endif

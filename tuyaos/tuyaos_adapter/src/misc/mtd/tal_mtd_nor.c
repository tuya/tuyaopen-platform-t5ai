/**
 * @file tal_mtd_nor.c
 * @brief This is tuya tal_mtd_nor file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2031 Tuya Inc. All Rights Reserved.
 *
 */
#include "tuya_iot_config.h"


/***********************************************************************
 ** INCLUDE                                                           **
 **********************************************************************/
#include "tal_mtd_nor.h"
#include "tal_mtd_service.h"
#include "tkl_init_common.h"
#include "tkl_memory.h"
#include "tkl_qspi.h"
#include "tkl_spi.h"

/***********************************************************************
 ** CONSTANT ( MACRO AND ENUM )                                       **
 **********************************************************************/

/***********************************************************************
 ** STRUCT                                                            **
 **********************************************************************/

/***********************************************************************
 ** VARIABLE                                                          **
 **********************************************************************/

/***********************************************************************
 ** FUNCTION                                                           **
 **********************************************************************/

static inline bool tal_mtd_is_align(uint32_t n)
{
    if ((n & (n - 1)) == 0)
        return 1;
    else
        return 0;
}

static OPERATE_RET tal_mtd_nor_page_read(MTD_NOR_HANDLE handle, uint32_t addr,
                                         void *data, uint32_t size)
{
    OPERATE_RET ret = OPRT_COM_ERROR;
    if (handle->interface == MTD_IF_QSPI) {
        ret = tal_mtd_qspi_page_read(handle->qspi_handle, addr, data, size, 0);
    } else if (handle->interface == MTD_IF_SPI) {
        ret = tal_mtd_spi_page_read(handle->spi_handle, addr, data, size, 0);
    } else {
        return OPRT_INVALID_PARM;
    }
    return ret;
}

static OPERATE_RET tal_mtd_nor_page_write(MTD_NOR_HANDLE handle, uint32_t addr,
                                          const void *data, uint32_t size)
{
    OPERATE_RET ret = OPRT_COM_ERROR;
    if (handle->interface == MTD_IF_QSPI) {
        ret =
            tal_mtd_qspi_page_program(handle->qspi_handle, addr, data, size, 0);
    } else if (handle->interface == MTD_IF_SPI) {
        ret = tal_mtd_spi_page_program(handle->spi_handle, addr, data, size, 0);
    } else {
        return OPRT_INVALID_PARM;
    }
    return ret;
}

static OPERATE_RET tal_mtd_nor_erase_byType(MTD_NOR_HANDLE handle, uint32_t addr,
                                            uint32_t byType)
{
    OPERATE_RET ret = OPRT_COM_ERROR;
    if (handle->interface == MTD_IF_QSPI) {
        ret = tal_mtd_qspi_erase(handle->qspi_handle, addr, byType);
    } else if (handle->interface == MTD_IF_SPI) {
        ret = tal_mtd_spi_erase(handle->spi_handle, addr, byType);
    } else {
        return OPRT_INVALID_PARM;
    }

    return ret;
}

/**
 * @brief flash init
 *
 * @param[in] dev      Flash device information
 * @param[in] cfg      Flash config information
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
MTD_NOR_HANDLE tal_mtd_nor_init(MTD_NOR_DEV_T *dev, MTD_NOR_CFG_T *cfg)
{
    MTD_NOR_HANDLE handle = NULL;
    void *interface_handle = NULL;

    if (!tal_mtd_is_align(dev->total_size) ||
        !tal_mtd_is_align(dev->page_size) ||
        !tal_mtd_is_align(dev->block_size) ||
        ((dev->interface != MTD_IF_QSPI) && (dev->interface != MTD_IF_SPI))) {
        return NULL;
    }

    // 接口初始化应当在具体的flash初始化实现
    if (dev->interface == MTD_IF_QSPI) {
        interface_handle = tal_mtd_qspi_init(&dev->qspi_dev, &cfg->cfg_qspi);
        if (interface_handle == NULL) {
            return NULL;
        }
    } else {
        interface_handle = tal_mtd_spi_init(&dev->spi_dev, &cfg->cfg_spi);
        if (interface_handle == NULL) {
            return NULL;
        }
    }

    handle = (MTD_NOR_HANDLE)tkl_system_malloc(sizeof(MTD_NOR_HANDLE_T));
    if (handle == NULL) {
        return NULL;
    }

    handle->block_size = dev->block_size;
    handle->sector_size = dev->sector_size;
    handle->page_size = dev->page_size;
    handle->total_size = dev->total_size;
    handle->interface = dev->interface;
    if (dev->interface == MTD_IF_QSPI) {
        handle->qspi_handle = interface_handle;
    } else {
        handle->spi_handle = interface_handle;
    }
    return handle;
}

/**
 * @brief flash read
 *
 * @param[in] handle       Flash device handle
 * @param[in] addr      address  of start
 * @param[in] buf       address of buffer
 * @param[in] len       read length
 *
 * @return Bytes on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_nor_read(MTD_NOR_HANDLE handle, uint32_t addr, uint8_t *data,
                             uint32_t size)
{
    OPERATE_RET ret = OPRT_OK;

    if ((NULL == data) || (0 == size)) {
        return OPRT_INVALID_PARM;
    }
    if (addr >= handle->total_size || size > handle->total_size - addr) {
        return OPRT_INVALID_PARM;
    }
    uint32_t page_mask = handle->page_size - 1;
    uint32_t first_page_addr = addr & ~page_mask;
    uint32_t first_page_offset = addr & page_mask;
    uint32_t first_page_left_len = handle->page_size - first_page_offset;
    uint32_t first_page_copy_len =
        (size < first_page_left_len) ? size : first_page_left_len;
    uint32_t continue_page_num = 0;
    if (size > first_page_copy_len) {
        continue_page_num = (size - first_page_copy_len) / handle->page_size;
    }
    uint32_t continue_start_addr = first_page_addr + handle->page_size;
    uint32_t last_page_start_addr =
        continue_start_addr + continue_page_num * handle->page_size;
    uint32_t last_ofs =
        first_page_copy_len + continue_page_num * handle->page_size;
    uint32_t last_copy_len = size - last_ofs;
    uint8_t *buf = (uint8_t *)tkl_system_malloc(handle->page_size);
    if (buf == NULL) {
        return OPRT_MALLOC_FAILED;
    }
    memset(buf, 0, handle->page_size);

    // 1. 1st page handle

    ret =
        tal_mtd_nor_page_read(handle, first_page_addr, buf, handle->page_size);
    if (ret != OPRT_OK) {
        tkl_system_free(buf);
        buf = NULL;
        return OPRT_COM_ERROR;
    }
    memcpy(data, buf + first_page_offset, first_page_copy_len);

    if (size == first_page_copy_len) {
        tkl_system_free(buf);
        buf = NULL;
        return 0;
    }

    // 2. whole pages continue
    for (int i = 0; i < continue_page_num; i++) {
        uint32_t tmp_addr = continue_start_addr + i * handle->page_size;
        uint32_t r_ofs = first_page_copy_len + i * handle->page_size;
        ret = tal_mtd_nor_page_read(handle, tmp_addr, (uint8_t *)data + r_ofs,
                                    handle->page_size);
        if (ret != OPRT_OK) {
            tkl_system_free(buf);
            buf = NULL;
            return OPRT_COM_ERROR;
        }
    }

    if (last_copy_len > 0) {
        ret = tal_mtd_nor_page_read(handle, last_page_start_addr, buf,
                                    handle->page_size);
        if (ret != OPRT_OK) {
            tkl_system_free(buf);
            buf = NULL;
            return OPRT_COM_ERROR;
        }
        memcpy(data + last_ofs, buf, last_copy_len);
    }

    tkl_system_free(buf);
    buf = NULL;
    return ret;
}

/**
 * @brief flash write
 *
 * @param[in] handle       Flash device handle
 * @param[in] addr      address  of start
 * @param[in] buf       address of buffer
 * @param[in] len       write length
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_nor_write(MTD_NOR_HANDLE handle, uint32_t addr,
                              const uint8_t *data, uint32_t size)
{
    OPERATE_RET ret = OPRT_OK;

    if ((NULL == data) || (0 == size)) {
        return OPRT_INVALID_PARM;
    }
    if ((addr >= handle->total_size) || (size > (handle->total_size - addr))) {
        return OPRT_INVALID_PARM;
    }
    uint32_t page_mask = handle->page_size - 1;
    uint32_t first_page_addr = addr & ~page_mask;
    uint32_t first_page_offset = addr & page_mask;
    uint32_t first_page_left_len = handle->page_size - first_page_offset;
    uint32_t first_page_write_len =
        (size < first_page_left_len) ? size : first_page_left_len;
    uint32_t continue_page_num = 0;
    if (size > first_page_left_len) {
        continue_page_num = (size - first_page_left_len) / handle->page_size;
    }
    uint32_t continue_start_addr = first_page_addr + handle->page_size;
    uint32_t last_page_start_addr =
        continue_start_addr + continue_page_num * handle->page_size;
    uint32_t last_ofs =
        first_page_write_len + continue_page_num * handle->page_size;
    uint32_t last_page_write_len =
        (size - first_page_write_len) % handle->page_size;
    uint8_t *buf = (uint8_t *)tkl_system_malloc(handle->page_size);
    if (buf == NULL) {
        return OPRT_MALLOC_FAILED;
    }
    memset(buf, 0, handle->page_size);

    // 1. 1st page handle
    ret =
        tal_mtd_nor_page_read(handle, first_page_addr, buf, handle->page_size);
    if (ret != OPRT_OK) {
        tkl_system_free(buf);
        buf = NULL;
        return OPRT_COM_ERROR;
    }
    memcpy(buf + first_page_offset, data, first_page_write_len);
    ret =
        tal_mtd_nor_page_write(handle, first_page_addr, buf, handle->page_size);
    if (ret != OPRT_OK) {
        tkl_system_free(buf);
        buf = NULL;
        return OPRT_COM_ERROR;
    }

    // 2. whole pages continue
    for (int i = 0; i < continue_page_num; i++) {
        uint32_t tmp_addr = continue_start_addr + i * handle->page_size;
        uint32_t w_ofs = first_page_write_len + i * handle->page_size;
        ret = tal_mtd_nor_page_write(handle, tmp_addr, data + w_ofs,
                                     handle->page_size);
        if (ret != OPRT_OK) {
            tkl_system_free(buf);
            buf = NULL;
            return OPRT_COM_ERROR;
        }
    }

    if (last_page_write_len > 0) {
        memset(buf, 0, handle->page_size);
        ret = tal_mtd_nor_page_read(handle, last_page_start_addr, buf,
                                    handle->page_size);
        if (ret != OPRT_OK) {
            tkl_system_free(buf);
            buf = NULL;
            return OPRT_COM_ERROR;
        }
        memcpy(buf, data + last_ofs, last_page_write_len);
        ret = tal_mtd_nor_page_write(handle, last_page_start_addr, buf,
                                     handle->page_size);
        if (ret != OPRT_OK) {
            tkl_system_free(buf);
            buf = NULL;
            return OPRT_COM_ERROR;
        }
    }

    tkl_system_free(buf);
    buf = NULL;
    return ret;
}

/**
 * @brief flash erase
 *
 * @param[in] handle       Flash device handle
 * @param[in] addr      address  of start
 * @param[in] size       write length
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_nor_erase(MTD_NOR_HANDLE handle, uint32_t addr, uint32_t size)
{
    OPERATE_RET ret = OPRT_OK;
    const uint32_t block_size = handle->block_size;   // 块大小（字节）
    const uint32_t sector_size = handle->sector_size; // 页/扇区大小（字节）
    // 计算擦除区域的起始和结束地址
    const uint32_t end_addr = addr + size;

    if (handle->block_size == 0 || handle->page_size == 0 ||
        handle->total_size == 0) {
        return OPRT_INVALID_PARM;
    }
    if (size == 0 || addr >= handle->total_size ||
        size > handle->total_size - addr) {
        return OPRT_INVALID_PARM;
    }

    // 遍历所有涉及的块
    uint32_t current_block_start =
        (addr / block_size) * block_size; // 起始块的首地址
    uint32_t current_block_end = current_block_start + block_size;

    while (current_block_start < end_addr) {
        // 计算当前块内需要擦除的起始和结束地址
        uint32_t erase_start_in_block = MAX(addr, current_block_start);
        uint32_t erase_end_in_block = MIN(end_addr, current_block_end);

        // 情况1：整个块需要擦除
        if (erase_start_in_block == current_block_start &&
            erase_end_in_block == current_block_end) {
            ret = tal_mtd_nor_erase_byType(handle, current_block_start, 0);
            if (ret != OPRT_OK)
                return ret;
        }
        // 情况2：部分页需要擦除
        else {
            // 计算当前块内需要擦除的页
            uint32_t sector_start = erase_start_in_block / sector_size;
            uint32_t sector_end = (erase_end_in_block - 1) / sector_size;

            for (uint32_t sector = sector_start; sector <= sector_end; sector++) {
                uint32_t sector_addr = sector * sector_size;
                ret = tal_mtd_nor_erase_byType(handle, sector_addr, 1);
                if (ret != OPRT_OK)
                    return ret;
            }
        }

        // 移动到下一个块
        current_block_start += block_size;
        current_block_end += block_size;
    }

    return ret;
}

/**
 * @brief flash write
 *
 * @param[in] handle       Flash device handle
 * @param[out] id      id  of flash
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_nor_get_id(MTD_NOR_HANDLE handle, uint32_t *id)
{
    OPERATE_RET ret = OPRT_OK;

    if (handle->interface == MTD_IF_QSPI) {
        ret = tal_mtd_qspi_read_id(handle->qspi_handle, id);
    } else if (handle->interface == MTD_IF_SPI) {
        ret = tal_mtd_spi_read_id(handle->spi_handle, id);
    } else {
        ret = OPRT_INVALID_PARM;
    }
    return ret;
}

/**
 * @brief flash deinit
 *
 * @param[in] handle       Flash device handle
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_nor_deinit(MTD_NOR_HANDLE handle)
{
    OPERATE_RET ret = OPRT_OK;

    if (handle->interface == MTD_IF_QSPI) {
        ret = tal_mtd_qspi_deinit(handle->qspi_handle);
        handle->qspi_handle = NULL;
    } else if (handle->interface == MTD_IF_SPI) {
        ret = tal_mtd_spi_deinit(handle->spi_handle);
        handle->spi_handle = NULL;
    } else {
        ret = OPRT_INVALID_PARM;
    }
    tkl_system_free(handle);
    handle = NULL;
    return ret;
}


/**
 * @file tal_mtd_nand.c
 * @brief This is tuya tal_mtd_nand file
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
#include "tal_mtd_nand.h"
#include "tal_mtd_service.h"
#include "tkl_init_common.h"
#include "tkl_memory.h"
#include "tkl_spi.h"

/***********************************************************************
 ** CONSTANT ( MACRO AND ENUM )                                       **
 **********************************************************************/
#define MTD_CHECK_WORD_ALIGN (0) // check word align.
#define MTD_WORD_SIZE (4)        // unit: byte.
/***********************************************************************
 ** STRUCT                                                            **
 **********************************************************************/

/***********************************************************************
 ** VARIABLE                                                          **
 **********************************************************************/

/***********************************************************************
 ** FUNCTION                                                           **
 **********************************************************************/
static inline bool tal_mtd_is_align(UINT_T n)
{
    if ((n & (n - 1)) == 0)
        return 1;
    else
        return 0;
}

static OPERATE_RET tal_mtd_nand_erase_byType(MTD_NAND_HANDLE handle,
                                             uint32_t addr, UINT_T erase_type)
{
    OPERATE_RET ret = OPRT_OK;
    if (handle->interface == MTD_IF_QSPI) {
        ret = tal_mtd_qspi_erase(handle->qspi_handle, addr, erase_type);
    } else if (handle->interface == MTD_IF_SPI) {
        ret = tal_mtd_spi_erase(handle->spi_handle, addr, erase_type);
    } else {
        ret = OPRT_INVALID_PARM;
    }
    return ret;
}

static OPERATE_RET tal_mtd_nand_page_read(MTD_NAND_HANDLE handle, UINT_T addr,
                                          VOID_T *data, UINT_T size,
                                          UINT_T oob_size)
{
    OPERATE_RET ret = OPRT_OK;
    if (handle->interface == MTD_IF_QSPI) {
        ret = tal_mtd_qspi_page_read(handle->qspi_handle, addr, data,
                                     size + oob_size, true);
    } else if (handle->interface == MTD_IF_SPI) {
        ret = tal_mtd_spi_page_read(handle->spi_handle, addr, data,
                                    size + oob_size, true);
    } else {
        ret = OPRT_INVALID_PARM;
    }
    return ret;
}

static OPERATE_RET tal_mtd_nand_page_write(MTD_NAND_HANDLE handle, UINT_T addr,
                                           const VOID_T *data, UINT_T size,
                                           UINT_T oob_size)
{
    OPERATE_RET ret = OPRT_OK;
    if (handle->interface == MTD_IF_QSPI) {
        ret = tal_mtd_qspi_page_program(handle->qspi_handle, addr, data,
                                        size + oob_size, true);
    } else if (handle->interface == MTD_IF_SPI) {
        ret = tal_mtd_spi_page_program(handle->spi_handle, addr, data,
                                       size + oob_size, true);
    } else {
        ret = OPRT_INVALID_PARM;
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
MTD_NAND_HANDLE tal_mtd_nand_init(MTD_NAND_DEV_T *dev, MTD_NAND_CFG_T *cfg)
{
    MTD_NAND_HANDLE handle = NULL;
    VOID_T *interface_handle = NULL;
    if (!tal_mtd_is_align(dev->total_size) ||
        !tal_mtd_is_align(dev->page_size) ||
        !tal_mtd_is_align(dev->block_size) ||
        ((dev->interface != MTD_IF_QSPI) && (dev->interface != MTD_IF_SPI))) {
        return NULL;
    }

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

    handle = (MTD_NAND_HANDLE)tkl_system_malloc(sizeof(MTD_NAND_HANDLE_T));
    if (handle == NULL) {
        return NULL;
    }
    handle->block_size = dev->block_size;
    handle->sector_size = dev->sector_size;
    handle->page_size = dev->page_size;
    handle->total_size = dev->total_size;
    handle->oob_size = dev->oob_size;
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
OPERATE_RET tal_mtd_nand_read(MTD_NAND_HANDLE handle, UINT_T addr,
                              UINT8_T *buff, UINT_T size)
{
    OPERATE_RET ret = OPRT_COM_ERROR;

    if ((NULL == buff) || (addr >= handle->total_size) ||
        (handle->page_size == 0) || (size > handle->total_size) ||
        (0 == size)) {
        return OPRT_INVALID_PARM;
    }

    UINT_T page_mask = handle->page_size - 1;
    UINT_T first_page_addr = addr / handle->page_size;
    UINT_T first_page_offset = addr & page_mask;
    UINT_T first_page_left_len = handle->page_size - first_page_offset;
    UINT_T first_page_copy_len =
        (size < first_page_left_len) ? size : first_page_left_len;
    UINT_T continue_page_num = 0;
    if (size > first_page_copy_len) {
        continue_page_num = (size - first_page_copy_len) / handle->page_size;
    }
    UINT_T continue_start_addr = first_page_addr + 1;
    UINT_T last_page_start_addr = continue_start_addr + continue_page_num;
    UINT_T last_ofs =
        first_page_copy_len + continue_page_num * handle->page_size;
    UINT_T last_copy_len = size - last_ofs;
    // oob暂不处理
    UINT8_T *buf =
        (UINT8_T *)tkl_system_malloc(handle->page_size + handle->oob_size);
    if (buf == NULL) {
        return OPRT_MALLOC_FAILED;
    }

    memset(buf, 0, handle->page_size);

    // 1. 1st page handle

    ret = tal_mtd_nand_page_read(handle, first_page_addr, buf,
                                 handle->page_size, handle->oob_size);
    if (ret != OPRT_OK) {
        tkl_system_free(buf);
        buf = NULL;
        return OPRT_COM_ERROR;
    }
    memcpy(buff, buf + first_page_offset, first_page_copy_len);

    if (size == first_page_copy_len) {
        tkl_system_free(buf);
        buf = NULL;
        return 0;
    }

    // 2. whole pages continue
    for (int i = 0; i < continue_page_num; i++) {
        UINT_T tmp_addr = continue_start_addr + i;
        UINT_T r_ofs = first_page_copy_len + i * handle->page_size;
        memset(buf, 0, handle->page_size + handle->oob_size);
        ret = tal_mtd_nand_page_read(handle, tmp_addr, (UINT8_T *)buf,
                                     handle->page_size, handle->oob_size);
        if (ret != OPRT_OK) {
            tkl_system_free(buf);
            buf = NULL;
            return OPRT_COM_ERROR;
        }
        memcpy(buff + r_ofs, buf, handle->page_size);
    }

    if (last_copy_len > 0) {
        ret = tal_mtd_nand_page_read(handle, last_page_start_addr, buf,
                                     handle->page_size, handle->oob_size);
        if (ret != OPRT_OK) {
            tkl_system_free(buf);
            buf = NULL;
            return OPRT_COM_ERROR;
        }
        memcpy(buff + last_ofs, buf, last_copy_len);
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
 * @param[in] buff       address of buffer
 * @param[in] size       write length
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
OPERATE_RET tal_mtd_nand_write(MTD_NAND_HANDLE handle, UINT_T addr,
                               const UINT8_T *buff, UINT_T size)
{
    OPERATE_RET ret = OPRT_OK;

    if ((addr >= handle->total_size) || (size > handle->total_size) ||
        (0 == size) || (NULL == buff)) {
        return OPRT_INVALID_PARM;
    }

    UINT_T page_mask = handle->page_size - 1;
    UINT_T first_page_addr = addr / handle->page_size;
    UINT_T first_page_offset = addr & page_mask;
    UINT_T first_page_left_len = handle->page_size - first_page_offset;
    UINT_T first_page_write_len =
        (size < first_page_left_len) ? size : first_page_left_len;
    UINT_T continue_page_num = 0;
    if (size > first_page_left_len) {
        continue_page_num = (size - first_page_left_len) / handle->page_size;
    }
    UINT_T continue_start_addr = first_page_addr + 1;
    UINT_T last_page_start_addr = continue_start_addr + continue_page_num;
    UINT_T last_ofs =
        first_page_write_len + continue_page_num * handle->page_size;
    UINT_T last_page_write_len =
        (size - first_page_write_len) % handle->page_size;
    // oob暂不处理
    bk_printf("size %x %x\n", handle->page_size, &handle->page_size);
    UINT8_T *buf =
        (UINT8_T *)tkl_system_malloc(handle->page_size + handle->oob_size);
    if (buf == NULL) {
        return OPRT_MALLOC_FAILED;
    }
    memset(buf, 0, handle->page_size);

    // 1. 1st page handle
    ret = tal_mtd_nand_page_read(handle, first_page_addr, buf,
                                 handle->page_size, handle->oob_size);
    if (ret != OPRT_OK) {
        tkl_system_free(buf);
        buf = NULL;
        return OPRT_COM_ERROR;
    }
    memcpy(buf + first_page_offset, buff, first_page_write_len);
    ret = tal_mtd_nand_page_write(handle, first_page_addr, buf,
                                  handle->page_size, handle->oob_size);
    if (ret != OPRT_OK) {
        tkl_system_free(buf);
        buf = NULL;
        return OPRT_COM_ERROR;
    }

    // 2. whole pages continue
    for (int i = 0; i < continue_page_num; i++) {
        UINT_T tmp_addr = continue_start_addr + i;
        UINT_T w_ofs = first_page_write_len + i * handle->page_size;
        memset(buf, 0, handle->page_size + handle->oob_size);
        memcpy(buf, buff + w_ofs, handle->page_size);
        ret = tal_mtd_nand_page_write(handle, tmp_addr, buf, handle->page_size,
                                      handle->oob_size);
        if (ret != OPRT_OK) {
            tkl_system_free(buf);
            buf = NULL;
            return OPRT_COM_ERROR;
        }
    }

    if (last_page_write_len > 0) {
        memset(buf, 0, handle->page_size + handle->oob_size);
        ret = tal_mtd_nand_page_read(handle, last_page_start_addr, buf,
                                     handle->page_size, handle->oob_size);
        if (ret != OPRT_OK) {
            tkl_system_free(buf);
            buf = NULL;
            return OPRT_COM_ERROR;
        }
        memcpy(buf, buff + last_ofs, last_page_write_len);
        ret = tal_mtd_nand_page_write(handle, last_page_start_addr, buf,
                                      handle->page_size, handle->oob_size);
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
OPERATE_RET tal_mtd_nand_erase(MTD_NAND_HANDLE handle, UINT_T addr, UINT_T size)
{
    OPERATE_RET ret = OPRT_OK;
    // 遍历所有涉及的块
    const UINT_T block_size = handle->block_size; // 块大小（字节）
    const UINT_T page_nums_per_block =
        handle->block_size / handle->page_size; // 页大小（字节）

    const UINT_T start_block = addr / block_size;
    const UINT_T end_block = (addr + size - 1) / block_size;
    if (handle->block_size == 0 || handle->page_size == 0 ||
        handle->total_size == 0) {
        return OPRT_INVALID_PARM;
    }
    if (size == 0 || addr >= handle->total_size ||
        size > (handle->total_size - addr) || addr > (((UINT_T)-1L) - size)) {
        return OPRT_INVALID_PARM;
    }

    for (UINT_T block = start_block; block <= end_block; block++) {
        // 转换为行地址
        const UINT_T page_addr = block * page_nums_per_block;
        // if (tal_mtd_nand_is_bad_block(cfg, block_addr)) {
        //     continue; // 跳过坏块
        // }
        ret = tal_mtd_nand_erase_byType(handle, page_addr, 0); // 仅支持块擦除
        if (ret != OPRT_OK) {
            return ret;
        }
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
OPERATE_RET tal_mtd_nand_get_id(MTD_NAND_HANDLE handle, UINT_T *id)
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
OPERATE_RET tal_mtd_nand_deinit(MTD_NAND_HANDLE handle)
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

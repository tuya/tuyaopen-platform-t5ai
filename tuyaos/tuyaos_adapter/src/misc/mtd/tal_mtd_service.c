/**
 * @file tal_mtd_service.c
 * @brief This is tuya tal_mtd file
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
#include "tal_mtd_nor.h"
#include "tal_mtd_service.h"
#include "tkl_init_common.h"
#include "tkl_memory.h"

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

/**
 * @brief flash init
 *
 * @param[in] dev       Flash device handle
 * @param[in] cfg      Flash device information
 *
 * @return OPRT_OK on success. Others on error, please refer to tal_error_code.h
 */
MTD_HANDLE tal_mtd_init(MTD_DEVICE_T *dev, MTD_CFG_T *cfg)
{
    if ((dev == NULL) || (cfg == NULL) ||
        ((dev->type != MTD_NOR) && (dev->type != MTD_NAND))) {
        return NULL;
    }
    MTD_HANDLE handle = NULL;
    void *mtd_handle = NULL;
    if (dev->type == MTD_NOR) {
        mtd_handle = tal_mtd_nor_init(&dev->nor_dev, &cfg->nor_cfg);
        if (mtd_handle == NULL) {
            return NULL;
        }
    } else {
        mtd_handle = tal_mtd_nand_init(&dev->nand_dev, &cfg->nand_cfg);
        if (mtd_handle == NULL) {
            return NULL;
        }
    }
    handle = (MTD_HANDLE)tkl_system_malloc(sizeof(MTD_HANDLE_T));
    if (handle == NULL) {
        return NULL;
    }

    handle->type = dev->type;
    handle->name = dev->name;
    if (dev->type == MTD_NOR) {
        handle->nor_handle = mtd_handle;
    } else {
        handle->nand_handle = mtd_handle;
    }
    handle->is_init = TRUE;
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
OPERATE_RET tal_mtd_read(MTD_HANDLE handle, uint32_t addr, uint8_t *buf,
                         uint32_t len)
{
    OPERATE_RET ret = OPRT_COM_ERROR;
    if ((handle == NULL)) {
        return OPRT_INVALID_PARM;
    }
    if (handle->is_init == FALSE) {
        return OPRT_INVALID_PARM;
    }

    if (handle->type == MTD_NOR) {
        ret = tal_mtd_nor_read(handle->nor_handle, addr, buf, len);
    } else if (handle->type == MTD_NAND) {
        ret = tal_mtd_nand_read(handle->nand_handle, addr, buf, len);
    } else {
        return OPRT_INVALID_PARM;
    }
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
OPERATE_RET tal_mtd_write(MTD_HANDLE handle, uint32_t addr, const uint8_t *buf,
                          uint32_t len)
{
    OPERATE_RET ret = OPRT_OK;
    if ((handle == NULL)) {
        return OPRT_INVALID_PARM;
    }
    if (handle->is_init == FALSE) {
        return OPRT_INVALID_PARM;
    }

    if (handle->type == MTD_NOR) {
        ret = tal_mtd_nor_write(handle->nor_handle, addr, buf, len);
    } else if (handle->type == MTD_NAND) {
        ret = tal_mtd_nand_write(handle->nand_handle, addr, buf, len);
    } else {
        return OPRT_INVALID_PARM;
    }
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
OPERATE_RET tal_mtd_erase(MTD_HANDLE handle, uint32_t addr, uint32_t size)
{
    OPERATE_RET ret = OPRT_OK;
    if ((handle == NULL)) {
        return OPRT_INVALID_PARM;
    }
    if (handle->is_init == FALSE) {
        return OPRT_INVALID_PARM;
    }

    if (handle->type == MTD_NOR) {
        ret = tal_mtd_nor_erase(handle->nor_handle, addr, size);
    } else if (handle->type == MTD_NAND) {
        ret = tal_mtd_nand_erase(handle->nand_handle, addr, size);
    } else {
        return OPRT_INVALID_PARM;
    }
    return ret;
}

OPERATE_RET tal_mtd_get_id(MTD_HANDLE handle, uint32_t *id)
{
    OPERATE_RET ret = OPRT_NOT_SUPPORTED;
    if ((handle == NULL)) {
        return OPRT_INVALID_PARM;
    }
    if (handle->is_init == FALSE) {
        return OPRT_INVALID_PARM;
    }
    if (handle->type == MTD_NOR) {
        ret = tal_mtd_nor_get_id(handle->nor_handle, id);
    } else if (handle->type == MTD_NAND) {
        ret = tal_mtd_nand_get_id(handle->nand_handle, id);
    } else {
        return OPRT_INVALID_PARM;
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
OPERATE_RET tal_mtd_deinit(MTD_HANDLE handle)
{
    OPERATE_RET ret = OPRT_OK;
    if ((handle == NULL)) {
        return OPRT_INVALID_PARM;
    }
    if (handle->is_init == FALSE) {
        return OPRT_INVALID_PARM;
    }
    if (handle->type == MTD_NOR) {
        ret = tal_mtd_nor_deinit(handle->nor_handle);
        handle->nor_handle = NULL;
    } else if (handle->type == MTD_NAND) {
        ret = tal_mtd_nand_deinit(handle->nand_handle);
        handle->nand_handle = NULL;
    } else {
        return OPRT_INVALID_PARM;
    }
    handle->name = NULL;
    tkl_system_free(handle);
    handle = NULL;
    handle->is_init = FALSE;
    return ret;
}

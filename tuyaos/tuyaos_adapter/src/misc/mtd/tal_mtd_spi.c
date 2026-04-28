/**
 * @file tal_mtd_spi.c
 * @brief This is tuya tal_mtd_spi file
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
#include "tal_mtd_spi.h"
#include "tkl_init_common.h"
#include "tkl_memory.h"
#include "tkl_spi.h"

/***********************************************************************
 ** CONSTANT ( MACRO AND ENUM )                                       **
 **********************************************************************/
#define MTD_CHECK_WORD_ALIGN (0) // check word align.
#define MTD_WORD_SIZE (4)        // unit: byte.
#define MTD_ID_LEN (4)

#define ERASE_SECTOR (1)
#define ERASE_BLOCK (0)
/***********************************************************************
 ** STRUCT                                                            **
 **********************************************************************/

/***********************************************************************
 ** VARIABLE                                                          **
 **********************************************************************/

/***********************************************************************
 ** FUNCTION                                                           **
 **********************************************************************/

static void spi_flash_send_command(MTD_SPI_HANDLE handle, uint8_t cmd)
{
    uint8_t ucmd[] = {0x00, 0x00, 0x00, 0x00};
    MTD_SPI_CFG_T spi_cfg = {0};
    ucmd[0] = cmd;

    tkl_spi_transfer_with_length(handle->port, &ucmd, sizeof(ucmd), NULL, 0);
    spi_cfg.port = handle->port;
    handle->dev.ops.wait(&spi_cfg);
}

OPERATE_RET tal_mtd_spi_page_read(MTD_SPI_HANDLE handle, uint32_t addr,
                                  void *data, uint32_t size, BOOL_T is_nand)
{
    OPERATE_RET ret = OPRT_COM_ERROR;
    uint8_t ucmd[] = {0x00, 0x00, 0x00, 0x00};

    if (is_nand) {
        ucmd[0] = handle->dev.cmd_set.spi_read_cache;
        ucmd[1] = ((addr >> 16) & 0xff);
        ucmd[2] = ((addr >> 8) & 0xff);
        ucmd[3] = (addr & 0xff);
        // 执行读取1页到缓存，后续从缓存读取
        ret = tkl_spi_transfer_with_length(handle->port, ucmd, sizeof(ucmd),
                                           data, size);
        if (ret != 0) {
            return OPRT_COM_ERROR;
        }
    }
    ucmd[0] = handle->dev.cmd_set.spi_read_data;
    ucmd[1] = ((addr >> 16) & 0xff);
    ucmd[2] = ((addr >> 8) & 0xff);
    ucmd[3] = (addr & 0xff);
    ret = tkl_spi_transfer_with_length(handle->port, ucmd, sizeof(ucmd), data,
                                       size);
    return ret;
}

OPERATE_RET tal_mtd_spi_page_program(MTD_SPI_HANDLE handle, uint32_t addr,
                                     const void *data, uint32_t size,
                                     BOOL_T is_nand)
{
    OPERATE_RET ret = OPRT_COM_ERROR;
    uint8_t *ucmd = tkl_system_malloc(size + 4);
    if (!ucmd)
        return 1;

    memset(ucmd, 0, size + 4);

    ucmd[0] = handle->dev.cmd_set.spi_program_cache;
    ucmd[1] = ((addr >> 16) & 0xff);
    ucmd[2] = ((addr >> 8) & 0xff);
    ucmd[3] = (addr & 0xff);
    memcpy(&ucmd[4], data, size);

    ret = tkl_spi_transfer_with_length(handle->port, ucmd, size + 4, NULL, 0);
    if (is_nand) {
        spi_flash_send_command(handle, handle->dev.cmd_set.write_enable);
        ucmd[0] = handle->dev.cmd_set.spi_page_program;
        ucmd[1] = ((addr >> 16) & 0xff);
        ucmd[2] = ((addr >> 8) & 0xff);
        ucmd[3] = (addr & 0xff);
        ret =
            tkl_spi_transfer_with_length(handle->port, ucmd, size + 4, NULL, 0);
        if (ret != 0) {
            return OPRT_COM_ERROR;
        }
    }
    tkl_system_free(ucmd);
    return ret;
}

OPERATE_RET tal_mtd_spi_erase(MTD_SPI_HANDLE handle, uint32_t addr,
                              uint32_t erase_type)
{
    OPERATE_RET ret = OPRT_COM_ERROR;
    uint8_t ucmd[] = {0x00, 0x00, 0x00, 0x00};
    uint32_t send_len;
    if (erase_type == ERASE_SECTOR) {
        ucmd[0] = handle->dev.cmd_set.sector_erase;
    } else {
        ucmd[0] = handle->dev.cmd_set.block_erase;
    }
    ucmd[1] = ((addr >> 16) & 0xff);
    ucmd[2] = ((addr >> 8) & 0xff);
    ucmd[3] = (addr & 0xff);
    send_len = sizeof(ucmd);

    spi_flash_send_command(handle, handle->dev.cmd_set.write_enable);
    ret = tkl_spi_transfer_with_length(handle->port, ucmd, send_len, NULL, 0);

    return ret;
}

OPERATE_RET tal_mtd_spi_read_id(MTD_SPI_HANDLE handle, uint32_t *reg_id)
{
    OPERATE_RET ret = OPRT_COM_ERROR;
    uint8_t ucmd[] = {0x00, 0x00, 0x00, 0x00};
    uint8_t uid_buf[MTD_ID_LEN] = {0};

    ucmd[0] = handle->dev.cmd_set.read_id;

    ret = tkl_spi_transfer_with_length(handle->port, ucmd, sizeof(ucmd),
                                       uid_buf, MTD_ID_LEN);

    *reg_id = (uid_buf[0] << 16) | (uid_buf[1] << 8) | (uid_buf[2]);
    return ret;
}

MTD_SPI_HANDLE tal_mtd_spi_init(MTD_SPI_DEV_T *dev, MTD_SPI_CFG_T *cfg)
{
    OPERATE_RET ret = OPRT_COM_ERROR;
    MTD_SPI_HANDLE handle = NULL;

    // 接口初始化应当在具体的flash初始化实现
    ret = tkl_spi_init(cfg->port, &dev->spi);
    if (OPRT_OK != ret) {
        return NULL;
    }
    handle = (MTD_SPI_HANDLE)tkl_system_malloc(sizeof(MTD_SPI_HANDLE_T));
    if (handle == NULL) {
        return NULL;
    }
    handle->port = cfg->port;
    memcpy(&handle->dev, dev, sizeof(MTD_SPI_DEV_T));
    return handle;
}

OPERATE_RET tal_mtd_spi_deinit(MTD_SPI_HANDLE handle)
{
    int32_t ret = OPRT_OK;
    ret = tkl_spi_deinit(handle->port);
    if (OPRT_OK != ret) {
        ret = OPRT_COM_ERROR;
    }
    tkl_system_free(handle);
    handle = NULL;
    return ret;
}

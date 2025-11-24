/**
 * @file tal_mtd_qspi_qspi.c
 * @brief This is tuya tal_mtd_qspi_qspi file
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
#include "tal_mtd_qspi.h"
#include "tkl_init_common.h"
#include "tkl_memory.h"
#include "tkl_qspi.h"

/***********************************************************************
 ** CONSTANT ( MACRO AND ENUM )                                       **
 **********************************************************************/
#define MTD_ID_LEN (3)
#define ERASE_QSECTOR (1)
#define ERASE_QBLOCK (0)

uint32_t swap_24(uint32_t value) {
    return ((value & 0xFF0000) >> 16) | 
           (value & 0x00FF00)         |
           ((value & 0x0000FF) << 16);
}

uint16_t swap_16(uint16_t value) {
    return ((value & 0xFF00) >> 8) | ((value & 0x00FF) << 8);
}
/***********************************************************************
 ** STRUCT                                                            **
 **********************************************************************/
typedef struct {
    UINT8_T *data;
    UINT_T addr;
    UINT_T addr_size;
    UINT_T addr_lines;
    UINT_T cmd;
    UINT_T dummy;
    UINT_T lines;
} TUYA_QSPI_MTD_CMD_T;

/***********************************************************************
 ** VARIABLE                                                          **
 **********************************************************************/

/***********************************************************************
 ** FUNCTION                                                           **
 **********************************************************************/

OPERATE_RET tal_mtd_qspi_write_reg(MTD_QSPI_HANDLE handle, UINT_T addr,
                                   UINT_T size, TUYA_QSPI_MTD_CMD_T *temp_cmd)
{
    OPERATE_RET ret = OPRT_COM_ERROR;
    TUYA_QSPI_CMD_T reg_cmd = {0};
    MTD_QSPI_CFG_T qspi_cfg = {0};

    reg_cmd.op = TUYA_QSPI_WRITE;
    reg_cmd.cmd[0] = temp_cmd->cmd;
    reg_cmd.cmd_size = 1;
    reg_cmd.cmd_lines = TUYA_QSPI_1WIRE;
    if (temp_cmd->addr_size == 2)
    {
        addr = swap_16(addr);
    }
    else
    {
        addr = swap_24(addr);
    }
    memcpy(reg_cmd.addr, &addr, sizeof(UINT_T));
    reg_cmd.addr_size = temp_cmd->addr_size;
    reg_cmd.addr_lines = temp_cmd->addr_lines;
    reg_cmd.data = temp_cmd->data;
    reg_cmd.data_size = size;
    reg_cmd.data_lines = temp_cmd->lines;
    ret = tkl_qspi_comand(handle->port, &reg_cmd);
    if (ret != 0) {
        return OPRT_COM_ERROR;
    }
    qspi_cfg.port = handle->port;
    handle->dev.ops.wait(&qspi_cfg);
    return ret;
}

OPERATE_RET tal_mtd_qspi_read_reg(MTD_QSPI_HANDLE handle, UINT_T addr,
                                  UINT8_T *buf, UINT_T size,
                                  TUYA_QSPI_MTD_CMD_T *temp_cmd)
{
    OPERATE_RET ret = OPRT_COM_ERROR;
    TUYA_QSPI_CMD_T reg_cmd = {0};

    reg_cmd.op = TUYA_QSPI_READ;
    reg_cmd.cmd[0] = temp_cmd->cmd;
    reg_cmd.cmd_size = 1;
    reg_cmd.cmd_lines = TUYA_QSPI_1WIRE;
    if (temp_cmd->addr_size == 2)
    {
        addr = swap_16(addr);
    }
    else
    {
        addr = swap_24(addr);
    }
    
    memcpy(reg_cmd.addr, &addr, sizeof(UINT_T));
    reg_cmd.addr_size = temp_cmd->addr_size;
    reg_cmd.data_size = size;
    reg_cmd.addr_lines = temp_cmd->addr_lines;
    reg_cmd.data_lines = temp_cmd->lines;
    reg_cmd.dummy_cycle = temp_cmd->dummy;
    reg_cmd.data = buf;
    // 高速传输时flash需要dummy准备数据

    ret = tkl_qspi_comand(handle->port, &reg_cmd);
    if (ret != 0) {
        return OPRT_COM_ERROR;
    }
    // ret = tkl_qspi_recv(handle->port, buf, size);
    // if (ret != 0) {
    //     return OPRT_COM_ERROR;
    // }

    return ret;
}

static VOID_T tal_mtd_qspi_write_enable(MTD_QSPI_HANDLE handle)
{
    TUYA_QSPI_MTD_CMD_T temp_cmd;
    temp_cmd.addr_size = handle->dev.cmd_set.write_enable.addr_size;
    temp_cmd.cmd = handle->dev.cmd_set.write_enable.command;
    temp_cmd.dummy = handle->dev.cmd_set.write_enable.dummy;
    temp_cmd.lines = handle->dev.cmd_set.write_enable.wire_lines;
    temp_cmd.addr_lines = handle->dev.cmd_set.write_enable.addr_lines;
    tal_mtd_qspi_write_reg(handle, 0, 0, &temp_cmd);
}

static VOID_T tal_mtd_qspi_write_disable(MTD_QSPI_HANDLE handle)
{
    TUYA_QSPI_MTD_CMD_T temp_cmd;
    temp_cmd.addr_size = handle->dev.cmd_set.write_disable.addr_size;
    temp_cmd.cmd = handle->dev.cmd_set.write_disable.command;
    temp_cmd.dummy = handle->dev.cmd_set.write_disable.dummy;
    temp_cmd.addr_lines = handle->dev.cmd_set.write_disable.addr_lines;
    temp_cmd.lines = handle->dev.cmd_set.write_disable.wire_lines;
    tal_mtd_qspi_write_reg(handle, 0, 0, &temp_cmd);
}

OPERATE_RET tal_mtd_qspi_read_id(MTD_QSPI_HANDLE handle, UINT_T *reg_id)
{
    OPERATE_RET ret = OPRT_COM_ERROR;
    TUYA_QSPI_MTD_CMD_T temp_cmd;
    temp_cmd.addr_size = handle->dev.cmd_set.read_id.addr_size;
    temp_cmd.cmd = handle->dev.cmd_set.read_id.command;
    temp_cmd.dummy = handle->dev.cmd_set.read_id.dummy;
    temp_cmd.lines = handle->dev.cmd_set.read_id.wire_lines;
    temp_cmd.addr_lines = handle->dev.cmd_set.read_id.addr_lines;
    ret = tal_mtd_qspi_read_reg(handle, 0, (UINT8_T *)reg_id, MTD_ID_LEN,
                                &temp_cmd);
    if (ret != 0) {
        return OPRT_COM_ERROR;
    }
    return ret;
}

OPERATE_RET tal_mtd_qspi_page_program(MTD_QSPI_HANDLE handle, UINT_T addr,
                                      const VOID_T *data, UINT_T size,
                                      BOOL_T is_nand)
{
    OPERATE_RET ret = OPRT_COM_ERROR;
    TUYA_QSPI_MTD_CMD_T temp_cmd;
    UINT_T data_left = size;
    UINT_T transfer_len = 0;
    UINT_T data_off = 0U;
    UINT_T target_addr = addr;
    // nand flash需要先发送到缓存，存满1页再发起烧录执行命令

    for (size_t i = 0; i < ((size + (QSPI_FIFO_SIZE - 1)) / QSPI_FIFO_SIZE);
         i++) {
        transfer_len =
            (data_left > QSPI_FIFO_SIZE) ? QSPI_FIFO_SIZE : data_left;
        data_off = i * QSPI_FIFO_SIZE;
        if (is_nand) {
            // NAND: 地址 = 行地址 << col_bits | 列地址（页内偏移）
            // 注意：此处列地址为data_off，行地址已在首次命令中发送
            target_addr = data_off; // 仅发送列地址
            // tkl_qspi_send(handle->port, data + data_off, QSPI_FIFO_SIZE);
            temp_cmd.addr_size =
                handle->dev.cmd_set.quad_program_cache.addr_size;
            temp_cmd.cmd = handle->dev.cmd_set.quad_program_cache.command;
            temp_cmd.dummy = handle->dev.cmd_set.quad_program_cache.dummy;
            temp_cmd.lines = handle->dev.cmd_set.quad_program_cache.wire_lines;
            temp_cmd.addr_lines = handle->dev.cmd_set.quad_program_cache.addr_lines;
            temp_cmd.data = data + data_off;
            ret = tal_mtd_qspi_write_reg(handle, target_addr, transfer_len,
                                         &temp_cmd);
            if (ret != 0) {
                return OPRT_COM_ERROR;
            }
        } else {
            // NOR: 直接地址累加（addr + 偏移）
            target_addr = addr + data_off;
            // tkl_qspi_send(handle->port, data + data_off, QSPI_FIFO_SIZE);
            tal_mtd_qspi_write_enable(handle);
            temp_cmd.addr_size =
                handle->dev.cmd_set.quad_page_program.addr_size;
            temp_cmd.cmd = handle->dev.cmd_set.quad_page_program.command;
            temp_cmd.dummy = handle->dev.cmd_set.quad_page_program.dummy;
            temp_cmd.lines = handle->dev.cmd_set.quad_page_program.wire_lines;
            temp_cmd.addr_lines = handle->dev.cmd_set.quad_page_program.addr_lines;
            temp_cmd.data = data + data_off;
            ret = tal_mtd_qspi_write_reg(handle, target_addr, transfer_len,
                                         &temp_cmd);
            if (ret != 0) {
                return OPRT_COM_ERROR;
            }
        }
        data_left = data_left - transfer_len;
    }
    if (is_nand) {
        tal_mtd_qspi_write_enable(handle);
        temp_cmd.addr_size = handle->dev.cmd_set.quad_page_program.addr_size;
        temp_cmd.cmd = handle->dev.cmd_set.quad_page_program.command;
        temp_cmd.dummy = handle->dev.cmd_set.quad_page_program.dummy;
        temp_cmd.lines = handle->dev.cmd_set.quad_page_program.wire_lines;
        temp_cmd.addr_lines = handle->dev.cmd_set.quad_page_program.addr_lines;
        ret = tal_mtd_qspi_write_reg(handle, addr, 0, &temp_cmd);
        if (ret != 0) {
            return OPRT_COM_ERROR;
        }
    }
    return OPRT_OK;
}

OPERATE_RET tal_mtd_qspi_page_read(MTD_QSPI_HANDLE handle, UINT_T addr,
                                   VOID_T *data, UINT_T size, BOOL_T is_nand)
{
    OPERATE_RET ret = OPRT_COM_ERROR;
    TUYA_QSPI_MTD_CMD_T temp_cmd;
    UINT_T data_left = size;
    UINT_T transfer_len = 0;
    UINT_T data_off = 0U;
    UINT_T target_addr;
    if (is_nand) {
        temp_cmd.addr_size = handle->dev.cmd_set.quad_read_cache.addr_size;
        temp_cmd.cmd = handle->dev.cmd_set.quad_read_cache.command;
        temp_cmd.dummy = handle->dev.cmd_set.quad_read_cache.dummy;
        temp_cmd.lines = handle->dev.cmd_set.quad_read_cache.wire_lines;
        temp_cmd.addr_lines = handle->dev.cmd_set.quad_read_cache.addr_lines;
        // 最第一次执行读取1页到缓存，后续按照fifo从缓存分次读取
        ret = tal_mtd_qspi_write_reg(handle, addr, 0, &temp_cmd);
        if (ret != 0) {
            return OPRT_COM_ERROR;
        }
    }
    for (size_t i = 0; i < ((size + (QSPI_FIFO_SIZE - 1)) / QSPI_FIFO_SIZE);
         i++) {
        transfer_len =
            (data_left > QSPI_FIFO_SIZE) ? QSPI_FIFO_SIZE : data_left;
        data_off = i * QSPI_FIFO_SIZE;
        if (is_nand) {
            // NAND: 地址 = 行地址 << col_bits | 列地址（页内偏移）
            // 注意：此处列地址为data_off，行地址已在首次命令中发送
            target_addr = data_off; // 仅发送列地址
        } else {
            // NOR: 直接地址累加（addr + 偏移）
            target_addr = addr + data_off;
        }

        temp_cmd.addr_size = handle->dev.cmd_set.quad_read_data.addr_size;
        temp_cmd.cmd = handle->dev.cmd_set.quad_read_data.command;
        temp_cmd.dummy = handle->dev.cmd_set.quad_read_data.dummy;
        temp_cmd.lines = handle->dev.cmd_set.quad_read_data.wire_lines;
        temp_cmd.addr_lines = handle->dev.cmd_set.quad_read_data.addr_lines;
        ret = tal_mtd_qspi_read_reg(handle, target_addr, data + data_off,
                                    transfer_len, &temp_cmd);
        if (ret != 0) {
            return OPRT_COM_ERROR;
        }
        data_left = data_left - transfer_len;
    }
    return OPRT_OK;
}

OPERATE_RET tal_mtd_qspi_erase(MTD_QSPI_HANDLE handle, UINT_T addr,
                               UINT_T erase_type)
{
    OPERATE_RET ret = OPRT_COM_ERROR;
    TUYA_QSPI_MTD_CMD_T temp_cmd;
    MTD_QSPI_CFG_T qspi_cfg = {0};

    tal_mtd_qspi_write_enable(handle);
    if (erase_type == ERASE_QSECTOR) {
        temp_cmd.addr_size = handle->dev.cmd_set.sector_erase.addr_size;
        temp_cmd.cmd = handle->dev.cmd_set.sector_erase.command;
        temp_cmd.dummy = handle->dev.cmd_set.sector_erase.dummy;
        temp_cmd.lines = handle->dev.cmd_set.sector_erase.wire_lines;
        temp_cmd.addr_lines = handle->dev.cmd_set.sector_erase.addr_lines;
        ret = tal_mtd_qspi_write_reg(handle, addr, 0, &temp_cmd);
    } else {
        temp_cmd.addr_size = handle->dev.cmd_set.block_erase.addr_size;
        temp_cmd.cmd = handle->dev.cmd_set.block_erase.command;
        temp_cmd.dummy = handle->dev.cmd_set.block_erase.dummy;
        temp_cmd.lines = handle->dev.cmd_set.block_erase.wire_lines;
        temp_cmd.addr_lines = handle->dev.cmd_set.block_erase.addr_lines;
        ret = tal_mtd_qspi_write_reg(handle, addr, 0, &temp_cmd);
    }

    qspi_cfg.port = handle->port;
    handle->dev.ops.wait(&qspi_cfg);

    return ret;
}

MTD_QSPI_HANDLE tal_mtd_qspi_init(MTD_QSPI_DEV_T *dev, MTD_QSPI_CFG_T *cfg)
{
    OPERATE_RET ret = OPRT_COM_ERROR;
    MTD_QSPI_HANDLE handle = NULL;

    // 接口初始化应当在具体的flash初始化实现
    ret = tkl_qspi_init(cfg->port, &dev->qspi);
    if (OPRT_OK != ret) {
        return NULL;
    }
    if (dev->ops.init && dev->ops.unlock) {
        ret = dev->ops.init(cfg);
        if (OPRT_OK != ret) {
            return NULL;
        }
        ret = dev->ops.unlock(cfg);
        if (OPRT_OK != ret) {
            return NULL;
        }
    } else {
        return NULL;
    }
    handle = (MTD_QSPI_HANDLE)tkl_system_malloc(sizeof(MTD_QSPI_HANDLE_T));
    if (handle == NULL) {
        return NULL;
    }
    handle->port = cfg->port;
    memcpy(&handle->dev, dev, sizeof(MTD_QSPI_DEV_T));
    return handle;
}

OPERATE_RET tal_mtd_qspi_deinit(MTD_QSPI_HANDLE handle)
{
    OPERATE_RET ret = OPRT_OK;
    MTD_QSPI_CFG_T qspi_cfg = {0};

    if (handle->dev.ops.deinit) {
        qspi_cfg.port = handle->port;
        ret = handle->dev.ops.deinit(&qspi_cfg);
        if (OPRT_OK != ret) {
            ret = OPRT_COM_ERROR;
        }
    }
    ret = tkl_qspi_deinit(handle->port);

    tkl_system_free(handle);
    handle = NULL;
    return ret;
}

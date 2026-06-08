
#include "tal_mtd_service.h"
#include "tal_mtd_qspi.h"
#include "tkl_system.h"
#include "tkl_qspi.h"


/* Configuration, Status, Erase, Program Commands ***************************
 *      Command                  Value    Description:                      *
 *                                          Data sequence                   *
 */
#define W25Q_READ_STATUS_1          0x05 /* SRP|SEC|TB |BP2|BP1|BP0|WEL|BUSY       */
#define W25Q_READ_STATUS_2          0x35 /* SUS|CMP|LB3|LB2|LB1|(R)|QE |SRL        */
#define W25Q_WRITE_STATUS_1         0x01 /* SRP|SEC|TB |BP2|BP1|BP0|WEL|BUSY       */
#define W25Q_WRITE_STATUS_2         0x31 /* SUS|CMP|LB3|LB2|LB1|(R)|QE |SRL        */
#define W25Q_WRITE_ENABLE           0x06 /* Write enable                           */
#define W25Q_WRITE_DISABLE          0x04 /* Write disable                          */
#define W25Q_PAGE_PROGRAM           0x02 /* Page Program:                          *
                                        *  0x02 | A23-A16 | A15-A8 | A7-A0 | data */
#define W25Q_QUAD_PAGE_PROGRAM      0x32 /* Quad Page Program:                          *
                                        *  0x32 | A23-A16 | A15-A8 | A7-A0 | data */
#define W25Q_SECTOR_ERASE           0x20 /* Sector Erase (4 kB)                    *
                                               *  0x20 | A23-A16 | A15-A8 | A7-A0       */
#define W25Q_BLOCK_ERASE_32K        0x52 /* Block Erase (32 KB)                    *
                                               *  0x52 | A23-A16 | A15-A8 | A7-A0       */
#define W25Q_BLOCK_ERASE_64K        0xd8 /* Block Erase (64 KB)                    *
                                               *  0xd8 | A23-A16 | A15-A8 | A7-A0       */
#define W25Q_CHIP_ERASE             0x60 /* Chip Erase:                            *
                                               *  0xc7 or 0x60                          */

/* Read Commands ************************************************************
 *      Command                        Value   Description:                 *
 *                                               Data sequence              *
 */
#define W25Q_PAGE_READ              0x03  /* Read:        *
                                               *   0x03 | A23-A16 | A15-A8 | A7-A0 | data...    */
#define W25Q_QUAD_PAGE_READ         0x6b  /* Fast Read Quad I/O:        *
                                               *   0xeb | ADDR | data...    */
/* ID/Security Commands *****************************************************
 *      Command                  Value    Description:                      *
 *                                            Data sequence                 *
 */
#define W25Q_JEDEC_ID               0x9f  /* JEDEC ID:                        *
                                                 * 0x9f | Manufacturer |            *
                                                 * MemoryType | Capacity            */
/* W25QXXXJV Registers ******************************************************/
/* Status register 1 bit definitions                                      */
#define STATUS_BUSY_MASK            (1 << 0) /* Bit 0: Device ready/busy status  */
#define STATUS_READY                (0 << 0) /*   0 = Not Busy                   */
#define STATUS_BP_SHIFT             (2)      /* Bits 2-4: Block protect bits     */
#define STATUS_BP_MASK              (7 << STATUS_BP_SHIFT)
#define STATUS_SEC_64KB             (0 << 6) /*   0 = Protect 64KB Blocks        */
#define STATUS_SEC_4KB              (1 << 6) /*   1 = Protect 4KB Sectors        */
/* Status register 2 bit definitions                                      */
#define STATUS2_QE_MASK             (1 << 1) /* Bit 1: Quad Enable (QE)          */
#define STATUS2_QE_DISABLED         (0 << 1) /*  0 = Standard/Dual SPI modes     */
#define STATUS2_QE_ENABLED          (1 << 1) /*  1 = Standard/Dual/Quad modes    */

#define QSPI_FIFO_LEN_MAX           256

#define DELAY_CYCLE    (5)



static OPERATE_RET flash_w25q32_read_status(MTD_QSPI_CFG_T *cfg, UINT_T cmd)
{
    OPERATE_RET ret = OPRT_OK;
    UINT8_T status;
    TUYA_QSPI_CMD_T reg_cmd = {0};

    reg_cmd.op = TUYA_QSPI_READ;
    reg_cmd.cmd = cmd;
    reg_cmd.addr = 0;
    reg_cmd.addr_valid_bit = 0;
    reg_cmd.data_len = sizeof(UINT8_T);
    reg_cmd.addr_lines = TUYA_QSPI_1WIRE;
    reg_cmd.data_lines = TUYA_QSPI_1WIRE;
    reg_cmd.dummy_cycle = 0;
    //高速传输时flash需要dummy准备数据

    ret = tkl_qspi_comand(cfg->port, &reg_cmd);
    if (ret != 0)
    {
        return OPRT_COM_ERROR;
    }
    ret = tkl_qspi_recv(cfg->port, &status, sizeof(UINT8_T));
    if (ret != 0)
    {
        return OPRT_COM_ERROR;
    }

    return status;
}

static VOID_T flash_w25q32_wait_done(MTD_QSPI_CFG_T *cfg)
{
    UINT_T status_reg_data = 0;

    for(int i = 0; i <= (2000 / DELAY_CYCLE); i++) {
        status_reg_data = flash_w25q32_read_status(cfg, W25Q_READ_STATUS_1);
        if(STATUS_READY == (status_reg_data & STATUS_BUSY_MASK)) {
            break;
        }
        tkl_system_sleep(DELAY_CYCLE);
    }
}

static OPERATE_RET flash_w25q32_write_status(MTD_QSPI_CFG_T *cfg, UINT_T reg, UINT8_T status_reg_data)
{

    OPERATE_RET ret = OPRT_COM_ERROR;
    TUYA_QSPI_CMD_T reg_cmd = {0};

    reg_cmd.op = TUYA_QSPI_WRITE;
    reg_cmd.cmd = ((status_reg_data << 8) | reg);
    reg_cmd.cmd_lines = TUYA_QSPI_1WIRE;
    reg_cmd.addr = 0;
    reg_cmd.addr_valid_bit = 3;
    reg_cmd.addr_lines = TUYA_QSPI_1WIRE;
    reg_cmd.data_len = sizeof(UINT8_T);
    reg_cmd.data_lines = TUYA_QSPI_1WIRE;
    ret = tkl_qspi_comand(cfg->port, &reg_cmd);
    if (ret != 0)
    {
        return OPRT_COM_ERROR;
    }
    flash_w25q32_wait_done(cfg);
    return OPRT_OK;
}

static INT32_T flash_w25q32_nor_set_protect_none(MTD_QSPI_CFG_T *cfg)
{
    UINT8_T status_reg_data = 0;

    status_reg_data = flash_w25q32_read_status(cfg, W25Q_READ_STATUS_1) & 0xff;
    UINT8_T clean_bits = ~STATUS_BP_MASK;
    status_reg_data &= clean_bits;
    flash_w25q32_write_status(cfg, W25Q_WRITE_STATUS_1, status_reg_data);
    return OPRT_OK;
}


static INT32_T flash_w25q32_init(MTD_QSPI_CFG_T *cfg)
{
    //接口初始化应当在具体的flash初始化实现
    INT32_T ret = OPRT_OK;
    UINT_T status_reg_data = 0;

    status_reg_data = (UINT8_T)flash_w25q32_read_status(cfg, W25Q_READ_STATUS_2);
    if ((status_reg_data & STATUS2_QE_MASK) == STATUS2_QE_DISABLED) {
        status_reg_data |= STATUS2_QE_ENABLED;
        return flash_w25q32_write_status(cfg, W25Q_WRITE_STATUS_2, status_reg_data);
    }

    return ret;
}
static INT32_T flash_w25q32_deinit(MTD_QSPI_CFG_T *cfg)
{
    //接口初始化应当在具体的flash初始化实现
    INT32_T ret = OPRT_OK;
    UINT_T status_reg_data = 0;

    status_reg_data = (UINT8_T)flash_w25q32_read_status(cfg, W25Q_READ_STATUS_2);
    if ((status_reg_data & STATUS2_QE_MASK) != STATUS2_QE_DISABLED) {
        status_reg_data &= ~STATUS2_QE_ENABLED;
        return flash_w25q32_write_status(cfg, W25Q_WRITE_STATUS_2, status_reg_data);
    }
    return ret;
}

// 初始化Flash设备
MTD_DEVICE_T w25q32flash_cfg = {
    .name = "w25q32",
    .type = MTD_NOR,
    .nor_dev = {
    	.page_size = 256,
        .sector_size = 4096,
    	.block_size = 32 * 1024,
        .total_size = 4 * 1024 * 1024,
        .interface = MTD_IF_QSPI,
        .qspi_dev = {
            .cmd_set = {
                .read_id = {
                    .command = W25Q_JEDEC_ID,        // 读ID指令 (e.g., 0x9F)
                    .addr_size = 0,
                    .wire_lines = TUYA_QSPI_1WIRE,
                    .dummy = 8,
                },
                .quad_read_data = {
                    .command = W25Q_QUAD_PAGE_READ,      // 读数据指令 (e.g., 0x03)
                    .addr_size = 3,
                    .wire_lines = TUYA_QSPI_4WIRE,
                    .dummy = 8,
                },
                .quad_read_cache = {
                    .command = 0,
                    .addr_size = 0,
                    .wire_lines = TUYA_QSPI_1WIRE,
                    .dummy = 8,
                },
                .quad_program_cache = {
                    .command = 0,
                    .addr_size = 0,
                    .wire_lines = TUYA_QSPI_1WIRE,
                    .dummy = 0,
                },
                .quad_page_program = {
                    .command = W25Q_QUAD_PAGE_PROGRAM,
                    .addr_size = 3,
                    .wire_lines = TUYA_QSPI_4WIRE,
                    .dummy = 8,
                },
                .sector_erase = {
                    .command = W25Q_SECTOR_ERASE,
                    .addr_size = 3,
                    .wire_lines = TUYA_QSPI_1WIRE,
                    .dummy = 8,
                },
                .block_erase = {
                    .command = W25Q_BLOCK_ERASE_64K,
                    .addr_size = 3,
                    .wire_lines = TUYA_QSPI_1WIRE,
                    .dummy = 8,
                },
                .write_enable = {
                    .command = W25Q_WRITE_ENABLE,
                    .addr_size = 1,
                    .wire_lines = TUYA_QSPI_1WIRE,
                    .dummy = 8,
                },
                .write_disable = {
                    .command = W25Q_WRITE_DISABLE,
                    .addr_size = 1,
                    .wire_lines = TUYA_QSPI_1WIRE,
                    .dummy = 8,
                },
            },
            .qspi = {
                .role = TUYA_QSPI_ROLE_MASTER,
                .mode = TUYA_QSPI_MODE0,
                .baudrate = 104000000,
                .is_dma = 0,
            },
            .ops = {
                .init = flash_w25q32_init,
                .wait = flash_w25q32_wait_done,
                .unlock = flash_w25q32_nor_set_protect_none,
                .deinit = flash_w25q32_deinit,
            }
        }
    }
};
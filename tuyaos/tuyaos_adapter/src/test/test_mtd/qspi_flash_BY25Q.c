
#include "tal_mtd_service.h"
#include "tal_mtd_qspi.h"
#include "tkl_system.h"
#include "tkl_qspi.h"

#define QSPI_QUAD_ENABLE    1

/* BY25 Instructions *****************************************************************/

/*      Command                         Value     Description       Addr  Dummy     Data    */
#define BY25Q_ENTER_4_BYTE           0xb7
#define BY25Q_EXIT_4_BYTE            0xe9
#define BY25_WRITE_ENABLE               0x06 /*                     0       0       0     */
#define BY25_WRITE_DISABLE              0x04 /*                     0       0       0     */
#define BY25Q_READ_STATUS_1          0x05 /* SRP|SEC|TB |BP2|BP1|BP0|WEL|BUSY       */
#define BY25Q_READ_STATUS_2          0x35 /* SUS|CMP|LB3|LB2|LB1|(R)|QE |SRL        */
#define BY25Q_READ_STATUS_3          0x15 /* HOLD/RST|DRV1|DRV0|(R)|(R)|WPS|(R)|(R) */

#define BY25Q_WRITE_STATUS_1         0x01 /* SRP|SEC|TB |BP2|BP1|BP0|WEL|BUSY       */
#define BY25Q_WRITE_STATUS_2         0x31 /* SUS|CMP|LB3|LB2|LB1|(R)|QE |SRL        */
#define BY25Q_WRITE_STATUS_3         0x11 /* HOLD/RST|DRV1|DRV0|(R)|(R)|WPS|(R)|(R) */

#define BY25_PAGE_READ                  0x6c /* Array read          3       0       0     */

#define BY25_READ_ID                    0x9F /* Read device ID      0       1       2      */

#define BY25_QUAD_PROGRAM               0x34 /* Load program data
                                              * without cache reset 2       0       1-2112 */

#define BY25_SECTOR_ERASE               0x21 /* Sector erase         3       0       0     */

/* Feature register ******************************************************************/

/* JEDEC Read ID register values */
#define BY25_BLOCK_SHIFT            12    /* 4096 byte */
#define BY25_PAGE_SHIFT             8    /* 256 */
#define BY25_PAGE_SIZE              (1 << BY25_PAGE_SHIFT)
#define BY25_BLOCK_SIZE             (1 << BY25_BLOCK_SHIFT)
#define BY25_SECTOR_COUNT         (8192)
/* Register address */

/* Bit definitions */

/* Secure OTP (On-Time-Programmable) register */

#define BY25_SOTP_QE                (1 << 1)  /* Bit 1: Quad Enable */

/* Status register */

#define BY25_SR_OIP                 (1 << 0)  /* Bit 0: Operation in progress */

#define QSPI_FIFO_LEN_MAX          256
#define FLASH_PROTECT_NONE_DATA    0
#define DELAY_CYCLE    (1)

static OPERATE_RET flash_by25q_read_status(MTD_QSPI_CFG_T *cfg, UINT_T cmd, UINT_T addr, UINT_T addr_len)
{
    OPERATE_RET ret = OPRT_OK;
    UINT_T status = 0;
    tkl_qspi_force_cs_pin(cfg->port, 0);
    TUYA_QSPI_CMD_T reg_cmd = {0};

    reg_cmd.op = TUYA_QSPI_READ;
    reg_cmd.cmd[0] = cmd;
    reg_cmd.cmd_size = 1;
    reg_cmd.cmd_lines = TUYA_QSPI_1WIRE;

    reg_cmd.addr_size = 0;
    reg_cmd.addr_lines = TUYA_QSPI_1WIRE;
    reg_cmd.data_size = sizeof(UINT8_T);
    reg_cmd.data_lines = TUYA_QSPI_1WIRE;
    reg_cmd.dummy_cycle = 0;
    reg_cmd.data = &status;
    //????????flash???dummy???????

    ret = tkl_qspi_comand(cfg->port, &reg_cmd);
    if (ret != 0)
    {
        return OPRT_COM_ERROR;
    }
    tkl_qspi_force_cs_pin(cfg->port, 1);

    return status & 0xff;
}

static VOID_T flash_by25q_wait_done(MTD_QSPI_CFG_T *cfg)
{
    UINT_T status_reg_data = 0;

    for(int i = 0; i <= 20000; i++) {
        status_reg_data = flash_by25q_read_status(cfg, BY25Q_READ_STATUS_1, 0, 0);
        if(0 == (status_reg_data & BY25_SR_OIP)) {
            break;
        }
        tkl_system_sleep(DELAY_CYCLE);
    }
}
static void flash_by25q_write_enable(MTD_QSPI_CFG_T *cfg)
{

    tkl_qspi_force_cs_pin(cfg->port, 0);

    TUYA_QSPI_CMD_T cmd = {0};
    cmd.op = TUYA_QSPI_WRITE;

    cmd.cmd[0] = BY25_WRITE_ENABLE;
    cmd.cmd_size = 1;
    cmd.cmd_lines = TUYA_QSPI_1WIRE;
    
    cmd.addr_size = 0;
    cmd.addr_lines = TUYA_QSPI_1WIRE;

    cmd.data_size = 0;
    cmd.dummy_cycle = 0;
    tkl_qspi_comand(cfg->port, &cmd);

    tkl_qspi_force_cs_pin(cfg->port, 1);

    flash_by25q_wait_done(cfg);
}
static OPERATE_RET flash_by25q_write_status(MTD_QSPI_CFG_T *cfg, UINT_T cmd, UINT_T addr, UINT_T addr_len)
{
    OPERATE_RET ret = OPRT_COM_ERROR;
    flash_by25q_write_enable(cfg);
    TUYA_QSPI_CMD_T reg_cmd = {0};

    reg_cmd.op = TUYA_QSPI_WRITE;
    reg_cmd.cmd[0] = cmd;
    reg_cmd.cmd[1] = addr;
    reg_cmd.cmd_size = 2;
    reg_cmd.cmd_lines = TUYA_QSPI_1WIRE;
    reg_cmd.addr_size = 0;
    reg_cmd.addr_lines = TUYA_QSPI_1WIRE;
    reg_cmd.data_size = 0;
    reg_cmd.data_lines = TUYA_QSPI_1WIRE;
    tkl_qspi_force_cs_pin(cfg->port, 0);
    ret = tkl_qspi_comand(cfg->port, &reg_cmd);
    if (ret != 0)
    {
        return OPRT_COM_ERROR;
    }
    tkl_qspi_force_cs_pin(cfg->port, 1);
    flash_by25q_wait_done(cfg);
    return OPRT_OK;
}

static INT32_T flash_by25q_nor_set_protect_none(MTD_QSPI_CFG_T *cfg)
{
    UINT8_T status_reg_data = 0;

    bk_printf("-----------%s %d----------\n", __func__, __LINE__);
    status_reg_data = flash_by25q_read_status(cfg, BY25Q_READ_STATUS_1, 0, 0) & 0xff;
    UINT8_T clean_bits = ~(BY25_SOTP_QE | BY25_SR_OIP);
    status_reg_data &= clean_bits;
    
    flash_by25q_write_status(cfg, BY25Q_WRITE_STATUS_1, status_reg_data, 1);
    return OPRT_OK;
}


static INT32_T flash_by25q_init(MTD_QSPI_CFG_T *cfg)
{
    //????????????????flash????????
    INT32_T ret = OPRT_OK;
    UINT_T status_reg_data = 0;
    TUYA_QSPI_CMD_T cmd = {0};

    
    TUYA_GPIO_BASE_CFG_T gpio_cfg;
    gpio_cfg.direct = TUYA_GPIO_OUTPUT;
    gpio_cfg.level = TUYA_GPIO_LEVEL_LOW;
    tkl_gpio_init(TUYA_GPIO_NUM_13, &gpio_cfg);
    bk_printf("-----------%s %d----------\n", __func__, __LINE__);
    status_reg_data = (UINT8_T)flash_by25q_read_status(cfg, BY25Q_READ_STATUS_2, 0, 0);
    if ((status_reg_data & BY25_SOTP_QE) == 0) {
        status_reg_data |= BY25_SOTP_QE;
        return flash_by25q_write_status(cfg, BY25Q_WRITE_STATUS_2, status_reg_data, 1);
    }
    cmd.op = TUYA_QSPI_WRITE;

    cmd.cmd[0] = BY25Q_ENTER_4_BYTE; // enter 4 byte addr mode
    cmd.cmd_size = 1;
    cmd.cmd_lines = TUYA_QSPI_1WIRE;
    cmd.addr_size = 0;
    cmd.addr_lines = TUYA_QSPI_1WIRE;

    cmd.data_size = 0;
    cmd.dummy_cycle = 0;
    tkl_qspi_force_cs_pin(cfg->port, 0);
    tkl_qspi_comand(cfg->port, &cmd);
    tkl_qspi_force_cs_pin(cfg->port, 1);
    return ret;
}

static INT32_T flash_by25q_deinit(MTD_QSPI_CFG_T *cfg)
{
    //????????????????flash????????
    INT32_T ret = OPRT_OK;
    UINT_T status_reg_data = 0;

    status_reg_data = (UINT8_T)flash_by25q_read_status(cfg, BY25Q_READ_STATUS_2, 0, 0);
    if ((status_reg_data & BY25_SOTP_QE) == BY25_SOTP_QE) {
        status_reg_data &= ~BY25_SOTP_QE;
        return flash_by25q_write_status(cfg, BY25Q_WRITE_STATUS_2, status_reg_data, 1);
    }
    return ret;
}

// ?????Flash?��
MTD_DEVICE_T by25q_flash_cfg = {
    .name = "by25q",
    .type = MTD_NOR,
    .nor_dev = {
    	.page_size = BY25_PAGE_SIZE,
        .sector_size = 0,
    	.block_size = BY25_BLOCK_SIZE,
        .total_size = BY25_BLOCK_SIZE * BY25_SECTOR_COUNT,
        .interface = MTD_IF_QSPI,
        .qspi_dev = {
            .cmd_set = {
                .read_id = {
                    .command = BY25_READ_ID,        // ??ID??? (e.g., 0x9F)
                    .addr_size = 0,
                    .addr_lines = TUYA_QSPI_1WIRE,
                    .wire_lines = TUYA_QSPI_1WIRE,
                    .dummy = 0,
                },
                .quad_read_data = {
                    .command = BY25_PAGE_READ,      // ????????? (e.g., 0x03)
                    .addr_size = 4,
                    .addr_lines = TUYA_QSPI_1WIRE,
                    .wire_lines = TUYA_QSPI_4WIRE,
                    .dummy = 8,
                },
                .quad_page_program = {
                    .command = BY25_QUAD_PROGRAM,
                    .addr_size = 4,
                    .addr_lines = TUYA_QSPI_1WIRE,
                    .wire_lines = TUYA_QSPI_4WIRE,
                    .dummy = 0,
                },
                .sector_erase = {
                    .command = BY25_SECTOR_ERASE,
                    .addr_size = 4,
                    .addr_lines = TUYA_QSPI_1WIRE,
                    .wire_lines = TUYA_QSPI_1WIRE,
                },
                .write_enable = {
                    .command = BY25_WRITE_ENABLE,
                    .addr_size = 0,
                    .addr_lines = TUYA_QSPI_1WIRE,
                    .wire_lines = TUYA_QSPI_1WIRE,
                    .dummy = 0,
                },
                .write_disable = {
                    .command = BY25_WRITE_DISABLE,
                    .addr_size = 0,
                    .addr_lines = TUYA_QSPI_1WIRE,
                    .wire_lines = TUYA_QSPI_1WIRE,
                    .dummy = 0,
                },
            },
            .qspi = {
                .role = TUYA_QSPI_ROLE_MASTER,
                .mode = TUYA_QSPI_MODE0,
                .freq_hz = 40000000,
                .use_dma = 0,
                .dma_data_lines = TUYA_QSPI_4WIRE,
            },
            .ops = {
                .init = flash_by25q_init,
                .wait = flash_by25q_wait_done,
                .unlock = flash_by25q_nor_set_protect_none,
                .deinit = flash_by25q_deinit,
            }
        }
    }
};

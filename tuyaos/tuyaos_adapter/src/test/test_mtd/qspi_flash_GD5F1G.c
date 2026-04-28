
#include "tal_mtd_service.h"
#include "tal_mtd_qspi.h"
#include "tkl_system.h"
#include "tkl_qspi.h"

#define QSPI_QUAD_ENABLE    1

/* GD5F Instructions *****************************************************************/

/*      Command                         Value     Description       Addr  Dummy     Data    */

#define GD5F_WRITE_ENABLE               0x06 /*                     0       0       0     */
#define GD5F_WRITE_DISABLE              0x04 /*                     0       0       0     */
#define GD5F_GET_FEATURE                0x0F /* Get features        1       0       1     */
#define GD5F_SET_FEATURE                0x1F /* Set features        1       0       1     */

#define GD5F_PAGE_READ                  0x13 /* Array read          3       0       0     */
#define GD5F_READ_FROM_CACHE            0x03 /* Output cache data
                                              *  on SO                              1-2112 */
#define GD5F_QUAD_READ_FROM_CACHE       0xeB /* Output cache data   2       1
                                              *  on SIO 0/1/2/3                     1-2112 */

#define GD5F_READ_ID                    0x9F /* Read device ID      0       1       2      */

#define GD5F_PROGRAM_LOAD               0x02 /* Load program data with
                                              * cache reset first   2       0       1-2112 */
#define GD5F_QUAD_PROGRAM_LOAD          0x32 /* Load program data
                                              * without cache reset 2       0       1-2112 */
#define GD5F_QUAD_PROGRAM_LOAD_RANDOM   0xC4 /* Load program data
                                              * without cache reset 2       0       1-2112 */
#define GD5F_PROGRAM_EXECUTE            0x10 /* Enter block/page
                                              * address, execute    3   0   0     */

#define GD5F_BLOCK_ERASE                0xD8 /* Block erase         3       0       0     */

/* Feature register ******************************************************************/

/* JEDEC Read ID register values */
#define GD5F_BLOCK_SHIFT            17    /* 131072 byte */
#define GD5F_PAGE_SHIFT             11    /* 2048 */
#define GD5F_PAGE_SIZE              (1 << GD5F_PAGE_SHIFT)
#define GD5F_BLOCK_SIZE             (1 << GD5F_BLOCK_SHIFT)

/* Register address */

#define GD5F_SECURE_OTP             0xb0
#define GD5F_STATUS                 0xc0
#define GD5F_BLOCK_PROTECTION       0xa0

/* Bit definitions */

/* Secure OTP (On-Time-Programmable) register */

#define GD5F_SOTP_QE                (1 << 0)  /* Bit 0: Quad Enable */

/* Status register */

#define GD5F_SR_OIP                 (1 << 0)  /* Bit 0: Operation in progress */

/* Block Protection register */

#define GD5F_BP_BP0                 (1 << 3)  /* Bit 3: Block Protection 0 */
#define GD5F_BP_BP1                 (1 << 4)  /* Bit 4: Block Protection 1 */
#define GD5F_BP_BP2                 (1 << 5)  /* Bit 5: Block Protection 2 */

#define QSPI_FIFO_LEN_MAX          256
#define FLASH_PROTECT_NONE_DATA    0
#define DELAY_CYCLE    (5)

static uint32_t swap_24(uint32_t value) {
    return ((value & 0xFF0000) >> 16) | 
           (value & 0x00FF00)         |
           ((value & 0x0000FF) << 16);
}

static uint16_t swap_16(uint16_t value) {
    return ((value & 0xFF00) >> 8) | ((value & 0x00FF) << 8);
}

static OPERATE_RET flash_gd5f1g_read_status(MTD_QSPI_CFG_T *cfg, uint32_t cmd, uint32_t addr, uint32_t addr_len)
{
    OPERATE_RET ret = OPRT_OK;
    uint8_t status;
    TUYA_QSPI_CMD_T reg_cmd = {0};

    reg_cmd.op = TUYA_QSPI_READ;
    reg_cmd.cmd[0] = cmd;
    reg_cmd.cmd_size = 1;
    reg_cmd.cmd_lines = TUYA_QSPI_1WIRE;

    memcpy(reg_cmd.addr, &addr, sizeof(uint32_t));

    reg_cmd.addr_size = addr_len;
    reg_cmd.data_size = sizeof(uint8_t);
    reg_cmd.addr_lines = TUYA_QSPI_1WIRE;
    reg_cmd.data_lines = TUYA_QSPI_1WIRE;
    reg_cmd.dummy_cycle = 0;
    reg_cmd.data = &status;
    //????????flash???dummy???????

    ret = tkl_qspi_comand(cfg->port, &reg_cmd);
    if (ret != 0)
    {
        return OPRT_COM_ERROR;
    }
    // ret = tkl_qspi_recv(cfg->port, &status, sizeof(uint8_t));
    // if (ret != 0)
    // {
    //     return OPRT_COM_ERROR;
    // }

    return status;
}

static void flash_gd5f1g_wait_done(MTD_QSPI_CFG_T *cfg)
{
    uint32_t status_reg_data = 0;

    for(int i = 0; i <= (2000 / DELAY_CYCLE); i++) {
        status_reg_data = flash_gd5f1g_read_status(cfg, GD5F_GET_FEATURE, GD5F_STATUS, 1);
        if(0 == (status_reg_data & GD5F_SR_OIP)) {
            break;
        }
        tkl_system_sleep(DELAY_CYCLE);
    }
}

static OPERATE_RET flash_gd5f1g_write_status(MTD_QSPI_CFG_T *cfg, uint32_t cmd, uint32_t addr, uint32_t addr_len)
{

    OPERATE_RET ret = OPRT_COM_ERROR;
    TUYA_QSPI_CMD_T reg_cmd = {0};

    reg_cmd.op = TUYA_QSPI_WRITE;
    reg_cmd.cmd[0] = cmd;
    reg_cmd.cmd_size = 1;
    reg_cmd.cmd_lines = TUYA_QSPI_1WIRE;

    memcpy(reg_cmd.addr, &addr, sizeof(uint32_t));

    reg_cmd.addr_size = addr_len;
    reg_cmd.addr_lines = 0;
    reg_cmd.data_size = 0;
    reg_cmd.data_lines = 0;
    ret = tkl_qspi_comand(cfg->port, &reg_cmd);
    if (ret != 0)
    {
        return OPRT_COM_ERROR;
    }
    flash_gd5f1g_wait_done(cfg);
    return OPRT_OK;
}

static int32_t flash_gd5f1g_nor_set_protect_none(MTD_QSPI_CFG_T *cfg)
{
    uint8_t status_reg_data = 0;

    bk_printf("-----------%s %d----------\n", __func__, __LINE__);
    status_reg_data = flash_gd5f1g_read_status(cfg, GD5F_GET_FEATURE, GD5F_STATUS, 1) & 0xff;
    uint8_t clean_bits = ~(GD5F_BP_BP0 | GD5F_BP_BP1 | GD5F_BP_BP2);
    status_reg_data &= clean_bits;
    
    flash_gd5f1g_write_status(cfg, GD5F_WRITE_ENABLE, 0, 0);
    flash_gd5f1g_write_status(cfg, GD5F_SET_FEATURE, (GD5F_BLOCK_PROTECTION << 8) | status_reg_data, 2);
    return OPRT_OK;
}


static int32_t flash_gd5f1g_init(MTD_QSPI_CFG_T *cfg)
{
    //????????????????flash????????
    int32_t ret = OPRT_OK;
    uint32_t status_reg_data = 0;
    bk_printf("-----------%s %d----------\n", __func__, __LINE__);
    status_reg_data = (uint8_t)flash_gd5f1g_read_status(cfg, GD5F_GET_FEATURE, GD5F_SECURE_OTP, 1);
    if ((status_reg_data & GD5F_SOTP_QE) == 0) {
        status_reg_data |= GD5F_SOTP_QE;
        flash_gd5f1g_write_status(cfg, GD5F_WRITE_ENABLE, 0, 0);
        return flash_gd5f1g_write_status(cfg, GD5F_SET_FEATURE, (GD5F_SECURE_OTP << 8) | status_reg_data, 2);
    }

    return ret;
}

static int32_t flash_gd5f1g_deinit(MTD_QSPI_CFG_T *cfg)
{
    //????????????????flash????????
    int32_t ret = OPRT_OK;
    uint32_t status_reg_data = 0;

    status_reg_data = (uint8_t)flash_gd5f1g_read_status(cfg, GD5F_GET_FEATURE, GD5F_SECURE_OTP, 1);
    if ((status_reg_data & GD5F_SOTP_QE) == GD5F_SOTP_QE) {
        status_reg_data &= ~GD5F_SOTP_QE;
        flash_gd5f1g_write_status(cfg, GD5F_WRITE_ENABLE, 0, 2);
        return flash_gd5f1g_write_status(cfg, GD5F_SET_FEATURE, (GD5F_SECURE_OTP << 8) | status_reg_data, 2);
    }
    return ret;
}

// ?????Flash?��
MTD_DEVICE_T gd5f1g_flash_cfg = {
    .name = "gd5f1g",
    .type = MTD_NAND,
    .nand_dev = {
    	.page_size = GD5F_PAGE_SIZE,
        .sector_size = 0,
    	.block_size = GD5F_BLOCK_SIZE,
        .total_size = 128 * 1024 * 1024,
        .oob_size = 0,
        .interface = MTD_IF_QSPI,
        .qspi_dev = {
            .cmd_set = {
                .read_id = {
                    .command = GD5F_READ_ID,        // ??ID??? (e.g., 0x9F)
                    .addr_size = 1,
                    .addr_lines = TUYA_QSPI_1WIRE,
                    .wire_lines = TUYA_QSPI_1WIRE,
                    .dummy = 0,
                },
                .quad_read_data = {
                    .command = GD5F_QUAD_READ_FROM_CACHE,      // ????????? (e.g., 0x03)
                    .addr_size = 2,
                    .addr_lines = TUYA_QSPI_4WIRE,
                    .wire_lines = TUYA_QSPI_4WIRE,
                    .dummy = 4,
                },
                .quad_read_cache = {
                    .command = GD5F_PAGE_READ,
                    .addr_size = 3,
                    .addr_lines = TUYA_QSPI_1WIRE,
                    .wire_lines = TUYA_QSPI_1WIRE,
                    .dummy = 0,
                },
                .quad_program_cache = {
                    .command = GD5F_QUAD_PROGRAM_LOAD_RANDOM,
                    .addr_size = 2,
                    .addr_lines = TUYA_QSPI_1WIRE,
                    .wire_lines = TUYA_QSPI_4WIRE,
                    .dummy = 0,
                },
                .quad_page_program = {
                    .command = GD5F_PROGRAM_EXECUTE,
                    .addr_size = 3,
                    .addr_lines = TUYA_QSPI_1WIRE,
                    .wire_lines = TUYA_QSPI_1WIRE,
                    .dummy = 0,
                },
                .sector_erase = {
                    .command = 0,
                    .addr_size = 0,
                    .addr_lines = TUYA_QSPI_1WIRE,
                    .wire_lines = TUYA_QSPI_1WIRE,
                },
                .block_erase = {
                    .command = GD5F_BLOCK_ERASE,
                    .addr_size = 3,
                    .addr_lines = TUYA_QSPI_1WIRE,
                    .wire_lines = TUYA_QSPI_1WIRE,
                    .dummy = 0,
                },
                .write_enable = {
                    .command = GD5F_WRITE_ENABLE,
                    .addr_size = 0,
                    .addr_lines = TUYA_QSPI_1WIRE,
                    .wire_lines = TUYA_QSPI_1WIRE,
                    .dummy = 0,
                },
                .write_disable = {
                    .command = GD5F_WRITE_DISABLE,
                    .addr_size = 0,
                    .addr_lines = TUYA_QSPI_1WIRE,
                    .wire_lines = TUYA_QSPI_1WIRE,
                    .dummy = 0,
                },
            },
            .qspi = {
                .role = TUYA_QSPI_ROLE_MASTER,
                .mode = TUYA_QSPI_MODE0,
                .freq_hz = 104000000,
                .use_dma = 0,
            },
            .ops = {
                .init = flash_gd5f1g_init,
                .wait = flash_gd5f1g_wait_done,
                .unlock = flash_gd5f1g_nor_set_protect_none,
                .deinit = flash_gd5f1g_deinit,
            }
        }
    }
};

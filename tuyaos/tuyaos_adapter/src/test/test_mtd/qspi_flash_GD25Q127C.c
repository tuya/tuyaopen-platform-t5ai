
#include "tal_mtd_service.h"
#include "tal_mtd_qspi.h"
#include "tkl_system.h"
#include "tkl_qspi.h"

#define QSPI_QUAD_ENABLE    1

/* GD25Q127C Instructions *****************************************************************/

/*      Command                         Value     Description       Addr  Dummy     Data    */

#define GD25Q127C_WRITE_ENABLE               0x06 /*                     0       0       0     */
#define GD25Q127C_WRITE_DISABLE              0x04 /*                     0       0       0     */
#define GD25Q127C_READ_STATUS_REGISTER1      0x05 /* Read status register-1
                                              *                     1       0       1     */
#define GD25Q127C_READ_STATUS_REGISTER2      0x35 /* Read status register-2
                                              *                     1       0       1     */
#define GD25Q127C_READ_STATUS_REGISTER3      0x15 /* Read status register-3
                                              *                     1       0       1     */
#define GD25Q127C_WRITE_STATUS_REGISTER1     0x01 /* Write status register-1
                                              *                     1       0       1     */
#define GD25Q127C_WRITE_STATUS_REGISTER2     0x31 /* Write status register-2
                                              *                     1       0       1     */
#define GD25Q127C_WRITE_STATUS_REGISTER3     0x11 /* Write status register-3
                                              *                     1       0       1     */
#define GD25Q127C_READ_DATA                  0x03 /* Read data           3       0       1-2112 */
#define GD25Q127C_QUAD_OUTPUT_FAST_READ      0xEB /* Read data
                                              *  on SIO 0/1/2/3    3       1
                                              *  (Quad Output)      1-2112 */

#define GD25Q127C_READ_ID                    0x9F /* Read device ID      0       0       3      */

#define GD25Q127C_PAGE_PROGRAM               0x02 /* Load program data with
                                              * cache reset first   2       0       1-2112 */
#define GD25Q127C_QUAD_PAGE_PROGRAM          0x32 /* Load program data
                                              * without cache reset 2       0       1-2112 */
#define GD25Q127C_BLOCK_ERASE                0xD8 /* 64K Block erase         3       0       0     */
#define GD25Q127C_SECTOR_ERASE               0x20 /* 4K Block erase         3       0       0     */
#define GD25Q127C_CHIP_ERASE                 0xC7 /* Chip erase              0       0       0     */
#define GD25Q127C_ENABLE_RESET               0x66 /* ENABLE Reset        0       0       0     */
#define GD25Q127C_RESET                      0x99 /* Reset the device    0       0       0     */

/* Feature register ******************************************************************/

#define GD25Q127C_BLOCK_SHIFT            16    /* 65536 byte(64K) */
#define GD25Q127C_PAGE_SHIFT             8    /* 256 */
#define GD25Q127C_PAGE_MASK              ((1 << GD25Q127C_PAGE_SHIFT) - 1)
#define GD25Q127C_BLOCK_MASK             ((1 << GD25Q127C_BLOCK_SHIFT) - 1)
#define GD25Q127C_PAGE_SIZE              (1 << GD25Q127C_PAGE_SHIFT)
#define GD25Q127C_BLOCK_SIZE             (1 << GD25Q127C_BLOCK_SHIFT)

/* Bit definitions */

/* Status register */

/* Register1 */
#define GD25Q127C_SR_WIP                 (1 << 0)
#define GD25Q127C_SR_WEL                 (1 << 1)
#define GD25Q127C_SR_BP0                 (1 << 2)
#define GD25Q127C_SR_BP1                 (1 << 3)
#define GD25Q127C_SR_BP2                 (1 << 4)
#define GD25Q127C_SR_BP3                 (1 << 5)
#define GD25Q127C_SR_BP4                 (1 << 6)
#define GD25Q127C_SR_SRP0                (1 << 7)

/* Register2 */
#define GD25Q127C_SR_SRP1                (1 << 0)
#define GD25Q127C_SR_QE                  (1 << 1)
#define GD25Q127C_SR_CMP                 (1 << 6)

#define FLASH_PAGE_SIZE            0x100
#define FLASH_PAGE_MASK            (FLASH_PAGE_SIZE - 1)
#define FLASH_SECTOR_SIZE          0x1000

#define QSPI_FIFO_LEN_MAX          256
#define FLASH_PROTECT_NONE_DATA    0
#define DELAY_CYCLE    (5)

static OPERATE_RET flash_gd25q127c_read_status(MTD_QSPI_CFG_T *cfg, uint32_t cmd)
{
    OPERATE_RET ret = OPRT_OK;
    uint8_t status;
    TUYA_QSPI_CMD_T reg_cmd = {0};

    reg_cmd.op = TUYA_QSPI_READ;
    reg_cmd.cmd = cmd;
    reg_cmd.cmd_lines = TUYA_QSPI_1WIRE;
    reg_cmd.addr = 0;
    reg_cmd.addr_size = 0;
    reg_cmd.data_len = sizeof(uint8_t);
    reg_cmd.addr_lines = TUYA_QSPI_1WIRE;
    reg_cmd.data_lines = TUYA_QSPI_1WIRE;
    reg_cmd.dummy_cycle = 0;
    //���ٴ���ʱflash��Ҫdummy׼������

    ret = tkl_qspi_comand(cfg->port, &reg_cmd);
    if (ret != 0)
    {
        return OPRT_COM_ERROR;
    }
    ret = tkl_qspi_recv(cfg->port, &status, sizeof(uint8_t));
    if (ret != 0)
    {
        return OPRT_COM_ERROR;
    }

    return status;
}

static void flash_gd25q127c_wait_done(MTD_QSPI_CFG_T *cfg)
{
    uint32_t status_reg_data = 0;

    for(int i = 0; i <= (2000 / DELAY_CYCLE); i++) {
        status_reg_data = flash_gd25q127c_read_status(cfg, GD25Q127C_READ_STATUS_REGISTER1);
        if(0 == (status_reg_data & GD25Q127C_SR_WIP)) {
            break;
        }
        tkl_system_sleep(DELAY_CYCLE);
    }
}

static OPERATE_RET flash_gd25q127c_write_status(MTD_QSPI_CFG_T *cfg, uint32_t reg, uint8_t status_reg_data)
{

    OPERATE_RET ret = OPRT_COM_ERROR;
    TUYA_QSPI_CMD_T reg_cmd = {0};

    reg_cmd.op = TUYA_QSPI_WRITE;
    reg_cmd.cmd = ((status_reg_data << 8) | reg);
    reg_cmd.cmd_lines = TUYA_QSPI_1WIRE;
    reg_cmd.addr = 0;
    reg_cmd.addr_size = 0;
    reg_cmd.addr_lines = TUYA_QSPI_1WIRE;
    reg_cmd.data_len = sizeof(uint8_t);
    reg_cmd.data_lines = TUYA_QSPI_1WIRE;
    ret = tkl_qspi_comand(cfg->port, &reg_cmd);
    if (ret != 0)
    {
        return OPRT_COM_ERROR;
    }
    flash_gd25q127c_wait_done(cfg);
    return OPRT_OK;
}

static int32_t flash_gd25q127c_nor_set_protect_none(MTD_QSPI_CFG_T *cfg)
{
    uint8_t status_reg_data = 0;

    status_reg_data = flash_gd25q127c_read_status(cfg, GD25Q127C_READ_STATUS_REGISTER1) & 0xff;
    uint8_t clean_bits = ~(GD25Q127C_SR_BP0 | GD25Q127C_SR_BP1 |GD25Q127C_SR_BP2);
    status_reg_data &= clean_bits;
    flash_gd25q127c_write_status(cfg, GD25Q127C_WRITE_ENABLE, 0);
    flash_gd25q127c_write_status(cfg, GD25Q127C_WRITE_STATUS_REGISTER1, status_reg_data);
    return OPRT_OK;
}


static int32_t flash_gd25q127c_init(MTD_QSPI_CFG_T *cfg)
{
    //�ӿڳ�ʼ��Ӧ���ھ����flash��ʼ��ʵ��
    int32_t ret = OPRT_OK;
    uint32_t status_reg_data = 0;

    status_reg_data = (uint8_t)flash_gd25q127c_read_status(cfg, GD25Q127C_READ_STATUS_REGISTER2);
    if ((status_reg_data & GD25Q127C_SR_QE) == 0) {
        status_reg_data |= GD25Q127C_SR_QE;
        flash_gd25q127c_write_status(cfg, GD25Q127C_WRITE_ENABLE, 0);
        return flash_gd25q127c_write_status(cfg, GD25Q127C_WRITE_STATUS_REGISTER2, status_reg_data);
    }

    return ret;
}
static int32_t flash_gd25q127c_deinit(MTD_QSPI_CFG_T *cfg)
{
    //�ӿڳ�ʼ��Ӧ���ھ����flash��ʼ��ʵ��
    int32_t ret = OPRT_OK;
    uint32_t status_reg_data = 0;

    status_reg_data = (uint8_t)flash_gd25q127c_read_status(cfg, GD25Q127C_READ_STATUS_REGISTER2);
    if ((status_reg_data & GD25Q127C_SR_QE) != 0) {
        status_reg_data &= ~GD25Q127C_SR_QE;
        flash_gd25q127c_write_status(cfg, GD25Q127C_WRITE_ENABLE, 0);
        return flash_gd25q127c_write_status(cfg, GD25Q127C_WRITE_STATUS_REGISTER2, status_reg_data);
    }
    return ret;
}

// ��ʼ��Flash�豸
MTD_DEVICE_T gd25q127cflash_cfg = {
    .name = "gd25q127c",
    .type = MTD_NOR,
    .nor_dev = {
    	.page_size = 256,
        .sector_size = 4096,
    	.block_size = 32 * 1024,
        .total_size = 16 * 1024 * 1024,
        .interface = MTD_IF_QSPI,
        .qspi_dev = {
            .cmd_set = {
                .read_id = {
                    .command = GD25Q127C_READ_ID,        // ��IDָ�� (e.g., 0x9F)
                    .addr_size = 0,
                    .wire_lines = TUYA_QSPI_1WIRE,
                    .dummy = 0,
                },
                .quad_read_data = {
                    .command = GD25Q127C_QUAD_OUTPUT_FAST_READ,      // ������ָ�� (e.g., 0x03)
                    .addr_size = 3,
                    .wire_lines = TUYA_QSPI_4WIRE,
                    .dummy = 6,
                },
                .quad_read_cache = {
                    .command = 0,
                    .addr_size = 0,
                    .wire_lines = TUYA_QSPI_1WIRE,
                    .dummy = 0,
                },
                .quad_program_cache = {
                    .command = 0,
                    .addr_size = 0,
                    .wire_lines = TUYA_QSPI_1WIRE,
                    .dummy = 0,
                },
                .quad_page_program = {
                    .command = GD25Q127C_QUAD_PAGE_PROGRAM,
                    .addr_size = 3,
                    .wire_lines = TUYA_QSPI_4WIRE,
                    .dummy = 0,
                },
                .sector_erase = {
                    .command = GD25Q127C_SECTOR_ERASE,
                    .addr_size = 3,
                    .wire_lines = TUYA_QSPI_1WIRE,
                    .dummy = 0,
                },
                .block_erase = {
                    .command = GD25Q127C_BLOCK_ERASE,
                    .addr_size = 3,
                    .wire_lines = TUYA_QSPI_1WIRE,
                    .dummy = 0,
                },
                .write_enable = {
                    .command = GD25Q127C_WRITE_ENABLE,
                    .addr_size = 0,
                    .wire_lines = TUYA_QSPI_1WIRE,
                    .dummy = 0,
                },
                .write_disable = {
                    .command = GD25Q127C_WRITE_DISABLE,
                    .addr_size = 0,
                    .wire_lines = TUYA_QSPI_1WIRE,
                    .dummy = 0,
                },
            },
            .qspi = {
                .role = TUYA_QSPI_ROLE_MASTER,
                .mode = TUYA_QSPI_MODE0,
                .baudrate = 104000000,
                .is_dma = 0,
            },
            .ops = {
                .init = flash_gd25q127c_init,
                .wait = flash_gd25q127c_wait_done,
                .unlock = flash_gd25q127c_nor_set_protect_none,
                .deinit = flash_gd25q127c_deinit,
            }
        }
    }
};

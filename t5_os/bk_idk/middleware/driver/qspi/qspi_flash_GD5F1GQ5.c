// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <driver/qspi.h>
#include <driver/qspi_flash.h>
#include "qspi_hal.h"
#include <driver/int.h>
#include <os/mem.h>
#include <driver/qspi.h>
#include "qspi_driver.h"
#include "qspi_hal.h"
#include "qspi_statis.h"
#include <driver/trng.h>

/* GD5F Instructions *****************************************************************/

/*      Command                         Value     Description       Addr  Dummy     Data    */

#define GD5F_WRITE_ENABLE               0x06 /*                     0       0       0     */
#define GD5F_WRITE_DISABLE              0x04 /*                     0       0       0     */
#define GD5F_GET_FEATURE                0x0f /* Get features        1       0       1     */
#define GD5F_SET_FEATURE                0x1f /* Set features        1       0       1     */

#define GD5F_PAGE_READ                  0x13 /* Array read          3       0       0     */
#define GD5F_READ_FROM_CACHE            0x03 /* Output cache data
                                              *  on SO                              1-2112 */
#define GD5F_QUAD_READ_FROM_CACHE       0x6b /* Output cache data   2       1
                                              *  on SIO 0/1/2/3                     1-2112 */

#define GD5F_READ_ID                    0x9f /* Read device ID      0       1       2      */

#define GD5F_PROGRAM_LOAD               0x02 /* Load program data with
                                              * cache reset first   2       0       1-2112 */
#define GD5F_QUAD_PROGRAM_LOAD          0x32 /* Load program data
                                              * without cache reset 2       0       1-2112 */
#define GD5F_PROGRAM_LOAD_RANDOM        0x84 /* Load program data
                                              * without cache reset 2       0       1-2112 */
#define GD5F_QUAD_PROGRAM_LOAD_RANDOM   0xC4 /* Load program data
                                              * without cache reset 2       0       1-2112 */
#define GD5F_PROGRAM_EXECUTE            0x10 /* Enter block/page
                                              * address, execute    3   0   0     */

#define GD5F_BLOCK_ERASE                0xd8 /* Block erase         3       0       0     */

#define GD5F_RESET                      0xff /* Reset the device    0       0       0     */

#define GD5F_DUMMY                      0x00 /* No Operation        0       0       0     */

//#define GD5F_ECC_STATUS_READ            0x7c /* Internal ECC status
//                                              *  output             0       1       1     */

/* Feature register ******************************************************************/

/* JEDEC Read ID register values */

#define GD5F_MANUFACTURER           0xc8
#define GD5F_GD5F_CAPACITY_MASK     0x0f
#define GD5F_CAPACITY_1GBIT         0x01  /* 1 Gb */
#define GD5F_CAPACITY_2GBIT         0x02  /* 2 Gb */
#define GD5F_CAPACITY_4GBIT         0x04  /* 4 Gb */

#define GD5F_NSECTORS_1GBIT         1024  /* 1024x131072 = 1Gbit memory capacity */
#define GD5F_NSECTORS_2GBIT         2048  /* 2048x131072 = 2Gbit memory capacity */
#define GD5F_NSECTORS_4GBIT         4096  /* 4096x131072 = 4Gbit memory capacity */

#define GD5F_SECTOR_SHIFT           17    /* 131072 byte */
#define GD5F_PAGE_SHIFT             11    /* 2048 */
#define GD5F_PAGE_MASK              ((1 << GD5F_PAGE_SHIFT) - 1)
#define GD5F_PAGE_SIZE              (1 << GD5F_PAGE_SHIFT)

/* Register address */

#define GD5F_SECURE_OTP             0xb0
#define GD5F_STATUS                 0xc0
#define GD5F_BLOCK_PROTECTION       0xa0

/* Bit definitions */

/* Secure OTP (On-Time-Programmable) register */

#define GD5F_SOTP_QE                (1 << 0)  /* Bit 0: Quad Enable */
#define GD5F_SOTP_ECC               (1 << 4)  /* Bit 4: ECC enabled */
#define GD5F_SOTP_SOTP_EN           (1 << 6)  /* Bit 6: Secure OTP Enable */
#define GD5F_SOTP_SOTP_PROT         (1 << 7)  /* Bit 7: Secure OTP Protect */

/* Status register */

#define GD5F_SR_OIP                 (1 << 0)  /* Bit 0: Operation in progress */
#define GD5F_SR_WEL                 (1 << 1)  /* Bit 1: Write enable latch */
#define GD5F_SR_E_FAIL              (1 << 2)  /* Bit 2: Erase fail */
#define GD5F_SR_P_FAIL              (1 << 3)  /* Bit 3: Program Fail */
#define GD5F_SR_ECC_S0              (1 << 4)  /* Bit 4-5: ECC Status  */
#define GD5F_SR_ECC_S1              (1 << 5)

/* Block Protection register */

#define GD5F_BP_SP                  (1 << 0)  /* Bit 0: Solid-protection (1Gb only) */
#define GD5F_BP_COMPL               (1 << 1)  /* Bit 1: Complementary (1Gb only) */
#define GD5F_BP_INV                 (1 << 2)  /* Bit 2: Invert (1Gb only) */
#define GD5F_BP_BP0                 (1 << 3)  /* Bit 3: Block Protection 0 */
#define GD5F_BP_BP1                 (1 << 4)  /* Bit 4: Block Protection 1 */
#define GD5F_BP_BP2                 (1 << 5)  /* Bit 5: Block Protection 2 */
#define GD5F_BP_BPRWD               (1 << 7)  /* Bit 7: Block Protection Register
                                               *        Write Disable */

/* ECC Status register */

#define GD5F_FEATURE_ECC_MASK       (0x03 << 4)
#define GD5F_FEATURE_ECC_ERROR      (0x02 << 4)
#define GD5F_FEATURE_ECC_OFFSET     4
#define GD5F_ECC_STATUS_MASK        0x0f



#define FLASH_PAGE_SIZE            0x100
#define FLASH_PAGE_MASK            (FLASH_PAGE_SIZE - 1)
#define FLASH_SECTOR_SIZE          0x1000

#define QSPI_FIFO_LEN_MAX          256
#define FLASH_PROTECT_NONE_DATA    0

static void bk_qspi_flash_gd5f_wait_oip_done(qspi_id_t id);

bk_err_t bk_qspi_flash_gd5f_init(qspi_id_t id)
{
    // 11 ~ 12 MHz
    qspi_config_t config = {0};
    config.src_clk = QSPI_SCLK_320M;
    config.src_clk_div = 0x6;
    config.clk_div = 0x4;
    BK_LOG_ON_ERR(bk_qspi_init(id, &config));
    return BK_OK;
}

bk_err_t bk_qspi_flash_gd5f_deinit(qspi_id_t id)
{
    BK_LOG_ON_ERR(bk_qspi_deinit(id));
    return BK_OK;
}

static uint32_t bk_qspi_flash_gd5f_read_status(qspi_id_t id)
{
    qspi_cmd_t read_status_cmd = {0};
    uint32_t status_reg_data = 0;

    read_status_cmd.device = QSPI_FLASH;
    read_status_cmd.wire_mode = QSPI_1WIRE;
    read_status_cmd.work_mode = INDIRECT_MODE;
    read_status_cmd.op = QSPI_READ;
    read_status_cmd.cmd = GD5F_GET_FEATURE;
    read_status_cmd.addr = GD5F_STATUS;
    read_status_cmd.addr_valid_bit = QSPI_ADDR_VALID_BIT8;

    read_status_cmd.data_len = 1;

    BK_LOG_ON_ERR(bk_qspi_command(id, &read_status_cmd));
    bk_qspi_read(id, &status_reg_data, 1);

    return status_reg_data & 0xff;
}

static void bk_qspi_flash_gd5f_wait_oip_done(qspi_id_t id)
{
    uint32_t status_reg_data = 0;

    for(int i = 0; i <= 2000; i++) {
        status_reg_data = bk_qspi_flash_gd5f_read_status(id);
        if(0 == (status_reg_data & GD5F_SR_OIP)) {
            break;
        }
        if(i == 2000) {
            QSPI_LOGW("[%s]: wait flsh progress done timeout.\n", __func__);
        }
        rtos_delay_milliseconds(1);
    }
}

static void bk_qspi_flash_gd5f_write_enable(qspi_id_t id)
{
    qspi_cmd_t wren_cmd = {0};

    wren_cmd.device = QSPI_FLASH;
    wren_cmd.wire_mode = QSPI_1WIRE;
    wren_cmd.work_mode = INDIRECT_MODE;
    wren_cmd.op = QSPI_WRITE;
    wren_cmd.cmd = GD5F_WRITE_ENABLE;
    wren_cmd.data_len = 0;

    BK_LOG_ON_ERR(bk_qspi_command(id, &wren_cmd));
    bk_qspi_flash_gd5f_wait_oip_done(id);
}

static uint32_t bk_qspi_flash_gd5f_read_secure_status(qspi_id_t id)
{
    qspi_cmd_t read_status_cmd = {0};
    uint32_t status_reg_data = 0;

    read_status_cmd.device = QSPI_FLASH;
    read_status_cmd.wire_mode = QSPI_1WIRE;
    read_status_cmd.work_mode = INDIRECT_MODE;
    read_status_cmd.op = QSPI_READ;
    read_status_cmd.cmd = GD5F_GET_FEATURE;
    read_status_cmd.addr = GD5F_SECURE_OTP;
    read_status_cmd.addr_valid_bit = QSPI_ADDR_VALID_BIT8;
    read_status_cmd.data_len = 1;

    BK_LOG_ON_ERR(bk_qspi_command(id, &read_status_cmd));
    bk_qspi_read(id, &status_reg_data, 1);

    return status_reg_data;
}

static bk_err_t bk_qspi_flash_gd5f_write_secure_status(qspi_id_t id, uint8_t status_reg_data)
{
    qspi_cmd_t write_status_cmd = {0};

    bk_qspi_flash_gd5f_write_enable(id);

    write_status_cmd.device = QSPI_FLASH;
    write_status_cmd.wire_mode = QSPI_1WIRE;
    write_status_cmd.work_mode = INDIRECT_MODE;
    write_status_cmd.op = QSPI_WRITE;
    write_status_cmd.cmd = GD5F_SET_FEATURE;
    write_status_cmd.addr = (GD5F_SECURE_OTP << 8) | status_reg_data;
    write_status_cmd.addr_valid_bit = QSPI_ADDR_VALID_BIT16;
    write_status_cmd.data_len = 0;

    BK_LOG_ON_ERR(bk_qspi_command(id, &write_status_cmd));
    bk_qspi_flash_gd5f_wait_oip_done(id);

    return BK_OK;
}

static bk_err_t bk_qspi_flash_gd5f_write_block_status(qspi_id_t id, uint8_t status_reg_data)
{
    qspi_cmd_t write_status_cmd = {0};

    bk_qspi_flash_gd5f_write_enable(id);

    write_status_cmd.device = QSPI_FLASH;
    write_status_cmd.wire_mode = QSPI_1WIRE;
    write_status_cmd.work_mode = INDIRECT_MODE;
    write_status_cmd.op = QSPI_WRITE;
    write_status_cmd.cmd = GD5F_SET_FEATURE;
    write_status_cmd.addr = (GD5F_BLOCK_PROTECTION << 8) | status_reg_data;
    write_status_cmd.addr_valid_bit = QSPI_ADDR_VALID_BIT16;
    write_status_cmd.data_len = 0;

    BK_LOG_ON_ERR(bk_qspi_command(id, &write_status_cmd));
    bk_qspi_flash_gd5f_wait_oip_done(id);

    return BK_OK;
}

static bk_err_t bk_qspi_flash_gd5f_write_status(qspi_id_t id, uint8_t status_reg_data)
{
    qspi_cmd_t write_status_cmd = {0};

    bk_qspi_flash_gd5f_write_enable(id);

    bk_qspi_write(id, &status_reg_data, 1);

    write_status_cmd.device = QSPI_FLASH;
    write_status_cmd.wire_mode = QSPI_1WIRE;
    write_status_cmd.work_mode = INDIRECT_MODE;
    write_status_cmd.op = QSPI_WRITE;
    write_status_cmd.cmd = GD5F_SET_FEATURE;
    write_status_cmd.addr = GD5F_STATUS;
    write_status_cmd.addr_valid_bit = QSPI_ADDR_VALID_BIT8;
    write_status_cmd.data_len = 1;

    BK_LOG_ON_ERR(bk_qspi_command(id, &write_status_cmd));
    bk_qspi_flash_gd5f_wait_oip_done(id);

    return BK_OK;
}

static void bk_qspi_flash_gd5f_quad_enable(qspi_id_t id)
{
    uint32_t status_reg_data = 0;

    status_reg_data = (uint8_t)bk_qspi_flash_gd5f_read_secure_status(id);
    status_reg_data |= GD5F_SOTP_QE;
    bk_qspi_flash_gd5f_write_secure_status(id, status_reg_data);
    bk_qspi_flash_gd5f_read_secure_status(id);
}

static void bk_qspi_flash_gd5f_quad_disable(qspi_id_t id)
{
    uint32_t status_reg_data = 0;

    status_reg_data = (uint8_t)bk_qspi_flash_gd5f_read_secure_status(id);
    status_reg_data &= ~GD5F_SOTP_QE;
    bk_qspi_flash_gd5f_write_secure_status(id, status_reg_data);
    bk_qspi_flash_gd5f_read_secure_status(id);
}

bk_err_t bk_qspi_flash_gd5f_erase_sector(qspi_id_t id, uint32_t addr)
{
    qspi_cmd_t erase_sector_cmd = {0};

//    bk_printf("qspi flash erase block, address: %08x\r\n", addr);
    bk_qspi_flash_gd5f_write_enable(id);

    erase_sector_cmd.device = QSPI_FLASH;
    erase_sector_cmd.wire_mode = QSPI_1WIRE;
    erase_sector_cmd.work_mode = INDIRECT_MODE;
    erase_sector_cmd.op = QSPI_WRITE;
    erase_sector_cmd.cmd = GD5F_BLOCK_ERASE;
    erase_sector_cmd.addr = addr;
    erase_sector_cmd.addr_valid_bit = QSPI_ADDR_VALID_BIT24;
    erase_sector_cmd.data_len = 0;

    BK_LOG_ON_ERR(bk_qspi_command(id, &erase_sector_cmd));
    bk_qspi_flash_gd5f_wait_oip_done(id);
//    bk_printf("qspi flash erase complete\r\n");

    return BK_OK;
}

static bk_err_t bk_qspi_flash_gd5f_std_write_to_cache(qspi_id_t id, uint32_t addr, const void *data, int16_t size)
{
    qspi_cmd_t cache_program_cmd = {0};
    uint16_t ofs = addr & GD5F_PAGE_MASK;
    uint16_t len = 0;
    uint8_t *buffer = (uint8_t *)data;

    static volatile uint8_t cmd_conv = 0;

    cache_program_cmd.device = QSPI_FLASH;
    cache_program_cmd.wire_mode = QSPI_1WIRE;         // TODO QSPI_4WIRE
    cache_program_cmd.work_mode = INDIRECT_MODE;
    cache_program_cmd.op = QSPI_WRITE;
    cache_program_cmd.cmd = GD5F_PROGRAM_LOAD;
    cache_program_cmd.addr_valid_bit = QSPI_ADDR_VALID_BIT16;

    while(0 < size) {
        len = (size < QSPI_FIFO_LEN_MAX) ? size : QSPI_FIFO_LEN_MAX;
        bk_qspi_write(id, buffer, len);

        if (cmd_conv == 0) {
            cache_program_cmd.cmd = GD5F_PROGRAM_LOAD;
        } else {
            cache_program_cmd.cmd = GD5F_PROGRAM_LOAD_RANDOM;
        }
        cmd_conv++;

        cache_program_cmd.addr = ofs;
        cache_program_cmd.data_len = len;
        BK_LOG_ON_ERR(bk_qspi_command(id, &cache_program_cmd));
        ofs += len;
        buffer += len;
        size -= len;
    }
    cmd_conv = 0;

    return BK_OK;
}

static bk_err_t bk_qspi_flash_gd5f_execute_write(qspi_id_t id, uint32_t addr)
{
    qspi_cmd_t execute_cmd = {0};

    bk_qspi_flash_gd5f_write_enable(id);

    execute_cmd.device = QSPI_FLASH;
    execute_cmd.wire_mode = QSPI_1WIRE;
    execute_cmd.work_mode = INDIRECT_MODE;
    execute_cmd.op = QSPI_WRITE;
    execute_cmd.cmd = GD5F_PROGRAM_EXECUTE;
    execute_cmd.addr = addr;
    execute_cmd.addr_valid_bit = QSPI_ADDR_VALID_BIT24;
    execute_cmd.data_len = 0;

    BK_LOG_ON_ERR(bk_qspi_command(id, &execute_cmd));
    bk_qspi_flash_gd5f_wait_oip_done(id);

    return BK_OK;
}

bk_err_t bk_qspi_flash_gd5f_standrad_page_program(qspi_id_t id, uint32_t addr, const void *data, int32_t size)
{
//    bk_printf("===> entry %s\r\n", __func__);
    if (data == NULL || size == 0)
        return -1;

    // 1. issue write date to cache cmd <02h/32h>
    bk_qspi_flash_gd5f_std_write_to_cache(id, addr, data, size);

    // 2. issue write enable cmd <06h>
    bk_qspi_flash_gd5f_write_enable(id);

    // 3. issue program execute cmd <10h> + 24-bit address
    // the 24-bit address is actual address to be written to flash
    bk_qspi_flash_gd5f_execute_write(id, addr);
//    bk_printf("<=== exit %s\r\n\r\n", __func__);

    return BK_OK;
}

static bk_err_t bk_qspi_flash_gd5f_quad_write_to_cache(qspi_id_t id, uint32_t addr, const void *data, int16_t size)
{
    qspi_cmd_t cache_program_cmd = {0};
    uint16_t ofs = addr & GD5F_PAGE_MASK;
    uint16_t len = 0;
    uint32_t *buffer = (uint32_t *)data;

    cache_program_cmd.device = QSPI_FLASH;
    cache_program_cmd.wire_mode = QSPI_4WIRE;
    cache_program_cmd.work_mode = INDIRECT_MODE;
    cache_program_cmd.op = QSPI_WRITE;
    cache_program_cmd.cmd = GD5F_QUAD_PROGRAM_LOAD_RANDOM;
    cache_program_cmd.addr_valid_bit = QSPI_ADDR_VALID_BIT16;

    while(0 < size) {
        len = (size < QSPI_FIFO_LEN_MAX) ? size : QSPI_FIFO_LEN_MAX;
        bk_qspi_write(id, buffer, len);
        cache_program_cmd.addr = ofs;
        cache_program_cmd.data_len = len;
        BK_LOG_ON_ERR(bk_qspi_command(id, &cache_program_cmd));
        ofs += len;
        buffer += len;
        size -= len;
    }

    return BK_OK;
}

bk_err_t bk_qspi_flash_gd5f_quad_page_program(qspi_id_t id, uint32_t addr, const void *data, int32_t size)
{
    if (data == NULL || size == 0)
        return -1;

//    bk_printf("===> entry %s\r\n", __func__);
    // 1. issue write date to cache cmd <02h/32h>
    bk_qspi_flash_gd5f_quad_enable(id);
    bk_qspi_flash_gd5f_quad_write_to_cache(id, addr, data, size);
    bk_qspi_flash_gd5f_quad_disable(id);

    // 2. issue write enable cmd <06h>
    bk_qspi_flash_gd5f_write_enable(id);

    // 3. issue program execute cmd <10h> + 24-bit address
    // the 24-bit address is actual address to be written to flash
    bk_qspi_flash_gd5f_execute_write(id, addr);

//    bk_printf("<=== exit %s\r\n", __func__);
    return BK_OK;
}

static bk_err_t bk_qspi_flash_gd5f_read_page_to_cache(qspi_id_t id, uint32_t addr)
{
    qspi_cmd_t read_to_cache_cmd = {0};

    read_to_cache_cmd.device = QSPI_FLASH;
    read_to_cache_cmd.wire_mode = QSPI_1WIRE;
    read_to_cache_cmd.work_mode = INDIRECT_MODE;
    read_to_cache_cmd.op = QSPI_WRITE;
    read_to_cache_cmd.cmd = GD5F_PAGE_READ;
    read_to_cache_cmd.addr = addr;
    read_to_cache_cmd.addr_valid_bit = QSPI_ADDR_VALID_BIT24;
    read_to_cache_cmd.data_len = 0;

    BK_LOG_ON_ERR(bk_qspi_command(id, &read_to_cache_cmd));
    bk_qspi_flash_gd5f_wait_oip_done(id);

    return BK_OK;

}

static bk_err_t bk_qspi_flash_gd5f_std_read_cache(qspi_id_t id, uint32_t addr, void *data, int32_t size)
{
    qspi_cmd_t read_cache_cmd = {0};
    uint8_t *buffer = data;
    uint16_t ofs = addr & GD5F_PAGE_MASK;
    int16_t len = 0;

    if (data == NULL)
        return -1;

    read_cache_cmd.device = QSPI_FLASH;
    read_cache_cmd.wire_mode = QSPI_1WIRE;
    read_cache_cmd.work_mode = INDIRECT_MODE;
    read_cache_cmd.op = QSPI_READ;
    read_cache_cmd.cmd = GD5F_READ_FROM_CACHE;
    read_cache_cmd.addr_valid_bit = QSPI_ADDR_VALID_BIT24;
    //read_cache_cmd.addr = addr << 8;

    while(0 < size) {
        len = (size < QSPI_FIFO_LEN_MAX) ? size : QSPI_FIFO_LEN_MAX;
        read_cache_cmd.addr = ofs << 8;
        read_cache_cmd.data_len = len;
        BK_LOG_ON_ERR(bk_qspi_command(id, &read_cache_cmd));
        bk_qspi_read(id, buffer, len);

        ofs += len;
        buffer += len;
        size -= len;
    }

    return BK_OK;
}

bk_err_t bk_qspi_flash_gd5f_standrad_page_read(qspi_id_t id, uint32_t addr, void *data, uint32_t size)
{
    qspi_cmd_t single_read_cmd = {0};
    uint32_t cmd_data_len = 0;
    uint32_t *cmd_data = data;

//    bk_printf("===> entry %s\r\n", __func__);
    if (data == NULL || size == 0)
        return;

    // 1. issue read page to cache cmd <13h>, and wait done
    uint32_t page_addr = addr & ~GD5F_PAGE_MASK;
    bk_qspi_flash_gd5f_read_page_to_cache(id, page_addr);

    // 2. issue read cache cmd <03h/6bh>
    bk_qspi_flash_gd5f_std_read_cache(id, addr, data, size);
//    bk_printf("<=== exit %s\r\n\r\n", __func__);
    return BK_OK;
}

static bk_err_t bk_qspi_flash_gd5f_quad_read_cache(qspi_id_t id, uint32_t addr, void *data, int32_t size)
{
    qspi_cmd_t read_cache_cmd = {0};
    uint8_t *buffer = data;
    uint16_t ofs = addr & GD5F_PAGE_MASK;
    int16_t len = 0;

    if (buffer == NULL)
        return -1;

    read_cache_cmd.device = QSPI_FLASH;
    read_cache_cmd.wire_mode = QSPI_4WIRE;
    read_cache_cmd.work_mode = INDIRECT_MODE;
    read_cache_cmd.op = QSPI_READ;
    read_cache_cmd.cmd = GD5F_QUAD_READ_FROM_CACHE;
    read_cache_cmd.addr_valid_bit = QSPI_ADDR_VALID_BIT24;
    read_cache_cmd.addr = addr << 8;

    while(0 < size) {
        len = (size < QSPI_FIFO_LEN_MAX) ? size : QSPI_FIFO_LEN_MAX;
        //read_cache_cmd.addr = ofs << 8;
        read_cache_cmd.data_len = len;
        BK_LOG_ON_ERR(bk_qspi_command(id, &read_cache_cmd));
        bk_qspi_read(id, buffer, len);

        ofs += len;
        buffer += len;
        size -= len;
    }

    return BK_OK;
}

bk_err_t bk_qspi_flash_gd5f_quad_page_read(qspi_id_t id, uint32_t addr, void *data, uint32_t size)
{
    qspi_cmd_t single_read_cmd = {0};
    uint32_t cmd_data_len = 0;
    uint32_t *cmd_data = data;

//    bk_printf("===> entry %s\r\n", __func__);
    if (data == NULL || size == 0)
        return;

    // 1. issue read page to cache cmd <13h>, and wait done
    bk_qspi_flash_gd5f_read_page_to_cache(id, addr);

    // 2. issue read cache cmd <03h/6bh>
    bk_qspi_flash_gd5f_quad_enable(1);
    bk_qspi_flash_gd5f_quad_read_cache(id, addr, data, size);
    bk_qspi_flash_gd5f_quad_disable(1);
//    bk_printf("<=== exit %s\r\n", __func__);
    return BK_OK;
}

#if 0
bk_err_t bk_qspi_flash_gd5f_quad_page_program(qspi_id_t id, uint32_t addr, const void *data, uint32_t size)
{
    qspi_cmd_t page_program_cmd = {0};
    uint32_t cmd_data_len = 0;
    uint32_t *cmd_data = (uint32_t *)data;

    if (data == NULL || size == 0)
        return;

    page_program_cmd.device = QSPI_FLASH;
    page_program_cmd.wire_mode = QSPI_4WIRE;
    page_program_cmd.work_mode = INDIRECT_MODE;
    page_program_cmd.op = QSPI_WRITE;
    page_program_cmd.cmd = GD5F_QUAD_PROGRAM_LOAD;

    while(0 < size) {
        bk_qspi_flash_gd5f_write_enable(id);

        cmd_data_len = (size < QSPI_FIFO_LEN_MAX) ? size : QSPI_FIFO_LEN_MAX;
        bk_qspi_write(id, cmd_data, cmd_data_len);
        page_program_cmd.addr = addr;
        page_program_cmd.addr_valid_bit = QSPI_ADDR_VALID_BIT16;
        page_program_cmd.data_len = cmd_data_len;
        BK_LOG_ON_ERR(bk_qspi_command(id, &page_program_cmd));

        addr += cmd_data_len;
        cmd_data += cmd_data_len;
        size -= cmd_data_len;

        bk_qspi_flash_gd5f_wait_oip_done(id);
    }

    return BK_OK;
}

bk_err_t bk_qspi_flash_gd5f_quad_read(qspi_id_t id, uint32_t addr, void *data, uint32_t size)
{
    qspi_cmd_t quad_read_cmd = {0};
    uint32_t cmd_data_len = 0;
    uint32_t *cmd_data = data;

    if (data == NULL || size == 0)
        return;

    quad_read_cmd.device = QSPI_FLASH;
    quad_read_cmd.wire_mode = QSPI_4WIRE;
    quad_read_cmd.work_mode = INDIRECT_MODE;
    quad_read_cmd.op = QSPI_READ;
    quad_read_cmd.cmd = GD5F_QUAD_READ_FROM_CACHE;
    quad_read_cmd.dummy_cycle = 4;

    while(0 < size) {
        cmd_data_len = (size < QSPI_FIFO_LEN_MAX) ? size : QSPI_FIFO_LEN_MAX;
        quad_read_cmd.addr = addr;
        quad_read_cmd.data_len = cmd_data_len;
        BK_LOG_ON_ERR(bk_qspi_command(id, &quad_read_cmd));
        bk_qspi_read(id, cmd_data, cmd_data_len);

        addr += cmd_data_len;
        cmd_data += cmd_data_len;
        size -= cmd_data_len;
    }
    return BK_OK;
}
#endif

bk_err_t bk_qspi_flash_gd5f_write(qspi_id_t id, uint32_t base_addr, const void *data, uint32_t size)
{
    uint8_t *write_data = (uint8_t *)data;

    if (data == NULL || size == 0)
        return;

    /*
        for example: write 0x23456, len 5100

        0x0 ...    0x23000          0x23800             0x24000             0x24800             0x25000
        |           |                   |                   |                   |                   |
        |-----------|-------------------|-------------------|-------------------|-------------------|------------
                                 |938   |      2048         |      2048         |66|
                                 |      |                   |                   |  |                |
                              0x23456   |                                       |  0x24842

        start_page_address: 0x23456 & ~GD5F_PAGE_MASK = 0x23000
        start_page_offset:  0x23456 & GD5F_PAGE_MASK = 0x456
        start_paeg_len:     2048 - 0x456 = 938

        continue_page_num = 2
        continue_page_start_addr: start_page_address + 2048 = 0x23800

        last_page_address: continue_page_start_addr + n * 2048 = 0x24800
        last_page_offset:  0
        last_page_len:  total_len - start_paeg_len - n * 2048 = 5100 - 938 - 4096 = 66
    */

    uint8_t *buf = (uint8_t *)tkl_system_malloc(GD5F_PAGE_SIZE);
    if (buf == NULL) {
        bk_printf("read flash error, malloc failed\r\n");
        return -1;
    }

    // 1. 1st page handle
    uint32_t first_page_addr = base_addr & ~GD5F_PAGE_MASK;
    uint32_t first_page_offset = base_addr & GD5F_PAGE_MASK;
    uint32_t first_page_left_len = GD5F_PAGE_SIZE - first_page_offset;
    uint32_t first_page_write_len = 0;

    if (size < first_page_left_len) {
        first_page_write_len = size;
    } else {
        first_page_write_len = first_page_left_len;
    }

    os_memset(buf, 0, GD5F_PAGE_SIZE);
    bk_qspi_flash_gd5f_standrad_page_read(id, first_page_addr, buf, GD5F_PAGE_SIZE);
    os_memcpy(&buf[first_page_offset], write_data, first_page_write_len);
    bk_qspi_flash_gd5f_standrad_page_program(id, first_page_addr, buf, GD5F_PAGE_SIZE);

    if (size < first_page_left_len) {
        tkl_system_free(buf);
        buf = NULL;
        return;
    }

    // 2. whole pages continue
    uint32_t cuntinue_page_num = (size - first_page_left_len) / GD5F_PAGE_SIZE;
    uint32_t cuntinue_start_addr = first_page_addr + GD5F_PAGE_SIZE;
    for (int i = 0; i < cuntinue_page_num; i++) {
        uint32_t tmp_addr = cuntinue_start_addr + i * GD5F_PAGE_SIZE;
        os_memset(buf, 0, GD5F_PAGE_SIZE);
        os_memcpy(buf, write_data + first_page_offset + i * GD5F_PAGE_SIZE, GD5F_PAGE_SIZE);
        bk_qspi_flash_gd5f_standrad_page_read(id, tmp_addr, buf, GD5F_PAGE_SIZE);
    }

    // 3. last page handle
    uint32_t last_page_start_addr = cuntinue_start_addr + cuntinue_page_num * GD5F_PAGE_SIZE;
    uint32_t last_page_write_len = (size - first_page_write_len) % GD5F_PAGE_SIZE;

    os_memset(buf, 0, GD5F_PAGE_SIZE);
    bk_qspi_flash_gd5f_standrad_page_read(id, last_page_start_addr, buf, GD5F_PAGE_SIZE);
    os_memcpy(buf, write_data + first_page_offset + cuntinue_page_num * GD5F_PAGE_SIZE, last_page_write_len);
    bk_qspi_flash_gd5f_standrad_page_program(id, first_page_addr, buf, GD5F_PAGE_SIZE);


    tkl_system_free(buf);
    buf = NULL;

    return BK_OK;
}

bk_err_t bk_qspi_flash_gd5f_read(qspi_id_t id, uint32_t base_addr, void *data, uint32_t size)
{
    uint32_t left_len = size;
    uint32_t read_len= 0;
    uint32_t offset = 0;

    if (data == NULL || size == 0)
        return;

    uint8_t *buf = (uint8_t *)tkl_system_malloc(GD5F_PAGE_SIZE);
    if (buf == NULL) {
        bk_printf("read flash error, malloc failed\r\n");
        return -1;
    }

    /*
        for example: read 0x23456, len 5100

        0x0 ...    0x23000          0x23800             0x24000             0x24800             0x25000
        |           |                   |                   |                   |                   |
        |-----------|-------------------|-------------------|-------------------|-------------------|------------
                                 |938   |      2048         |      2048         |66|
                                 |      |                   |                   |  |                |
                              0x23456   |                                       |  0x24842

        start_page_address: 0x23456 & ~GD5F_PAGE_MASK = 0x23000
        start_page_offset:  0x23456 & GD5F_PAGE_MASK = 0x456
        start_paeg_len:     2048 - 0x456 = 938

        continue_page_num = 2
        continue_page_start_addr: start_page_address + 2048 = 0x23800

        last_page_address: continue_page_start_addr + n * 2048 = 0x24800
        last_page_offset:  0
        last_page_len:  total_len - start_paeg_len - n * 2048 = 5100 - 938 - 4096 = 66
    */

    // 1. 1st page handle
    uint32_t first_page_addr = base_addr & ~GD5F_PAGE_MASK;
    uint32_t first_page_offset = base_addr & GD5F_PAGE_MASK;
    uint32_t first_page_left_len = GD5F_PAGE_SIZE - first_page_offset;
    uint32_t first_page_read_len = 0;

    if (size < first_page_left_len) {
        first_page_read_len = size;
    } else {
        first_page_read_len = first_page_left_len;
    }
    os_memset(buf, 0, GD5F_PAGE_SIZE);

    bk_qspi_flash_gd5f_standrad_page_read(id, base_addr, buf, first_page_read_len);

    os_memcpy(data, buf, first_page_read_len);

    if (size <= first_page_left_len) {
        tkl_system_free(buf);
        buf = NULL;
        return;
    }

    // 2. whole pages continue
    uint32_t cuntinue_page_num = (size - first_page_left_len) / GD5F_PAGE_SIZE;
    uint32_t cuntinue_start_addr = first_page_addr + GD5F_PAGE_SIZE;
    for (int i = 0; i < cuntinue_page_num; i++) {
        uint32_t tmp_addr = cuntinue_start_addr + i * GD5F_PAGE_SIZE;
        os_memset(buf, 0, GD5F_PAGE_SIZE);
        bk_qspi_flash_gd5f_standrad_page_read(id, tmp_addr, buf, GD5F_PAGE_SIZE);
        os_memcpy((uint8_t *)data + first_page_offset + i * GD5F_PAGE_SIZE, buf, GD5F_PAGE_SIZE);
    }

    // 3. last page handle
    uint32_t last_page_start_addr = cuntinue_start_addr + cuntinue_page_num * GD5F_PAGE_SIZE;
    uint32_t last_page_copy_len = (size - first_page_read_len) % GD5F_PAGE_SIZE;

    os_memset(buf, 0, GD5F_PAGE_SIZE);
    bk_qspi_flash_gd5f_standrad_page_read(id, last_page_start_addr, buf, GD5F_PAGE_SIZE);
    os_memcpy((uint8_t *)data + first_page_offset + cuntinue_page_num * GD5F_PAGE_SIZE, buf, last_page_copy_len);

    tkl_system_free(buf);
    buf = NULL;

    return BK_OK;
}

void bk_qspi_flash_gd5f_set_protect_none(qspi_id_t id)
{
    uint8_t status_reg_data = 0;

    status_reg_data = bk_qspi_flash_gd5f_read_status(id) & 0xff;
    uint8_t clean_bits = ~(GD5F_BP_BP0 | GD5F_BP_BP1 | GD5F_BP_BP2);
    status_reg_data &= clean_bits;
    bk_qspi_flash_gd5f_write_block_status(id, status_reg_data);
}

uint32_t bk_qspi_flash_gd5f_read_id(qspi_id_t id)
{
    qspi_cmd_t read_id_cmd = {0};
    uint32_t read_id_data = 0;

    read_id_cmd.device = QSPI_FLASH;
    read_id_cmd.wire_mode = QSPI_1WIRE;
    read_id_cmd.work_mode = INDIRECT_MODE;
    read_id_cmd.op = QSPI_READ;
    read_id_cmd.cmd = GD5F_READ_ID;
    read_id_cmd.addr = 0x00;
    read_id_cmd.addr_valid_bit = QSPI_ADDR_VALID_BIT8;
    read_id_cmd.data_len = 2;

    BK_LOG_ON_ERR(bk_qspi_command(id, &read_id_cmd));

    bk_qspi_read(id, &read_id_data, 2);

    return read_id_data;
}

uint32_t bk_qspi_flash_gd5f_read_test(qspi_id_t id, uint32_t cmd, uint32_t addr, uint32_t addr_len, uint32_t data_len)
{
    qspi_cmd_t read_id_cmd = {0};
    uint32_t read_id_data = 0;

    read_id_cmd.device = QSPI_FLASH;
    read_id_cmd.wire_mode = QSPI_1WIRE;
    read_id_cmd.work_mode = INDIRECT_MODE;
    read_id_cmd.op = QSPI_READ;
    read_id_cmd.cmd = cmd;
    read_id_cmd.addr = addr;
    read_id_cmd.addr_valid_bit = addr_len;
    read_id_cmd.data_len = data_len;

    BK_LOG_ON_ERR(bk_qspi_command(id, &read_id_cmd));

    bk_qspi_read(id, &read_id_data, data_len);

    return read_id_data;
}

#if 0
void qspi_flash_gd5f_test_case(qspi_id_t id, uint32_t base_addr, void *data, uint32_t size)
{
    uint32_t read_id = 0;
    uint32_t *read_data = (uint32_t *)os_zalloc(size);
    uint32_t *origin_data = (uint32_t *)data;
    if (read_data == NULL) {
        QSPI_LOGE("send buffer malloc failed\r\n");
        return;
    }

    read_id = bk_qspi_flash_read_id(id);
    QSPI_LOGI("%s read_id = 0x%x\n", __func__, read_id);

    bk_qspi_flash_set_protect_none(id);
    bk_qspi_flash_quad_enable(id);

    /* quad write, then quad/single read to check data*/
    bk_qspi_flash_erase_sector(id, base_addr);
    bk_qspi_flash_read(id, base_addr, read_data, size);

    for (int i = 0; i < size/4; i++) {
        if(read_data[i] != 0xFFFFFFFF) {
            QSPI_LOGI("[ERASE ERROR]: read_data[%d]=0x%x, should be 0xFFFFFFFF\n", i, read_data[i]);
        }
        QSPI_LOGD("[ERASE DBG]: read_data[%d]=0x%x, should be 0xFFFFFFFF\n", i, read_data[i]);
    }

    bk_qspi_flash_write(id, base_addr, data, size);
    bk_qspi_flash_read(id, base_addr, read_data, size);
    for (int i = 0; i < size/4; i++) {
        if(read_data[i] != origin_data[i]) {
            QSPI_LOGI("[QUAD WRITE - QUAD READ ERROR]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
        }
        QSPI_LOGD("[QUAD WRITE - QUAD READ DBG]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
    }

    bk_qspi_flash_standrad_read(id, base_addr, read_data, size);
    for (int i = 0; i < size/4; i++) {
        if(read_data[i] != origin_data[i]) {
            QSPI_LOGI("[QUAD WRITE - SINGLE READ ERROR]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
        }
        QSPI_LOGD("[QUAD WRITE - SINGLE READ DBG]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
    }

    /* singel write, then single/quad read to check data*/
    bk_qspi_flash_erase_sector(id, base_addr);
    bk_qspi_flash_standrad_read(id, base_addr, read_data, size);
    for (int i = 0; i < size/4; i++) {
        if(read_data[i] != 0xFFFFFFFF) {
            QSPI_LOGI("[ERASE ERROR]: read_data[%d]=0x%x, should be 0xFFFFFFFF\n", i, read_data[i]);
        }
        QSPI_LOGD("[ERASE DBG]: read_data[%d]=0x%x, should be 0xFFFFFFFF\n", i, read_data[i]);
    }

    bk_qspi_flash_standrad_page_program(id, base_addr, data, size);
    bk_qspi_flash_standrad_read(id, base_addr, read_data, size);
    for (int i = 0; i < size/4; i++) {
        if(read_data[i] != origin_data[i]) {
            QSPI_LOGI("[SINGLE WRITE - SINGLE READ ERROR]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
        }
        QSPI_LOGD("[SINGLE WRITE - SINGLE READ DBG]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
    }

    bk_qspi_flash_read(id, base_addr, read_data, size);
    for (int i = 0; i < size/4; i++) {
        if(read_data[i] != origin_data[i]) {
            QSPI_LOGI("[SINGLE WRITE - QUAD READ ERROR]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
        }
        QSPI_LOGD("[SINGLE WRITE - QUAD READ DBG]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
    }

    if (read_data) {
        os_free(read_data);
        read_data = NULL;
    }
}

void test_qspi_flash_gd5f(qspi_id_t id, uint32_t base_addr, uint32_t buf_len)
{
    uint32_t *send_data = (uint32_t *)os_zalloc(buf_len);
    //	uint32_t rand_val = bk_rand() % (0x100000000);

    if (send_data == NULL) {
        QSPI_LOGE("send buffer malloc failed\r\n");
        return;
    }
    for (int i = 0; i < (buf_len/4); i++) {
        send_data[i] = (0x03020100 + i*0x04040404) & 0xffffffff;
    }
    qspi_flash_test_case(id, base_addr, send_data, buf_len);

    if (send_data) {
        os_free(send_data);
        send_data = NULL;
    }
}
#endif


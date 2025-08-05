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

#define FLASH_READ_ID_CMD          0x9F
#define FLASH_WR_S0_S7_CMD         0x1
#define FLASH_WR_S8_S15_CMD        0x31
#define FLASH_RD_S0_S7_CMD         0x5
#define FLASH_RD_S8_S15_CMD        0x35
#define FLASH_WR_EN_CMD            0x6
#define FLASH_WR_CMD               0x2
#define FLASH_RD_CMD               0x3
#define FLASH_QUAD_WR_CMD          0x32
#define FLASH_QUAD_RD_CMD          0xeb
#define FLASH_ERASE_SECTOR_CMD     0x20
#define FLASH_ERASE_32K_CMD        0x52
#define FLASH_ERASE_64K_CMD        0xd8
#define FLASH_PAGE_SIZE            0x100
#define FLASH_PAGE_MASK            (FLASH_PAGE_SIZE - 1)
#define FLASH_SECTOR_SIZE          0x1000

#define FLASH_STATUS_REG_SIZE      1
#define FLASH_READ_ID_SIZE         4
#define QSPI_FIFO_LEN_MAX          256
#define FLASH_QE_DATA              BIT(1)
#define FLASH_PROTECT_NONE_DATA    0
#define QSPI_CMD1_LEN              8

#define QFLASH_MAX_CAPACITY (16*1024*1024)  // unit: byte.
#define QFLASH_SECTOR_SIZE  (4*1024)        // unit: byte.
#define QFLASH_PAGE_SIZE    (256)           // unit: byte.
#define QFLASH_WORD_SIZE    (4)             // unit: byte.

static void bk_qspi_flash_wait_wip_done(qspi_id_t id);

bk_err_t bk_qspi_flash_init(qspi_id_t id)
{
	qspi_config_t config = {0};
	config.src_clk = QSPI_SCLK_480M;
	config.src_clk_div = 0x4;
	config.clk_div = 0x2;
	BK_LOG_ON_ERR(bk_qspi_init(id, &config));

	bk_qspi_flash_set_protect_none(id);
	bk_qspi_flash_quad_enable(id);

	return BK_OK;
}

bk_err_t bk_qspi_flash_deinit(qspi_id_t id)
{
	BK_LOG_ON_ERR(bk_qspi_deinit(id));
	return BK_OK;
}

static void bk_qspi_flash_wren(qspi_id_t id) {
	qspi_cmd_t wren_cmd = {0};

	wren_cmd.device = QSPI_FLASH;
	wren_cmd.wire_mode = QSPI_1WIRE;
	wren_cmd.work_mode = INDIRECT_MODE;
	wren_cmd.op = QSPI_WRITE;
	wren_cmd.cmd = FLASH_WR_EN_CMD;
	wren_cmd.data_len = 0;

	BK_LOG_ON_ERR(bk_qspi_command(id, &wren_cmd));
	bk_qspi_flash_wait_wip_done(id);
}

uint32_t bk_qspi_flash_read_s0_s7(qspi_id_t id)
{
	qspi_cmd_t read_status_cmd = {0};
	uint32_t status_reg_data = 0;

	read_status_cmd.device = QSPI_FLASH;
	read_status_cmd.wire_mode = QSPI_1WIRE;
	read_status_cmd.work_mode = INDIRECT_MODE;
	read_status_cmd.op = QSPI_READ;
	read_status_cmd.cmd = FLASH_RD_S0_S7_CMD;
	read_status_cmd.data_len = FLASH_STATUS_REG_SIZE;

	BK_LOG_ON_ERR(bk_qspi_command(id, &read_status_cmd));
	bk_qspi_read(id, &status_reg_data, FLASH_STATUS_REG_SIZE);
	QSPI_LOGV("[%s]: status_reg_data = 0x%x.\n", __func__, (uint8_t)status_reg_data);

	return status_reg_data;
}

uint32_t bk_qspi_flash_read_s8_s15(qspi_id_t id)
{
	qspi_cmd_t read_status_cmd = {0};
	uint32_t status_reg_data = 0;

	read_status_cmd.device = QSPI_FLASH;
	read_status_cmd.wire_mode = QSPI_1WIRE;
	read_status_cmd.work_mode = INDIRECT_MODE;
	read_status_cmd.op = QSPI_READ;
	read_status_cmd.cmd = FLASH_RD_S8_S15_CMD;
	read_status_cmd.data_len = FLASH_STATUS_REG_SIZE;

	BK_LOG_ON_ERR(bk_qspi_command(id, &read_status_cmd));
	bk_qspi_read(id, &status_reg_data, FLASH_STATUS_REG_SIZE);
	QSPI_LOGV("[%s]: status_reg_data = 0x%x.\n", __func__, (uint8_t)status_reg_data);

	return status_reg_data;
}

bk_err_t bk_qspi_flash_write_s0_s7(qspi_id_t id, uint8_t status_reg_data)
{
	qspi_cmd_t write_status_cmd = {0};

	bk_qspi_flash_wren(id);
	write_status_cmd.device = QSPI_FLASH;
	write_status_cmd.wire_mode = QSPI_1WIRE;
	write_status_cmd.work_mode = INDIRECT_MODE;
	write_status_cmd.op = QSPI_WRITE;
	write_status_cmd.cmd = (status_reg_data << QSPI_CMD1_LEN) | FLASH_WR_S0_S7_CMD;
	write_status_cmd.data_len = 0;

	BK_LOG_ON_ERR(bk_qspi_command(id, &write_status_cmd));
	QSPI_LOGD("[%s]: status_reg_data to be writen is 0x%x.\n", __func__, status_reg_data);
	bk_qspi_flash_wait_wip_done(id);

	return BK_OK;
}

bk_err_t bk_qspi_flash_write_s8_s15(qspi_id_t id, uint8_t status_reg_data)
{
	qspi_cmd_t write_status_cmd = {0};
	bk_qspi_flash_wren(id);
	write_status_cmd.device = QSPI_FLASH;
	write_status_cmd.wire_mode = QSPI_1WIRE;
	write_status_cmd.work_mode = INDIRECT_MODE;
	write_status_cmd.op = QSPI_WRITE;
	write_status_cmd.cmd = (status_reg_data << QSPI_CMD1_LEN) | FLASH_WR_S8_S15_CMD;
	write_status_cmd.data_len = 0;

	BK_LOG_ON_ERR(bk_qspi_command(id, &write_status_cmd));
	QSPI_LOGD("[%s]: status_reg_data to be writen is 0x%x.\n", __func__, status_reg_data);
	bk_qspi_flash_wait_wip_done(id);

	return BK_OK;
}

bk_err_t bk_qspi_flash_write_s0_s15(qspi_id_t id, uint16_t status_reg_data)
{
	qspi_cmd_t write_status_cmd = {0};

	bk_qspi_flash_wren(id);
	write_status_cmd.device = QSPI_FLASH;
	write_status_cmd.wire_mode = QSPI_1WIRE;
	write_status_cmd.work_mode = INDIRECT_MODE;
	write_status_cmd.op = QSPI_WRITE;
	write_status_cmd.cmd = (status_reg_data << QSPI_CMD1_LEN) | FLASH_WR_S0_S7_CMD;
	write_status_cmd.data_len = 0;

	BK_LOG_ON_ERR(bk_qspi_command(id, &write_status_cmd));
	QSPI_LOGD("[%s]: status_reg_data to be writen is 0x%x.\n", __func__, status_reg_data);
	QSPI_LOGD("[%s]: write_status_cmd.cmd is 0x%x.\n", __func__, write_status_cmd.cmd);
	bk_qspi_flash_wait_wip_done(id);

	return BK_OK;
}

static void bk_qspi_flash_wait_wip_done(qspi_id_t id)
{
	uint32_t status_reg_data = 0;

	for(int i = 0; i <= 2000; i++) {
		status_reg_data = bk_qspi_flash_read_s0_s7(id);
		if(0 == (status_reg_data & BIT(0))) {
			break;
		}
		if(i == 2000) {
			QSPI_LOGW("[%s]: wait write_in_progress done timeout.\n", __func__);
		}
		rtos_delay_milliseconds(1);
	}
}

bk_err_t bk_qspi_flash_erase_sector(qspi_id_t id, uint32_t addr)
{
	qspi_cmd_t erase_sector_cmd = {0};

	bk_qspi_flash_wren(id);

	erase_sector_cmd.device = QSPI_FLASH;
	erase_sector_cmd.wire_mode = QSPI_1WIRE;
	erase_sector_cmd.work_mode = INDIRECT_MODE;
	erase_sector_cmd.op = QSPI_WRITE;
	erase_sector_cmd.cmd = FLASH_ERASE_SECTOR_CMD;
	erase_sector_cmd.addr = addr;

	BK_LOG_ON_ERR(bk_qspi_command(id, &erase_sector_cmd));
	bk_qspi_flash_wait_wip_done(id);

	return BK_OK;
}

bk_err_t bk_qspi_flash_erase_32k(qspi_id_t id, uint32_t addr)
{
	qspi_cmd_t erase_sector_cmd = {0};

	bk_qspi_flash_wren(id);

	erase_sector_cmd.device = QSPI_FLASH;
	erase_sector_cmd.wire_mode = QSPI_1WIRE;
	erase_sector_cmd.work_mode = INDIRECT_MODE;
	erase_sector_cmd.op = QSPI_WRITE;
	erase_sector_cmd.cmd = FLASH_ERASE_32K_CMD;
	erase_sector_cmd.addr = addr;

	BK_LOG_ON_ERR(bk_qspi_command(id, &erase_sector_cmd));
	bk_qspi_flash_wait_wip_done(id);

	return BK_OK;
}

bk_err_t bk_qspi_flash_erase_64k(qspi_id_t id, uint32_t addr)
{
	qspi_cmd_t erase_sector_cmd = {0};

	bk_qspi_flash_wren(id);

	erase_sector_cmd.device = QSPI_FLASH;
	erase_sector_cmd.wire_mode = QSPI_1WIRE;
	erase_sector_cmd.work_mode = INDIRECT_MODE;
	erase_sector_cmd.op = QSPI_WRITE;
	erase_sector_cmd.cmd = FLASH_ERASE_64K_CMD;
	erase_sector_cmd.addr = addr;

	BK_LOG_ON_ERR(bk_qspi_command(id, &erase_sector_cmd));
	bk_qspi_flash_wait_wip_done(id);

	return BK_OK;
}

bk_err_t bk_qspi_flash_erase_type(qspi_id_t id, uint32_t addr, uint32_t type)
{
	bk_err_t ret = BK_OK;

	switch(type)
	{
		case FLASH_ERASE_SECTOR_CMD:
			bk_qspi_flash_erase_sector(id, addr);
		break;
		case FLASH_ERASE_32K_CMD:
			bk_qspi_flash_erase_32k(id, addr);
			break;
		case FLASH_ERASE_64K_CMD:
			bk_qspi_flash_erase_64k(id, addr);
		break;
		default:
			bk_qspi_flash_erase_sector(id, addr);
		break;
	}

	return ret;
}

#if 0
//bk_err_t qflash_erase(uint32_t addr, uint32_t size)
bk_err_t bk_qspi_flash_erase(qspi_id_t id, uint32_t addr, uint32_t size)
{
    QSPI_LOGV("[%s]addr=0x%08X, size=%u.\r\n", __func__, addr, size);

    if ( (addr >= QFLASH_MAX_CAPACITY) || (size > QFLASH_MAX_CAPACITY) || ((addr + size) > QFLASH_MAX_CAPACITY) )
    {
        QSPI_LOGE("[%s] addr or size paras error!\r\n", __func__);
        return BK_FAIL;
    }

    if ( 0 == size )
    {
        QSPI_LOGE("[%s] buff or size paras error!\r\n", __func__);
        return BK_FAIL;
    }

    bk_err_t ret = BK_OK;
    uint32_t temp;

    for (temp=0; temp<size; temp+=QFLASH_SECTOR_SIZE)
    {
        #if (CONFIG_TASK_WDT)
            extern void bk_task_wdt_feed(void);
            bk_task_wdt_feed();
        #endif

        ret = bk_qspi_flash_erase_sector(id, addr+temp);
        if (BK_OK != ret)
        {
            QSPI_LOGE("[%s] bk_qspi_flash_erase_sector fail!\r\n", __func__);
            return BK_FAIL;
        }
    }

    QSPI_LOGV("[%s] erase sector range[%u ~ %u].\r\n", __func__, addr/QFLASH_SECTOR_SIZE, addr/QFLASH_SECTOR_SIZE + temp/QFLASH_SECTOR_SIZE - 1);

    return ret;
}
#endif

bk_err_t bk_qspi_flash_erase(qspi_id_t id, uint32_t addr, uint32_t size)
{
    bk_err_t ret = BK_OK;

    QSPI_LOGV("[%s]addr=0x%08X, size=%u.\r\n", __func__, addr, size);

    if ( (addr >= QFLASH_MAX_CAPACITY) || (size > QFLASH_MAX_CAPACITY) || ((addr + size) > QFLASH_MAX_CAPACITY) )
    {
        QSPI_LOGE("[%s] addr or size paras error!\r\n", __func__);
        return BK_FAIL;
    }

    if ( 0 == size )
    {
        QSPI_LOGE("[%s] buff or size paras error!\r\n", __func__);
        return BK_FAIL;
    }

    int left_size = (int)size;
    while (left_size > 0)
    {
        uint32_t erase_size = 0, erase_mode;

        #if (CONFIG_TASK_WDT)
            extern void bk_task_wdt_feed(void);
            bk_task_wdt_feed();
        #endif

        if(left_size <= 4 * 1024)
        {
            erase_size = 4 * 1024;
            erase_mode = FLASH_ERASE_SECTOR_CMD;
        }
        else if(size <= 32 * 1024)
        {
            erase_size = 32 * 1024;
            erase_mode = FLASH_ERASE_32K_CMD;
        }
        else
        {
            erase_size = 64 * 1024;
            erase_mode = FLASH_ERASE_64K_CMD;
        }

        ret = bk_qspi_flash_erase_type(id, addr, erase_mode);
        if (BK_OK != ret)
        {
            QSPI_LOGE("[%s] bk_qspi_flash_erase_type fail, addr=0x%x, erase_mode=%d.\r\n", __func__, addr, erase_mode);
            return BK_FAIL;
        }

        if(addr & (erase_size - 1))
        {
            size = erase_size - (addr & (erase_size - 1));
        }
        else
        {
            size = erase_size;
        }

        QSPI_LOGV("addr:%d,erase_mode:%d,left_size:%d,size:%d\r\n",addr,erase_mode,left_size,size);

        left_size -= size;
        addr += size;
    }

    return BK_OK;
}


bk_err_t bk_qspi_flash_single_page_program(qspi_id_t id, uint32_t addr, const void *data, uint32_t size)
{
	qspi_cmd_t page_program_cmd = {0};
	uint32_t cmd_data_len = 0;
	uint32_t *cmd_data = (uint32_t *)data;

	page_program_cmd.device = QSPI_FLASH;
	page_program_cmd.wire_mode = QSPI_1WIRE;
	page_program_cmd.work_mode = INDIRECT_MODE;
	page_program_cmd.op = QSPI_WRITE;
	page_program_cmd.cmd = FLASH_WR_CMD;

	while(0 < size) {
		bk_qspi_flash_wren(id);

		cmd_data_len = (size < QSPI_FIFO_LEN_MAX) ? size : QSPI_FIFO_LEN_MAX;
		bk_qspi_write(id, cmd_data, cmd_data_len);
		page_program_cmd.addr = addr;
		page_program_cmd.data_len = cmd_data_len;
		BK_LOG_ON_ERR(bk_qspi_command(id, &page_program_cmd));
		addr += cmd_data_len;
		cmd_data += cmd_data_len;
		size -= cmd_data_len;

		bk_qspi_flash_wait_wip_done(id);
	}

	return BK_OK;
}

bk_err_t bk_qspi_flash_single_read(qspi_id_t id, uint32_t addr, void *data, uint32_t size)
{
	qspi_cmd_t single_read_cmd = {0};
	uint32_t cmd_data_len = 0;
	uint32_t *cmd_data = data;

	single_read_cmd.device = QSPI_FLASH;
	single_read_cmd.wire_mode = QSPI_1WIRE;
	single_read_cmd.work_mode = INDIRECT_MODE;
	single_read_cmd.op = QSPI_READ;
	single_read_cmd.cmd = FLASH_RD_CMD;

	while(0 < size) {
		cmd_data_len = (size < QSPI_FIFO_LEN_MAX) ? size : QSPI_FIFO_LEN_MAX;
		single_read_cmd.addr = addr;
		single_read_cmd.data_len = cmd_data_len;
		BK_LOG_ON_ERR(bk_qspi_command(id, &single_read_cmd));
		bk_qspi_read(id, cmd_data, cmd_data_len);

		addr += cmd_data_len;
		cmd_data += cmd_data_len;
		size -= cmd_data_len;
	}
	return BK_OK;
}

bk_err_t bk_qspi_flash_quad_page_program(qspi_id_t id, uint32_t addr, const void *data, uint32_t size)
{
	qspi_cmd_t page_program_cmd = {0};
	uint32_t cmd_data_len = 0;
	uint32_t *cmd_data = (uint32_t *)data;

	page_program_cmd.device = QSPI_FLASH;
	page_program_cmd.wire_mode = QSPI_4WIRE;
	page_program_cmd.work_mode = INDIRECT_MODE;
	page_program_cmd.op = QSPI_WRITE;
	page_program_cmd.cmd = FLASH_QUAD_WR_CMD;

	while(0 < size) {
		bk_qspi_flash_wren(id);

		cmd_data_len = (size < QSPI_FIFO_LEN_MAX) ? size : QSPI_FIFO_LEN_MAX;
		bk_qspi_write(id, cmd_data, cmd_data_len);
		page_program_cmd.addr = addr;
		page_program_cmd.data_len = cmd_data_len;
		BK_LOG_ON_ERR(bk_qspi_command(id, &page_program_cmd));

		addr += cmd_data_len;
		cmd_data += cmd_data_len;
		size -= cmd_data_len;

		bk_qspi_flash_wait_wip_done(id);
	}

	return BK_OK;
}

bk_err_t bk_qspi_flash_quad_read(qspi_id_t id, uint32_t addr, void *data, uint32_t size)
{
	qspi_cmd_t quad_read_cmd = {0};
	uint32_t cmd_data_len = 0;
	uint32_t *cmd_data = data;

	quad_read_cmd.device = QSPI_FLASH;
	quad_read_cmd.wire_mode = QSPI_4WIRE;
	quad_read_cmd.work_mode = INDIRECT_MODE;
	quad_read_cmd.op = QSPI_READ;
	quad_read_cmd.cmd = FLASH_QUAD_RD_CMD;
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

#if 1
bk_err_t bk_qspi_flash_write(qspi_id_t id, uint32_t base_addr, const void *data, uint32_t size)
{
	uint8_t buf[QSPI_FIFO_LEN_MAX] = {0};
	uint32_t left_len = size;
	uint32_t write_len= 0;
	uint32_t write_addr = 0;
	uint32_t offset = 0;
	uint32_t page_write_len = 0;


	if(0 != (base_addr & FLASH_PAGE_MASK)) {
		write_addr = base_addr & (~FLASH_PAGE_MASK);
		bk_qspi_flash_quad_read(id, write_addr, buf, QSPI_FIFO_LEN_MAX);
		page_write_len = (QSPI_FIFO_LEN_MAX - (base_addr & FLASH_PAGE_MASK));
		write_len = (page_write_len > left_len) ? left_len : page_write_len;
		os_memcpy(buf + (base_addr & FLASH_PAGE_MASK), data, write_len);
		bk_qspi_flash_quad_page_program(id, write_addr, buf, QSPI_FIFO_LEN_MAX);
		offset += write_len;
		left_len -= write_len;
		if(left_len == 0) {
			return BK_OK;
		}
	}

	for (write_addr = base_addr + offset; write_addr < ((base_addr + size) & (~FLASH_PAGE_MASK)); write_addr += write_len) {
		write_len = (left_len > QSPI_FIFO_LEN_MAX) ? QSPI_FIFO_LEN_MAX : left_len;
		bk_qspi_flash_quad_page_program(id, write_addr, data + offset, write_len);
		offset += write_len;
		left_len -= write_len;
	}

	if(left_len == 0) {
		return BK_OK;
	}
	if(0 != ((base_addr + size) & FLASH_PAGE_MASK)) {
		bk_qspi_flash_quad_read(id, write_addr, buf, QSPI_FIFO_LEN_MAX);
		write_len = (base_addr + size) & FLASH_PAGE_MASK;
		os_memcpy(buf, data + offset, write_len);
		bk_qspi_flash_quad_page_program(id, write_addr, buf, write_len);
	}

	return BK_OK;
}
#else
//bk_err_t qflash_write(qspi_id_t id, uint32_t addr, const uint8_t *buff, uint32_t size)
bk_err_t bk_qspi_flash_write(qspi_id_t id, uint32_t addr, const void *buff, uint32_t size)
{
    QSPI_LOGV("[%s] addr=0x%08X, buff=0x%08X, size=%u.\r\n", __func__, addr, buff, size);

    bk_err_t ret = BK_OK;

    if ( (addr >= QFLASH_MAX_CAPACITY) || (size > QFLASH_MAX_CAPACITY) || ((addr + size) > QFLASH_MAX_CAPACITY) )
    {
        QSPI_LOGE("[%s] addr or size paras error!\r\n", __func__);
        return BK_FAIL;
    }

    if ( (NULL==buff) || (0 == size) )
    {
        QSPI_LOGE("[%s] buff or size paras error!\r\n", __func__);
        return BK_FAIL;
    }

    uint32_t head_addr = 0, head_size = 0;
    uint32_t mid_addr = 0, mid_size = 0;
    uint32_t tail_addr = 0, tail_size = 0;
    uint32_t head_frag_addr = 0, head_frag_size = 0;
    uint32_t tail_frag_addr = 0, tail_frag_size = 0;

    if (addr%QFLASH_PAGE_SIZE)
    {
        head_addr = addr;
        head_size = ( size > (QFLASH_PAGE_SIZE - (addr%QFLASH_PAGE_SIZE)) )?  (QFLASH_PAGE_SIZE - (addr%QFLASH_PAGE_SIZE)) : size;
        size -= head_size;
    }

    if (size >= QFLASH_PAGE_SIZE)
    {
        mid_addr = addr + head_size;
        mid_size = (size / QFLASH_PAGE_SIZE) * QFLASH_PAGE_SIZE;
        size -= mid_size;
    }

    if (size > 0)
    {
        tail_addr = addr + head_size + mid_size;
        tail_size = size;
    }

    QSPI_LOGV("[%s] h_addr=0x%08X, h_size=%u, m_addr=0x%08X, m_size=%u, t_addr=0x%08X, t_size=%u.\r\n", __func__, head_addr, head_size, mid_addr, mid_size, tail_addr, tail_size);

    if (head_size)
    {
        if (head_addr % QFLASH_WORD_SIZE)
        {
            head_frag_addr = (head_addr/QFLASH_WORD_SIZE)*QFLASH_WORD_SIZE;
            head_frag_size = (head_size > (QFLASH_WORD_SIZE - (head_addr % QFLASH_WORD_SIZE)))? (QFLASH_WORD_SIZE - (head_addr % QFLASH_WORD_SIZE)) : head_size;
            head_size -= head_frag_size;
        }

        QSPI_LOGV("[%s] hf_addr=0x%08X, hf_size=%u, h_addr=0x%08X, h_size=%u.\r\n", __func__, head_frag_addr, head_frag_size, head_addr+head_frag_size, head_size);

        #if (CONFIG_TASK_WDT)
            extern void bk_task_wdt_feed(void);
            bk_task_wdt_feed();
        #endif

        if (head_frag_size > 0)
        {
            uint8_t temp_word[QFLASH_WORD_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF};

            ret = bk_qspi_flash_read(id, head_frag_addr, temp_word, sizeof(temp_word));
            if (BK_OK != ret)
            {
                QSPI_LOGE("[%s] qflash_read head frag fail[ret=%d]!\r\n", __func__, ret);
                return ret;
            }

            os_memcpy((uint8_t *)(temp_word + (head_addr % QFLASH_WORD_SIZE)), (uint8_t *)buff, head_frag_size);
            ret = bk_qspi_flash_write(id, head_frag_addr, temp_word, sizeof(temp_word));
            if (BK_OK != ret)
            {
                QSPI_LOGE("[%s] bk_qspi_flash_write head fail[ret=%d]!\r\n", __func__, ret);
                return ret;
            }
        }

        if (head_size > 0)
        {
            ret = bk_qspi_flash_write(id, head_addr+head_frag_size, buff+head_frag_size, head_size);
            if (BK_OK != ret)
            {
                QSPI_LOGE("[%s] bk_qspi_flash_write head fail[ret=%d]!\r\n", __func__, ret);
                return ret;
            }
        }
    }

    if (mid_size)
    {
        for (uint32_t offset=0; offset<mid_size; offset+=QFLASH_PAGE_SIZE)
        {
            #if (CONFIG_TASK_WDT)
                extern void bk_task_wdt_feed(void);
                bk_task_wdt_feed();
            #endif

            ret = bk_qspi_flash_write(id, mid_addr+offset, buff+head_frag_size+head_size+offset, QFLASH_PAGE_SIZE);
            if (BK_OK != ret)
            {
                QSPI_LOGE("[%s] bk_qspi_flash_write middle fail[ret=%d, offset=%d]!\r\n", __func__, ret, offset);
                return ret;
            }
        }
    }

    if (tail_size)
    {
        if (tail_size % QFLASH_WORD_SIZE)
        {
            tail_frag_size = tail_size % QFLASH_WORD_SIZE;
            tail_size -= tail_frag_size;
            tail_frag_addr = tail_addr + tail_size;
        }

        QSPI_LOGV("[%s] t_addr=0x%08X, t_size=%u, tf_addr=0x%08X, tf_size=%u.\r\n", __func__, tail_addr, tail_size, tail_frag_addr, tail_frag_size);

        #if (CONFIG_TASK_WDT)
            extern void bk_task_wdt_feed(void);
            bk_task_wdt_feed();
        #endif

        if (tail_size > 0)
        {
            ret = bk_qspi_flash_write(id, tail_addr, buff+head_frag_size+head_size+mid_size, tail_size);
            if (BK_OK != ret)
            {
                QSPI_LOGE("[%s] bk_qspi_flash_write tail fail[ret=%d]!\r\n", __func__, ret);
                return ret;
            }
        }

        if (tail_frag_size > 0)
        {
            uint8_t temp_word[QFLASH_WORD_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF};

            ret = bk_qspi_flash_read(id, tail_frag_addr, temp_word, sizeof(temp_word));
            if (BK_OK != ret)
            {
                QSPI_LOGE("[%s] qflash_read tail frag fail[ret=%d]!\r\n", __func__, ret);
                return ret;
            }

            os_memcpy((uint8_t *)(temp_word), (uint8_t *)(buff+head_frag_size+head_size+mid_size+tail_size), tail_frag_size);
            ret = bk_qspi_flash_write(id, tail_frag_addr, temp_word, sizeof(temp_word));
            if (BK_OK != ret)
            {
                QSPI_LOGE("[%s] bk_qspi_flash_write tail frag fail[ret=%d]!\r\n", __func__, ret);
                return ret;
            }
        }
    }

    return ret;
}
#endif

#if 1
bk_err_t bk_qspi_flash_read(qspi_id_t id, uint32_t base_addr, void *data, uint32_t size)
{
	uint8_t buf[QSPI_FIFO_LEN_MAX] = {0};
	uint32_t left_len = size;
	uint32_t read_len= 0;
	uint32_t offset = 0;

	for (uint32_t addr = base_addr; addr < (base_addr + size); addr += QSPI_FIFO_LEN_MAX) {
		offset += read_len;
		read_len = (left_len >= QSPI_FIFO_LEN_MAX) ? QSPI_FIFO_LEN_MAX : left_len;
		bk_qspi_flash_quad_read(id, addr, buf, QSPI_FIFO_LEN_MAX);
		os_memcpy(data + offset, buf, read_len);
		left_len -= QSPI_FIFO_LEN_MAX;
	}

	return BK_OK;
}
#else
//bk_err_t qflash_read(uint32_t addr, uint8_t *buff, uint32_t size)
bk_err_t bk_qspi_flash_read(qspi_id_t id, uint32_t base_addr, void *data, uint32_t size)
{
    QSPI_LOGV("[%s] base_addr=0x%08X, data=0x%08X, size=%u.\r\n", __func__, base_addr, data, size);

    bk_err_t ret = BK_OK;

    if ( (base_addr >= QFLASH_MAX_CAPACITY) || (size > QFLASH_MAX_CAPACITY) || ((base_addr + size) > QFLASH_MAX_CAPACITY) )
    {
        QSPI_LOGE("[%s] base_addr or size paras error!\r\n", __func__);
        return BK_FAIL;
    }

    if ( (NULL==data) || (0 == size) )
    {
        QSPI_LOGE("[%s] data or size paras error!\r\n", __func__);
        return BK_FAIL;
    }

    uint32_t total_read_size = 0;
    uint32_t word_read_size = (size/QFLASH_WORD_SIZE)*QFLASH_WORD_SIZE;
    uint32_t tail_read_size = size - word_read_size;

    if (word_read_size > 0)
    {
        uint32_t temp_size = 0;
        for (uint32_t offset=0; offset<word_read_size; offset+=QFLASH_PAGE_SIZE)
        {
            #if (CONFIG_TASK_WDT)
                extern void bk_task_wdt_feed(void);
                bk_task_wdt_feed();
            #endif

            temp_size = ((word_read_size - offset) >= QFLASH_PAGE_SIZE)? QFLASH_PAGE_SIZE : (word_read_size - offset);

            ret = bk_qspi_flash_quad_read(id, base_addr+offset, (uint8_t *)data+offset, temp_size);
            if (BK_OK != ret)
            {
                QSPI_LOGE("[%s] bk_qspi_flash_quad_read fail[ret=%d]!\r\n", __func__, ret);
                return BK_FAIL;
            }
            else
            {
                total_read_size += temp_size;
            }
        }
    }
    
    if (tail_read_size)
    {
        #if (CONFIG_TASK_WDT)
            extern void bk_task_wdt_feed(void);
            bk_task_wdt_feed();
        #endif

        uint8_t temp_word[QFLASH_WORD_SIZE] = {0};

        ret = bk_qspi_flash_quad_read(id, base_addr+total_read_size, temp_word, sizeof(temp_word));
        if (BK_OK != ret)
        {
            QSPI_LOGE("[%s] bk_qspi_flash_quad_read fail[ret=%d]!\r\n", __func__, ret);
            return BK_FAIL;
        }
        else
        {
            os_memcpy((uint8_t *)((uint8_t *)data+total_read_size), (uint8_t *)temp_word, tail_read_size);
            total_read_size += tail_read_size;
        }
    }

    return ret;
}
#endif

void bk_qspi_flash_quad_enable(qspi_id_t id) {
	uint32_t status_reg_data = 0;

	status_reg_data = (uint8_t)bk_qspi_flash_read_s8_s15(id);
	status_reg_data |= FLASH_QE_DATA;
#if CONFIG_QSPI_FLASH_GD
	bk_qspi_flash_write_s8_s15(id, status_reg_data);
#else
	status_reg_data = (status_reg_data << 8);
	bk_qspi_flash_write_s0_s15(id, status_reg_data);
#endif
	bk_qspi_flash_read_s8_s15(id);
}

void bk_qspi_flash_set_protect_none(qspi_id_t id) {
	uint32_t status_reg_data = 0;

	status_reg_data = bk_qspi_flash_read_s0_s7(id);
	status_reg_data &= FLASH_PROTECT_NONE_DATA;
	bk_qspi_flash_write_s0_s7(id, status_reg_data);
}

uint32_t bk_qspi_flash_read_id(qspi_id_t id) {
	qspi_cmd_t read_id_cmd = {0};
	uint32_t read_id_data = 0;

	read_id_cmd.device = QSPI_FLASH;
	read_id_cmd.wire_mode = QSPI_1WIRE;
	read_id_cmd.work_mode = INDIRECT_MODE;
	read_id_cmd.op = QSPI_READ;
	read_id_cmd.cmd = FLASH_READ_ID_CMD;
	read_id_cmd.data_len = FLASH_READ_ID_SIZE;

	BK_LOG_ON_ERR(bk_qspi_command(id, &read_id_cmd));
	
	bk_qspi_read(id, &read_id_data, FLASH_READ_ID_SIZE);

	return read_id_data;
}

void qspi_flash_test_case(qspi_id_t id, uint32_t base_addr, void *data, uint32_t size)
{
	uint32_t read_id = 0;
	uint32_t *read_data = (uint32_t *)os_zalloc(size);
	uint32_t *origin_data = (uint32_t *)data;
	if (read_data == NULL) {
		QSPI_LOGE("send buffer malloc failed\r\n");
		return;
	}

	read_id = bk_qspi_flash_read_id(id);
	QSPI_LOGD("%s read_id = 0x%x\n", __func__, read_id);

	bk_qspi_flash_set_protect_none(id);
	bk_qspi_flash_quad_enable(id);

	/* quad write, then quad/single read to check data*/
	bk_qspi_flash_erase_sector(id, base_addr);
	bk_qspi_flash_read(id, base_addr, read_data, size);

	for (int i = 0; i < size/4; i++) {
		if(read_data[i] != 0xFFFFFFFF) {
			QSPI_LOGD("[ERASE ERROR]: read_data[%d]=0x%x, should be 0xFFFFFFFF\n", i, read_data[i]);
		}
		QSPI_LOGV("[ERASE DBG]: read_data[%d]=0x%x, should be 0xFFFFFFFF\n", i, read_data[i]);
	}

	bk_qspi_flash_write(id, base_addr, data, size);
	bk_qspi_flash_read(id, base_addr, read_data, size);
	for (int i = 0; i < size/4; i++) {
		if(read_data[i] != origin_data[i]) {
			QSPI_LOGD("[QUAD WRITE - QUAD READ ERROR]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
		}
		QSPI_LOGV("[QUAD WRITE - QUAD READ DBG]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
	}

	bk_qspi_flash_single_read(id, base_addr, read_data, size);
	for (int i = 0; i < size/4; i++) {
		if(read_data[i] != origin_data[i]) {
			QSPI_LOGD("[QUAD WRITE - SINGLE READ ERROR]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
		}
		QSPI_LOGV("[QUAD WRITE - SINGLE READ DBG]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
	}

	/* singel write, then single/quad read to check data*/
	bk_qspi_flash_erase_sector(id, base_addr);
	bk_qspi_flash_single_read(id, base_addr, read_data, size);
	for (int i = 0; i < size/4; i++) {
		if(read_data[i] != 0xFFFFFFFF) {
			QSPI_LOGD("[ERASE ERROR]: read_data[%d]=0x%x, should be 0xFFFFFFFF\n", i, read_data[i]);
		}
		QSPI_LOGV("[ERASE DBG]: read_data[%d]=0x%x, should be 0xFFFFFFFF\n", i, read_data[i]);
	}

	bk_qspi_flash_single_page_program(id, base_addr, data, size);
	bk_qspi_flash_single_read(id, base_addr, read_data, size);
	for (int i = 0; i < size/4; i++) {
		if(read_data[i] != origin_data[i]) {
			QSPI_LOGD("[SINGLE WRITE - SINGLE READ ERROR]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
		}
		QSPI_LOGV("[SINGLE WRITE - SINGLE READ DBG]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
	}

	bk_qspi_flash_read(id, base_addr, read_data, size);
	for (int i = 0; i < size/4; i++) {
		if(read_data[i] != origin_data[i]) {
			QSPI_LOGD("[SINGLE WRITE - QUAD READ ERROR]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
		}
		QSPI_LOGV("[SINGLE WRITE - QUAD READ DBG]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
	}

	if (read_data) {
		os_free(read_data);
		read_data = NULL;
	}
}

void test_qspi_flash(qspi_id_t id, uint32_t base_addr, uint32_t buf_len)
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

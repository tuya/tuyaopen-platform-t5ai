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

#include <os/os.h>
#include <driver/spi.h>
#include <driver/spi_flash.h>
#include <driver/dma.h>
#include <os/mem.h>
#include "spi_driver.h"

#define FLASH_PHY_PAGE_SIZE       256
#define FLASH_PHY_SECTOR_SIZE     4096
#define FLASH_PHY_BLK_32K         (32*1024)
#define FLASH_PHY_BLK_64K         (64*1024)

#define CMD_READ_ID               0x9F
#define READ_ID_RESPONE_LEN       3

#define CMD_READ_STATUS_S7_0      0x05
#define CMD_READ_STATUS_S15_8     0x35
#define READ_STATUS_LEN           1
#define FLASH_STATUS_WIP_BIT      (1 << 0)
#define FLASH_STATUS_WEL_BIT      (1 << 1)
#define FLASH_STATUS_PRETECT_MASK (0x3F << 2)

#define CMD_WRITE_STATUS          0x01
#define ERASE_MODE_ALL            0x01
#define ERASE_MODE_BLOCK_64K      0x02
#define ERASE_MODE_BLOCK_32K      0x03
#define ERASE_MODE_SECTOR         0x04
#define CMD_ERASE_ALL             0xc7  // 0x60
#define CMD_ERASE_BLK_64K         0xD8
#define CMD_ERASE_BLK_32K         0x52
#define CMD_ERASE_SECTOR          0x20
#define DELAY_WHEN_BUSY_MS        0
#define CMD_READ_DATA             0x03
#define CMD_PAGE_PROG             0x02
#define CMD_WRITE_ENABLE          0x06
#define CMD_WRITE_DISABLE         0x04
#define SFLASH_SPI_BAUD_RATE      13000000

struct spi_message
{
    uint8_t*send_buf;
    uint32_t send_len;

    uint8_t*recv_buf;
    uint32_t recv_len;
};

static int bk_spi_master_xfer(spi_id_t id, struct spi_message *msg)
{
	SPI_LOGV("tx_size:%d,rx_size:%d\r\n", msg->send_len, msg->recv_len);
	uint32_t buf_len = msg->send_len + msg->recv_len;

	uint8_t *send_data = (uint8_t *)os_zalloc(buf_len);
	if (send_data == NULL) {
		SPI_LOGE("send buffer zalloc failed\r\n");
		return BK_FAIL;
	}
	uint8_t *recv_data = (uint8_t *)os_zalloc(buf_len);
	if (recv_data == NULL) {
		if (send_data) {
			os_free(send_data);
			send_data = NULL;
		}
		SPI_LOGE("recv buffer zalloc failed\r\n");
		return BK_FAIL;
	}

	for (int i = 0; i < msg->send_len; i++) {
		send_data[i] = *(msg->send_buf + i);
	}

#if CONFIG_SPI_DMA
	bk_spi_dma_duplex_init(id);
	BK_LOG_ON_ERR(bk_spi_dma_duplex_xfer(id, send_data, buf_len, recv_data, buf_len));
	bk_spi_dma_duplex_deinit(id);
#endif

	for (int i = 0; i < msg->recv_len; i++) {
		msg->recv_buf[i] = *(recv_data + msg->send_len + i);
	}

	if (recv_data) {
		os_free(recv_data);
		recv_data = NULL;
	}

	if (send_data) {
		os_free(send_data);
		send_data = NULL;
	}

	return BK_OK;
}

uint32_t bk_spi_flash_read_id(spi_id_t id)
{
    uint32_t uid = 0;
    uint8_t uid_buf[READ_ID_RESPONE_LEN] = {0};
    uint8_t uid_cmd[] = {CMD_READ_ID};
    struct spi_message msg = {0};

    os_memset(&msg, 0, sizeof(struct spi_message));
    msg.send_buf = uid_cmd;
    msg.send_len = sizeof(uid_cmd);
    msg.recv_buf = uid_buf;
    msg.recv_len = READ_ID_RESPONE_LEN;

    if(bk_spi_master_xfer(id, &msg))
		return 0;

    uid = (uid_buf[0] << 16) | (uid_buf[1] << 8) | (uid_buf[2]);

    SPI_LOGD("spi flash id:%08x\r\n", uid);

    return uid;
}

static uint32_t spi_flash_is_busy(spi_id_t id)
{
    uint8_t ustatus_buf[READ_STATUS_LEN] = {0};
    uint8_t ustatus_cmd[] = {CMD_READ_STATUS_S7_0};
    struct spi_message msg;

    os_memset(&msg, 0, sizeof(struct spi_message));
    msg.send_buf = ustatus_cmd;
    msg.send_len = sizeof(ustatus_cmd);
    msg.recv_buf = ustatus_buf;
    msg.recv_len = READ_STATUS_LEN;
    bk_spi_master_xfer(id, &msg);

    return (ustatus_buf[0] & FLASH_STATUS_WIP_BIT);
}

static int spi_flash_read_page(spi_id_t id, uint32_t base_addr, uint32_t size, uint8_t *dst_data)
{
    struct spi_message msg;
    uint8_t ucmd[] = {CMD_READ_DATA, 0x00, 0x00, 0x00};

    if(dst_data == NULL)
        return 1;

    if(size > FLASH_PHY_SECTOR_SIZE)
        return 1;

    if(size == 0)
        return 0;

    os_memset(&msg, 0, sizeof(struct spi_message));
    ucmd[1] = ((base_addr >> 16) & 0xff);
    ucmd[2] = ((base_addr >> 8) & 0xff);
    ucmd[3] = (base_addr & 0xff);

    msg.send_buf = ucmd;
    msg.send_len = sizeof(ucmd);
    msg.recv_buf = dst_data;
    msg.recv_len = size;

    while(spi_flash_is_busy(id))
    {
        rtos_delay_milliseconds(DELAY_WHEN_BUSY_MS);
    }

    bk_spi_master_xfer(id, &msg);

    return 0;
}

bk_err_t bk_spi_flash_init(spi_id_t id)
{
    bk_err_t ret = BK_OK;
 
    ret = bk_spi_driver_init();
    if (BK_OK != ret)
    {
        SPI_LOGE("[%s] bk_spi_driver_init fail[ret=%d]!\r\n", __func__, ret);
        return ret;
    }
 
    spi_config_t config = {0};
       config.role = SPI_ROLE_MASTER;
       config.bit_width = SPI_BIT_WIDTH_8BITS;
       config.polarity = 0;
       config.phase = 0;
       config.wire_mode = SPI_4WIRE_MODE;
       config.baud_rate = SFLASH_SPI_BAUD_RATE;
       config.bit_order = SPI_MSB_FIRST;
#if CONFIG_SPI_DMA
       config.dma_mode = 1;
       config.spi_tx_dma_chan = bk_dma_alloc(DMA_DEV_DTCM);
       config.spi_rx_dma_chan = bk_dma_alloc(DMA_DEV_DTCM);
#endif
    ret = bk_spi_init(id, &config);
    if (BK_OK != ret)
    {
        SPI_LOGE("[%s] bk_spi_init fail[ret=%d]!\r\n", __func__, ret);
        return ret;
    }
 
    return ret;
}

bk_err_t bk_spi_flash_deinit(spi_id_t id)
{
    bk_err_t ret = BK_OK;
 
    ret = bk_spi_deinit(id);
    if (BK_OK != ret)
    {
        SPI_LOGE("[%s] bk_spi_deinit fail[ret=%d]!\r\n", __func__, ret);
        return ret;
    }
 
    ret = bk_spi_driver_deinit();
    if (BK_OK != ret)
    {
        SPI_LOGE("[%s] bk_spi_driver_deinit fail[ret=%d]!\r\n", __func__, ret);
    }
 
    return ret;
}

bk_err_t bk_spi_flash_read(spi_id_t id, uint32_t base_addr, uint8_t *dst_data, uint32_t size)
{
    if(dst_data == NULL)
        return 1;

    if(size == 0)
    {
        return 0;
    }

    for(int i=0; i<size; )
    {
        int ret;
        uint32_t dsize;

        if((size - i) >= FLASH_PHY_SECTOR_SIZE)
            dsize = FLASH_PHY_SECTOR_SIZE;
        else
            dsize = size - i;

        ret = spi_flash_read_page(id, base_addr, dsize, dst_data);
        if(ret)
        {
            SPI_LOGE("spiff read page err:%d\r\n", ret);
            return 1;
        }

        base_addr = base_addr + dsize;
        dst_data = dst_data + dsize;
        i = i + dsize;
    }

    return 0;
}

static void spi_flash_send_command(spi_id_t id, uint8_t cmd)
{
    uint8_t ucmd = cmd;
    struct spi_message msg;

    os_memset(&msg, 0, sizeof(struct spi_message));

    msg.send_buf = &ucmd;
    msg.send_len = sizeof(ucmd);
    msg.recv_buf = NULL;
    msg.recv_len = 0;
    bk_spi_master_xfer(id, &msg);
}

static void spi_flash_earse(spi_id_t id, uint32_t base_addr, uint32_t mode)
{
    struct spi_message msg;
    uint8_t ucmd[] = {0x00, 0x00, 0x00, 0x00};
    uint32_t send_len;

    os_memset(&msg, 0, sizeof(struct spi_message));

    if(mode == ERASE_MODE_ALL)
    {
        ucmd[0] = CMD_ERASE_ALL;
        send_len = 1;
    }
    else
    {
        if(mode == ERASE_MODE_BLOCK_64K)
        {
            ucmd[0] = CMD_ERASE_BLK_64K;
        }
        else if(mode == ERASE_MODE_BLOCK_32K)
        {
            ucmd[0] = CMD_ERASE_BLK_32K;
        }
        else if(mode == ERASE_MODE_SECTOR)
        {
            ucmd[0] = CMD_ERASE_SECTOR;
        }
        else
        {
            SPI_LOGE("earse wrong mode:%d\r\n", mode);
            return;
        }

        ucmd[1] = ((base_addr >> 16) & 0xff);
        ucmd[2] = ((base_addr >> 8) & 0xff);
        ucmd[3] = (base_addr & 0xff);
        send_len = 4;
    }

    msg.send_buf = ucmd;
    msg.send_len = send_len;

    msg.recv_buf = NULL;
    msg.recv_len = 0;

    while(spi_flash_is_busy(id))
    {
        rtos_delay_milliseconds(DELAY_WHEN_BUSY_MS);
    }

    spi_flash_send_command(id, CMD_WRITE_ENABLE);

    while(spi_flash_is_busy(id))
    {
        rtos_delay_milliseconds(DELAY_WHEN_BUSY_MS);
    }

    bk_spi_master_xfer(id, &msg);
}

bk_err_t bk_spi_flash_erase(spi_id_t id, uint32_t base_addr, uint32_t size)
{
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
            erase_mode = ERASE_MODE_SECTOR;
        }
        else if(size <= 32 * 1024)
        {
            erase_size = 32 * 1024;
            erase_mode = ERASE_MODE_BLOCK_32K;
        }
        else
        {
            erase_size = 64 * 1024;
            erase_mode = ERASE_MODE_BLOCK_64K;
        }

        spi_flash_earse(id, base_addr, erase_mode);

        if(base_addr & (erase_size - 1))
        {
            size = erase_size - (base_addr & (erase_size - 1));
        }
        else
        {
            size = erase_size;
        }

        SPI_LOGV("base_addr:%d,erase_mode:%d,left_size:%d,size:%d\r\n",base_addr,erase_mode,left_size,size);

        left_size -= size;
        base_addr += size;
    }

    return 0;
}

static int spi_flash_program_page(spi_id_t id, uint32_t base_addr, uint32_t size, const void *src_data)
{
    struct spi_message msg;
    uint8_t *ucmd;

    if(src_data == NULL)
        return 1;

    if(size > FLASH_PHY_PAGE_SIZE)
        return 1;

    if(size == 0)
        return 0;

    ucmd = os_malloc(size + 4);
    if(!ucmd)
        return 1;

    os_memset(&msg, 0, sizeof(struct spi_message));
    os_memset(ucmd, 0, size + 4);

    ucmd[0] = CMD_PAGE_PROG;
    ucmd[1] = ((base_addr >> 16) & 0xff);
    ucmd[2] = ((base_addr >> 8) & 0xff);
    ucmd[3] = (base_addr & 0xff);
    os_memcpy(&ucmd[4], src_data, size);

    msg.send_buf = ucmd;
    msg.send_len = size + 4;
    msg.recv_buf = NULL;
    msg.recv_len = 0;

    while(spi_flash_is_busy(id))
    {
        rtos_delay_milliseconds(DELAY_WHEN_BUSY_MS);
    }

    spi_flash_send_command(id, CMD_WRITE_ENABLE);

    while(spi_flash_is_busy(id))
    {
        rtos_delay_milliseconds(DELAY_WHEN_BUSY_MS);
    }

    bk_spi_master_xfer(id, &msg);

    os_free(ucmd);

    return 0;
}

bk_err_t bk_spi_flash_write(spi_id_t id, uint32_t base_addr, const void *src_data, uint32_t size)
{
    if(src_data == NULL)
        return 1;

    if(size == 0)
    {
        return 0;
    }

    for(int i=0; i<size; )
    {
        int ret;
        uint32_t dsize;

        if((size - i) >= FLASH_PHY_PAGE_SIZE)
            dsize = FLASH_PHY_PAGE_SIZE;
        else
            dsize = size - i;

        ret = spi_flash_program_page(id, base_addr, dsize, src_data);
        if(ret)
        {
            SPI_LOGE("spi write page err:%d\r\n", ret);
            return 1;
        }

        base_addr = base_addr + dsize;
        src_data = src_data + dsize;
        i = i + dsize;
    }

    return 0;
}

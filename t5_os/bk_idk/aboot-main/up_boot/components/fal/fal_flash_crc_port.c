/*
 * File      : fal_flash_beken7231_crc_port.c
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-26     armink       the first version
 * 2018-05-31     Murphy       Adapted beken7231 board feature
 */

#include <fal.h>

#ifdef RT_FLASH_PORT_DRIVER_BEKEN_CRC
#include "driver_flash.h"
#include <string.h>
#include <stdint.h>

#define debug_printf(...)
#define progress_printf         printf

/*
 * CRC16 checksum
 * MSB first; Polynomial: 8005; Initial value: 0xFFFF
 * */
static unsigned short CRC16_CCITT_FALSE(unsigned char *puchMsg, unsigned int usDataLen)
{
    unsigned short wCRCin = 0xFFFF;
    unsigned short wCPoly = 0x8005;
    unsigned char wChar = 0;

    while (usDataLen--)
    {
        wChar = *(puchMsg++);
        wCRCin ^= (wChar << 8);
        for (int i = 0; i < 8; i++)
        {
            if (wCRCin & 0x8000)
                wCRCin = (wCRCin << 1) ^ wCPoly;
            else
                wCRCin = wCRCin << 1;
        }
    }
    return (wCRCin);
}

static int init(void)
{
    /* do nothing now */
    return 0;
}

static int read(long offset, uint8_t *buf, size_t size)
{
    uint32_t pos = offset % 32;
    uint32_t stage0_offset = 0;
    uint32_t stage0_len = 0;
    uint32_t stage1_offset = 1;
    uint32_t stage1_len = 0;
    uint32_t index = 0;
    uint32_t len = 0;

    if (pos > 0) //have stage0
    {
        stage0_offset = beken_onchip_flash_crc.addr + offset / 32 * 34 + pos;
        stage0_len = size > 32 ? 32 - pos : size;
        stage1_offset = stage0_offset + stage0_len + 2;
        stage1_len = size - stage0_len;
    }
    else
    {
        stage1_offset = beken_onchip_flash_crc.addr + offset / 32 * 34;
        stage1_len = size;
    }

    if (stage0_len > 0)
    {
        debug_printf("%s:%d,stage0: offset:%d, size:%d\r\n", __FUNCTION__, __LINE__, stage0_offset, stage0_len);
        flash_read_data(&buf[index], (unsigned long)stage0_offset, (unsigned long)stage0_len);
        index += stage0_len;
    }

    if (stage1_len > 0)
    {
        debug_printf("%s:%d,stage1: offset:%d, size:%d\r\n", __FUNCTION__, __LINE__, stage1_offset, stage1_len);
        while (stage1_len > 0)
        {
            len = stage1_len > 32 ? 32 : stage1_len;
            flash_read_data(&buf[index], (unsigned long)stage1_offset, (unsigned long)len);
            stage1_offset += 34;
            index += len;
            stage1_len -= len;
        }
    }
    return size;
}

/* buffer size = 4096/32*34, size need a multiple of 34!!!! */
#define WRITE_BUFFER_SIZE (4352)

static int write(long offset, const uint8_t *buf, size_t size)
{
    char *buff_temp = NULL;
    char buff_crc16[34] = {0xff};

    uint32_t addr;
    uint16_t crc_value = 0;
    uint32_t len = 32;

    uint32_t index_buff       = 0; // buf index
    uint32_t index_buff_crc16 = 0; // buff_crc16 index
    uint32_t index_buff_temp  = 0; // buff_temp index
    uint32_t n = 0;

    /* offset must be 32 bytes aligned */
    if (offset & 0x1F)
    {
        bk_printf("\n\n\n%s:%d 0x%08lx, size:%d.\n", __FUNCTION__, __LINE__, offset, size);
        return 0;
    }

    addr = beken_onchip_flash_crc.addr + offset / 32 * 34;

    buff_temp = (char *)malloc(WRITE_BUFFER_SIZE);
    if (buff_temp == NULL)
    {
        printf("error.\n");
        return 0;
    }

    memset(buff_temp, 0xFF, WRITE_BUFFER_SIZE);
    memset(buff_crc16, 0xFF, 34);

    bk_flash_unlock();
    while (size)
    {
        if (size < 32)
        {
            len   = size;
            size  = 0;
        }
        else
        {
            len   = 32;
            size -= 32;
        }

        memcpy(&buff_crc16[index_buff_crc16], &buf[index_buff], len);
        index_buff += len;
        index_buff_crc16 += len;

        crc_value = CRC16_CCITT_FALSE((unsigned char *)buff_crc16, 32);
        buff_crc16[32] = (unsigned char)(crc_value >> 8);
        buff_crc16[33] = (unsigned char)(crc_value & 0x00FF);
        index_buff_crc16 = 0;

        memcpy(&buff_temp[index_buff_temp], &buff_crc16[0], 34);
        index_buff_temp += 34;

        if (size == 0)
        {
            flash_write_data((unsigned char *)(&buff_temp[0]), addr + WRITE_BUFFER_SIZE * n, (unsigned long)index_buff_temp);
        }
        else if (index_buff_temp == WRITE_BUFFER_SIZE)
        {
            flash_write_data((unsigned char *)(&buff_temp[0]), addr + WRITE_BUFFER_SIZE * n, (unsigned long)WRITE_BUFFER_SIZE);
            memset(&buff_temp[0], 0xFF, WRITE_BUFFER_SIZE);
            index_buff_temp = 0;
            n++;
        }
    }
    bk_flash_lock();
    free(buff_temp);

    return index_buff;
}

static int erase(long offset, size_t size)
{
    unsigned int _size = (size + 31) / 32 * 34;
    uint32_t addr;

    offset = offset / 32 * 34;

    debug_printf("%s:%d, offset:%d, size:%d\r\n", __FUNCTION__, __LINE__, offset, size);

    /* start erase */
    bk_flash_unlock();

    /* Calculate the start address of the flash sector(4kbytes) */
    offset = offset & 0x00FFF000;

    addr = beken_onchip_flash_crc.addr + offset;

    // step 1:
    debug_printf("%s:%d, offset:%d, addr:%d\r\n", __FUNCTION__, __LINE__, offset, addr);
    if (addr & (1024 * 64 - 1))
    {
        uint32_t end;
        end = (addr + (1024 * 64 - 1)) & 0x00FF0000;
        if (end > (addr + _size))
        {
            debug_printf("%s:%d, len: %d ==> %d\r\n", __FUNCTION__, __LINE__, end, addr + _size);
            end = addr + _size;
        }

        while (addr < end)
        {
            flash_erase_sector((unsigned long)addr);
            progress_printf("#");
            debug_printf("%s:%d, flash_erase_sector:%d\r\n", __FUNCTION__, __LINE__, addr);
            addr += 4096;

            if (_size < 4096)
            {
                _size = 0;
            }
            else
            {
                _size -= 4096;
            }
        }
        debug_printf("%s:%d, len:%d\r\n", __FUNCTION__, __LINE__, len);
    }

    // step 2:
    while (_size > (1024 * 64))
    {
        flash_erase_block((unsigned long)addr);
        progress_printf("B");
        debug_printf("%s:%d, flash_erase_block:%d\r\n", __FUNCTION__, __LINE__, addr);
        addr += 1024 * 64;

        _size -= 1024 * 64;
    }

    // step 3:
    while (_size)
    {
        flash_erase_sector((unsigned long)addr);
        progress_printf("#");
        debug_printf("%s:%d, flash_erase_sector:%d\r\n", __FUNCTION__, __LINE__, addr);
        addr += 4096;

        if (_size < 4096)
        {
            _size = 0;
        }
        else
        {
            _size -= 4096;
        }
    }

    bk_flash_lock();

    addr = addr - (beken_onchip_flash_crc.addr + offset); // return true erase size
    debug_printf("%s:%d, return:%d\r\n", __FUNCTION__, __LINE__, addr);
    return addr;
}

/* beken_onchip dev */
const struct fal_flash_dev beken_onchip_flash_crc =
{
    RT_BK_FLASH_DEV_CRC_NAME,
    RT_FLASH_BEKEN_START_ADDR,
    RT_FLASH_BEKEN_SIZE,
    RT_FLASH_BEKEN_SECTOR_SIZE,
    {init, read, write, erase}
};
#endif /* RT_FLASH_PORT_DRIVER_BEKEN_CRC */

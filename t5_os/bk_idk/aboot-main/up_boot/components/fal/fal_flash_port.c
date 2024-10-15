/*
 * File      : fal_flash_beken7231_port.c
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-26     armink       the first version
 * 2018-05-31     Murphy       Adapted beken7231 board feature 
 */

#include <fal.h>

#ifdef RT_FLASH_PORT_DRIVER_BEKEN
#include "driver_flash.h"
#include <stdint.h>
#include <string.h>

#define debug_printf(...)
#define progress_printf         printf

static int init(void)
{
    /* do nothing now */
    return 0;
}

static int read(long offset, uint8_t *buf, size_t size)
{
    flash_read_data((unsigned char *)buf, (unsigned long)(beken_onchip_flash.addr + offset), (unsigned long)size);

    return size;
}

static int write(long offset, const uint8_t *buf, size_t size)
{
    bk_flash_unlock();

    flash_write_data((unsigned char *)buf, beken_onchip_flash.addr + offset, (unsigned long)size);

    bk_flash_lock();

    return size;
}

static int erase(long offset, size_t size)
{
    unsigned int _size = size;
    uint32_t addr;
    
    /* start erase */
    bk_flash_unlock();

    debug_printf("[%s:%d] offset:0x%08X size:0x%08X offset2:0x%08X\r\n", __FILE__, __LINE__, offset, size, offset & 0x00FFF000);

    /* Calculate the start address of the flash sector(4kbytes) */
    offset = offset & 0x00FFF000;

    addr = beken_onchip_flash.addr + offset;

    // step 1:
    debug_printf("%s:%d, offset:%d, addr:%d\r\n", __FUNCTION__, __LINE__, offset, addr);
    if (addr & (1024 * 64 - 1))
    {
        uint32_t end;
        end = (addr + (1024 * 64 - 1)) & 0x00FF0000;
        if (end > (addr + _size))
        {
            debug_printf("%s:%d, end: 0x%08X ==> 0x%08X\r\n", __FUNCTION__, __LINE__, end, addr + _size);
            end = addr + _size;
        }

        while (addr < end)
        {
            flash_erase_sector((unsigned long)addr);
            progress_printf("#");
            debug_printf("%s:%d, flash_erase_sector: 0x%08X\r\n", __FUNCTION__, __LINE__, addr);
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
    }

    // step 2:
    while (_size > (1024 * 64))
    {
        flash_erase_block((unsigned long)addr);
        progress_printf("B");
        debug_printf("%s:%d, flash_erase_block: 0x%08X\r\n", __FUNCTION__, __LINE__, addr);
        addr += 1024 * 64;

        _size -= 1024 * 64;
    }

    // step 3:
    while (_size)
    {
        flash_erase_sector((unsigned long)addr);
        progress_printf("#");
        debug_printf("%s:%d, flash_erase_sector: 0x%08X\r\n", __FUNCTION__, __LINE__, addr);
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

    return addr - ( beken_onchip_flash.addr + offset); // return true erase size
}

/* beken_onchip dev */
const struct fal_flash_dev beken_onchip_flash = 
{
    RT_BK_FLASH_DEV_NAME, 
    RT_FLASH_BEKEN_START_ADDR, 
    RT_FLASH_BEKEN_SIZE, 
    RT_FLASH_BEKEN_SECTOR_SIZE,
    {init, read, write, erase} 
};
#endif /* RT_FLASH_PORT_DRIVER_BEKEN */

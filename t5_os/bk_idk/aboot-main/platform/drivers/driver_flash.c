/*************************************************************
 * @file        driver_flash.c
 * @brief       code of FLASH driver of BK7231
 * @author      GuWenFu
 * @version     V1.2
 * @date        2016-09-29
 * @par
 * @attention
 *
 * @history     2016-09-29 gwf    create this file
 */
#include <stdio.h>
#include "string.h"
#include "BK_System.h"
#include "driver_flash.h"
#include "drv_uart.h"

unsigned int flash_id;
unsigned int flash_size;
static const flash_config_t flash_config[] =
{
    {0x1C7016, 1, 0x1F, 0x00}, //en_25qh32b
    {0x1C7015, 1, 0x1F, 0x00}, //en_25qh32b    
    {0x0B4014, 2, 0x1F, 0x00}, //xtx_25f08b
    {0x0B4015, 2, 0x1F, 0x00}, //xtx_25f16b
    {0x0B4016, 2, 0x1F, 0x00}, //xtx_25f32b
    {0x0E4016, 2, 0x1F, 0x00}, //xtx_FT25H32
    {0xC84015, 2, 0x1F, 0x00}, //gd_25q16c
    {0xC84016, 1, 0x1F, 0x00}, //gd_25q32c
    {0x204016, 2, 0x1F, 0x00}, //xmc_25qh32b
    {0xC22315, 1, 0x0F, 0x00}, //mx_25v16b
    {0xEF4016, 2, 0x1F, 0x00}, //w_25q32b
    {0xEF4017, 2, 0x1F, 0x00}, //w_25q64b
    {0x856016, 2, 0x1F, 0x00}, //py_25q32h
    {0x204016, 2, 0x1F, 0x00}, //py_25q32h
    {0x0B4017, 2, 0x05, 0x00}, ////xtx_25f64b
    {0xEB6015, 2, 0x1F, 0x00}, //zg_th25q16b
    {0x000000, 2, 0x00, 0x00}, //default
};

const flash_config_t *flash_current_config = NULL;

void bk_flash_lock(void)
{
}

void bk_flash_unlock(void)
{
}

void flash_get_current_flash_config(void)
{
    int i;
    for(i = 0; i < (sizeof(flash_config) / sizeof(flash_config_t) - 1); i++)
    {
        if(flash_id == flash_config[i].flash_id)
        {
            flash_current_config = &flash_config[i];
            break;
        }
    }
    if(i == (sizeof(flash_config) / sizeof(flash_config_t) - 1))
    {
        flash_current_config = &flash_config[i];
    }
}


void set_flash_clk(unsigned char clk_conf)
{
    unsigned long temp0;

    temp0 = REG_FLASH_CONFIG;
    REG_FLASH_CONFIG = (temp0 & (~FLASH_CONFIG_CLK_CONF_MASK))
                       | ((clk_conf << FLASH_CONFIG_CLK_CONF_POSI) & FLASH_CONFIG_CLK_CONF_MASK);
}

void get_flash_ID(void)
{
    while(REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);

        REG_FLASH_OPERATE_SW = ((FLASH_OPCODE_RDID << FLASH_OPERATE_SW_OP_TYPE_SW_POSI)
                                | (0x1 << FLASH_OPERATE_SW_OP_START_POSI)
                                | (0x1 << FLASH_OPERATE_SW_WP_VALUE_POSI)); // make WP equal 1 not protect SRP
        while (REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);

    flash_id = REG_FLASH_READ_ID_DATA;//manufacture ID && flash size
    flash_size = 2 << ((flash_id & 0xff) - 1);
}

void printf_flash_ID(void)
{
    bk_printf("flash id:\r\n");
    bk_print_hex(flash_id);
}

unsigned short flash_read_sr(unsigned char byte)
{
    unsigned short sr;

    while(REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);
    REG_FLASH_OPERATE_SW = ((FLASH_OPCODE_RDSR << FLASH_OPERATE_SW_OP_TYPE_SW_POSI)
                            | (0x1 << FLASH_OPERATE_SW_OP_START_POSI)
                            | (0x1 << FLASH_OPERATE_SW_WP_VALUE_POSI));
    while (REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);

    sr = REG_FLASH_CRC_CHECK & 0x00FF;

    if(byte == 2)
    {
        REG_FLASH_OPERATE_SW = ((FLASH_OPCODE_RDSR2 << FLASH_OPERATE_SW_OP_TYPE_SW_POSI)
                                | (0x1 << FLASH_OPERATE_SW_OP_START_POSI)
                                | (0x1 << FLASH_OPERATE_SW_WP_VALUE_POSI));
        while(REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);
        sr |=  (REG_FLASH_CRC_CHECK & 0xff) << 8;
    }

    return sr;
}

void flash_write_enable(void)
{
    REG_FLASH_OPERATE_SW = ((FLASH_OPCODE_WREN << FLASH_OPERATE_SW_OP_TYPE_SW_POSI)
                            | (0x1 << FLASH_OPERATE_SW_OP_START_POSI)
                            | (0x1 << FLASH_OPERATE_SW_WP_VALUE_POSI));
    while (REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);
}

void flash_write_disable(void)
{
    REG_FLASH_OPERATE_SW = ((FLASH_OPCODE_WRDI << 
FLASH_OPERATE_SW_OP_TYPE_SW_POSI)
                            | (0x1 << FLASH_OPERATE_SW_OP_START_POSI)
                            | (0x1 << FLASH_OPERATE_SW_WP_VALUE_POSI));
    while (REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);
}
void flash_write_sr(unsigned char bytes,  unsigned short val)
{
    uint32_t value;
    
    while (REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);

    value = REG_FLASH_CONFIG;
    value &= ~(FLASH_CONFIG_WRSR_DATA_MASK);
    value |= (val << FLASH_CONFIG_WRSR_DATA_POSI);
    REG_FLASH_CONFIG = value;

    while (REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);
    if (bytes == 1)
    {
        REG_FLASH_OPERATE_SW = ((FLASH_OPCODE_WRSR << FLASH_OPERATE_SW_OP_TYPE_SW_POSI)
                                | (0x1 << FLASH_OPERATE_SW_OP_START_POSI)
                                | (0x1 << FLASH_OPERATE_SW_WP_VALUE_POSI));
    }
    else if (bytes == 2)
    {
        REG_FLASH_OPERATE_SW = ((FLASH_OPCODE_WRSR2 << FLASH_OPERATE_SW_OP_TYPE_SW_POSI)
                                | (0x1 << FLASH_OPERATE_SW_OP_START_POSI)
                                | (0x1 << FLASH_OPERATE_SW_WP_VALUE_POSI));
    }

    while (REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);
}


void clr_flash_protect(void)
{
    unsigned short param = 0;
    unsigned char bytes = 2;

    if(param != flash_read_sr(bytes))
    {
        flash_write_sr(flash_current_config->sr_size, 0);
    }
}


void flash_wp_256k(unsigned char flash_id)
{
    switch (flash_id)
    {
    case 0x51:      // MDxx
        flash_write_sr(1, 0x18 );
        break;
    case 0xc8:      // QDxx
        flash_write_sr(2, 0X002c);
        break;
    case 0xa1:      // FMXX
        flash_write_sr(1, 0x18);
        break;
    default:
        flash_write_sr(2, 0X002c);
        break;
    }
}

void set_flash_protect(PROTECT_TYPE type)
{
    u32 param;

    switch (type)
    {
        case NONE:
            param = flash_current_config->protect_none << 2;
            break;
            
        case ALL:
            param = flash_current_config->protect_all << 2;//protect all
            break;

        default:
            param = flash_current_config->protect_all << 2;
            break;
    }
    
    if(param != flash_read_sr(flash_current_config->sr_size))
    {
        flash_write_sr(flash_current_config->sr_size, param);
    }

}

void flash_init(void)
{
    REG_FLASH_CONFIG = 0x0C;
    while (REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);
	//flash_disable_cpu_data_wr();
}

void set_flash_qe(void)
{
    unsigned int temp0;

    while (REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);
    // WRSR QE=1
    temp0 = REG_FLASH_CONFIG; //????WRSR Status data
    REG_FLASH_CONFIG = (temp0 & (~FLASH_CONFIG_WRSR_DATA_MASK))
                       | (0x200 << FLASH_CONFIG_WRSR_DATA_POSI); // SET QE=1

    // Start WRSR
    temp0 = REG_FLASH_OPERATE_SW;
    REG_FLASH_OPERATE_SW = ((temp0              &  FLASH_OPERATE_SW_OP_ADDR_SW_MASK))
                           | (FLASH_OPCODE_WRSR2 << FLASH_OPERATE_SW_OP_TYPE_SW_POSI)
                           | (0x1                << FLASH_OPERATE_SW_OP_START_POSI)
                           | (0x1                << FLASH_OPERATE_SW_WP_VALUE_POSI); // make WP equal 1 not protect SRP
    while (REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);
}

static void set_flash_qwfr(void)
{
    unsigned int temp0;

    temp0 = REG_FLASH_CONFIG;
    REG_FLASH_CONFIG = (temp0 & (~FLASH_CONFIG_MODE_SEL_MASK))
                       | (0xA   << FLASH_CONFIG_MODE_SEL_POSI);
}

void clr_flash_qwfr(void)
{
    unsigned int temp0;

    temp0 = REG_FLASH_CONFIG;
    REG_FLASH_CONFIG = (temp0 & (~FLASH_CONFIG_MODE_SEL_MASK))
                       | (0x0   << FLASH_CONFIG_MODE_SEL_POSI);

    temp0 = REG_FLASH_OPERATE_SW;
    REG_FLASH_OPERATE_SW = ((0                 << FLASH_OPERATE_SW_OP_ADDR_SW_POSI)
                            | (FLASH_OPCODE_CRMR << FLASH_OPERATE_SW_OP_TYPE_SW_POSI)
                            | (0x1               << FLASH_OPERATE_SW_OP_START_POSI)
                            | (temp0             &  FLASH_OPERATE_SW_WP_VALUE_MASK));
    while (REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);
}


void flash_set_line_mode(unsigned char mode)
{
    if (mode == 1)
    {
        clr_flash_qwfr();
    }
    else if (mode == 2)
    {
        REG_FLASH_CONFIG = (REG_FLASH_CONFIG & (~FLASH_CONFIG_MODE_SEL_MASK))
                           | (0x01UL << FLASH_CONFIG_MODE_SEL_POSI);
        while(REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);
    }
    else if (mode == 4)
    {
        set_flash_qwfr();
    }
}

void flash_set_clk(unsigned char clk)
{
    while(REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);

    REG_FLASH_CONFIG = (REG_FLASH_CONFIG & (~FLASH_CONFIG_CLK_CONF_MASK))
                       | ((clk << FLASH_CONFIG_CLK_CONF_POSI) & FLASH_CONFIG_CLK_CONF_MASK);
    while(REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);

}

void flash_erase_sector(unsigned long address)
{
    unsigned int temp0;

    if(address >= flash_size)
    {
        return;
    }

    while (REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);
    temp0 = REG_FLASH_OPERATE_SW;
    REG_FLASH_OPERATE_SW = (((address        << FLASH_OPERATE_SW_OP_ADDR_SW_POSI) & FLASH_OPERATE_SW_OP_ADDR_SW_MASK)
                            | (FLASH_OPCODE_SE << FLASH_OPERATE_SW_OP_TYPE_SW_POSI)
                            | (0x1             << FLASH_OPERATE_SW_OP_START_POSI)
                            | (temp0           &  FLASH_OPERATE_SW_WP_VALUE_MASK));
    while (REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);
}

void flash_erase_block(unsigned long address)
{
    unsigned int temp0;
    if(address >= flash_size)
    {
        return;
    }

    while (REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);
    temp0 = REG_FLASH_OPERATE_SW;
    REG_FLASH_OPERATE_SW = (((address        << FLASH_OPERATE_SW_OP_ADDR_SW_POSI) & FLASH_OPERATE_SW_OP_ADDR_SW_MASK)
                            | (FLASH_OPCODE_BE2 << FLASH_OPERATE_SW_OP_TYPE_SW_POSI)
                            | (0x1             << FLASH_OPERATE_SW_OP_START_POSI)
                            | (temp0           &  FLASH_OPERATE_SW_WP_VALUE_MASK));
    while(REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);
}

int flash_erase(long addr, size_t size)
{
    unsigned int _size = size;

    /* Calculate the start address of the flash sector(4kbytes) */
    addr = addr & 0x00FFF000;

    // step 1:
    if (addr & (1024 * 64 - 1))
    {
        uint32_t end;
        end = (addr + (1024 * 64 - 1)) & 0x00FF0000;
        if (end > (addr + _size))
        {
            end = addr + _size;
        }

        while (addr < end)
        {
            flash_erase_sector((unsigned long)addr);
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

        addr += 1024 * 64;
        _size -= 1024 * 64;
    }

    // step 3:
    while (_size)
    {
        flash_erase_sector((unsigned long)addr);
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

    return size;
}

void flash_read_data(unsigned char *buffer, unsigned long address, unsigned long len)
{
    unsigned long i, reg_value;
    unsigned long addr = address & (~0x1f);
    unsigned long buf[8];
    unsigned char *pb = (unsigned char *)&buf[0];

    if (len == 0)
        return;

    while (REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);
    while (len)
    {
        reg_value = REG_FLASH_OPERATE_SW;
        REG_FLASH_OPERATE_SW = (((addr             << FLASH_OPERATE_SW_OP_ADDR_SW_POSI) & FLASH_OPERATE_SW_OP_ADDR_SW_MASK)
                                | (FLASH_OPCODE_READ << FLASH_OPERATE_SW_OP_TYPE_SW_POSI)
                                | (0x1               << FLASH_OPERATE_SW_OP_START_POSI)
                                | (reg_value         &  FLASH_OPERATE_SW_WP_VALUE_MASK));
        while (REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);
        addr += 32;

        for (i = 0; i < 8; i++)
        {
            buf[i] = REG_FLASH_DATA_READ_FLASH;
        }

        for (i = address % 32; i < 32; i++)
        {
            *buffer++ = pb[i];
            address++;
            len--;
            if (len == 0)
                break;
        }
    }
}


void flash_write_data(unsigned char *buffer, unsigned long address, unsigned long len)
{
    unsigned long i, reg_value;
    unsigned long addr = address & (~0x1f);
    unsigned long buf[8];
    unsigned char *pb = (unsigned char *)&buf[0];

    if (address % 32)
        flash_read_data(pb, addr, 32);

    while (REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);
    while (len)
    {
        for (i = address % 32; i < 32; i++)
        {
            pb[i] = *buffer++;
            address++;
            len--;
            if (len == 0)
                break;
        }

        for (i = 0; i < 8; i++)
        {
            REG_FLASH_DATA_WRITE_FLASH = buf[i];
        }

        reg_value = REG_FLASH_OPERATE_SW;
        REG_FLASH_OPERATE_SW = ((addr            << FLASH_OPERATE_SW_OP_ADDR_SW_POSI)
                                | (FLASH_OPCODE_PP << FLASH_OPERATE_SW_OP_TYPE_SW_POSI)
                                | (0x1             << FLASH_OPERATE_SW_OP_START_POSI)
                                | (reg_value       &  FLASH_OPERATE_SW_WP_VALUE_MASK));
        while (REG_FLASH_OPERATE_SW & FLASH_OPERATE_SW_BUSY_SW_MASK);
        addr += 32;
        memset(pb, 0XFF, 32);
    }
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BK_System.h"
#include "driver_system.h"
#include "driver_gpio.h"
#include "driver_uart0.h"
#include "driver_uart1.h"
#include "driver_flash.h"

extern void wdt_reboot();
extern u32 fclk_get_tick(void);

#define WR_ENABLE_CMD						0x06
#define WR_DISABLE_CMD						0x04

extern unsigned int flash_id;
u8 flag_boot  =0;
u8 read_sr_flag =0;
u8 bim_uart_cmd[16];
u8 bim_uart_data[4096 + 8];
u32 uart_download_status = 0;
u32 uart_buff_write = 0;
u8 bim_uart_rx_buf[4096];
u32 erase_fenable = 0;
u32 crc32_table[256];
u8 cmd_res_buff[16];
u16 cmd_res_length =0;

enum
{
    FLASH_ADDR_WRITE_CMD          = 0x06,
    FLASH_4K_WRITE_CMD         	 = 0x07,
    FLASH_ADDR_READ_CMD            = 0x08,
    FLASH_4K_READ_CMD     		 = 0x09,
    FLASH_CHIP_ERRASE_CMD       	 = 0x0A,
    FLASH_4K_ERRASE_CMD       	 = 0x0B,
    FLASH_SR_READ_CMD       		 = 0x0c,
    FLASH_SR_WRITE_CMD       	 = 0x0D,
    FLASH_SPI_OP_CMD      		 = 0x0E,
    FLASH_SIZE_ERRASE_CMD       	 = 0x0F,
};// FLASH_operate cmd

//other cmd
#define LINK_CHECK_CMD     0X00
#define CRC_CHECK_CMD      0X10
#define SET_RESET_CMD      0X0E
#define SET_BAUDRATE_CMD   0X0F
#define STAY_ROM_CMD       0XAA
#define SYS_REBOOT_CMD     0x0E
#define BEKEN_UART_REGISTER_WRITE_CMD  0x01
#define BEKEN_UART_REGISTER_READ_CMD  0x03

enum
{
    UART_CMD_STATE_HEAD = 0,
    UART_CMD_STATE_OPCODE_ONE,
    UART_CMD_STATE_OPCODE_TWO,
    UART_CMD_STATE_LENGTH,
    UART_CMD_STATE_CMD,
    UART_CMD_STATE_CMD_FLASH,
    UART_CMD_STATE_LENGTH_FLASH_LEN0,
    UART_CMD_STATE_LENGTH_FLASH_LEN1,
    UART_CMD_STATE_LENGTH_FLASH_SCMD,
    UART_CMD_STATE_PAYLOAD,
    UART_CMD_STATE_ERROR_ONE,
    UART_CMD_STATE_ERROR_TWO,
    UART_CMD_STATE_ERROR_THREE,
    UART_CMD_STATE_ERROR_FOUR,
    UART_CMD_STATE_PACKET
};//cmd status

int make_crc32_table(void)
{
    u32 c;
    int i = 0;
    int bit = 0;
    for(i = 0; i < 256; i++)
    {
        c = (u32)i;
        for(bit = 0; bit < 8; bit++)
        {
            if(c & 1)
            {
                c = (c >> 1) ^ (0xEDB88320);
            }
            else
            {
                c = c >> 1;
            }
        }
        crc32_table[i] = c;
    }
    return 0;
}

u32 make_crc32(u32 crc, unsigned char *string, u32 size)
{
    while(size--)
    {
        crc = (crc >> 8) ^ (crc32_table[(crc ^ *string++) & 0xff]);
    }
    return crc;
}

void  uart_download_rx(UINT8 val)
{
    bim_uart_rx_buf[uart_buff_write++] = (u8)val;
    if(uart_buff_write == 4096)
        uart_buff_write = 0;
}


void dl_uart_init(unsigned long ulBaudRate)
{
    if(uart_dl_port == DEBUG_PORT_UART0)
    {
        uart0_init(ulBaudRate);
    }
    else if(uart_dl_port == DEBUG_PORT_UART1)
    {
        uart1_init(ulBaudRate);
    }
}

void dl_uart_send(unsigned char *buff, int len)
{
    if(uart_dl_port == DEBUG_PORT_UART0)
    {
        uart0_send(buff, len);
    }
    else if(uart_dl_port == DEBUG_PORT_UART1)
    {
        uart1_send(buff, len);
    }
}

void cmd_response( u8 cmd, u8 length, u8 *payload )
{
    u8 i = 0;
    cmd_res_buff[0] = 0x04;
    cmd_res_buff[1] = 0x0e;
    cmd_res_buff[2] = length;
    cmd_res_buff[3] = 0x01;
    cmd_res_buff[4] = 0xe0;
    cmd_res_buff[5] = 0xfc;
    cmd_res_buff[6] = cmd;
    for(i = 0; i < length - 4; i++)
    {
        cmd_res_buff[7 + i] = payload[i];
    }
    dl_uart_send(cmd_res_buff, length + 3);
}

void operate_flash_cmd_response( u8 cmd, u8 status, u16 length, u8 *payload )
{
    u8 response_buff[1024];
    u16    i = 0;

    cmd_res_length = length;
    response_buff[0] = 0x04;
    response_buff[1] = 0x0e;
    response_buff[2] = 0xff;
    response_buff[3] = 0x01;
    response_buff[4] = 0xe0;
    response_buff[5] = 0xfc;
    response_buff[6] = 0xf4;
    response_buff[7] = cmd_res_length;//(u8)(length & 0xff);
    response_buff[8] =0x0;//(u8)((length >> 8));
    response_buff[9] = cmd;
    response_buff[10] = status;
    for(i = 0; i < (cmd_res_length - 2); i++)
    {
        response_buff[11 + i] = payload[i];
    }
    dl_uart_send(response_buff, cmd_res_length + 9);
}

void uart_cmd_dispath(u8 *buff, u8 len)
{
    u8 payload[16];
    u8 read_data[256];
    u32  calcuCrc = 0xffffffff;
    u32 cur_clk_cnt, delay_unit;
    u32 read_flash_addr;
    u32 uart_clk_div, baudrate_set;
    u32 crc_start_addr, crc_end_addr;
    uint16  i = 0;

    switch(buff[0])
    {
        case STAY_ROM_CMD:
        {
            if(!flag_boot)
            {
                flag_boot =1;
                uart_download_status = 1;
                payload[0] = buff[1];
                cmd_response(STAY_ROM_CMD, 5, payload);
                erase_fenable = 1;
            } 
        }
        break;	
       
        case CRC_CHECK_CMD:
        {
            crc_start_addr = ( buff[1] + (buff[2] << 8) + (buff[3] << 16) + (buff[4] << 24) );
            crc_end_addr = ( buff[5] + (buff[6] << 8) + (buff[7] << 16) + (buff[8] << 24) );
            make_crc32_table();
            read_flash_addr = crc_start_addr;
            for(i = 0; i < (crc_end_addr - crc_start_addr+1) /256; i++)
            {
                flash_read_data(read_data, read_flash_addr, 256);
                calcuCrc = make_crc32(calcuCrc, read_data, 256);
                read_flash_addr += 256;
            }            
            payload[0] = calcuCrc;
            payload[1] = calcuCrc >> 8;
            payload[2] = calcuCrc >> 16;
            payload[3] = calcuCrc >> 24;
            cmd_response(CRC_CHECK_CMD, 8, payload);
        }
        break;
        
        case SET_BAUDRATE_CMD:
        {
            baudrate_set =  buff[1] + (buff[2] << 8) + (buff[3] << 16) + (buff[4] << 24) ;
            dl_uart_init(baudrate_set);
            DelayMS(buff[5]);
            cmd_response(SET_BAUDRATE_CMD, 9, &buff[1]);
        }
        break;
        
        case SET_RESET_CMD:
        if(buff[1] == 0xa5)
        {
            DelayMS(1);
            *((volatile uint32_t *)(0x44010000 + 0xf*4)) &= ~(0x1<<1);
            wdt_reboot();
        }
        break;
   }
}

void boot_uart_data_callback( u8 *buff, u16 len)
{
    static u8 cmd_status = UART_CMD_STATE_HEAD;
    static u32 index = 0, index_cnt = 0;
    static u16 length;
    static u16 scmd_length;
    static u32 write_addr;
    static u32 read_addr;
    u8 reg_addr;
    u8 reg_value;

    while(len > 0)
    {
        switch(cmd_status)
        {
            case UART_CMD_STATE_HEAD:
            {       
                if(buff[0] == 0x01)     
                {      
                    cmd_status = UART_CMD_STATE_OPCODE_ONE;      
                }    
                else
                {
                    cmd_status = UART_CMD_STATE_HEAD;
                }
            }
            break;

            case UART_CMD_STATE_OPCODE_ONE:
            {      
                if(buff[0] == 0xe0) 
                {
                    cmd_status = UART_CMD_STATE_OPCODE_TWO; 
                }
                else   
                {
                    cmd_status = UART_CMD_STATE_HEAD;
                }
            }
            break;
            
            case UART_CMD_STATE_OPCODE_TWO:
            {
                if(buff[0] == 0xfc)
                {
                    cmd_status = UART_CMD_STATE_LENGTH;
                }
                else
                {
                    cmd_status = UART_CMD_STATE_HEAD;
                }
            }
            break;
            
            case UART_CMD_STATE_LENGTH:
            {
                length = buff[0];
                if(buff[0] == 0xff)   
                {               
                    cmd_status = UART_CMD_STATE_CMD_FLASH;             
                }        
                else if(buff[0] > 0 && buff[0] != 0xff)    
                {
                    if((length == 2)&& (flag_boot ==1))
                    {
                        len--;
                        buff++;
                        cmd_status = UART_CMD_STATE_HEAD;  
                    }
                    else
                    {
                        cmd_status = UART_CMD_STATE_CMD;           
                        index = 0;  
                    }
                }       
                else 
                {
                    cmd_status = UART_CMD_STATE_HEAD;
                }
            }
            break;
            
            case UART_CMD_STATE_CMD:
            {
                bim_uart_cmd[index++] = buff[0]; 
                if(index == length)     
                {       
                    uart_cmd_dispath(bim_uart_cmd, length);      
                    cmd_status = UART_CMD_STATE_HEAD;          
                }
            }
            break;
            case UART_CMD_STATE_CMD_FLASH:
            {   
                if(buff[0] == 0xf4)
                {  
                    cmd_status = UART_CMD_STATE_LENGTH_FLASH_LEN0;   
                }
                else 
                {
                    cmd_status = UART_CMD_STATE_HEAD;
                }
            }
            break;
            
            case UART_CMD_STATE_LENGTH_FLASH_LEN0:
            {
                cmd_status = UART_CMD_STATE_LENGTH_FLASH_LEN1;
                scmd_length = buff[0];
            }
            break;
            
            case UART_CMD_STATE_LENGTH_FLASH_LEN1:
            {
                scmd_length += (buff[0] << 8);
                if(scmd_length > 0)
                    cmd_status = UART_CMD_STATE_LENGTH_FLASH_SCMD;      
                else
                    cmd_status = UART_CMD_STATE_HEAD;
                    index = 0;     
                    index_cnt = 0;
            }
            break;

            case UART_CMD_STATE_LENGTH_FLASH_SCMD:
            {
                bim_uart_data[index++] = buff[0];
                if(bim_uart_data[0] == FLASH_4K_READ_CMD  && index == scmd_length) //read id
                {
                    read_addr = bim_uart_data[1] + (bim_uart_data[2] << 8) + (bim_uart_data[3] << 16) + (bim_uart_data[4] << 24) ;
                    if(read_addr < UART_DL_LIMIT_ADDR)
                    {
                        bim_uart_data[5] = scmd_length - 5;
                        operate_flash_cmd_response(FLASH_4K_READ_CMD, 6, 7, &bim_uart_data[1]);
                    }
                    else
                    {
                        flash_read_data(&bim_uart_data[5], read_addr, 4096);
                        operate_flash_cmd_response(FLASH_4K_READ_CMD, 0, 4102, &bim_uart_data[1]);
                    }
                    cmd_status = UART_CMD_STATE_HEAD;
                }
                else if(bim_uart_data[0] == FLASH_SIZE_ERRASE_CMD && index ==  scmd_length) //erase
                {	
                    INT32 addr = bim_uart_data[2] + (bim_uart_data[3] << 8) + (bim_uart_data[4] << 16) + (bim_uart_data[5] << 24) ;
                    if(addr < UART_DL_LIMIT_ADDR)
                    {
                        operate_flash_cmd_response(0x0f, 6, 0x07, &bim_uart_data[1]);
                    }
                    else
                    {
                        if(erase_fenable == 1)
                        {
                            set_flash_protect(NONE);
                            erase_fenable = 0;
                        }
                        if(bim_uart_data[1] == 0x20)    //0x1000
                        {
                            flash_erase_sector(addr);
                        }
                        else if(bim_uart_data[1] == 0xd8)  //0x10000
                        {
                            flash_erase_block(addr);
                        }
                        else if(bim_uart_data[1] == 0x52)  //0x8000
                        {
                             flash_erase(addr, 0x8000);
                        }
                        operate_flash_cmd_response(FLASH_SIZE_ERRASE_CMD, 0, 0x07, &bim_uart_data[1]);
                    }
                        cmd_status = UART_CMD_STATE_HEAD;
                }
                else if(bim_uart_data[0] == FLASH_ADDR_WRITE_CMD && index == scmd_length) //write bt addr
                {
                    write_addr = bim_uart_data[1] + (bim_uart_data[2] << 8) + (bim_uart_data[3] << 16) + (bim_uart_data[4] << 24) ;
                    if(write_addr < UART_DL_LIMIT_ADDR)
                    {
                        bim_uart_data[5] = scmd_length - 5;
                        operate_flash_cmd_response(FLASH_ADDR_WRITE_CMD, 6, 7, &bim_uart_data[1]);
                    }
                    else
                    {
                        flash_write_data(&bim_uart_data[5], write_addr, (scmd_length - 5));
                        bim_uart_data[5] = scmd_length - 5;
                        operate_flash_cmd_response(FLASH_ADDR_WRITE_CMD, 0, 7, &bim_uart_data[1]);
                    }
                    cmd_status = UART_CMD_STATE_HEAD;
                }
                else if( bim_uart_data[0] == FLASH_4K_WRITE_CMD ) //write code data
                {
                    //__asm volatile ("j .");
                    if(index >= (256 * (index_cnt + 1) + 5) )
                    {
                        write_addr = (bim_uart_data[1] + (bim_uart_data[2] << 8) + (bim_uart_data[3] << 16) + (bim_uart_data[4] << 24) );
                        if(write_addr < 0x2000)
                        {
                            index_cnt++;
                            if(index == scmd_length )
                            {
                                bim_uart_data[1] = 0x00;
                                bim_uart_data[2] = 0x20;
                                operate_flash_cmd_response(FLASH_4K_WRITE_CMD, 6, 6, &bim_uart_data[1]);
                                cmd_status = UART_CMD_STATE_HEAD;
                                index_cnt = 0;
                            }
                        }
                        else
                        { 
                            flash_write_data(&bim_uart_data[5 + 256 * (index_cnt)], (write_addr + 256 * (index_cnt)), 256);
                            index_cnt++;      
                            if(index == scmd_length )
                            {
                                operate_flash_cmd_response(FLASH_4K_WRITE_CMD, 0, 6, &bim_uart_data[1]);
                                cmd_status = UART_CMD_STATE_HEAD;
                                index_cnt = 0;
                            }
                        }
                    }
                }
                else if( bim_uart_data[0] == FLASH_SR_READ_CMD )//reg_addr    //(01 E0 FC FF F4)  02 00  0c 05(addr) // 04 0e ff 01 e0 fc f4 04 00  0c 00 reg_addr reg_value
                {							
                    reg_addr = buff[1];
                    reg_value = flash_read_sr(reg_addr);  
                    bim_uart_data[1] = reg_addr;
                    bim_uart_data[2] = reg_value;
                    operate_flash_cmd_response(FLASH_SR_READ_CMD, 0, 0x4, &bim_uart_data[1]);
                    cmd_status = UART_CMD_STATE_HEAD;
                    set_flash_protect(ALL);
                }
                else if( bim_uart_data[0] == FLASH_SPI_OP_CMD ) //get flash ID         												
                {          
                    clr_flash_protect();     
                    bim_uart_data[4] = (flash_id>>24);
                    bim_uart_data[5] = (flash_id>>16);
                    bim_uart_data[6] = ((flash_id>>8)&(~0xff00));
                    bim_uart_data[7] = (flash_id&0xff);	
                    operate_flash_cmd_response(FLASH_SPI_OP_CMD, 0, 0x6, &bim_uart_data[4]);
                    cmd_status = UART_CMD_STATE_HEAD; 
                }
                else if( bim_uart_data[0] == FLASH_SR_WRITE_CMD ) //write sr	//(01 E0 FC FF F4)   04 00 0D  reg_addr reg_value0  // 04 0e ff 01 e0 fc f4 05 00 0D 00 reg_addr reg_value0           														
                {
                    bim_uart_data[1] = buff[1];//reg_addr
                    bim_uart_data[2] = buff[2];//reg_value
                    clr_flash_protect(); 
                    flash_write_sr(bim_uart_data[1],bim_uart_data[2]);
                    reg_value = flash_read_sr(bim_uart_data[1]);
                    bim_uart_data[2] = reg_value;
                    operate_flash_cmd_response(FLASH_SR_WRITE_CMD, 0, 0x4, &bim_uart_data[1]);
                    cmd_status = UART_CMD_STATE_HEAD;
                }
                else if(index == scmd_length)
                {
                    cmd_status = UART_CMD_STATE_HEAD;
                }
            }
            break;
        }
        len--;
        buff++;
    }
}



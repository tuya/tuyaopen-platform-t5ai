/**
 * @file   encrypt.c
 * @author nRzo <nrzo@nrzo-laptop>
 * @date   Mon Dec 12 21:16:14 2011
 *
 * @brief
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "abc.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define ENCRYPT_SIZE      0X04
#define CRC_SIZE          0X02
#define CRC_PAKEIT        0X20

 u32    coef0 ;//= (u32)0x510fb093 ;//0x12345678;
 u32    coef1 ;//= (u32)0xa3cbeadc ;//0x2faa55aa;
 u32    coef2 ;//= (u32)0x5993a17e ;//0x3aee63dd;
 u32    coef3 ;//= (u32)0xc7adeb03 ;//0x4feeaa00;

typedef  struct __attribute__((packed)) encrypt_packet 
{
    u8 data[CRC_PAKEIT];
    
    struct __attribute__((packed)) 
    {
        u16 bit8  : 1;
        u16 bit9  : 1;
        u16 bit10 : 1;
        u16 bit11 : 1;
        u16 bit12 : 1;
        u16 bit13 : 1;
        u16 bit14 : 1;
        u16 bit15 : 1;
        
        u16 bit0  : 1;
        u16 bit1  : 1;
        u16 bit2  : 1;
        u16 bit3  : 1;
        u16 bit4  : 1;
        u16 bit5  : 1;
        u16 bit6  : 1;
        u16 bit7  : 1;
    }crc;
}ENCRYPT_PACKET_T;

/**
 * Convert string to integer data.
 * @param str: String to be converted
 * @return Result of the conversion
 */
int str2int(char *str)
{
    int result = 0;
    int negative = TRUE;

    while((*str != '\0') && (*str != ' '))
    {
        if (*str == '-')
            negative = FALSE;
        else if((*str >= '0') && (*str <= '9'))
            result = result * 16 + ((*str++) - '0'); /**< hex fomat */
        else if ((*str >= 'a') && (*str <= 'f'))
            result = result * 16 + 10 + (*(str++) - 'a');
        else if ((*str >= 'A') && (*str <= 'F'))
            result = result * 16 + 10 + (*(str++) - 'A');
        else
        {
            printf(" str2int fail\n");
            result = 0x0;
            break;
        }
    }
	
    if (negative == FALSE)
        result = -result;

    return result;
}

u32 get_file_size (FILE *fp)
{
    u32 pos, file_size;

    pos = ftell( fp );			/**< Save the current position. */
    fseek (fp, 0L, SEEK_END);	/**< Jump to the end of the file. */
    file_size = ftell (fp);		/**< Get the end position. */
    fseek (fp, pos, SEEK_SET);	/**< Jump back to the original position. */

    return (file_size);
}

void calc_crc(u32 *buf, u32 packet_num)
{
    int i, bit, byte;

    for (i = 0; i < (int)packet_num; i++)
    {
        ENCRYPT_PACKET_T *p = (struct encrypt_packet *)((u32)buf + i * sizeof(ENCRYPT_PACKET_T));
        ENCRYPT_PACKET_T bak;
		
        for (byte = 0; byte < CRC_PAKEIT; byte++)
        {
            for (bit = 7; bit >= 0; bit--)
            {
                u8 input;
				
                memcpy((char *)&bak, (char *)p, sizeof(ENCRYPT_PACKET_T));
				
                input = (bak.data[byte] >> bit) & 0x01;
                p->crc.bit15 =  bak.crc.bit14 ^ input ^ bak.crc.bit15;
                p->crc.bit14 =  bak.crc.bit13;
                p->crc.bit13 =  bak.crc.bit12;
                p->crc.bit12 =  bak.crc.bit11;
                p->crc.bit11 =  bak.crc.bit10;
                p->crc.bit10 =  bak.crc.bit9;
                p->crc.bit9  =  bak.crc.bit8;
                p->crc.bit8  =  bak.crc.bit7;
                p->crc.bit7  =  bak.crc.bit6;
                p->crc.bit6  =  bak.crc.bit5;
                p->crc.bit5  =  bak.crc.bit4;
                p->crc.bit4  =  bak.crc.bit3;
                p->crc.bit3  =  bak.crc.bit2;
                p->crc.bit2  =  bak.crc.bit1 ^ input ^ bak.crc.bit15;
                p->crc.bit1  =  bak.crc.bit0;
                p->crc.bit0  =  input ^ bak.crc.bit15;
            }												
        }
    }
}
    
void encrypt(u32 *rx, u8 *tx, u32 num,u32  addr0)
{
    u8 *tmp_rx;
    u32 rx_value, ret;
    u32 i, j, addr, addr1;
    u32 len = CRC_PAKEIT / sizeof(u32);

    addr0 = (addr0 * 32)/34;
    
    for (i = 0; i < num; i++)
    {
        for (j = 0; j < len; j++)
        {
            addr = i * (CRC_PAKEIT + sizeof(u16)) + j * sizeof(u32);
            addr1 = addr0 + i * (CRC_PAKEIT) + j * sizeof(u32);

            tmp_rx = (u8 *)rx;
            rx_value = (tmp_rx[0] << (0 * 8)) + (tmp_rx[1] << (1 * 8))
                                                + (tmp_rx[2] << (2 * 8))
                                                + (tmp_rx[3] << (3 * 8));
            ret = enc_data_my(addr1, rx_value);
            tx[addr + 0] = (ret >> (0 * 8)) & 0xff;
            tx[addr + 1] = (ret >> (1 * 8)) & 0xff;
            tx[addr + 2] = (ret >> (2 * 8)) & 0xff;
            tx[addr + 3] = (ret >> (3 * 8)) & 0xff;
            
            rx ++;
        }

        tx[addr + sizeof(u32) + 0] = 0xff;
        tx[addr + sizeof(u32) + 1] = 0xff;
        
        rx = (u32 *)((u32)rx + 2);
    }
}
// eof


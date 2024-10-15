/*
 * bl_spi.h
 *
 *  Created on: 2017-4-27
 *      Author: gang.cheng
 */

#ifndef BL_SPI_H_
#define BL_SPI_H_

#include <stdint.h>
#include "type.h"
#include "bl_bk_reg.h"

#define  SPI_INT_MODE  0

#define  NUMBER_ROUND_UP(a,b)        ((a) / (b) + (((a) % (b)) ? 1 : 0))

#define  SPI_DEFAULT_CLK             26000000


/// 1MHZ baudrate supported by this SPI (in bps)
#define SPI_BAUD_1MHZ         1000000

/// Default baudrate supported by this SPI (in bps)
#define SPI_BAUD_4MHZ         4000000

/// Default baudrate supported by this SPI (in bps)
#define SPI_DEFAULT_BAUD      512000


#define SPI_CLK_DIVID_SET(baud) 		(NUMBER_ROUND_UP(SPI_DEFAULT_CLK/2, baud))


//#define GPIO_SPI_CFG            REG_APB5_GPIOA_CFG
#define GPIO_SPI_NSS_PIN        6
#define GPIO_SPI_SCK_PIN        7
#define GPIO_SPI_MOSI_PIN       8
#define GPIO_SPI_MISO_PIN       9






// SPI

#define  REG_SPI_RST 			(*((volatile unsigned int *)(BK3000_SPI_BASE_ADDR + 2*4)))

#define REG_SPI_CTRL				(*((volatile unsigned int *)(BK3000_SPI_BASE_ADDR + 4*4)))
#define REG_SPI_CN  				(*((volatile unsigned int *)(BK3000_SPI_BASE_ADDR + 5*4)))
#define REG_SPI_STAT				(*((volatile unsigned int *)(BK3000_SPI_BASE_ADDR + 6*4)))
#define REG_SPI_DAT			   	    (*((volatile unsigned int *)(BK3000_SPI_BASE_ADDR + 7*4)))

#define REG_SPI_DAT_CHAR			 (*((volatile unsigned char *)(BK3000_SPI_BASE_ADDR + 7*4)))



#define spi_Write_Byte(v)               (REG_SPI_DAT = (v & 0xff))
#define BIT_SPI_CTRL_SPIEN              23
#define BIT_SPI_CTRL_MSTEN              22
#define BIT_SPI_CTRL_CKPHA              21
#define BIT_SPI_CTRL_CKPOL              20
#define BIT_SPI_CTRL_BIT_WIDTH          18
#define BIT_SPI_CTRL_WIRE3_EN           17
#define BIT_SPI_CTRL_NSSMD              16
#define BIT_SPI_CTRL_SPI_CKR            8
#define BIT_SPI_CTRL_RXINT_EN           7
#define BIT_SPI_CTRL_TXINT_EN           6
#define BIT_SPI_CTRL_RXOVR_EN           5
#define BIT_SPI_CTRL_TXOVR_EN           4
#define BIT_SPI_CTRL_RXFIFO_CLR         3
#define BIT_SPI_CTRL_TXFIFO_CLR         2
#define BIT_SPI_CTRL_RXINT_MODE         1
#define BIT_SPI_CTRL_TXINT_MODE         0

#define BIT_SPI_STAT_SPIBUSY            15
#define BIT_SPI_STAT_SLVSEL             14
#define BIT_SPI_STAT_RXOVR              12
#define BIT_SPI_STAT_TXOVR              11
#define BIT_SPI_STAT_MODF               10
#define BIT_SPI_STAT_RXINT              9
#define BIT_SPI_STAT_TXINT              8
#define BIT_SPI_STAT_RXFIFO_FULL        3
#define BIT_SPI_STAT_RXFIFO_READY       2
#define BIT_SPI_STAT_TXFIFO_READY       1
#define BIT_SPI_STAT_TXFIFO_EMPTY       0



#define SPI_TX_FIFO_EMPTY               (REG_SPI_STAT & (0x01 << BIT_SPI_STAT_TXFIFO_EMPTY))
#define SPI_TX_FIFO_FULL                (!(REG_SPI_STAT & (0x01 << BIT_SPI_STAT_TXFIFO_READY)))    //changed by ypj 20181204
//#define SPI_RX_FIFO_EMPTY               (REG_SPI_STAT & (0x01 << BIT_SPI_STAT_RXFIFO_EMPTY)) 
#define SPI_RX_FIFO_EMPTY               (!(REG_SPI_STAT & (0x01 << BIT_SPI_STAT_RXFIFO_READY)))    //changed by ypj 20181204
#define SPI_RX_FIFO_FULL                (REG_SPI_STAT & (0x01 << BIT_SPI_STAT_RXFIFO_FULL))

#define GET_SPI_TX_INT_STAT()              (REG_SPI_STAT & (0x01 << BIT_SPI_STAT_TXINT))
#define CLEAR_SPI_TX_INT_STAT()            ((REG_SPI_STAT &= ~(0x01 << BIT_SPI_STAT_TXINT)))



/*
 * ENUMERATION DEFINITIONS
 *****************************************************************************************
 */




typedef enum
{
  SPI_SLAVE = 0,
	SPI_MASTER = 1
}SPI_ROLE;


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Initializes the SPI to default values.
 *****************************************************************************************
 */

void spi_initial(SPI_ROLE role,uint32_t baud);

uint8_t spi_write_read(uint8_t *wbuf, uint32_t w_size, uint8_t *rbuf, uint32_t r_size,uint32_t r_offset);



uint8_t spi_write_read_4K_W(uint8_t *wbuf, uint32_t w_size, uint8_t *rbuf, uint32_t r_size,uint32_t r_offset);

uint8_t spi_write_read_4K_R(uint8_t *wbuf, uint32_t w_size, uint8_t *rbuf, uint32_t r_size,uint32_t r_offset);

uint8_t spi_write_read_4K_R_for_crc(uint8_t *wbuf, uint32_t w_size, uint8_t *rbuf, uint32_t r_size,uint32_t r_offset);

uint8_t spi_is_busy(void);

#if 1
void spi_tx_start(void);

void spi_tx_end(void);

uint8_t spi_write_read_no_cs(uint8_t *wbuf, uint32_t w_size, uint8_t *rbuf, uint32_t r_size,uint32_t r_offset);

#endif
#endif /* BL_SPI_H_ */

/*************************************************************
 * @file        driver_uart0.c
 * @brief       code of UART0 driver of BK7231
 * @author      GuWenFu
 * @version     V1.0
 * @date        2016-09-29
 * @par
 * @attention
 *
 * @history     2016-09-29 gwf    create this file
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "BK_System.h"
#include "driver_system.h"
#include "driver_gpio.h"
#include "driver_uart0.h"


#define UART0_DEFAULT_CLK           PER_CLK
#define UART0_BAUD_115200           115200
#define UART0_DEFAULT_BAUD          UART0_BAUD_115200
#define UART0_CLK_DIVID_SET         MAX((NUMBER_ROUND_UP(UART0_DEFAULT_CLK, UART0_DEFAULT_BAUD) - 1), 4)


#define UART0_WRITE_BYTE(v)         (REG_UART0_FIFO_PORT = (v) & 0x00ff)
#define UART0_READ_BYTE()           ((REG_UART0_FIFO_PORT & 0xff00) >> 0x8)
#define UART0_TX_FIFO_EMPTY         (REG_UART0_FIFO_STATUS & UART0_FIFO_STATUS_TX_FIFO_EMPTY_MASK)
#define UART0_TX_WRITE_READY        (REG_UART0_FIFO_STATUS & UART0_FIFO_STATUS_WR_READY_MASK)
#define UART0_RX_READ_READY         (REG_UART0_FIFO_STATUS & UART0_FIFO_STATUS_RD_READY_MASK)

u32 uart_dl_port = DEBUG_PORT_UART0;
unsigned char uart0_rx_buf[UART0_RX_FIFO_MAX_COUNT];
unsigned char uart0_tx_buf[UART0_TX_FIFO_MAX_COUNT];
volatile bool uart0_rx_done = FALSE;
volatile unsigned long uart0_rx_index = 0;

void uart0_init(unsigned long ulBaudRate)
{
    unsigned long ul_freq_divide;
    unsigned long ul_f_divide;

    REG_UART0_CONFIG = 0x00;        // by gwf

        if (ulBaudRate == 0)
        {
                ul_f_divide = NUMBER_ROUND_UP(UART0_DEFAULT_CLK, UART0_DEFAULT_BAUD);
                ul_f_divide = ul_f_divide - 1;
                ul_freq_divide = MIN((MAX(ul_f_divide, 4)) , 0x1FFFUL);
        }
        else
        {
                ul_f_divide = NUMBER_ROUND_UP(UART0_DEFAULT_CLK, ulBaudRate);
                ul_f_divide = ul_f_divide - 1;
                ul_freq_divide = MIN((MAX(ul_f_divide, 4)) , 0x1FFFUL);
        }
        // UART clock enable
        ICU_PERI_CLK_PWD_SET(ICU_PERI_CLK_PWD_UART0_MASK);
        //    ICU_PERI_CLK_GATE_DIS_CLEAR(ICU_PERI_CLK_GATE_DIS_UART0_MASK);

    GPIO_UART_function_enable(0);

    // UART HW Rx FIFO threshold is 8 to avoid set RxNeedRead=1 interrupt when Rx every character.
    REG_UART0_FIFO_CONFIG = (0x40  << UART0_FIFO_CONFIG_TX_THRESHOLD_POSI)
                            |(0x30  << UART0_FIFO_CONFIG_RX_THRESHOLD_POSI)
                            |(0  << UART0_FIFO_CONFIG_RX_STOP_TIME_POSI);
#if 1
    REG_UART0_INT_ENABLE  = (0x1 << UART0_INT_ENABLE_RX_NEED_RD_POSI)       // UART Data RX interrupt enable
                            | (0x1 << UART0_INT_ENABLE_RX_STOP_END_POSI);     // Enable Rx Stop for 4-Byte-time of no-data
#else
    REG_UART0_INT_ENABLE  &= ~((0x1 << UART0_INT_ENABLE_RX_NEED_RD_POSI)       // UART Data RX interrupt enable
                               | (0x1 << UART0_INT_ENABLE_RX_STOP_END_POSI));     // Disable Rx Stop for 4-Byte-time of no-data
#endif

    REG_UART0_FLOW_CONTROL = 0x00;
    REG_UART0_WAKEUP_CONFIG = 0x00;

    REG_UART0_CONFIG = UART0_CONFIG_TX_ENABLE_SET  
                       |UART0_CONFIG_RX_ENABLE_SET
                       |UART0_CONFIG_IRDA_MODE_CLEAR
                       |UART0_CONFIG_LENGTH_8_BIT
                       |UART0_CONFIG_PAR_ENABLE_CLEAR
                       |UART0_CONFIG_PAR_MODE_EVEN
                       |UART0_CONFIG_STOP_LEN_1_BIT
                       |(ul_freq_divide << UART0_CONFIG_CLK_DIVID_POSI); 
    uart0_rx_done = FALSE;
    uart0_rx_index = 0;

    ICU_INT_ENABLE_SET(ICU_INT_ENABLE_IRQ_UART0_MASK);
}

void uart0_disable(void)
{
    //REG_UART0_CONFIG =0;
    REG_UART0_CONFIG &= ~((0x01 << UART0_CONFIG_TX_ENABLE_POSI)
                        |(0x01 << UART0_CONFIG_RX_ENABLE_POSI)
                        |(0x03 << UART0_CONFIG_LENGTH_POSI));

    REG_UART0_FIFO_CONFIG &= ~((0x40 << UART0_FIFO_CONFIG_TX_THRESHOLD_POSI)
                        |(0x30 << UART0_FIFO_CONFIG_RX_THRESHOLD_POSI)) ;
        
    REG_UART0_INT_ENABLE &= ~((0x1 << UART0_INT_ENABLE_RX_NEED_RD_POSI)  // UART Data RX interrupt disble
                        |(0x1 << UART0_INT_ENABLE_RX_STOP_END_POSI));   // Disable Rx Stop for 4-Byte-time of no-data

    ICU_INT_ENABLE_CLEAR(ICU_INT_ENABLE_IRQ_UART0_MASK);
    //GPIO_UART_function_disable(0);
    ICU_PERI_CLK_PWD_CLEAR(ICU_PERI_CLK_PWD_UART0_MASK);
}
//----------------------------------------------
// UART0 Interrupt Service Rountine
//----------------------------------------------
void UART0_InterruptHandler(void)
{
    unsigned long uart0_int_status;
    //    unsigned char uart0_fifo_rdata;
    u32 val;

    //    uart0_send_string("in UART0_InterruptHandler\r\n");
    uart0_int_status = REG_UART0_INT_STATUS;
    if (uart0_int_status & (UART0_INT_ENABLE_RX_NEED_RD_MASK | UART0_INT_ENABLE_RX_STOP_END_MASK))
    {
            if(boot_downloading == TRUE && ((!uart_dl_port) || (uart_dl_port == DEBUG_PORT_UART0)))
            {
                while(UART0_FIFO_STATUS_RD_READY_GET)
                {
                    if(!uart_dl_port)
                    {
                        uart_dl_port = DEBUG_PORT_UART0;
                    }
                    val = (unsigned char)UART0_READ_BYTE();
                    uart_download_rx(val);
                }
            }
            else
            {
                while (UART0_RX_READ_READY)
                {
                    //            uart0_fifo_rdata = (unsigned char)UART0_READ_BYTE();
                    uart0_rx_buf[uart0_rx_index++] = (unsigned char)UART0_READ_BYTE();
                    if (uart0_rx_index == UART0_RX_FIFO_MAX_COUNT)
                    {
                        uart0_rx_index = 0;
                    }
                }
                if (uart0_int_status & UART0_INT_ENABLE_RX_STOP_END_MASK)
                {
                    uart0_rx_done = TRUE;
                }
            }
    }

    REG_UART0_INT_STATUS = uart0_int_status;
}

//----------------------------------------------
// UART Tx/Rx character driver
//----------------------------------------------
static void uart0_send_byte(unsigned char data)
{
    while (!UART0_TX_WRITE_READY);
    UART0_WRITE_BYTE(data);
   	//bk_printf("\r\n ");
	//bk_print_hex(data);
}
void uart0_send(unsigned char *buff, int len)
{
    while (len--)
    {	
        uart0_send_byte(*buff++);
    }
}

void uart0_send_string(const char *buff)
{
    if(uart_dl_port == DEBUG_PORT_UART0 && boot_downloading == TRUE)
    {
        return ;
    }

    while (*buff)
        uart0_send_byte(*buff++);
}

void uart0_wait_tx_finish(void)
{
    while (UART0_TX_FIFO_EMPTY == 0)
    {
    }
}


#if (PRINT_PORT == DEBUG_PORT_UART0)
struct __FILE
{
    int handle;     // Add whatever you need here
};
FILE __stdout;
FILE __stdin;

static unsigned char uart0_receive_byte(void)
{
    while (!UART0_RX_READ_READY);
    return ((unsigned char)UART0_READ_BYTE());
}

/*----------------------------------------------------------------------------
 * fputc
 */
int fputc(int ch, FILE *f)
{
    if(uart_dl_port == DEBUG_PORT_UART0 && boot_downloading == TRUE)
    {
        return ch;
    }

    if (ch == '\n')
    {
        uart0_send_byte('\r');
        uart0_send_byte('\n');
        return ch;
    }
    uart0_send_byte(ch);

    return ch;
}

/*----------------------------------------------------------------------------
 * fgetc
 */
int fgetc(FILE *f)
{
        int ch = -1;
#if 1
        if(uart_dl_port == DEBUG_PORT_UART0 && boot_downloading == TRUE)
        {
                return ch;
        }
#endif

        while (REG_UART0_FIFO_STATUS & UART0_FIFO_STATUS_RX_FIFO_EMPTY_MASK)
        {
        }
        ch = uart0_receive_byte();

        return (ch);
}
#endif
//eof


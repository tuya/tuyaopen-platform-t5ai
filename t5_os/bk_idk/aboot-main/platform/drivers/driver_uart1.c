/*************************************************************
 * @file        driver_uart1.c
 * @brief       code of UART1 driver of BK7231
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

#include "BK_System.h"
#include "driver_system.h"
#include "driver_gpio.h"
#include "driver_uart1.h"


#define UART1_DEFAULT_CLK           PER_CLK
#define UART1_BAUD_115200           115200
#define UART1_BAUD_3M25             3250000
#define UART1_BAUD_6M5              6500000
#define UART1_DEFAULT_BAUD          UART1_BAUD_115200
#define UART1_CLK_DIVID_SET         MAX(NUMBER_ROUND_UP(UART1_DEFAULT_CLK, UART1_DEFAULT_BAUD), 4)


#define UART1_WRITE_BYTE(v)         (REG_UART1_FIFO_PORT = (v) & 0x00ff)
#define UART1_READ_BYTE()           (REG_UART1_FIFO_PORT & 0x00ff)
#define UART1_TX_FIFO_EMPTY         (REG_UART1_FIFO_STATUS & UART1_FIFO_STATUS_TX_FIFO_EMPTY_MASK)
#define UART1_TX_WRITE_READY        (REG_UART1_FIFO_STATUS & UART1_FIFO_STATUS_WR_READY_MASK)
#define UART1_RX_READ_READY         (REG_UART1_FIFO_STATUS & UART1_FIFO_STATUS_RD_READY_MASK)

unsigned char uart1_rx_buf[UART1_RX_FIFO_MAX_COUNT];
unsigned char uart1_tx_buf[UART1_TX_FIFO_MAX_COUNT];
volatile bool uart1_rx_done = FALSE;
volatile unsigned long uart1_rx_index = 0;


void uart1_init(unsigned long ulBaudRate)
{
    unsigned long ul_freq_divide;
    unsigned long ul_f_divide;

    REG_UART1_CONFIG = 0x00;        // by gwf

    if (ulBaudRate == 0)
    {
        ul_f_divide = NUMBER_ROUND_UP(UART1_DEFAULT_CLK, UART1_DEFAULT_BAUD);
        ul_f_divide = ul_f_divide - 1;
        ul_freq_divide = MIN((MAX(ul_f_divide, 4)) , 0x1FFFUL);
    }
    else
    {
        ul_f_divide = NUMBER_ROUND_UP(UART1_DEFAULT_CLK, ulBaudRate);
        ul_f_divide = ul_f_divide - 1;
        ul_freq_divide = MIN((MAX(ul_f_divide, 4)) , 0x1FFFUL);
    }

        // UART clock enable
        //ICU_PERI_CLK_PWD_CLEAR(ICU_PERI_CLK_PWD_UART1_MASK);
        ICU_PERI_CLK_PWD_SET(ICU_PERI_CLK_PWD_UART1_MASK);

    GPIO_UART_function_enable(1);

    // UART HW Rx FIFO threshold is 8 to avoid set RxNeedRead=1 interrupt when Rx every character.
    REG_UART1_FIFO_CONFIG = (0x40  << UART1_FIFO_CONFIG_TX_THRESHOLD_POSI)
                            | (0x30  << UART1_FIFO_CONFIG_RX_THRESHOLD_POSI)
                            | (0  << UART1_FIFO_CONFIG_RX_STOP_TIME_POSI);

#if 1
    REG_UART1_INT_ENABLE  = (0x1 << UART1_INT_ENABLE_RX_NEED_RD_POSI)    // UART Data RX interrupt enable
                            | (0x1 << UART1_INT_ENABLE_RX_STOP_END_POSI);    // Enable Rx Stop for 4-Byte-time of no-data
#else
    // disable rx interupt
    REG_UART1_INT_ENABLE  &= ~((0x1 << UART1_INT_ENABLE_RX_NEED_RD_POSI)       // UART Data RX interrupt enable
                               | (0x1 << UART1_INT_ENABLE_RX_STOP_END_POSI));     // Enable Rx Stop for 4-Byte-time of no-data
#endif

    REG_UART1_FLOW_CONTROL = 0x00;
    REG_UART1_WAKEUP_CONFIG = 0x00;

    REG_UART1_CONFIG = (0x01 << UART1_CONFIG_TX_ENABLE_POSI)
                       | (0x01 << UART1_CONFIG_RX_ENABLE_POSI)
                       | (0x00 << UART1_CONFIG_IRDA_MODE_POSI)
                       | (0x03 << UART1_CONFIG_LENGTH_POSI)
                       | (0x00 << UART1_CONFIG_PAR_ENABLE_POSI)
                       | (0x00 << UART1_CONFIG_PAR_MODE_POSI)
                       | (0x00 << UART1_CONFIG_STOP_LEN_POSI)
                       | (ul_freq_divide << UART1_CONFIG_CLK_DIVID_POSI);

    uart1_rx_done = FALSE;
    uart1_rx_index = 0;

    ICU_INT_ENABLE_SET(ICU_INT_ENABLE_IRQ_UART1_MASK);
}

void uart1_disable(void)
{
    //REG_UART1_CONFIG =0;
    REG_UART1_CONFIG &= ~((0x01 << UART1_CONFIG_TX_ENABLE_POSI)
                        |(0x01<< UART1_CONFIG_RX_ENABLE_POSI)
                        |(0x03UL << UART1_CONFIG_LENGTH_POSI));

    REG_UART1_FIFO_CONFIG &= ~((0x40 << UART1_FIFO_CONFIG_TX_THRESHOLD_POSI)
                        |(0x30 << UART1_FIFO_CONFIG_RX_THRESHOLD_POSI)) ;

    REG_UART1_INT_ENABLE &= ~((0x1 << UART1_INT_ENABLE_RX_NEED_RD_POSI)// UART Data RX interrupt disble
                        |(0x1 << UART1_INT_ENABLE_RX_STOP_END_POSI));  // Disable Rx Stop for 4-Byte-time of no-data
    ICU_INT_ENABLE_CLEAR(ICU_INT_ENABLE_IRQ_UART1_MASK);
    //GPIO_UART_function_disable(1);
    ICU_PERI_CLK_PWD_CLEAR(ICU_PERI_CLK_PWD_UART1_MASK);
}

//----------------------------------------------
// UART1 Interrupt Service Rountine
//----------------------------------------------
void UART1_InterruptHandler(void)
{
    unsigned long uart1_int_status;
    u32 val;

    uart1_int_status = REG_UART1_INT_STATUS;

    if (uart1_int_status & (UART1_INT_ENABLE_RX_NEED_RD_MASK | UART1_INT_ENABLE_RX_STOP_END_MASK))
    {

#if 1
        if(boot_downloading == TRUE && ((!uart_dl_port) || (uart_dl_port == DEBUG_PORT_UART1)))
        {
            while(UART1_FIFO_STATUS_RD_READY_GET)
            {
                if(!uart_dl_port)
                {
                    uart_dl_port = DEBUG_PORT_UART1;
                }
                val = (unsigned char)UART1_READ_BYTE();
                uart_download_rx(val);
            }
        }
        else
#endif
        {
            while (UART1_RX_READ_READY)
            {
                uart1_rx_buf[uart1_rx_index++] = (unsigned char)UART1_READ_BYTE();
                if (uart1_rx_index == UART1_RX_FIFO_MAX_COUNT)
                {
                    uart1_rx_index = 0;
                }
            }
            if (uart1_int_status & UART1_INT_ENABLE_RX_STOP_END_MASK)
            {
                uart1_rx_done = TRUE;
            }
        }
    }

    REG_UART1_INT_STATUS = uart1_int_status;
}


//----------------------------------------------
// UART Tx/Rx character driver
//----------------------------------------------
static void uart1_send_byte(unsigned char data)
{
    while (!UART1_TX_WRITE_READY);
    UART1_WRITE_BYTE(data);
}

void uart1_send(unsigned char *buff, int len)
{
    while (len--)
        uart1_send_byte(*buff++);
}

void uart1_send_string(const char *buff)
{
#if 1
    if(uart_dl_port == DEBUG_PORT_UART1 && boot_downloading == TRUE)
    {
        return ;
    }
#endif

    while (*buff)
        uart1_send_byte(*buff++);
}

void uart1_wait_tx_finish(void)
{
    while (UART1_TX_FIFO_EMPTY == 0)
    {
    }
}



#if (PRINT_PORT == DEBUG_PORT_UART1)
struct __FILE
{
    int handle;     // Add whatever you need here
};
FILE __stdout;
FILE __stdin;

static u8 uart1_receive_byte(void)
{
    while (!UART1_RX_READ_READY);
    return ((unsigned char)UART1_READ_BYTE());
}

/*----------------------------------------------------------------------------
 * fputc
 */
int fputc(int ch, FILE *f)
{
    //    REG_APB5_GPIOB_DATA = 0xFF;
#if 1
    if(uart_dl_port == DEBUG_PORT_UART1 && boot_downloading == TRUE)
    {
        return ch;
    }
#endif

    if (ch == '\n')
    {
        uart1_send_byte('\r');
        uart1_send_byte('\n');
        return ch;
    }
    uart1_send_byte(ch);
    //    REG_APB5_GPIOB_DATA = 0x00;
    return ch;
}

/*----------------------------------------------------------------------------
 * fgetc
 */
int fgetc(FILE *f)
{
    int ch = -1;
#if 1
    if(uart_dl_port == DEBUG_PORT_UART1 && boot_downloading == TRUE)
    {
        return ch;
    }
#endif

    while (REG_UART1_FIFO_STATUS & UART1_FIFO_STATUS_RX_FIFO_EMPTY_MASK)
    {
    }
    ch = uart1_receive_byte();

    return (ch);
}
#endif



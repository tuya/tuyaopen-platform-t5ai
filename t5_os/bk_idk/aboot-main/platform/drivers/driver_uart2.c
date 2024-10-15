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
#if ((SOC_BK7271 == CFG_SOC_NAME) || (SOC_BK7256 == CFG_SOC_NAME))
#include "driver_system.h"
#include "driver_gpio.h"
#include "driver_uart2.h"

#define UART2_DEFAULT_CLK           PER_CLK
#define UART2_BAUD_115200           115200
#define UART2_BAUD_3M25             3250000
#define UART2_BAUD_6M5              6500000
#define UART2_DEFAULT_BAUD          UART2_BAUD_115200
#define UART2_CLK_DIVID_SET         MAX(NUMBER_ROUND_UP(UART2_DEFAULT_CLK, UART2_DEFAULT_BAUD), 4)

#define UART2_WRITE_BYTE(v)         (REG_UART2_FIFO_PORT = (v) & 0x00ff)
#define UART2_READ_BYTE()           (REG_UART2_FIFO_PORT & 0x00ff)
#define UART2_TX_FIFO_EMPTY         (REG_UART2_FIFO_STATUS & UART2_FIFO_STATUS_TX_FIFO_EMPTY_MASK)
#define UART2_TX_WRITE_READY        (REG_UART2_FIFO_STATUS & UART2_FIFO_STATUS_WR_READY_MASK)
#define UART2_RX_READ_READY         (REG_UART2_FIFO_STATUS & UART2_FIFO_STATUS_RD_READY_MASK)

unsigned char uart2_rx_buf[UART2_RX_FIFO_MAX_COUNT];
unsigned char uart2_tx_buf[UART2_TX_FIFO_MAX_COUNT];
volatile bool uart2_rx_done = FALSE;
volatile unsigned long uart2_rx_index = 0;

void uart2_init(unsigned long ulBaudRate)
{
    unsigned long ul_freq_divide;
    unsigned long ul_f_divide;

    REG_UART2_CONFIG = 0x00;

    if (ulBaudRate == 0)
    {
        ul_f_divide = NUMBER_ROUND_UP(UART2_DEFAULT_CLK, UART2_DEFAULT_BAUD);
        ul_f_divide = ul_f_divide - 1;
        ul_freq_divide = MIN((MAX(ul_f_divide, 4)) , 0x1FFFUL);
    }
    else
    {
        ul_f_divide = NUMBER_ROUND_UP(UART2_DEFAULT_CLK, ulBaudRate);
        ul_f_divide = ul_f_divide - 1;
        ul_freq_divide = MIN((MAX(ul_f_divide, 4)) , 0x1FFFUL);
    }

        // UART clock enable
        //ICU_PERI_CLK_PWD_CLEAR(ICU_PERI_CLK_PWD_UART2_MASK);
        ICU_PERI_CLK_PWD_SET(ICU_PERI_CLK_PWD_UART2_MASK);

    GPIO_UART_function_enable(2);

    // UART HW Rx FIFO threshold is 8 to avoid set RxNeedRead=1 interrupt when Rx every character.
    REG_UART2_FIFO_CONFIG = (0x40  << UART2_FIFO_CONFIG_TX_THRESHOLD_POSI)
                            | (0x30  << UART2_FIFO_CONFIG_RX_THRESHOLD_POSI)
                            | (0  << UART2_FIFO_CONFIG_RX_STOP_TIME_POSI);

    REG_UART2_INT_ENABLE  = (0x1 << UART2_INT_ENABLE_RX_NEED_RD_POSI)    // UART Data RX interrupt enable
                            | (0x1 << UART2_INT_ENABLE_RX_STOP_END_POSI);    // Enable Rx Stop for 4-Byte-time of no-data

    REG_UART2_FLOW_CONTROL = 0x00;
    REG_UART2_WAKEUP_CONFIG = 0x00;

    REG_UART2_CONFIG = (0x01 << UART2_CONFIG_TX_ENABLE_POSI)
                       | (0x01 << UART2_CONFIG_RX_ENABLE_POSI)
                       | (0x00 << UART2_CONFIG_IRDA_MODE_POSI)
                       | (0x03 << UART2_CONFIG_LENGTH_POSI)
                       | (0x00 << UART2_CONFIG_PAR_ENABLE_POSI)
                       | (0x00 << UART2_CONFIG_PAR_MODE_POSI)
                       | (0x00 << UART2_CONFIG_STOP_LEN_POSI)
                       | (ul_freq_divide << UART2_CONFIG_CLK_DIVID_POSI);

    uart2_rx_done = FALSE;
    uart2_rx_index = 0;

    ICU_INT_ENABLE_SET(ICU_INT_ENABLE_IRQ_UART2_MASK);
}

void uart2_disable(void)
{
    REG_UART2_CONFIG &= ~((0x01 << UART2_CONFIG_TX_ENABLE_POSI)
                        |(0x01 << UART2_CONFIG_RX_ENABLE_POSI)
                        |(0x03 << UART2_CONFIG_LENGTH_POSI));

    REG_UART2_FIFO_CONFIG &= ~((0x40 << UART2_FIFO_CONFIG_TX_THRESHOLD_POSI)
                        |(0x30 << UART2_FIFO_CONFIG_RX_THRESHOLD_POSI)) ;

    REG_UART2_INT_ENABLE &= ~((0x1 << UART2_INT_ENABLE_RX_NEED_RD_POSI)  // UART Data RX interrupt disble
                        |(0x1 << UART2_INT_ENABLE_RX_STOP_END_POSI));   // Disable Rx Stop for 4-Byte-time of no-data

    ICU_INT_ENABLE_CLEAR(ICU_INT_ENABLE_IRQ_UART2_MASK);
    //GPIO_UART_function_disable(0);
    ICU_PERI_CLK_PWD_CLEAR(ICU_PERI_CLK_PWD_UART2_MASK);
}

//----------------------------------------------
// UART1 Interrupt Service Rountine
//----------------------------------------------
void UART2_InterruptHandler(void)
{
    unsigned long uart2_int_status;
    u32 val;

    uart2_int_status = REG_UART2_INT_STATUS;

    if (uart2_int_status & (UART2_INT_ENABLE_RX_NEED_RD_MASK | UART2_INT_ENABLE_RX_STOP_END_MASK))
    {

#if 1
        if(boot_downloading == TRUE && ((!uart_dl_port) || (uart_dl_port == DEBUG_PORT_UART2)))
        {
            while(UART2_FIFO_STATUS_RD_READY_GET)
            {
                if(!uart_dl_port)
                {
                    uart_dl_port = DEBUG_PORT_UART2;
                }
                val = (unsigned char)UART2_READ_BYTE();
                uart_download_rx(val);
            }
        }
        else
#endif
        {
            while (UART2_RX_READ_READY)
            {
                uart2_rx_buf[uart2_rx_index++] = (unsigned char)UART2_READ_BYTE();
                if (uart2_rx_index == UART2_RX_FIFO_MAX_COUNT)
                {
                    uart2_rx_index = 0;
                }
            }
            if (uart2_int_status & UART2_INT_ENABLE_RX_STOP_END_MASK)
            {
                uart2_rx_done = TRUE;
            }
        }
    }

    REG_UART2_INT_STATUS = uart2_int_status;
}


//----------------------------------------------
// UART Tx/Rx character driver
//----------------------------------------------
static void uart2_send_byte(unsigned char data)
{
    while (!UART2_TX_WRITE_READY);
    UART2_WRITE_BYTE(data);
}

void uart2_send(unsigned char *buff, int len)
{
    while (len--)
        uart2_send_byte(*buff++);
}

void uart2_send_string(const char *buff)
{
#if 1
    if(uart_dl_port == DEBUG_PORT_UART2 && boot_downloading == TRUE)
    {
        return ;
    }
#endif

    while (*buff)
        uart2_send_byte(*buff++);
}

void uart2_wait_tx_finish(void)
{
    while (UART2_TX_FIFO_EMPTY == 0)
    {
    }
}

#endif

#if ((PRINT_PORT == DEBUG_PORT_UART2) && ((CFG_SOC_NAME == SOC_BK7271) || (CFG_SOC_NAME == SOC_BK7256)))
struct __FILE
{
    int handle;     // Add whatever you need here
};
FILE __stdout;
FILE __stdin;

static u8 uart2_receive_byte(void)
{
    while (!UART2_RX_READ_READY);
    return ((unsigned char)UART2_READ_BYTE());
}

/*----------------------------------------------------------------------------
 * fputc
 */
int fputc(int ch, FILE *f)
{
    //    REG_APB5_GPIOB_DATA = 0xFF;
#if 1
    if(uart_dl_port == DEBUG_PORT_UART2 && boot_downloading == TRUE)
    {
        return ch;
    }
#endif

    if (ch == '\n')
    {
        uart2_send_byte('\r');
        uart2_send_byte('\n');
        return ch;
    }
    uart2_send_byte(ch);
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
    if(uart_dl_port == DEBUG_PORT_UART2 && boot_downloading == TRUE)
    {
        return ch;
    }
#endif

    while (REG_UART2_FIFO_STATUS & UART2_FIFO_STATUS_RX_FIFO_EMPTY_MASK)
    {
    }
    ch = uart2_receive_byte();

    return (ch);
}
#endif



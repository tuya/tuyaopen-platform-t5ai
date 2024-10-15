/*************************************************************
 * @file        BK_config.h
 * @brief       Header file for system configuration
 * @author      GuWenFu
 * @version     V1.0
 * @date        2016-09-29
 * @par
 * @attention
 *
 * @history     2016-09-29 gwf    create this file
 */


#ifndef __CONFIG_H__

#define __CONFIG_H__


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <stdint.h>

#define ADC_3262N_STYLE
#define I2C0_3231_STYLE
#define I2C_FM_7211_STYLE
#define I2S_3252_STYLE
#define SPI_3231_STYLE
#define SPI_DMA_7211_STYLE
#define PWM_3231_STYLE
#define WDT_3231_STYLE
#define UART0_3231_STYLE
#define UART1_3231_STYLE
#define GPIO_3254_STYLE
#define FLASH_3231_STYLE


#define MCU_CLK_32KHz       32000
#define MCU_CLK_16MHz       16000000
#define MCU_CLK_26MHz       26000000
#define MCU_CLK_48MHz       48000000
#define MCU_CLK_96MHz       96000000
#define MCU_CLK_120MHz      120000000
#define MCU_CLK_180MHz      180000000

#define MCU_CLK             MCU_CLK_120MHz

#define SOC_BK7231                                 1
#define SOC_BK7231U                                2
#define SOC_BK7221U                                3
#define SOC_BK7271                                 4 //use BK7271.sct
#define SOC_BK7231N                                5 //use BK7231N.sct
#define SOC_BK7236                                 6 //use BK7236.sct
#define SOC_BK7256                                 7 //use BK7236.sct
#define CFG_SOC_NAME                               SOC_BK7256

#define PER_CLK_32KHz       32000
#define PER_CLK_16MHz       16000000
#define PER_CLK_26MHz       26000000
#define PER_CLK_48MHz       48000000
#define PER_CLK_96MHz       96000000
#define PER_CLK_120MHz      120000000

#define PER_CLK             PER_CLK_26MHz


#define DEBUG_PORT_UART0        1
#define DEBUG_PORT_UART1        2
#define DEBUG_PORT_SPI          3
#define DEBUG_PORT_I2C0         4
#define DEBUG_PORT_I2C1         5
#if ((CFG_SOC_NAME == SOC_BK7271) || (CFG_SOC_NAME == SOC_BK7256))
#define DEBUG_PORT_UART2        6
#endif
#define DEBUG_PORT_IO_SIM_UART  10

#define PRINT_PORT              DEBUG_PORT_UART1



    /*UART_DOWNLOAD_PORT != DEBUG_PORT ,UART_DOWNLOAD_PORT != PRINT_PORT*/
#define UART_DOWNLOAD_PORT      DEBUG_PORT_UART0

#if ((PRINT_PORT == UART_DOWNLOAD_PORT) || (DEBUG_PORT == UART_DOWNLOAD_PORT) )
    //#error "UART_DOWNLOAD_PORT set error!"
#endif

#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif      /* __CONFIG_H__ */

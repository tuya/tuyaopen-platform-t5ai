/*************************************************************
 * @file        driver_system.h
 * @brief       Header file of driver_system.c
 * @author      GuWenFu
 * @version     V1.0
 * @date        2016-09-29
 * @par
 * @attention
 *
 * @history     2016-09-29 gwf    create this file
 */

#ifndef __DRIVER_ICU_H__

#define __DRIVER_ICU_H__


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


#include "BK_config.h"


//bk7256 adapt
#define REG_ICU_BASE_ADDR                   (0x44010000UL)

#define ICU_PERI_CLK_PWD_UART0_POSI         2
#define ICU_PERI_CLK_PWD_UART1_POSI         10
#define ICU_PERI_CLK_PWD_UART2_POSI         11


#define ICU_PERI_CLK_PWD_UART0_MASK         (0x01UL << ICU_PERI_CLK_PWD_UART0_POSI)
#define ICU_PERI_CLK_PWD_UART1_MASK         (0x01UL << ICU_PERI_CLK_PWD_UART1_POSI)
#define ICU_PERI_CLK_PWD_UART2_MASK         (0x01UL << ICU_PERI_CLK_PWD_UART2_POSI)



#define REG_ICU_PERI_CLK_PWD_ADDR           (REG_ICU_BASE_ADDR + 0xc * 4)
#define REG_ICU_PERI_CLK_PWD                (*((volatile unsigned long *) REG_ICU_PERI_CLK_PWD_ADDR))

#define ICU_PERI_CLK_PWD_SET(x)             do {REG_ICU_PERI_CLK_PWD |=  (x);} while(0)
#define ICU_PERI_CLK_PWD_CLEAR(x)           do {REG_ICU_PERI_CLK_PWD &= ~ (x);} while(0)

#define REG_ICU_PWM_CLK_MUX_ADDR            (REG_ICU_BASE_ADDR + 0x8 * 4)
#define REG_ICU_PWM_CLK_MUX                 (*((volatile unsigned long *) REG_ICU_PWM_CLK_MUX_ADDR))


#define ICU_INT_ENABLE_IRQ_GPIO_POSI        55
#define ICU_INT_ENABLE_IRQ_UART0_POSI       4
#define ICU_INT_ENABLE_IRQ_UART1_POSI       15
#define ICU_INT_ENABLE_IRQ_UART2_POSI       16

#define ICU_INT_ENABLE_IRQ_GPIO_MASK        (0x01UL << ICU_INT_ENABLE_IRQ_GPIO_POSI)
#define ICU_INT_ENABLE_IRQ_UART0_MASK       (0x01UL << ICU_INT_ENABLE_IRQ_UART0_POSI)
#define ICU_INT_ENABLE_IRQ_UART1_MASK       (0x01UL << ICU_INT_ENABLE_IRQ_UART1_POSI)
#define ICU_INT_ENABLE_IRQ_UART2_MASK       (0x01UL << ICU_INT_ENABLE_IRQ_UART2_POSI)


#define REG_ICU_INT_ENABLE_ADDR             (REG_ICU_BASE_ADDR + 0x20 * 4)
//#define REG_ICU_INT_ENABLE_MASK             0x07FF7FFFUL
#define REG_ICU_INT_ENABLE                  (*((volatile unsigned long *) REG_ICU_INT_ENABLE_ADDR))

#define ICU_INT_ENABLE_SET(x)               do {                                    \
                REG_ICU_INT_ENABLE |=  (x);                                         \
            } while(0)
#define ICU_INT_ENABLE_CLEAR(x)             do {                                    \
                REG_ICU_INT_ENABLE &= ~(x);                                         \
            } while(0)

extern void ICU_init(void);

void sys_uart0_interrupt_strl(uint32 value);

#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif      /* __DRIVER_SYSTEM_H__ */

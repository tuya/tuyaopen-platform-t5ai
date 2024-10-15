/*************************************************************
 * @file        interrupt.h
 * @brief       Header file of interrupt.c
 * @author      GuWenFu
 * @version     V1.0
 * @date        2016-09-29
 * @par
 * @attention
 *
 * @history     2016-09-29 gwf    create this file
 */

#ifndef __DRIVER_INTRRUPT_H__
#define __DRIVER_INTRRUPT_H__

#include "BK_System.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

void int_init(void);
void int_enable(int index);
void int_disable(int index);

#if (SOC_BK7256 != CFG_SOC_NAME)
// IRQ handler
__IRQ void int_irq(void);

// FIQ handler
__FIQ void int_fiq(void);
#endif

void sys_forbidden_interrupts(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  /* __DRIVER_INTRRUPT_H__ */


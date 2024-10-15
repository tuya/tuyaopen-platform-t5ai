/*************************************************************
 * @file        interrupt.c
 * @brief       code of interrupt driver of BK7231
 * @author      GuWenFu
 * @version     V1.0
 * @date        2016-09-29
 * @par
 * @attention
 *
 * @history     2016-09-29 gwf    create this file
 */
#include <stdio.h>
#include "interrupt.h"
#include "BK_System.h"
#include "driver_system.h"
#include "driver_gpio.h"
#include "driver_uart0.h"
#include "driver_uart1.h"

extern void wdt_reboot(void);

void intc_start(void)
{
    u32 reg;
    u32 param;

    param = ICU_INT_GLOBAL_ENABLE_IRQ_SET;

    reg = REG_READ(REG_ICU_INT_GLOBAL_ENABLE_ADDR);
    reg |= param;
    REG_WRITE(REG_ICU_INT_GLOBAL_ENABLE_ADDR, reg);

    return;
}

void intc_disable_top(void)
{
    u32 reg;
    u32 param;

    param = 0xffffffff;
    reg = REG_READ(REG_ICU_INT_GLOBAL_ENABLE_ADDR);
    reg &= ~(param);
    REG_WRITE(REG_ICU_INT_GLOBAL_ENABLE_ADDR, reg);
}

void intc_forbidden_all(void)
{
    REG_WRITE(REG_ICU_INT_ENABLE_ADDR, 0);
    REG_WRITE(REG_ICU_INT_GLOBAL_ENABLE_ADDR, 0);
    REG_WRITE(REG_ICU_INT_RAW_STATUS_ADDR, REG_READ(REG_ICU_INT_RAW_STATUS_ADDR));
    REG_WRITE(REG_ICU_INT_STATUS_ADDR, REG_READ(REG_ICU_INT_STATUS_ADDR));
}

void sys_forbidden_interrupts(void)
{
    __disable_irq();
    intc_disable_top();
    intc_forbidden_all();
}

//__IRQ void int_irq(void)
void intc_irq(void)
{
    UINT32 irq_status;

    irq_status = REG_ICU_INT_STATUS & ICU_INT_ENABLE_IRQ_MASK;

    if (irq_status & ICU_INT_ENABLE_IRQ_UART0_MASK)
    {
        UART0_InterruptHandler();
    }

    if (irq_status & ICU_INT_ENABLE_IRQ_UART1_MASK)
    {
        UART1_InterruptHandler();
    }
    #if (CFG_SOC_NAME != SOC_BK7231)
    if (irq_status & ICU_INT_ENABLE_IRQ_TIMER_MASK)
    {
        bk_timer_isr();
    }
    #endif
}

void do_irq(void)
{
    intc_irq();
}

void jump_pc_address(u32 ptr)
{
    typedef void (*jump_point)(void);
    jump_point jump = (jump_point)ptr;

    jump();
}

void bad_mode (void)
{
    wdt_reboot();
}

void do_undefined_instruction (void)
{
    *((volatile uint32_t *)START_TYPE_ADDR) = (uint32_t)CRASH_UNDEFINED_VALUE;
    bad_mode ();
}

void do_software_interrupt (void)
{
}

void do_prefetch_abort (void)
{
    *((volatile uint32_t *)START_TYPE_ADDR) = (uint32_t)CRASH_PREFETCH_ABORT_VALUE;
    bad_mode ();
}

void do_data_abort (void)
{
    *((volatile uint32_t *)START_TYPE_ADDR) = (uint32_t)CRASH_DATA_ABORT_VALUE;
    bad_mode ();
}

void do_not_used (void)
{
    *((volatile uint32_t *)START_TYPE_ADDR) = (uint32_t)CRASH_UNUSED_VALUE;
    bad_mode ();
}

void do_fiq(void)
{
}
// eof


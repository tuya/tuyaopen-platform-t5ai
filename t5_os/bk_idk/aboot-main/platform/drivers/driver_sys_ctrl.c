/*************************************************************
 * @file        driver_sys_ctrl.c
 * @brief       code of sys control driver of BK7231
 * @author      GuWenFu
 * @version     V1.0
 * @date        2016-09-29
 * @par
 * @attention
 *
 * @history     2016-09-29 gwf    create this file
 */

#include <stdio.h>

#include "BK_System.h"
#include "driver_sys_ctrl.h"


#if ((CFG_SOC_NAME == SOC_BK7231N) || (CFG_SOC_NAME == SOC_BK7236))
#define GPIO_WAKEUP_INT_BAK_ADDR 0x00808000
#else
#define GPIO_WAKEUP_INT_BAK_ADDR 0x0080a084
#endif

#define GPIO_WAKEUP_INT_STATUS_BAK  (GPIO_WAKEUP_INT_BAK_ADDR + 0x1 * 4)
#define GPIO_WAKEUP_INT_STATUS1_BAK  (GPIO_WAKEUP_INT_BAK_ADDR + 0x2 * 4)

unsigned long get_chip_id(void)
{
    return REG_SYS_CTRL_CHIP_ID;
}

unsigned long get_device_id(void)
{
    return REG_SYS_CTRL_DEVICE_ID;
}

void sys_ctrl_save_deep_gpio_wake_status(void)
{
	unsigned int reg;

	REG_WRITE(GPIO_WAKEUP_INT_STATUS_BAK, REG_READ(SCTRL_GPIO_WAKEUP_INT_STATUS));
#if (CFG_SOC_NAME != SOC_BK7231N) && (CFG_SOC_NAME != SOC_BK7236)
	REG_WRITE(GPIO_WAKEUP_INT_STATUS1_BAK, REG_READ(SCTRL_GPIO_WAKEUP_INT_STATUS1));
#endif

	reg = REG_READ(SCTRL_SLEEP);
	reg &= ~GPIO_SLEEP_SWITCH_BIT;
	REG_WRITE(SCTRL_SLEEP, reg);
}



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "BK_System.h"
#include "driver_system.h"
#include "driver_gpio.h"
#include "drv_pwm.h"

#define WDT_FAILURE                (1)
#define WDT_SUCCESS                (0)

#define WDT_DEV_NAME                "wdt"

#define WDT_CMD_MAGIC              (0xe330000)
enum
{
    WCMD_POWER_UP = WDT_CMD_MAGIC + 1,
	WCMD_SET_PERIOD,
	WCMD_RELOAD_PERIOD,
    WCMD_POWER_DOWN
};

#if (CFG_SOC_NAME == SOC_BK7271)
#define WDT_BASE                                     (0x00802700)
#elif (CFG_SOC_NAME == SOC_BK7256)
#define WDT_BASE                                     (0x44000600)
#define WDT1_BASE                                    (0x44800000)
#else
#define WDT_BASE                                     (0x00802900)
#endif

#define WDT_CTRL_REG                                     (WDT_BASE + 0 * 4)
#define WDT1_CTRL_REG                                    (WDT1_BASE + 0 * 4)
#define WDT_KEY_POSI                                              (16)
#define WDT_KEY_MASK                                              (0xFF)
#define WDT_1ST_KEY                                               (0x5A)
#define WDT_2ND_KEY                                               (0xA5)

#define WDT_PERIOD_POSI                                           (0)
#define WDT_PERIOD_MASK                                           (0xFFFF)

static uint32_t g_wdt_period = 0;
extern void DelayUS(volatile unsigned long timesUS);

UINT32 wdt_ctrl(UINT32 cmd, void *param)
{
	UINT32 ret;
	UINT32 reg;
	UINT32 parameter;

	ret = WDT_SUCCESS;	
	switch(cmd)
	{		
		case WCMD_POWER_DOWN:
			g_wdt_period = 0;
			parameter = PWD_ARM_WATCHDOG_CLK_BIT;
            #if (CFG_SOC_NAME == SOC_BK7271)
            reg = REG_READ(ICU_FUNC_CLK_PWD);
            reg |= parameter;
            REG_WRITE(ICU_FUNC_CLK_PWD, reg);
            #else
            reg = REG_READ(REG_ICU_PERI_CLK_PWD_ADDR);
            reg |= parameter;
            REG_WRITE(REG_ICU_PERI_CLK_PWD_ADDR, reg);
            #endif
			break;
			
		case WCMD_POWER_UP:
			parameter = PWD_ARM_WATCHDOG_CLK_BIT;
            #if (CFG_SOC_NAME == SOC_BK7271)
            reg = REG_READ(ICU_FUNC_CLK_PWD);
            reg &= ~(parameter);
            REG_WRITE(ICU_FUNC_CLK_PWD, reg);
            #else
            reg = REG_READ(REG_ICU_PERI_CLK_PWD_ADDR);
            reg &= ~(parameter);
            REG_WRITE(REG_ICU_PERI_CLK_PWD_ADDR, reg);
            #endif
			break;
			
		case WCMD_RELOAD_PERIOD:
			reg = WDT_1ST_KEY << WDT_KEY_POSI;
			reg |= (g_wdt_period & WDT_PERIOD_MASK) << WDT_PERIOD_POSI;
			REG_WRITE(WDT_CTRL_REG, reg);

            DelayUS(300);
	
			reg = WDT_2ND_KEY << WDT_KEY_POSI;
			reg |= (g_wdt_period & WDT_PERIOD_MASK) << WDT_PERIOD_POSI;
			REG_WRITE(WDT_CTRL_REG, reg);
        
            DelayUS(300);
			break;

		case WCMD_SET_PERIOD:				
			g_wdt_period = (*(UINT32 *)param);
			
			reg = WDT_1ST_KEY << WDT_KEY_POSI;
			reg |= ((*(UINT32 *)param) & WDT_PERIOD_MASK) << WDT_PERIOD_POSI;
			REG_WRITE(WDT_CTRL_REG, reg);

			DelayUS(300);

			reg = WDT_2ND_KEY << WDT_KEY_POSI;
			reg |= ((*(UINT32 *)param) & WDT_PERIOD_MASK) << WDT_PERIOD_POSI;
			REG_WRITE(WDT_CTRL_REG, reg);
        
            DelayUS(300);
			break;
			
		default:
			break;
	}
	
    return ret;
}

void wdt_reboot()
{
    UINT32 wdt_val = 6;

    wdt_ctrl( WCMD_SET_PERIOD, &wdt_val);
    wdt_ctrl( WCMD_POWER_UP, NULL);
    while(1);
}
void wdt_close()
{
	REG_WRITE(WDT_CTRL_REG, 0x5A0000);
	REG_WRITE(WDT_CTRL_REG, 0xA50000);
	REG_WRITE(WDT1_CTRL_REG, 0x5A0000);
	REG_WRITE(WDT1_CTRL_REG, 0xA50000);
}


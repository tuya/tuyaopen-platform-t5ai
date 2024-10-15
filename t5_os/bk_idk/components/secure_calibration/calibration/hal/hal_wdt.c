// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "wdt_hal.h"
#include "sys_hal.h"

//************************************************************//
//AON_PMU
//************************************************************//
#define BASEADDR_AON_PMU            (0x44000000)
#define AON_PMU_BASE_ADDR           (BASEADDR_AON_PMU)

#define AON_PMU_REG_0x41            (AON_PMU_BASE_ADDR + 0x41*4)
#define SET_WDT_CLK_26M_DIV         (*((volatile u32 *)(AON_PMU_REG_0x41)) &= ~0x03)

//************************************************************//
//WDT
//************************************************************//
#define WDT_BASE_ADDR               (0x44800000)
#define AON_WDT_BASE_ADDR           (0x44000600)

#define WDT_CTRL_REG                (WDT_BASE_ADDR + 0x4*4)
#define AON_WDT_CTRL_REG            (AON_WDT_BASE_ADDR + 0x0*4)

#define WDT_RESET_CFG_REG           (AON_PMU_BASE_ADDR + 0x2*4)
#define WDT_RESET_ALL               (0x1FF)
#define WDT_RESET_DEVS              (WDT_RESET_ALL)

static wdt_hal_t s_hal = {0};

int hal_wdt_init(void)
{
	return wdt_hal_init(&s_hal);
}

int hal_wdt_start(uint32_t timeout)
{
	return wdt_hal_init_wdt(&s_hal, timeout);
}

int hal_wdt_stop(void)
{
	wdt_hal_reset_config_to_default(&s_hal);
	sys_hal_clk_pwr_ctrl(CLK_PWR_ID_WDG_CPU, CLK_PWR_CTRL_PWR_DOWN);
	return BK_OK;
}

void hal_wdt_close(void)
{
	REG_WRITE(0x44000600, 0x5A0000);
	REG_WRITE(0x44000600, 0xA50000);
	REG_BITS_SET(0x44800000 + 4 * 2, 1, 1, 1);
	REG_WRITE(0x44800000 + 4 * 4, 0x5A0000);
	REG_WRITE(0x44800000 + 4 * 4, 0xA50000);
}


void wdt_time_set(u32 val)
{
	u32 reset_dev = 0;

	reset_dev = REG_READ(WDT_RESET_CFG_REG);
	reset_dev &= ~(WDT_RESET_ALL);
	reset_dev |= WDT_RESET_DEVS;

	REG_WRITE(WDT_RESET_CFG_REG, reset_dev);
	REG_WRITE(WDT_CTRL_REG, (0x5A0000 | val));
	REG_WRITE(WDT_CTRL_REG, (0xA50000 | val));

	REG_WRITE(AON_WDT_CTRL_REG, (0x5A0000 | val));
	REG_WRITE(AON_WDT_CTRL_REG, (0xA50000 | val));
}
void wdt_reboot()
{
	/*Mp version need this -normal wdt*/
	//*((volatile u32 *) (0x20007FF8)) = 0xAA55AA55; // set the user reset request flag.
	SET_WDT_CLK_26M_DIV;
	//*((volatile uint32_t *)SAVE_JUMPAPP_ADDR) = 0x0;
	wdt_time_set(6);

	while(1);
}
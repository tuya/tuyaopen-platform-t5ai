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

#include "sys_types.h"
#include "sys_hal.h"
#include "sys_ll.h"

static sys_hal_t s_sys_hal;

bk_err_t sys_hal_init()
{
	s_sys_hal.hw = (sys_hw_t *)SYS_LL_REG_BASE;
	return BK_OK;
}

/* UART select clock **/
void sys_hal_uart_select_clock(uart_id_t id, uart_src_clk_t mode)
{
	int sel_xtal = 0;
	int sel_appl = 1;

	switch(id)
	{
		case UART_ID_0:
			{
				if(mode == UART_SCLK_APLL)
					sys_ll_set_cpu_clk_div_mode1_clksel_uart0(sel_appl);
				else
					sys_ll_set_cpu_clk_div_mode1_clksel_uart0(sel_xtal);
				break;
			}
		case UART_ID_1:
			{
				if(mode == UART_SCLK_APLL)
					sys_ll_set_cpu_clk_div_mode1_cksel_uart1(sel_appl);
				else
					sys_ll_set_cpu_clk_div_mode1_cksel_uart1(sel_xtal);
				break;
			}
		case UART_ID_2:
			{
				if(mode == UART_SCLK_APLL)
					sys_ll_set_cpu_clk_div_mode1_cksel_uart2(sel_appl);
				else
					sys_ll_set_cpu_clk_div_mode1_cksel_uart2(sel_xtal);
				break;
			}
		default:
			break;
	}

}

/* NOTICE: NOTICE: NOTICE: NOTICE: NOTICE: NOTICE: NOTICE
 * BK7256 clock, power is different with previous products(2022-01-10).
 * Previous products peripheral devices use only one signal of clock enable.
 * BK7256 uses clock and power signal to control one device,
 * This function only enable clock signal, we needs to enable power signal also
 * if we want to enable one device.
 */
void sys_hal_clk_pwr_ctrl(dev_clk_pwr_id_t dev, dev_clk_pwr_ctrl_t power_up)
{
	uint32 v = sys_ll_get_cpu_device_clk_enable_value();

	if(CLK_PWR_CTRL_PWR_UP == power_up)
		v |= (1 << dev);
	else
		v &= ~(1 << dev);

	sys_ll_set_cpu_device_clk_enable_value(v);
}

void sys_hal_flash_set_dpll(void)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_flash(FLASH_CLK_APLL);
}




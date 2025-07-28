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


#include <common/bk_include.h>
#include "slcd_ll_macro_def.h"
#include <driver/slcd_types.h>
#include "bk_misc.h"


void slcd_hal_soft_reset(void)
{
	slcd_ll_set_clk_control_soft_reset(0);
	bk_delay_us(10);
	slcd_ll_set_clk_control_soft_reset(1);
	bk_delay_us(10);
}

void slcd_hal_lcd_on(void)
{
	slcd_ll_set_ctrl_reg_lcd_on(0x1);
}

void slcd_hal_lcd_off(void)
{
	slcd_ll_set_ctrl_reg_lcd_on(0);
}

void slcd_hal_set_com_number(slcd_com_num_t com_num)
{
	slcd_ll_set_ctrl_reg_com_number(com_num);
}

void slcd_hal_set_lcd_bias(slcd_bias_t lcd_bias)
{
	slcd_ll_set_ctrl_reg_lcd_bias(lcd_bias);
}

void slcd_hal_set_lcd_rate(slcd_rate_t rate_value)
{
	slcd_ll_set_ctrl_reg_lcd_rate(rate_value);
}

void slcd_hal_set_seg_value(slcd_seg_id_t seg_id, uint8_t value)
{
	switch(seg_id)
	{
		case SLCD_SEG_0:
			slcd_ll_set_seg00_03_seg00(value);
			break;

		case SLCD_SEG_1:
			slcd_ll_set_seg00_03_seg01(value);
			break;

		case SLCD_SEG_2:
			slcd_ll_set_seg00_03_seg02(value);
			break;

		case SLCD_SEG_3:
			slcd_ll_set_seg00_03_seg03(value);
			break;

		case SLCD_SEG_4:
			slcd_ll_set_seg04_07_seg04(value);
			break;

		case SLCD_SEG_5:
			slcd_ll_set_seg04_07_seg05(value);
			break;

		case SLCD_SEG_6:
			slcd_ll_set_seg04_07_seg06(value);
			break;

		case SLCD_SEG_7:
			slcd_ll_set_seg04_07_seg07(value);
			break;

		case SLCD_SEG_8:
			slcd_ll_set_seg08_11_seg08(value);
			break;

		case SLCD_SEG_9:
			slcd_ll_set_seg08_11_seg09(value);
			break;

		case SLCD_SEG_10:
			slcd_ll_set_seg08_11_seg10(value);
			break;

		case SLCD_SEG_11:
			slcd_ll_set_seg08_11_seg11(value);
			break;

		case SLCD_SEG_12:
			slcd_ll_set_seg12_15_seg12(value);
			break;

		case SLCD_SEG_13:
			slcd_ll_set_seg12_15_seg13(value);
			break;

		case SLCD_SEG_14:
			slcd_ll_set_seg12_15_seg14(value);
			break;

		case SLCD_SEG_15:
			slcd_ll_set_seg12_15_seg15(value);
			break;

		case SLCD_SEG_16:
			slcd_ll_set_seg16_19_seg16(value);
			break;

		case SLCD_SEG_17:
			slcd_ll_set_seg16_19_seg17(value);
			break;

		case SLCD_SEG_18:
			slcd_ll_set_seg16_19_seg18(value);
			break;

		case SLCD_SEG_19:
			slcd_ll_set_seg16_19_seg19(value);
			break;

		case SLCD_SEG_20:
			slcd_ll_set_seg20_23_seg20(value);
			break;

		case SLCD_SEG_21:
			slcd_ll_set_seg20_23_seg21(value);
			break;

		case SLCD_SEG_22:
			slcd_ll_set_seg20_23_seg22(value);
			break;

		case SLCD_SEG_23:
			slcd_ll_set_seg20_23_seg23(value);
			break;

		case SLCD_SEG_24:
			slcd_ll_set_seg24_27_seg24(value);
			break;

		case SLCD_SEG_25:
			slcd_ll_set_seg24_27_seg25(value);
			break;

		case SLCD_SEG_26:
			slcd_ll_set_seg24_27_seg26(value);
			break;

		case SLCD_SEG_27:
			slcd_ll_set_seg24_27_seg27(value);
			break;

		case SLCD_SEG_28:
			slcd_ll_set_seg28_31_seg28(value);
			break;

		case SLCD_SEG_29:
			slcd_ll_set_seg28_31_seg29(value);
			break;

		case SLCD_SEG_30:
			slcd_ll_set_seg28_31_seg30(value);
			break;

		case SLCD_SEG_31:
			slcd_ll_set_seg28_31_seg31(value);
			break;
	}
}

void slcd_hal_set_seg00_03_value(uint32_t value)
{
    slcd_ll_set_seg00_03_value(value);
}

void slcd_hal_set_seg04_07_value(uint32_t value)
{
    slcd_ll_set_seg04_07_value(value);
}

void slcd_hal_set_seg08_11_value(uint32_t value)
{
    slcd_ll_set_seg08_11_value(value);
}

void slcd_hal_set_seg12_15_value(uint32_t value)
{
    slcd_ll_set_seg12_15_value(value);
}

void slcd_hal_set_seg16_19_value(uint32_t value)
{
    slcd_ll_set_seg16_19_value(value);
}

void slcd_hal_set_seg20_23_value(uint32_t value)
{
    slcd_ll_set_seg20_23_value(value);
}

void slcd_hal_set_seg24_27_value(uint32_t value)
{
    slcd_ll_set_seg24_27_value(value);
}

void slcd_hal_set_seg28_31_value(uint32_t value)
{
    slcd_ll_set_seg28_31_value(value);
}


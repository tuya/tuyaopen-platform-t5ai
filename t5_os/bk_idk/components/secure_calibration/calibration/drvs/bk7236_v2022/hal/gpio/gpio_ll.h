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

#pragma once

#include "soc.h"
#include "hal_gpio_types.h"
#include "gpio_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef volatile struct {
	uint32_t gpio_0_31_int_status;
	uint32_t gpio_32_64_int_status;
} gpio_interrupt_status_t;

#define GPIO_LL_REG_BASE			(SOC_AON_GPIO_REG_BASE)
#define GPIO_LL_SYSTEM_REG_BASE		(SOC_SYSTEM_REG_BASE + 0x30 * 4)
#define GPIO_NUM_MAX                (SOC_GPIO_NUM)
#define GPIO_EXIST                  (0xffffffffffff)

extern system_gpio_func_mode_hw_t *gpio_system_gpio_func_mode;

static inline void gpio_ll_init(gpio_hw_t *hw)
{
	gpio_system_gpio_func_mode = (system_gpio_func_mode_hw_t *)GPIO_LL_SYSTEM_REG_BASE;
}

static inline void gpio_ll_set_io_mode(gpio_hw_t *hw, uint32 index, const gpio_config_t *config)
{
	uint32 io_mode = 0;

	switch (config->io_mode) {
	case GPIO_OUTPUT_ENABLE:
		io_mode = 0;
		break;

	case GPIO_INPUT_ENABLE:
		io_mode = 3;
		break;

	case GPIO_IO_DISABLE:
		io_mode = 2;
		break;

	default:
		break;
	}

	REG_SET_BITS(&hw->gpio_num[index], GPIO_F_IO_MODE_SET(io_mode), GPIO_F_IO_MODE_EN_M) ;

}


static inline void gpio_ll_set_pull_mode(gpio_hw_t *hw, uint32 index, const gpio_config_t *config)
{
	uint32 pull_mode = 0;

	switch (config->pull_mode) {
	case GPIO_PULL_DISABLE:
		pull_mode = 0;
		break;

	case GPIO_PULL_DOWN_EN:
		pull_mode = 2;
		break;

	case GPIO_PULL_UP_EN:
		pull_mode = 3;
		break;

	default:
		break;
	}

	REG_SET_BITS(&hw->gpio_num[index], GPIO_F_PULL_SET(pull_mode), GPIO_F_PULL_EN_M) ;
}

static inline void gpio_ll_set_func_mode(gpio_hw_t *hw, uint32 index, const gpio_config_t *config)
{
	switch (config->func_mode) {
	case GPIO_SECOND_FUNC_DISABLE:
		REG_CLR_BIT(&hw->gpio_num[index], BIT(6));
		break;
	case GPIO_SECOND_FUNC_ENABLE:
		REG_SET_BIT(&hw->gpio_num[index], BIT(6));
		break;
	default:
		break;
	}
}

static inline void gpio_ll_set_mode(gpio_hw_t *hw, uint32 index, const gpio_config_t *config)
{
	gpio_ll_set_io_mode(hw, index, config);
	gpio_ll_set_pull_mode(hw, index, config);
	gpio_ll_set_func_mode(hw, index, config);
}

//GPIO output enbale low active
static inline void gpio_ll_output_enable(gpio_hw_t *hw, uint32 index, uint32_t enable)
{
	if (enable)
		enable = 0;
	else
		enable = 1;

	hw->gpio_num[index].cfg.gpio_output_en = enable;
}

//GPIO input enable, high active
static inline void gpio_ll_input_enable(gpio_hw_t *hw, uint32 index, uint32_t enable)
{
	hw->gpio_num[index].cfg.gpio_input_en = enable;
}

static inline void gpio_ll_pull_up_enable(gpio_hw_t *hw, uint32 index, uint32_t enable)
{
	hw->gpio_num[index].cfg.gpio_pull_mode = enable;
}

static inline void gpio_ll_pull_enable(gpio_hw_t *hw, uint32 index, uint32_t enable)
{
	hw->gpio_num[index].cfg.gpio_pull_mode_en = enable;
}

static inline void gpio_ll_second_function_enable(gpio_hw_t *hw, uint32 index, uint32_t enable)
{

	hw->gpio_num[index].cfg.gpio_2_func_en = enable;
}

static inline void gpio_ll_monitor_input_value_enable(gpio_hw_t *hw, uint32 index, uint32_t enable)
{
	hw->gpio_num[index].cfg.gpio_input_monitor = enable;
}

static inline void gpio_ll_set_capacity(gpio_hw_t *hw, uint32 index, uint32_t capacity)
{
	hw->gpio_num[index].cfg.gpio_capacity = capacity;
}


static inline uint32 gpio_ll_check_func_mode_enable(gpio_hw_t *hw, uint32 index)
{
	return (hw->gpio_num[index].cfg.gpio_2_func_en == 1);
}

static inline uint32 gpio_ll_check_output_enable(gpio_hw_t *hw, uint32 index)
{
	return (hw->gpio_num[index].cfg.gpio_output_en == 0);
}

static inline uint32 gpio_ll_check_input_enable(gpio_hw_t *hw, uint32 index)
{
	return (hw->gpio_num[index].cfg.gpio_input_en == 1);
}


static inline void gpio_ll_set_output_value(gpio_hw_t *hw, uint32 index, bool value)
{
	hw->gpio_num[index].cfg.gpio_output = value;
}

static inline bool gpio_ll_get_input_value(gpio_hw_t *hw, uint32 index)
{
	return hw->gpio_num[index].cfg.gpio_input;
}


#define gpio_ll_set_gpio_output_value(hw,index,value)		gpio_ll_set_output_value(hw, index, value)
#define gpio_ll_get_gpio_input_value(hw,index)			gpio_ll_get_input_value(hw, index)


static inline void gpio_ll_set_interrupt_type(gpio_hw_t *hw, uint32 index, gpio_int_type_t mode)
{
	hw->gpio_num[index].cfg.gpio_int_type = mode;
}

static inline void gpio_ll_enable_interrupt(gpio_hw_t *hw, uint32 index)
{
	hw->gpio_num[index].cfg.gpio_int_ena = 1;
}

static inline void gpio_ll_disable_interrupt(gpio_hw_t *hw, uint32 index)
{
	hw->gpio_num[index].cfg.gpio_int_ena = 0;
}

static inline uint32 gpio_ll_get_interrupt_status(gpio_hw_t *hw, gpio_interrupt_status_t* gpio_status)
{
	return 0;
}

static inline void gpio_ll_clear_interrupt_status(gpio_hw_t *hw, gpio_interrupt_status_t *gpio_status)
{
}

static inline void gpio_ll_clear_chan_interrupt_status(gpio_hw_t *hw, uint32 index)
{
	hw->gpio_num[index].cfg.gpio_int_clr = 1;
}

static inline bool gpio_ll_is_interrupt_triggered(gpio_hw_t *hw, uint32 index, gpio_interrupt_status_t *gpio_status)
{
	if (index < GPIO_32)
		return !!((gpio_status->gpio_0_31_int_status) & (GPIO_F_INT_EN << (GPIO_F_INT_EN_MS(index))));
	else
		return !!((gpio_status->gpio_32_64_int_status) & (GPIO_F_INT_EN << (GPIO_F_INT_EN_MS(index-GPIO_32))));

}


static inline void gpio_ll_set_perial_mode(gpio_hw_t *hw, uint32 index, uint32_t mode)
{
	if (index < GPIO_8)
		REG_MCHAN_SET_FIELD(index, &gpio_system_gpio_func_mode->gpio_sys_num[0], GPIO_F_PERIAL_MODE, mode);
	else if (index < GPIO_16)
		REG_MCHAN_SET_FIELD(index-GPIO_8, &gpio_system_gpio_func_mode->gpio_sys_num[1], GPIO_F_PERIAL_MODE, mode);
	else if (index < GPIO_24)
		REG_MCHAN_SET_FIELD(index-GPIO_16, &gpio_system_gpio_func_mode->gpio_sys_num[2], GPIO_F_PERIAL_MODE, mode);
	else if (index < GPIO_32)
		REG_MCHAN_SET_FIELD(index-GPIO_24, &gpio_system_gpio_func_mode->gpio_sys_num[3], GPIO_F_PERIAL_MODE, mode);
	else if (index < GPIO_40)
		REG_MCHAN_SET_FIELD(index-GPIO_32, &gpio_system_gpio_func_mode->gpio_sys_num[4], GPIO_F_PERIAL_MODE, mode);
	else
		REG_MCHAN_SET_FIELD(index-GPIO_40, &gpio_system_gpio_func_mode->gpio_sys_num[5], GPIO_F_PERIAL_MODE, mode);
}

static inline uint32 gpio_ll_get_perial_mode(gpio_hw_t *hw, uint32 index)
{
	if (index < GPIO_8)
		return (REG_MCHAN_GET_FIELD(index, &gpio_system_gpio_func_mode->gpio_sys_num[0], GPIO_F_PERIAL_MODE));
	else if (index < GPIO_16)
		return ((REG_MCHAN_GET_FIELD(index-GPIO_8, &gpio_system_gpio_func_mode->gpio_sys_num[1], GPIO_F_PERIAL_MODE)));
	else if (index < GPIO_24)
		return ((REG_MCHAN_GET_FIELD(index-GPIO_16, &gpio_system_gpio_func_mode->gpio_sys_num[2], GPIO_F_PERIAL_MODE)));
	else if (index < GPIO_32)
		return ((REG_MCHAN_GET_FIELD(index-GPIO_24, &gpio_system_gpio_func_mode->gpio_sys_num[3], GPIO_F_PERIAL_MODE)));
	else if (index < GPIO_40)
		return ((REG_MCHAN_GET_FIELD(index-GPIO_32, &gpio_system_gpio_func_mode->gpio_sys_num[4], GPIO_F_PERIAL_MODE)));
	else
		return ((REG_MCHAN_GET_FIELD(index-GPIO_40, &gpio_system_gpio_func_mode->gpio_sys_num[5], GPIO_F_PERIAL_MODE)));
}

/* gpio save */
static inline void gpio_ll_reg_save(uint32_t*  gpio_cfg)
{
    int  i = 0;
    if(gpio_cfg == NULL)
		return;

    for(i = 0; i < GPIO_NUM_MAX; i ++)
    {
        //if(GPIO_EXIST & BIT(i))//temp modify
        {
            gpio_cfg[i] = REG_READ(GPIO_LL_REG_BASE+i*4);
        }
    }
}

/* gpio restore */
static inline void gpio_ll_reg_restore(uint32_t*  gpio_cfg)
{
    int i = 0;
    if(gpio_cfg == NULL)
        return;
    for(i = 0; i < GPIO_NUM_MAX; i ++)
    {
        //if(GPIO_EXIST & BIT(i))//temp modify
        {
            REG_WRITE(GPIO_LL_REG_BASE+i*4, gpio_cfg[i]);
        }
    }
}

/* config gpio wakeup type and enable. @type_l: low/high or pos/nege. @type_h: level or edge */
static inline void gpio_ll_wakeup_enable(uint64_t index, uint64_t type_l, uint64_t type_h)
{
}

static inline void gpio_ll_wakeup_interrupt_clear()
{
}
#define gpio_ll_set_gpio_perial_mode(hw, index, mode)	gpio_ll_set_perial_mode(hw, index, mode)
#define gpio_ll_get_gpio_perial_mode(hw, index)			gpio_ll_get_perial_mode(hw, index)


#ifdef __cplusplus
}
#endif


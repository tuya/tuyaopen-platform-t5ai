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
#include <os/os.h>
#include "sys_driver.h"
#include "slcd_hal.h"
#include "gpio_map.h"
#include "gpio_driver.h"
#include <driver/gpio.h>
#include <slcd_driver.h>
#include <driver/slcd_types.h>


static void slcd_gpio_init(void)
{
    int i = 0;
    const slcd_gpio_map_t slcd_gpio_map[] = SLCD_GPIO_MAP;

    for (i = 0; i < sizeof(slcd_gpio_map) / sizeof(slcd_gpio_map_t); i++) {
        gpio_dev_unmap(slcd_gpio_map[i].gpio_id);
        gpio_dev_map(slcd_gpio_map[i].gpio_id, slcd_gpio_map[i].dev);
    }
}

void bk_slcd_set_com_port_enable(uint8_t com_enable)
{
    #if SLCD_COM_NUM == 4
    sys_drv_set_ana_com_port_enable(com_enable & 0xF);
    #elif SLCD_COM_NUM == 8
    sys_drv_set_ana_com_port_enable(com_enable & 0xF);
    uint32_t enable_status = sys_drv_get_ana_seg_port_enable_status();
    sys_drv_set_ana_seg_port_enable(((com_enable << 24) & 0xF0000000) | enable_status);
    #endif
}

void bk_slcd_set_seg_port_enable(uint32_t seg_enable)
{
    #if SLCD_COM_NUM == 4
    sys_drv_set_ana_seg_port_enable(seg_enable);
    #elif SLCD_COM_NUM == 8
    uint32_t enable_status = sys_drv_get_ana_seg_port_enable_status();
    sys_drv_set_ana_seg_port_enable((seg_enable & 0x0FFFFFFF) | (enable_status & 0xF0000000));
    #endif
}

void bk_slcd_set_seg_value(slcd_seg_id_t seg_id, uint8_t value)
{
    slcd_hal_set_seg_value(seg_id, value);
}

void bk_slcd_set_seg00_03_value(uint32_t value)
{
    slcd_hal_set_seg00_03_value(value);
}

void bk_slcd_set_seg04_07_value(uint32_t value)
{
    slcd_hal_set_seg04_07_value(value);
}

void bk_slcd_set_seg08_11_value(uint32_t value)
{
    slcd_hal_set_seg08_11_value(value);
}

void bk_slcd_set_seg12_15_value(uint32_t value)
{
    slcd_hal_set_seg12_15_value(value);
}

void bk_slcd_set_seg16_19_value(uint32_t value)
{
    slcd_hal_set_seg16_19_value(value);
}

void bk_slcd_set_seg20_23_value(uint32_t value)
{
    slcd_hal_set_seg20_23_value(value);
}

void bk_slcd_set_seg24_27_value(uint32_t value)
{
    slcd_hal_set_seg24_27_value(value);
}

void bk_slcd_set_seg28_31_value(uint32_t value)
{
    slcd_hal_set_seg28_31_value(value);
}

void bk_slcd_driver_init(slcd_config_t slcd_config)
{
    slcd_gpio_init();

    slcd_hal_soft_reset();

    if (slcd_config.slcd_bias == SLCD_BIAS_1_PER_OF_3) {
        sys_drv_set_ana_sw_bias(1);
    } else {
        sys_drv_set_ana_sw_bias(0);
    }

    sys_drv_set_ana_crb(0xE);

    slcd_hal_set_com_number(slcd_config.com_num);

    slcd_hal_set_lcd_bias(slcd_config.slcd_bias);

    slcd_hal_set_lcd_rate(slcd_config.slcd_rate);

    sys_drv_slcd_clock_enable();

    slcd_hal_lcd_on();

    sys_drv_set_ana_lcd_enable();
}

void bk_slcd_driver_deinit(void)
{
    sys_drv_set_ana_lcd_disable();
    slcd_hal_lcd_off();
    sys_drv_slcd_clock_disable();
}



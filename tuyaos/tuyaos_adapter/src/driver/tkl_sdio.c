/*
 * tkl_sdio.c
 * Copyright (C) 2025 cc <cc@tuya>
 *
 * Distributed under terms of the TUYA license.
 */


#include <driver/gpio.h>
#include "gpio_driver.h"
#include "tuya_cloud_types.h"
#include "sdkconfig.h"

#define TKL_SDIO_G0_CLK         GPIO_2
#define TKL_SDIO_G0_CMD         GPIO_3
#define TKL_SDIO_G0_D0          GPIO_4
#define TKL_SDIO_G0_D1          GPIO_5
#define TKL_SDIO_G0_D2          GPIO_10
#define TKL_SDIO_G0_D3          GPIO_11

#define TKL_SDIO_G1_CLK         GPIO_14
#define TKL_SDIO_G1_CMD         GPIO_15
#define TKL_SDIO_G1_D0          GPIO_16
#define TKL_SDIO_G1_D1          GPIO_17
#define TKL_SDIO_G1_D2          GPIO_18
#define TKL_SDIO_G1_D3          GPIO_19

struct tkl_sdio_pin_s {
    gpio_id_t clk;
    gpio_id_t cmd;
    gpio_id_t d0;
    gpio_id_t d1;
    gpio_id_t d2;
    gpio_id_t d3;
};

static struct tkl_sdio_pin_s current_sdio_gpio = {
    .clk = TKL_SDIO_G0_CLK,
    .cmd = TKL_SDIO_G0_CMD,
    .d0  = TKL_SDIO_G0_D0,
    .d1  = TKL_SDIO_G0_D1,
    .d2  = TKL_SDIO_G0_D2,
    .d3  = TKL_SDIO_G0_D3,
};

void __tkl_sdio_set_clk_pin(TUYA_SDIO_NUM_E port, TUYA_PIN_NAME_E pin)
{
    current_sdio_gpio.clk = pin;
}

void __tkl_sdio_set_cmd_pin(TUYA_SDIO_NUM_E port, TUYA_PIN_NAME_E pin)
{
    current_sdio_gpio.cmd = pin;
}

void __tkl_sdio_set_d0_pin(TUYA_SDIO_NUM_E port, TUYA_PIN_NAME_E pin)
{
    current_sdio_gpio.d0 = pin;
}

void __tkl_sdio_set_d1_pin(TUYA_SDIO_NUM_E port, TUYA_PIN_NAME_E pin)
{
    current_sdio_gpio.d1 = pin;
}

void __tkl_sdio_set_d2_pin(TUYA_SDIO_NUM_E port, TUYA_PIN_NAME_E pin)
{
    current_sdio_gpio.d2 = pin;
}

void __tkl_sdio_set_d3_pin(TUYA_SDIO_NUM_E port, TUYA_PIN_NAME_E pin)
{
    current_sdio_gpio.d3 = pin;
}

static int __tkl_sdio_gpio_check(void)
{
    static const struct tkl_sdio_pin_s valid_pin_groups[] = {
        {TKL_SDIO_G0_CLK, TKL_SDIO_G0_CMD, TKL_SDIO_G0_D0, TKL_SDIO_G0_D1, TKL_SDIO_G0_D2, TKL_SDIO_G0_D3},
        {TKL_SDIO_G1_CLK, TKL_SDIO_G1_CMD, TKL_SDIO_G1_D0, TKL_SDIO_G1_D1, TKL_SDIO_G1_D2, TKL_SDIO_G1_D3},
    };

    for (int i = 0; i < sizeof(valid_pin_groups) / sizeof(valid_pin_groups[0]); i++) {
        if ((current_sdio_gpio.clk == valid_pin_groups[i].clk) &&
            (current_sdio_gpio.cmd == valid_pin_groups[i].cmd) &&
            (current_sdio_gpio.d0  == valid_pin_groups[i].d0)
#if CONFIG_SDIO_4LINES_EN
            && (current_sdio_gpio.d1  == valid_pin_groups[i].d1)
            &&(current_sdio_gpio.d2  == valid_pin_groups[i].d2)
            &&(current_sdio_gpio.d3  == valid_pin_groups[i].d3)
#endif
        ) {
            return BK_OK;
        }
    }

    return BK_FAIL;
}

void user_sdio_gpio_init(void)
{
    BK_ASSERT(__tkl_sdio_gpio_check() == BK_OK);

    bk_printf("sdio init, %d %d %d\r\n", current_sdio_gpio.clk, current_sdio_gpio.cmd, current_sdio_gpio.d0);

	gpio_dev_unmap(current_sdio_gpio.clk);
	gpio_dev_unmap(current_sdio_gpio.cmd);
	gpio_dev_unmap(current_sdio_gpio.d0);
#if CONFIG_SDIO_4LINES_EN
	gpio_dev_unmap(current_sdio_gpio.d1);
	gpio_dev_unmap(current_sdio_gpio.d2);
	gpio_dev_unmap(current_sdio_gpio.d3);
#endif


#if CONFIG_SDIO_4LINES_EN
    if (current_sdio_gpio.d0 == GPIO_4)
        gpio_sdio_sel(GPIO_SDIO_MAP_MODE0);
    else
        gpio_sdio_sel(GPIO_SDIO_MAP_MODE1);
#else
    if (current_sdio_gpio.d0 == GPIO_4)
        gpio_sdio_one_line_sel(GPIO_SDIO_MAP_MODE0);
    else
        gpio_sdio_one_line_sel(GPIO_SDIO_MAP_MODE1);
#endif


#if 0
	gpio_dev_map(current_sdio_gpio.clk, GPIO_DEV_SDIO_HOST_CLK);
	gpio_dev_map(current_sdio_gpio.cmd, GPIO_DEV_SDIO_HOST_CMD);
	gpio_dev_map(current_sdio_gpio.d0, GPIO_DEV_SDIO_HOST_DATA0);
#if CONFIG_SDIO_4LINES_EN
	gpio_dev_map(current_sdio_gpio.d1, GPIO_DEV_SDIO_HOST_DATA1);
	gpio_dev_map(current_sdio_gpio.d2, GPIO_DEV_SDIO_HOST_DATA2);
	gpio_dev_map(current_sdio_gpio.d3, GPIO_DEV_SDIO_HOST_DATA3);
#endif // CONFIG_SDIO_4LINES_EN
#endif

	bk_gpio_pull_up(current_sdio_gpio.cmd);
	bk_gpio_pull_up(current_sdio_gpio.clk);
	bk_gpio_pull_up(current_sdio_gpio.d0);
#if CONFIG_SDIO_4LINES_EN
	bk_gpio_pull_up(current_sdio_gpio.d1);
	bk_gpio_pull_up(current_sdio_gpio.d2);
	bk_gpio_pull_up(current_sdio_gpio.d3);
#endif

	bk_gpio_set_capacity(current_sdio_gpio.cmd, 3);
	bk_gpio_set_capacity(current_sdio_gpio.clk, 3);
	bk_gpio_set_capacity(current_sdio_gpio.d0, 3);
#if CONFIG_SDIO_4LINES_EN
	bk_gpio_set_capacity(current_sdio_gpio.d1, 3);
	bk_gpio_set_capacity(current_sdio_gpio.d2, 3);
	bk_gpio_set_capacity(current_sdio_gpio.d3, 3);
#endif
}


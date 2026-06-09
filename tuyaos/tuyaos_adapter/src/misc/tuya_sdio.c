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
#include "usr_gpio_cfg.h"

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

static uint8_t tkl_sdio_line_mode = TUYA_SDIO_BUS_WIDTH_1BIT;
static const gpio_default_map_t s_default_map[] = GPIO_DEFAULT_DEV_CONFIG;

// gpio config
// 6 个 setter 体同构，仅赋值字段不同，用宏生成以消除重复，
// 同时保留各自独立的导出符号（tkl_pinmux.c 通过 switch 分别调用）。
#define TKL_SDIO_PIN_SETTERS(X) \
    X(clk) X(cmd) X(d0) X(d1) X(d2) X(d3)

#define TKL_SDIO_DEF_SETTER(field) \
    void __tkl_sdio_set_##field##_pin(TUYA_SDIO_NUM_E port, TUYA_PIN_NAME_E pin) \
    { \
        current_sdio_gpio.field = pin; \
    }

TKL_SDIO_PIN_SETTERS(TKL_SDIO_DEF_SETTER)

#undef TKL_SDIO_DEF_SETTER

// 运行期获取当前 SDIO 数据线宽，返回线数：1 或 4
uint8_t tuya_sdio_line_mode(void)
{
    return (tkl_sdio_line_mode == TUYA_SDIO_BUS_WIDTH_4BIT) ? 4 : 1;
}

static gpio_default_map_t* get_gpio_config(gpio_id_t gpio_id)
{
  for (int i = 0; i < sizeof(s_default_map)/sizeof(gpio_default_map_t); i++)
  {
    if (s_default_map[i].gpio_id == gpio_id)
    {
      return &s_default_map[i];
    }
  }
  return NULL;
}

static int __tkl_sdio_gpio_check(void)
{
    static const struct tkl_sdio_pin_s valid_pin_groups[] = {
        {TKL_SDIO_G0_CLK, TKL_SDIO_G0_CMD, TKL_SDIO_G0_D0, TKL_SDIO_G0_D1, TKL_SDIO_G0_D2, TKL_SDIO_G0_D3},
        {TKL_SDIO_G1_CLK, TKL_SDIO_G1_CMD, TKL_SDIO_G1_D0, TKL_SDIO_G1_D1, TKL_SDIO_G1_D2, TKL_SDIO_G1_D3},
    };
    bool four_bit;

    if (tkl_sdio_line_mode == TUYA_SDIO_BUS_WIDTH_1BIT) {
        four_bit = false;
    } else if (tkl_sdio_line_mode == TUYA_SDIO_BUS_WIDTH_4BIT) {
        four_bit = true;
    } else {
        return BK_FAIL;
    }

    for (int i = 0; i < sizeof(valid_pin_groups) / sizeof(valid_pin_groups[0]); i++) {
        const struct tkl_sdio_pin_s *g = &valid_pin_groups[i];

        if (current_sdio_gpio.clk != g->clk ||
            current_sdio_gpio.cmd != g->cmd ||
            current_sdio_gpio.d0  != g->d0) {
            continue;
        }
        if (four_bit &&
            (current_sdio_gpio.d1 != g->d1 ||
             current_sdio_gpio.d2 != g->d2 ||
             current_sdio_gpio.d3 != g->d3)) {
            continue;
        }
        return BK_OK;
    }

    return BK_FAIL;
}

static inline __sdio_gpio_capacity_set(gpio_id_t id)
{
    gpio_default_map_t *cfg = get_gpio_config(id);
    if (cfg)
        bk_gpio_set_capacity(id, cfg->driver_capacity);
    else
        bk_gpio_set_capacity(id, 1);        // default
}

void user_sdio_gpio_init(void)
{
    // check
    BK_ASSERT(__tkl_sdio_gpio_check() == BK_OK);

    bool four_bit = (tkl_sdio_line_mode == TUYA_SDIO_BUS_WIDTH_4BIT);

    // 1-line 用 clk/cmd/d0；4-line 再加 d1/d2/d3
    gpio_id_t pins[] = {
        current_sdio_gpio.clk,
        current_sdio_gpio.cmd,
        current_sdio_gpio.d0,
        current_sdio_gpio.d1,
        current_sdio_gpio.d2,
        current_sdio_gpio.d3,
    };
    int pin_num = four_bit ? 6 : 3;

    bk_printf("sdio init, %d %d %d", current_sdio_gpio.clk, current_sdio_gpio.cmd, current_sdio_gpio.d0);
    if (four_bit) {
        bk_printf(" %d %d %d", current_sdio_gpio.d1, current_sdio_gpio.d2, current_sdio_gpio.d3);
    }
    bk_printf("\r\n");

    // unmap
    for (int i = 0; i < pin_num; i++) {
        gpio_dev_unmap(pins[i]);
    }

    // sdio function map
    if (four_bit) {
        if (current_sdio_gpio.d0 == GPIO_4)
            gpio_sdio_sel(GPIO_SDIO_MAP_MODE0);
        else
            gpio_sdio_sel(GPIO_SDIO_MAP_MODE1);
    } else {
        if (current_sdio_gpio.d0 == GPIO_4)
            gpio_sdio_one_line_sel(GPIO_SDIO_MAP_MODE0);
        else
            gpio_sdio_one_line_sel(GPIO_SDIO_MAP_MODE1);
    }

    // pull up
    for (int i = 0; i < pin_num; i++) {
        bk_gpio_pull_up(pins[i]);
    }

    // set capacity
    for (int i = 0; i < pin_num; i++) {
        __sdio_gpio_capacity_set(pins[i]);
    }
}


/**
 * @brief tuya kernel sdio init
 *
 * @param[in] port: sdio unit number
 * @param[in] cfg: sdio config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 *
 * @note 目前该接口仅用于配置gpio模式，sdio驱动后续待接口完善后再适配,GPIO驱动能力
 *      非通用能力，T5依赖json配置获取
 */
OPERATE_RET tkl_sdio_init(int port, const TUYA_SDIO_BASE_CFG_T *cfg)
{
    if (cfg == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (cfg->bus_width == TUYA_SDIO_BUS_WIDTH_1BIT) {
        // 1-line mode
        tkl_sdio_line_mode = TUYA_SDIO_BUS_WIDTH_1BIT;
    } else if (cfg->bus_width == TUYA_SDIO_BUS_WIDTH_4BIT) {
        // 4-line mode
        tkl_sdio_line_mode = TUYA_SDIO_BUS_WIDTH_4BIT;
    } else {
        // not support
        bk_printf("sdio not support width %d\r\n", cfg->bus_width);
        return OPRT_INVALID_PARM;
    }

    (void) port;
    return OPRT_OK;
}


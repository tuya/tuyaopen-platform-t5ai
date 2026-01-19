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
#include "bk_intc.h"
#include "bk_icu.h"
#include <os/mem.h>
#include "gpio_hal.h"
#include "gpio_map.h"
#include "gpio_driver.h"
#include "gpio_driver_base.h"
#include "icu_driver.h"
#include <driver/gpio.h>
#include "aon_pmu_hal.h"
#include "sys_hal.h"
#if CONFIG_USR_GPIO_CFG_EN
#include "usr_gpio_cfg.h"
#endif

extern gpio_driver_t s_gpio;

#define GPIO_REG_LOCK_WAIT_TIME_MS  6
#define GPIO_RETURN_ON_INVALID_PERIAL_MODE(mode, mode_max) do {\
				if ((mode) >= (mode_max)) {\
					return BK_ERR_GPIO_SET_INVALID_FUNC_MODE;\
				}\
			} while(0)
#if CONFIG_GPIO_RETENTION_SUPPORT
#define GPIO_RETENTION_MAP_DUMP            (0)
#define GPIO_RETENTION_MAP_SIZE            (8)
#define GPIO_RETENTION_MAP                 {GPIO_24, GPIO_NUM_MAX, GPIO_NUM_MAX, GPIO_NUM_MAX, GPIO_NUM_MAX, GPIO_NUM_MAX, GPIO_NUM_MAX, GPIO_NUM_MAX}
#define GPIO_RETENTION_EN_CHECK(id, i, f)  ((id < GPIO_NUM_MAX) && (s_gpio_retention_en_bitmap & BIT(i) || f))

static uint32_t s_gpio_retention_map[GPIO_RETENTION_MAP_SIZE] = GPIO_RETENTION_MAP;
static uint32_t s_gpio_retention_en_bitmap = 0;
#endif
#if CONFIG_GPIO_DEFAULT_SET_SUPPORT
static const gpio_default_map_t s_default_map[] = GPIO_DEFAULT_DEV_CONFIG;

static const gpio_default_map_t* get_gpio_config(gpio_id_t gpio_id)
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
#endif

bk_err_t gpio_dev_map(gpio_id_t gpio_id, gpio_dev_t dev)
{
#if CONFIG_GPIO_DEFAULT_SET_SUPPORT
	const gpio_default_map_t *config = get_gpio_config(gpio_id);

	if (config == NULL)
	{
		GPIO_LOGW("GPIO%d not found in GPIO_DEFAULT_DEV_CONFIG table\r\n",gpio_id);
		return BK_ERR_GPIO_INVALID_OPERATE;
	}

#if 0
    if (config->gpio_time_sharing_multiplex == GPIO_TIME_SHARING_MULTIPLEX_DISABLE)
    {
      GPIO_LOGW("GPIO%d is not set time_sharing_multiplex, please set it in GPIO_DEFAULT_DEV_CONFIG table\r\n",gpio_id);
      return BK_ERR_GPIO_INVALID_OPERATE;
    }
#endif
#endif
	/* Restore a configuration that is not a secondary function to its initial state. */
	gpio_hal_output_enable(&s_gpio.hal, gpio_id, 0);
	gpio_hal_input_enable(&s_gpio.hal, gpio_id, 0);
	gpio_hal_pull_enable(&s_gpio.hal, gpio_id, 0);
	gpio_hal_disable_interrupt(&s_gpio.hal, gpio_id);
	gpio_hal_func_map(&s_gpio.hal, gpio_id, dev);

	return BK_OK;
}

bk_err_t gpio_dev_unmap(gpio_id_t gpio_id)
{
#if CONFIG_GPIO_DEFAULT_SET_SUPPORT
	const gpio_default_map_t *config = get_gpio_config(gpio_id);

	if (config == NULL)
	{
		GPIO_LOGW("GPIO%d not found in GPIO_DEFAULT_DEV_CONFIG table\r\n",gpio_id);
		return BK_ERR_GPIO_INVALID_OPERATE;
	}

#if 0
    if (config->gpio_time_sharing_multiplex == GPIO_TIME_SHARING_MULTIPLEX_DISABLE)
    {
      GPIO_LOGW("GPIO%d is not set time_sharing_multiplex, please set it in GPIO_DEFAULT_DEV_CONFIG table\r\n",gpio_id);
      return BK_ERR_GPIO_INVALID_OPERATE;
    }
#endif
#endif

	/* Restore a configuration that is not a secondary function to its initial state. */
	gpio_hal_output_enable(&s_gpio.hal, gpio_id, 0);
	gpio_hal_input_enable(&s_gpio.hal, gpio_id, 0);
	gpio_hal_pull_enable(&s_gpio.hal, gpio_id, 0);
	gpio_hal_disable_interrupt(&s_gpio.hal, gpio_id);
	gpio_hal_func_unmap(&s_gpio.hal, gpio_id);

	return BK_OK;
}

bk_err_t gpio_i2c1_sel(gpio_i2c1_map_mode_t mode)
{
	GPIO_RETURN_ON_INVALID_PERIAL_MODE(mode, GPIO_I2C1_MAP_MODE_MAX);

	GPIO_MAP_TABLE(GPIO_I2C1_USED_GPIO_NUM, GPIO_I2C1_MAP_MODE_MAX, i2c1_gpio_map) = GPIO_I2C1_MAP_TABLE;

	gpio_hal_devs_map(&s_gpio.hal, i2c1_gpio_map[mode].gpio_bits, i2c1_gpio_map[mode].devs, GPIO_I2C1_USED_GPIO_NUM);

	return BK_OK;
}

bk_err_t gpio_i2s_sel(gpio_i2s_map_mode_t mode)
{
	GPIO_RETURN_ON_INVALID_PERIAL_MODE(mode, GPIO_I2S_MAP_MODE_MAX);

	GPIO_MAP_TABLE(GPIO_I2S_USED_GPIO_NUM, GPIO_I2S_MAP_MODE_MAX, i2s_gpio_map) = GPIO_I2S_MAP_TABLE;

	gpio_hal_devs_map(&s_gpio.hal, i2s_gpio_map[mode].gpio_bits, i2s_gpio_map[mode].devs, GPIO_I2S_USED_GPIO_NUM);

	return BK_OK;
}

bk_err_t gpio_spi_sel(gpio_spi1_map_mode_t mode)
{
	GPIO_RETURN_ON_INVALID_PERIAL_MODE(mode, GPIO_SPI_MAP_MODE_MAX);

	GPIO_MAP_TABLE(GPIO_SPI0_USED_GPIO_NUM, GPIO_SPI_MAP_MODE_MAX, spi_gpio_map) = GPIO_SPI0_MAP_TABLE;

	gpio_hal_devs_map(&s_gpio.hal, spi_gpio_map[mode].gpio_bits, spi_gpio_map[mode].devs, GPIO_SPI0_USED_GPIO_NUM);

	return BK_OK;
}

bk_err_t gpio_sdio_sel(gpio_sdio_map_mode_t mode)
{
	GPIO_RETURN_ON_INVALID_PERIAL_MODE(mode, GPIO_SDIO_MAP_MODE_MAX);

	GPIO_MAP_TABLE(GPIO_SDIO_USED_GPIO_NUM, GPIO_SDIO_MAP_MODE_MAX, sdio_gpio_map) = GPIO_SDIO_MAP_TABLE;

	gpio_hal_devs_map(&s_gpio.hal, sdio_gpio_map[mode].gpio_bits, sdio_gpio_map[mode].devs, GPIO_SDIO_USED_GPIO_NUM);

	return BK_OK;
}

bk_err_t gpio_sdio_one_line_sel(gpio_sdio_map_mode_t mode)
{
	GPIO_RETURN_ON_INVALID_PERIAL_MODE(mode, GPIO_SDIO_MAP_MODE_MAX);

	GPIO_MAP_TABLE(GPIO_SDIO_ONE_LINE_USED_GPIO_NUM, GPIO_SDIO_MAP_MODE_MAX, sdio_gpio_map) = GPIO_SDIO_ONE_LINE_MAP_TABLE;

	gpio_hal_devs_map(&s_gpio.hal, sdio_gpio_map[mode].gpio_bits, sdio_gpio_map[mode].devs, GPIO_SDIO_ONE_LINE_USED_GPIO_NUM);

	return BK_OK;
}


bk_err_t gpio_jtag_sel(gpio_jtag_map_group_t group_id)
{
	if (group_id == GPIO_JTAG_MAP_GROUP0) {
		gpio_hal_func_unmap(&s_gpio.hal,GPIO_20);
		gpio_hal_func_unmap(&s_gpio.hal,GPIO_21);
		gpio_hal_func_map(&s_gpio.hal,GPIO_20, GPIO_DEV_JTAG_TCK);
		gpio_hal_func_map(&s_gpio.hal,GPIO_21, GPIO_DEV_JTAG_TMS);
	} else if (group_id == GPIO_JTAG_MAP_GROUP1) {
		gpio_hal_func_unmap(&s_gpio.hal,GPIO_0);
		gpio_hal_func_unmap(&s_gpio.hal,GPIO_1);
		gpio_hal_func_map(&s_gpio.hal,GPIO_0, GPIO_DEV_JTAG_TCK);
		gpio_hal_func_map(&s_gpio.hal,GPIO_1, GPIO_DEV_JTAG_TMS);
	} else {
		GPIO_LOGD("Unsupported group id(%d).\r\n", group_id);
		return BK_FAIL;
	}

	return BK_OK;
}

#if CONFIG_GPIO_RETENTION_SUPPORT
bk_err_t bk_gpio_retention_set(gpio_id_t gpio_id, gpio_output_state_e gpio_output_state)
{
	bk_err_t ret;

	ret = gpio_retention_map_set(gpio_id, gpio_output_state);

	if (BK_OK != ret)
	{
		GPIO_LOGW("gpio retention set fail: gpio_%d type: %d\r\n", gpio_id, gpio_output_state);
	}

#if GPIO_RETENTION_MAP_DUMP
	GPIO_LOGD("gpio retention map:\r\n");
	uint32_t bitmap = aon_pmu_hal_gpio_retention_bitmap_get();
	for (uint32_t i = 0; i < GPIO_RETENTION_MAP_SIZE; i++)
	{
		gpio_id = s_gpio_retention_map[i];
		gpio_output_state = !!(bitmap & BIT(i));
		if (GPIO_RETENTION_EN_CHECK(gpio_id, i))
		{
			GPIO_LOGD("gpio_%d type: %d\r\n", gpio_id, gpio_output_state);
		}
	}
#endif

	return ret;
}

bk_err_t bk_gpio_retention_clr(gpio_id_t gpio_id)
{
	bk_err_t ret;

	ret = gpio_retention_map_clr(gpio_id);

	if (BK_OK != ret)
	{
		GPIO_LOGW("gpio retention set fail: gpio_%d\r\n", gpio_id);
	}

#if GPIO_RETENTION_MAP_DUMP
	GPIO_LOGD("gpio retention map:\r\n");
	uint32_t bitmap = aon_pmu_hal_gpio_retention_bitmap_get();
	gpio_output_state_e gpio_output_state;
	for (uint32_t i = 0; i < GPIO_RETENTION_MAP_SIZE; i++)
	{
		gpio_id = s_gpio_retention_map[i];
		gpio_output_state = !!(bitmap & BIT(i));
		if (GPIO_RETENTION_EN_CHECK(gpio_id, i, false))
		{
			GPIO_LOGD("gpio_%d type: %d\r\n", gpio_id, gpio_output_state);
		}
	}
#endif

	return ret;
}

bk_err_t gpio_retention_map_set(gpio_id_t id, gpio_output_state_e gpio_output_state)
{
	bk_err_t ret = BK_FAIL;
	gpio_id_t gpio_id;
	uint32_t bitmap = aon_pmu_hal_gpio_retention_bitmap_get();

	for (uint32_t i = 0; i < GPIO_RETENTION_MAP_SIZE; i++)
	{
		gpio_id = s_gpio_retention_map[i];
		if (id == gpio_id && gpio_id < GPIO_NUM_MAX)
		{
			s_gpio_retention_en_bitmap |= BIT(i);
			if (GPIO_OUTPUT_STATE_HIGH == gpio_output_state) {
				bitmap |= BIT(i);
			} else if (GPIO_OUTPUT_STATE_LOW == gpio_output_state) {
				bitmap &= ~BIT(i);
			}
			ret = BK_OK;
		}
	}

	aon_pmu_hal_gpio_retention_bitmap_set(bitmap);

	return ret;
}

bk_err_t gpio_retention_map_clr(gpio_id_t id)
{
	bk_err_t ret = BK_FAIL;
	gpio_id_t gpio_id;
	uint32_t bitmap = aon_pmu_hal_gpio_retention_bitmap_get();

	for (uint32_t i = 0; i < GPIO_RETENTION_MAP_SIZE; i++)
	{
		gpio_id = s_gpio_retention_map[i];
		if (id == gpio_id && gpio_id < GPIO_NUM_MAX)
		{
			s_gpio_retention_en_bitmap &= ~BIT(i);
			bitmap &= ~BIT(i);
			ret = BK_OK;
		}
	}

	aon_pmu_hal_gpio_retention_bitmap_set(bitmap);

	return ret;
}

uint64_t gpio_retention_map_get(void)
{
	gpio_id_t gpio_id;
	uint64_t gpio_bitmap = 0;

	for (uint32_t i = 0; i < GPIO_RETENTION_MAP_SIZE; i++)
	{
		gpio_id = s_gpio_retention_map[i];
		if (GPIO_RETENTION_EN_CHECK(gpio_id, i, false))
		{
			gpio_bitmap |= BIT64(gpio_id);
		}
	}

	return gpio_bitmap;
}

void gpio_retention_sync(bool force_flag)
{
	gpio_id_t gpio_id;
	uint32_t bitmap = aon_pmu_hal_gpio_retention_bitmap_get();

	for (uint32_t i = 0; i < GPIO_RETENTION_MAP_SIZE; i++)
	{
		gpio_id = s_gpio_retention_map[i];
		if (GPIO_RETENTION_EN_CHECK(gpio_id, i, force_flag))
		{
			if (bitmap & BIT(i)) {
				GPIO_UP(gpio_id);
			} else {
				GPIO_DOWN(gpio_id);
			}
		}
		if (force_flag)
		{
#if CONFIG_GPIO_RETENTION_DISPOSABLE
			gpio_retention_map_clr(gpio_id);
#else
			s_gpio_retention_en_bitmap |= BIT64(i);
#endif
		}
	}
}
#endif

#if CONFIG_GPIO_DUMP_MAP_DEV_DEBUG
bk_err_t gpio_dump_map_dev_cfg(void)
{
	return gpio_hal_dump_map_dev_cfg(&s_gpio.hal);
}
#endif

#if CONFIG_GPIO_DEFAULT_SET_SUPPORT
/*according to GPIO_DEFAULT_DEV_CONFIG to check dev match and capacity match
if dev or capacity mismatch, will print dev mismatch info,please check GPIO_DEFAULT_DEV_CONFIG
table and fix it
*/
bk_err_t gpio_check_dev_match(gpio_id_t gpio_id, gpio_dev_t dev)
{
	const gpio_default_map_t *config = get_gpio_config(gpio_id);
	bool func_mode_en = true;

	if (config == NULL)
	{
		GPIO_LOGW("GPIO%d not found in GPIO_DEFAULT_DEV_CONFIG table\r\n",gpio_id);
		return BK_ERR_GPIO_INVALID_OPERATE;
	}

	if (config->second_func_en != GPIO_SECOND_FUNC_ENABLE) 
	{
		func_mode_en = false;
		GPIO_LOGW("GPIO%d not enable second func\r\n",gpio_id);
	}
	
	if (config->second_func_dev != dev)
	{
		func_mode_en = false;
		GPIO_LOGW("GPIO%d dev mismatch, expect 0x%x, but current is 0x%x\r\n",gpio_id, dev, config->second_func_dev);
	}

	if (!func_mode_en)
	{
		GPIO_LOGW("gpio id=%d func_mode is not match\r\n", gpio_id);
		return BK_ERR_GPIO_CONFIG_MISMATCH;
	}

	return BK_OK;
}

bk_err_t gpio_check_capacity_match(gpio_id_t gpio_id, uint8_t expect_capacity)
{
	const gpio_default_map_t *config = get_gpio_config(gpio_id);

	if (config == NULL)
	{
		GPIO_LOGW("GPIO%d not found in GPIO_DEFAULT_DEV_CONFIG table\r\n",gpio_id);
		return BK_ERR_GPIO_INVALID_OPERATE;
	}

	if (config->driver_capacity != expect_capacity)
	{
		GPIO_LOGW("GPIO%d driver capacity mismatch, expect %d, but current is %d\r\n",gpio_id, expect_capacity, config->driver_capacity);
		return BK_ERR_GPIO_CONFIG_MISMATCH;
	}

	return BK_OK;
}
#endif
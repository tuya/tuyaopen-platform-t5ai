/**
 * Copyright (C), 2018-2018, Arm Technology (China) Co., Ltd.
 * All rights reserved
 *
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 * License Agreement between Licensee and Arm Technology (China) Co., Ltd
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and,,
 * any derivative work shall retain this copyright notice.
 */
#include "hal.h"
#include "pal.h"
#include "hal_src_internal.h"
#include "mem_layout.h"
#include "flash_operation.h"
#include "soc.h"
#include "gpio_cap.h"

#ifdef CONFIG_BK_BOOT

boot_ctrl_data_t s_boot_ctrl_data = {0};

static inline void dump_ctrl_data(const char *msg, const boot_ctrl_data_t *data)
{
	PAL_LOG_DEBUG("%s\r\n", msg);
	PAL_LOG_DEBUG("magic: %x\r\n", data->magic);
	PAL_LOG_DEBUG("boot_flag: %x\r\n", data->boot_flag);
	PAL_LOG_DEBUG("primary manifest addr: %x\r\n", data->primary_manifest_addr);
	PAL_LOG_DEBUG("recovery manifest addr: %x\r\n", data->recovery_manifest_addr);

	PAL_LOG_DEBUG("pll ena: %x\r\n", data->pll_ena);
	PAL_LOG_DEBUG("security boot supported: %x\r\n", data->security_boot_supported);
	PAL_LOG_DEBUG("security boot ena: %x\r\n", data->security_boot_ena);
	PAL_LOG_DEBUG("print disable: %x\r\n", data->security_boot_print_dis);
	PAL_LOG_DEBUG("jtag disable: %x\r\n", data->jtag_dis);
	PAL_LOG_DEBUG("sw fih delay enabled: %x\r\n", data->sw_fih_delay_ena);
#if CONFIG_GPIO_ENA
	PAL_LOG_DEBUG("user gpio ena: %x\r\n", data->user_gpio_ena);
	PAL_LOG_DEBUG("user gpio id: %x(%d)\r\n", data->user_gpio_id, data->user_gpio_id);
	PAL_LOG_DEBUG("perf gpio ena: %x\r\n", data->perf_gpio_ena);
	PAL_LOG_DEBUG("perf pivot gpio id: %x(%d)\r\n", data->perf_pivot_gpio_id, data->perf_pivot_gpio_id);
	PAL_LOG_DEBUG("perf updown gpio id: %x(%d)\r\n", data->perf_updown_gpio_id, data->perf_updown_gpio_id);
#endif
}

static void dump_ctrl_data_compact(const boot_ctrl_data_t *data)
{
	uint32_t bc_bits = 0;

	if (data->pll_ena) {
		bc_bits |= HAL_EFUSE_PLL_ENABLE_BIT;
	}

	if (data->security_boot_supported) {
		bc_bits |= HAL_EFUSE_SECURE_BOOT_SUPPORTED_BIT;
	}

	if (data->security_boot_ena) {
		bc_bits |= HAL_EFUSE_SECURE_BOOT_ENABLE_BIT;
	}

	if (data->security_boot_print_dis) {
		bc_bits |= HAL_EFUSE_SECURE_BOOT_DEBUG_DISABLE_BIT;
	}

	if (data->jtag_dis) {
		bc_bits |= HAL_EFUSE_JTAG_DISABLE_BIT;
	}

	PAL_LOG_INFO("bc=%x bf=%d pma=%x rma=%x\r\n", bc_bits,
		data->boot_flag, data->primary_manifest_addr,
		data->recovery_manifest_addr);
}

static hal_ret_t hal_ctrl_overwrite_ctrl_data(const boot_ctrl_data_t *data)
{
	dump_ctrl_data("write control partition\n", data);

	if (flash_op_write(LAYOUT_CTRL_PARTITION_FLASH_OFFSET,
		(const uint8_t *)(&s_boot_ctrl_data),
		sizeof(boot_ctrl_data_t)) != HAL_OK) {
		PAL_LOG_ERR("Update boot ctrl failed!\n");
		return HAL_ERR_GENERIC;
	}
	return HAL_OK;
}

static bool is_addr_in_flash_range(hal_addr_t addr)
{
	hal_addr_t flash_begin = LAYOUT_SPI_FLASH_START + LAYOUT_RECV_MANIFEST_FLASH_OFFSET; //skip ctrl/otp simulation partition
	hal_addr_t flash_end = LAYOUT_SPI_FLASH_START + SZ_16M - MIN_MANIFEST_SIZE;

	if ((addr < flash_begin) || (addr > flash_end)) {
		return false;
	}
	return true;
}

static hal_ret_t check_manifest_addr(hal_addr_t primary_addr, hal_addr_t recovery_addr)
{
	if ((is_addr_in_flash_range(primary_addr) == false) ||
		(is_addr_in_flash_range(recovery_addr) == false)) {
		PAL_LOG_DEBUG("primary(%x) or recovery(%x) address out of range\r\n", primary_addr, recovery_addr);
		return HAL_ERR_GENERIC;
	}

	if (HAL_ABS_MINUS(primary_addr, recovery_addr) < MIN_MANIFEST_SIZE) {
		PAL_LOG_DEBUG("primary(%x) recovery(%x) too small\r\n", primary_addr, recovery_addr);
		return HAL_ERR_GENERIC;
	}

	return HAL_OK;
}

#if CONFIG_GPIO_ENA
static void hal_ctrl_set_gpio(void)
{
	if (s_boot_ctrl_data.user_gpio_ena) {
		if (s_boot_ctrl_data.user_gpio_id < SOC_GPIO_NUM)
			GPIO_UP(s_boot_ctrl_data.user_gpio_id);
	}

	if (s_boot_ctrl_data.perf_gpio_ena) {
		if (s_boot_ctrl_data.perf_pivot_gpio_id < SOC_GPIO_NUM)
			GPIO_UP_DOWN(s_boot_ctrl_data.perf_pivot_gpio_id);

		if (s_boot_ctrl_data.perf_updown_gpio_id < SOC_GPIO_NUM)
			GPIO_UP(s_boot_ctrl_data.perf_updown_gpio_id);
	}
}
#endif


HAL_API hal_ret_t hal_ctrl_partition_load_and_init(void)
{
	hal_ret_t ret		 = HAL_OK;

	ret = flash_op_read(LAYOUT_CTRL_PARTITION_FLASH_OFFSET,
						(uint8_t *)(&s_boot_ctrl_data),
						sizeof(boot_ctrl_data_t));
	HAL_CHECK_CONDITION(HAL_OK == ret, HAL_ERR_GENERIC, "hal flash read failed!\n");

	if (s_boot_ctrl_data.magic != _CTRL_CTRL_MAGIC) {
		PAL_LOG_DEBUG("incorrect BC magic(%x)\r\n", s_boot_ctrl_data.magic);
		pal_memset(&s_boot_ctrl_data, 0, sizeof(s_boot_ctrl_data));
		s_boot_ctrl_data.magic = _CTRL_CTRL_MAGIC;
	}

	if ((s_boot_ctrl_data.boot_flag != HAL_BOOT_FLAG_PRIMARY) &&
		(s_boot_ctrl_data.boot_flag != HAL_BOOT_FLAG_RECOVERY)) {
		PAL_LOG_DEBUG("incorrect BC bootflag(%x)\r\n", s_boot_ctrl_data.boot_flag);
		s_boot_ctrl_data.boot_flag = HAL_BOOT_FLAG_PRIMARY;
	}

	if (hal_efuse_is_bc_disabled()
		|| (check_manifest_addr(s_boot_ctrl_data.primary_manifest_addr,
		s_boot_ctrl_data.recovery_manifest_addr) != HAL_OK)) {
		s_boot_ctrl_data.primary_manifest_addr = LAYOUT_SPI_FLASH_START + LAYOUT_PRIM_MANIFEST_FLASH_OFFSET;
		s_boot_ctrl_data.recovery_manifest_addr = LAYOUT_SPI_FLASH_START + LAYOUT_RECV_MANIFEST_FLASH_OFFSET;
	}

#if CONFIG_GPIO_ENA
	hal_ctrl_set_gpio();
#endif
	dump_ctrl_data("load control partition\n", &s_boot_ctrl_data);
	dump_ctrl_data_compact(&s_boot_ctrl_data);

finish:
	return ret;
}

HAL_API hal_ret_t hal_read_preferred_boot_flag(hal_boot_flag_t *flag)
{
#ifdef CONFIG_FORCE_RECOVERY_BOOT
	*flag = HAL_BOOT_FLAG_RECOVERY;
	return HAL_OK;
#endif
	*flag = s_boot_ctrl_data.boot_flag;
	return HAL_OK;
}

HAL_API hal_ret_t hal_write_preferred_boot_flag(const hal_boot_flag_t flag)
{
	hal_ret_t ret		 = HAL_OK;

	HAL_CHECK_CONDITION((flag == HAL_BOOT_FLAG_PRIMARY) ||
		(flag == HAL_BOOT_FLAG_RECOVERY),
		HAL_ERR_BAD_PARAM,
		"Parameter flag is invalid: %d!\n",
		flag);

	if (flag == s_boot_ctrl_data.boot_flag) {
		return HAL_OK;
	}
	s_boot_ctrl_data.boot_flag = flag;

	return hal_ctrl_overwrite_ctrl_data(&s_boot_ctrl_data);

finish:
	return ret;
}

HAL_API hal_ret_t hal_read_manifest_address(hal_manifest_addr_t *man)
{
	man->primary = s_boot_ctrl_data.primary_manifest_addr;
	man->recovery = s_boot_ctrl_data.recovery_manifest_addr;
	return HAL_OK;
}

HAL_API hal_ret_t hal_write_manifest_address(const hal_manifest_addr_t *man)
{
	if (check_manifest_addr(man->primary, man->recovery) != HAL_OK) {
                PAL_LOG_ERR("Invalid manifest addr!\n");
                return HAL_ERR_GENERIC;
	}
	
	if ( (man->primary != s_boot_ctrl_data.primary_manifest_addr) ||
		(man->recovery != s_boot_ctrl_data.recovery_manifest_addr)) {
		s_boot_ctrl_data.primary_manifest_addr = man->primary;
		s_boot_ctrl_data.recovery_manifest_addr = man->recovery;
		return hal_ctrl_overwrite_ctrl_data(&s_boot_ctrl_data);
	}

	return HAL_OK;
}

#endif

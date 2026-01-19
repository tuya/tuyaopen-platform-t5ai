#include <common/bk_include.h>
#include "bk_icu.h"
#include "bk_drv_model.h"
#include "bk_uart.h"

#include "bk_arm_arch.h"
#include "arch_interrupt.h"
#include "sys_ctrl.h"
#include "bk_sys_ctrl.h"

#include "reset_reason.h"
#include <components/log.h>
#include "aon_pmu_hal.h"
#include "driver/gpio.h"
#include "driver/pwr_clk.h"

#define TAG "init"
#define DISPLAY_START_TYPE_STR 1


static volatile bool s_initialized = false;
static uint32_t s_start_type = 0;
static uint32_t s_misc_value_save = 0;
static uint32_t s_mem_value_save = 0;

uint32_t bk_misc_get_reset_reason(void)
{
	return s_start_type;
}

void persist_memory_init(void)
{
	*((volatile uint32_t *)(PERSIST_MEMORY_ADDR)) = (uint32_t)CRASH_ILLEGAL_JUMP_VALUE;
}

uint32_t persist_memory_get(void)
{
	return (*((volatile uint32_t *)(PERSIST_MEMORY_ADDR)));
}

bool persist_memory_is_lost(void)
{
	if ((uint32_t)CRASH_ILLEGAL_JUMP_VALUE == persist_memory_get())
		return false;
	else
		return true;
}

static char *misc_get_start_type_str(uint32_t start_type)
{
#if DISPLAY_START_TYPE_STR
	switch (start_type) {
	case RESET_SOURCE_POWERON:
		return "power on";

	case RESET_SOURCE_REBOOT:
		return "software reboot";

	case RESET_SOURCE_WATCHDOG:
		return "interrupt watchdog";

	case RESET_SOURCE_DEEPPS_GPIO:
		return "deep sleep gpio";

	case RESET_SOURCE_DEEPPS_RTC:
		return "deep sleep rtc";

	case RESET_SOURCE_DEEPPS_TOUCH:
		return "deep sleep touch";

	case RESET_SOURCE_CRASH_ILLEGAL_JUMP:
		return "illegal jump";

	case RESET_SOURCE_CRASH_UNDEFINED:
		return "undefined";

	case RESET_SOURCE_CRASH_PREFETCH_ABORT:
		return "prefetch abort";

	case RESET_SOURCE_CRASH_DATA_ABORT:
		return "data abort";

	case RESET_SOURCE_CRASH_UNUSED:
		return "unused";

	case RESET_SOURCE_CRASH_ILLEGAL_INSTRUCTION:
		return "illegal instruction";

	case RESET_SOURCE_CRASH_MISALIGNED:
		return "misaligned";

	case RESET_SOURCE_CRASH_ASSERT:
		return "assert";

	case RESET_SOURCE_DEEPPS_USB:
		return "deep sleep usb";

	case RESET_SOURCE_SUPER_DEEP:
		return "super deep sleep";

	case RESET_SOURCE_NMI_WDT:
		return "nmi watchdog";

	case RESET_SOURCE_HARD_FAULT:
		return "hard fault";

	case RESET_SOURCE_MPU_FAULT:
		return "mpu fault";

	case RESET_SOURCE_BUS_FAULT:
		return "bus fault";

	case RESET_SOURCE_USAGE_FAULT:
		return "usage fault";

	case RESET_SOURCE_SECURE_FAULT:
		return "secure fault";

	case RESET_SOURCE_DEFAULT_EXCEPTION:
		return "default exception";

	case RESET_SOURCE_OTA_REBOOT:
		return "ota reboot";

	case RESET_SOURCE_UNKNOWN:
	default:
		return "unknown";
	}
#else
	return "";
#endif
}

void show_reset_reason(void)
{
	BK_LOGD(TAG, "reason - %s\r\n", misc_get_start_type_str(s_start_type));
	if(RESET_SOURCE_DEEPPS_GPIO == s_start_type)
	{
#if CONFIG_DEEP_PS
		BK_LOGD(TAG, "by gpio - %d\r\n", bk_misc_wakeup_get_gpio_num());
#else
#ifdef CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT
		BK_LOGD(TAG, "by gpio - %d\r\n", bk_gpio_get_wakeup_gpio_id());
#endif
#endif
	}
	BK_LOGD(TAG, "regs - %x, %x, %x\r\n", s_start_type, s_misc_value_save, s_mem_value_save);
}

static inline void bk_misc_save_cp_reset_reason(uint32_t type) {
	FIXED_ADDR_CP_RESET_REASON = type;
}

static inline void bk_misc_save_ap_reset_reason(uint32_t type) {
	FIXED_ADDR_AP_RESET_REASON = type;
}

uint32_t bk_misc_get_cp_reset_reason(void)
{
	return FIXED_ADDR_CP_RESET_REASON;
}

uint32_t bk_misc_get_ap_reset_reason(void)
{
	return FIXED_ADDR_AP_RESET_REASON;
}



// typedef volatile union {
// 	struct {
// 		uint32_t memchk_bps               :  1; /**<bit[0 : 0] */
// 		uint32_t fast_boot                :  1; /**<bit[1 : 1] */
// 		uint32_t ota_finish               :  1; /**<bit[2 : 2] */
// 		uint32_t bl2_deep_sleep           :  1; /**<bit[3 : 3] */
// 		uint32_t reset_reason_cp          :  8; /**<bit[4 : 11] */
// 		uint32_t gpio_retention_bitmap    :  8; /**<bit[12 : 19] */
// 		uint32_t reset_count              :  4 ;/**<bit[20 : 23] */
// 		uint32_t reset_reason_ap          :  7; /**<bit[24 : 30] */
// 		uint32_t gpio_sleep               :  1; /**<bit[31 : 31] */
// 	};
// 	uint32_t v;
// } aon_pmu_r0_t;

void bk_misc_set_cp_reset_reason(uint32_t type)
{
	if (type > 0xff) {
		BK_DUMP_OUT("Invalid cp rr type: 0x%x", type);
		return;
	}

	/* use PMU_REG0 bit[4:11] for reset reason */
	uint32_t misc_value = aon_pmu_hal_get_r0();

	/* clear last reset reason */
	misc_value &= ~(0xff << 4);

	misc_value |= ((type & 0xff) << 4);
	aon_pmu_hal_set_r0(misc_value);
}

void bk_misc_set_ap_reset_reason(uint32_t type)
{
	if (type > 0x7f) {
		BK_DUMP_OUT("Invalid ap rr type: 0x%x\r\n", type);
		return;
	}

	/* use PMU_REG0 bit[24:30] for reset reason */
	uint32_t misc_value = aon_pmu_hal_get_r0();

	BK_DUMP_OUT("set ap rr: 0x%x\r\n", type);
	/* clear last reset reason */
	misc_value &= ~(0x7f << 24);

	misc_value |= ((type & 0x7f) << 24);
	aon_pmu_hal_set_r0(misc_value);
}

void bk_misc_set_reset_reason(uint32_t type)
{
	bk_misc_set_cp_reset_reason(type);
}



uint32_t reset_reason_init(void)
{
	uint32_t misc_value;
	uint32_t cp_reset_reason = 0;
	uint32_t ap_reset_reason = 0;

	if (s_initialized != 0) {
		return s_start_type;
	}

	misc_value = aon_pmu_ll_get_r7a();
	cp_reset_reason = ((misc_value >> 4) & 0xff);
	ap_reset_reason = ((misc_value >> 24) & 0x7f);
	
	s_start_type = cp_reset_reason;
	s_misc_value_save = misc_value;


	bk_misc_save_cp_reset_reason(cp_reset_reason);
	bk_misc_save_ap_reset_reason(ap_reset_reason);

	bk_misc_set_cp_reset_reason(RESET_SOURCE_POWERON);
	bk_misc_set_ap_reset_reason(RESET_SOURCE_POWERON);

	arch_init_exception_magic_status();
	s_initialized = true;
	return s_start_type;
}

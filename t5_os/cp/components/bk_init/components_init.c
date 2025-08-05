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

// Notes:
// This file only contain OS-independent components initialization code,
// For OS-dependently initialization, put them to OSK or bk_system.

#include <common/bk_include.h>
#include "bk_sys_ctrl.h"
#include "stdlib.h"
#include "bk_drv_model.h"
#include <components/ate.h>
#include <driver/wdt.h>
#include "bk_wdt.h"
#include <common/sys_config.h>
#include "release.h"
#include "sys_driver.h"
#include "bk_pm_model.h"
#include "bk_private/bk_init.h"
#if CONFIG_SOC_BK7256XX
#include "BK7256_RegList.h"
#endif
#if CONFIG_FULLY_HOSTED
#include "mmgmt.h"
#endif

#include "reset_reason.h"

#if CONFIG_EASY_FLASH
#include "easyflash.h"
#include "bk_ef.h"
#endif

#include <components/log.h>
#include <components/sensor.h>
#include <driver/trng.h>

#include "bk_arch.h"
#include "bk_private/bk_driver.h"

#include "mb_ipc_cmd.h"

#if (CONFIG_PSRAM)
#include <driver/psram.h>
#endif
#if (CONFIG_OTP_V1)
#include <driver/otp.h>
#endif

#define TAG "init"


//TODO move to better place
#if CONFIG_MEM_DEBUG
/* memory leak timer at 1sec */
static beken_timer_t memleak_timer = {0};
int bmsg_memleak_check_sender();

void memleak_timer_cb(void *arg)
{
	bmsg_memleak_check_sender();
}

void mem_debug_start_timer(void)
{
	bk_err_t err;

	err = rtos_init_timer(&memleak_timer,
						  1000,
						  memleak_timer_cb,
						  (void *)0);
	if(kNoErr != err)
	{
		BK_LOGE(TAG, "rtos_init_timer fail\r\n");
		return;
	}
	err = rtos_start_timer(&memleak_timer);
	if(kNoErr != err)
	{
		BK_LOGE(TAG, "rtos_start_timer fail\r\n");
		return;
	}
}
#endif

int bandgap_init(void)
{
#if (CONFIG_SOC_BK7236XX) && (CONFIG_OTP_V1)
	uint8_t device_id[2]; //only check seqNUM
	uint8_t new_bandgap;
	uint8_t old_bandgap;
	bk_err_t result;

	old_bandgap = (uint8_t)sys_drv_get_bgcalm();

	result = bk_otp_apb_read(OTP_VDDDIG_BANDGAP, &new_bandgap, sizeof(new_bandgap));
	if ((result != BK_OK) || (new_bandgap == 0) || (new_bandgap > 0x3F)) {
		goto default_bandgap;
	}

	result = bk_otp_apb_read(OTP_DEVICE_ID, device_id, sizeof(device_id));
	if ((result != BK_OK) || ((device_id[0] == 0x32) && (device_id[1] == 0x31))) {
		goto default_bandgap;
	}

	BK_LOGD(TAG, "bandgap_calm_in_otp=0x%x\r\n", new_bandgap);
	if (old_bandgap != new_bandgap) {
		sys_drv_set_bgcalm(new_bandgap);
	}
	return BK_OK;

default_bandgap:
	//tenglong20240717: increase 10mV as default
	if (old_bandgap > 6) {
		sys_drv_set_bgcalm(old_bandgap - 6);
	}
#endif
	return BK_OK;
}

int random_init(void)
{
#if (CONFIG_TRNG_SUPPORT)
	BK_LOGV(TAG, "create srand seed\r\n");
	srand(bk_rand());
#endif
	return BK_OK;
}

__IRAM_SEC int wdt_init(void)
{
#if (CONFIG_FREERTOS)
#if CONFIG_INT_WDT
	BK_LOGV(TAG, "int watchdog enabled, period=%u\r\n", CONFIG_INT_WDT_PERIOD_MS);
	bk_wdt_start(CONFIG_INT_WDT_PERIOD_MS);
#else
	BK_LOGD(TAG, "watchdog disabled\r\n");
	bk_wdt_start(CONFIG_INT_WDT_PERIOD_MS);
	bk_wdt_feed();
	bk_wdt_stop();
#endif //CONFIG_INT_WDT
#endif //CONFIG_FREERTOS

#if !(CONFIG_ALIOS)
#if CONFIG_TASK_WDT
	bk_task_wdt_start();
	BK_LOGV(TAG, "task watchdog enabled, period=%u\r\n", CONFIG_TASK_WDT_PERIOD_MS);
#endif
#endif
	return BK_OK;
}

int memory_debug_todo(void)
{
#if CONFIG_MEM_DEBUG
	mem_debug_start_timer();
#endif
	return BK_OK;
}

__attribute__((unused)) static int pm_init(void)
{

#if CONFIG_DEEP_PS
	bk_init_deep_wakeup_gpio_status();
#endif

	return BK_OK;
}

static inline void show_sdk_version(void)
{
#if (CONFIG_CMAKE)
	//BK_LOGD(TAG, "armino rev: %s\r\n", ARMINO_VER);
	BK_LOGD(TAG, "armino rev: %s\r\n", "");
#else
	//BK_LOGD(TAG, "armino rev: %s\r\n", BEKEN_SDK_REV);
	BK_LOGD(TAG, "armino rev: %s\r\n", "");
#endif
}

static inline void show_chip_id(void)
{
	// BK_LOGD(TAG, "armino soc id:%x_%x\r\n",sddev_control(DD_DEV_TYPE_SCTRL,CMD_GET_DEVICE_ID, NULL),
	// 	sddev_control(DD_DEV_TYPE_SCTRL,CMD_GET_CHIP_ID, NULL));
	BK_LOGD(TAG, "armino soc id:%x_%x\r\n", sys_drv_get_device_id(), sys_drv_get_chip_id());
}

static inline void show_sdk_lib_version(void)
{
	extern char* bk_get_internal_lib_version(void);
	char* ver = bk_get_internal_lib_version();
	BK_LOGD(TAG, "armino internal lib rev: %s\n", ver);
}

static void show_armino_version(void)
{
	show_sdk_version();
	show_chip_id();
}

static void show_init_info(void)
{
	show_reset_reason();
	show_armino_version();
}


void *__stack_chk_guard = NULL;

// Intialize random stack guard, must after trng start.
void bk_stack_guard_setup(void)
{
    BK_LOGD(TAG, "Intialize random stack guard.\r\n");
#if CONFIG_TRNG_SUPPORT
    __stack_chk_guard = (void *)bk_rand();
#endif
}

void __stack_chk_fail (void)
{
    BK_DUMP_OUT("Stack guard warning, local buffer overflow!!!\r\n");
    BK_ASSERT(0);
}


int components_early_init(void)
{
#if CONFIG_RESET_REASON
	reset_reason_init();
#endif
	app_phy_init();

	if(driver_early_init())
		return BK_FAIL;

	pm_init();

	bandgap_init();
	random_init();

	bk_stack_guard_setup();

	return BK_OK;
}

__attribute__((weak)) void bk_module_init(void) {

}

// Run in task environment
int components_init(void)
{
	if(driver_init())
		return BK_FAIL;

	wdt_init();

#if (CONFIG_TEMP_DETECT || CONFIG_VOLT_DETECT)
	bk_sensor_init();
#endif

	bk_module_init();

	show_init_info();

	return BK_OK;
}

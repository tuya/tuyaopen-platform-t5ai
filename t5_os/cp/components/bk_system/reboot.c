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
#include <components/log.h>
#include <common/bk_err.h>
#include <components/system.h>
#include <driver/wdt.h>
#include "bk_misc.h"
#include "reset_reason.h"
#include "drv_model_pub.h"
#include "bk_wifi_types.h"
#include "bk_wifi.h"
#include "aon_pmu_driver.h"
#include "wdt_driver.h"
#include <modules/pm.h>

#define TAG "sys"



#if (CONFIG_SOC_BK7256XX)
static uint32_t bk_get_return_address_value()
{
	UINT32 return_address_value;

	__asm volatile( "mv %0, ra":"=r"( return_address_value ) );

	return return_address_value;
}
#endif

void bk_reboot_ex(uint32_t reset_reason)
{
	static uint32_t entry_cnt = 0;
	if(entry_cnt == 0)	//first time come here, or force reboot:avoid these codes cause system abnormal.
	{
		entry_cnt++;

		if (reset_reason < RESET_SOURCE_UNKNOWN) {
			bk_misc_set_reset_reason(reset_reason);
		}

		BK_LOGD(TAG, "bk_reboot\r\n");
		delay_ms(100); //add delay for bk_writer BEKEN_DO_REBOOT cmd
		bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT,PM_CPU_FRQ_60M);

		BK_LOGD(TAG, "wdt reboot\r\n");
		rtos_disable_int();
		if (reset_reason < RESET_SOURCE_UNKNOWN) {
			bk_misc_set_reset_reason(reset_reason);
		}
	}

	bk_wdt_force_reboot();
}

void bk_reboot(void)
{
	bk_reboot_ex(RESET_SOURCE_REBOOT);
}

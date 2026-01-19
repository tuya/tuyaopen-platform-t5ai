// Copyright 2025-2026 Beken
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
#include <driver/pwr_clk.h>
#include <modules/pm.h>
#include "sys_driver.h"
#include "sys_types.h"
#include <driver/aon_rtc.h>
#include <os/os.h>
#include <components/system.h>
#include <common/bk_kernel_err.h>
#include "aon_pmu_hal.h"
#include <components/log.h>

/*=====================DEFINE  SECTION  START=====================*/

#define PWR_MISC_TAG "pwr_misc"
#define PWR_MISC_LOGD(...) BK_LOGD(PWR_MISC_TAG, ##__VA_ARGS__)
#define PWR_MISC_LOGW(...) BK_LOGW(PWR_MISC_TAG, ##__VA_ARGS__)
#define PWR_MISC_LOGE(...) BK_LOGE(PWR_MISC_TAG, ##__VA_ARGS__)
#define PWR_MISC_LOGV(...) BK_LOGV(PWR_MISC_TAG, ##__VA_ARGS__)

/*=====================DEFINE  SECTION  END=====================*/

/*=====================VARIABLE  SECTION  START=================*/
uint64_t static s_startup_rtc_tick = 0;

/*=====================VARIABLE  SECTION  END=================*/

/*================FUNCTION DECLARATION  SECTION  START========*/


/*================FUNCTION DECLARATION  SECTION  END========*/
bk_err_t bk_low_pwr_misc_rtc_enter_deepsleep(uint32_t time_interval , aon_rtc_isr_t callback)
{
	#if CONFIG_AON_RTC || CONFIG_ANA_RTC
	bk_err_t ret = BK_FAIL;
	alarm_info_t deep_sleep_alarm = {
									"pwr_misc",
									time_interval*AON_RTC_MS_TICK_CNT,
									1,
									callback,
									NULL
									};
	bk_alarm_unregister(AON_RTC_ID_1, deep_sleep_alarm.name);
	ret = bk_alarm_register(AON_RTC_ID_1, &deep_sleep_alarm);
	if(ret != BK_OK)
    {
	 	return ret;
    }
	bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);
	#endif //CONFIG_AON_RTC
	bk_pm_sleep_mode_set(PM_MODE_DEEP_SLEEP);
	return BK_OK;
}
bk_err_t bk_low_pwr_misc_get_time_interval_from_startup(uint32_t* time_interval)
{
	#if CONFIG_AON_RTC
	uint32_t tick_count = 0.0;
	uint64_t entry_tick  =0;
	if(time_interval == NULL)
    {
      	return BK_FAIL;
    }
	entry_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);

	pm_lpo_src_e lpo_src = bk_pm_lpo_src_get();
	if(lpo_src == PM_LPO_SRC_X32K)
	{
		tick_count = AON_RTC_EXTERN_32K_CLOCK_FREQ;
	}
	else
	{
		tick_count = AON_RTC_DEFAULT_CLOCK_FREQ;
	}

	*time_interval = (uint32_t)(((entry_tick - s_startup_rtc_tick)*1000000)/tick_count);
	#endif
	return BK_OK;
}
bk_err_t bk_low_pwr_misc_startup_rtc_tick_set(uint64_t time_tick)
{
	s_startup_rtc_tick = time_tick;
	return BK_OK;
}
#if CONFIG_DEEPSLEEP_USING_WDT_PROTECT
bk_err_t bk_low_pwr_deepsleep_using_wdt_protect()
{
	if(aon_pmu_hal_get_reset_reason() == RESET_SOURCE_FORCE_DEEPSLEEP)
    {
        /*Get the deepsleep protect count*/
        sleep_count = aon_pmu_hal_get_wdt_deepsleep_pt_count();
        sleep_count = sleep_count -1;
        if (sleep_count > 0)
        {
            /*Clear RTC INT */
            uint32_t value = REG_READ(0x44000200);
            value  |= (0x1 >> 4);
            value  |= (0x1 >> 5);
            REG_WRITE(0x44000200,value);

            value = REG_READ(0x44000200+0x3*4);
            value = value+60000*32;
            REG_WRITE(0x44000200+0x2*4,value);
            aon_pmu_hal_set_wdt_deepsleep_pt_count(sleep_count,false);
            sys_drv_enter_deep_sleep(NULL);
        }
    }
	return BK_OK;
}

#endif
// Copyright 2020-2023 Beken
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

#include<components/sensor.h>
#include "sys_driver.h"
#include "bk_pm_internal_api.h"

#include "driver/aon_rtc.h"
#include "bk_pm_internal_api.h"


/*=====================DEFINE  SECTION  START=====================*/

#define TAG "pm"

#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define PM_SOC_SYS_REG_BASE                  (SOC_SYS_REG_BASE)
#define PM_SYS_REG_0x8                       (SOC_SYS_REG_BASE + 0x8*4)
#define PM_SYS_REG_0x4                       (SOC_SYS_REG_BASE + 0x4*4)
#define PM_SYS_REG_0x5                       (SOC_SYS_REG_BASE + 0x5*4)
#define PM_CPU_SRC_480M                      (3)
#define PM_CPU_SRC_320M                      (2)
#define PM_CPU_SRC_POS                       (4)
#define PM_CPU_SRC_MASK                      (0x3)

#define PM_CP1_SOC_AON_RTC_REG_BASE          (SOC_AON_RTC_REG_BASE)
#define PM_AON_RTC_CNT_VAL_L_OFFSET          (0x3*4)
#define PM_AON_RTC_CNT_VAL_H_OFFSET          (0xa*4)

#define PM_AON_RTC_CNT_VAL_L_ADDR            (PM_CP1_SOC_AON_RTC_REG_BASE + PM_AON_RTC_CNT_VAL_L_OFFSET)
#define PM_AON_RTC_CNT_VAL_H_ADDR            (PM_CP1_SOC_AON_RTC_REG_BASE + PM_AON_RTC_CNT_VAL_H_OFFSET)

#if CONFIG_PM_LV_TIME_COST_DEBUG
#define PM_SOC_AON_PMU_REG_BASE              (SOC_AON_PMU_REG_BASE)
#define PM_AON_PMU_LPO_OFFSET                (0x41*4)

#define PM_AON_PMU_LPO_ADDR                  (SOC_AON_PMU_REG_BASE + PM_AON_PMU_LPO_OFFSET)

#define PM_LPO_EX32K_TICK_COUNT              (32768)
#define PM_LPO_ROSC_TICK_COUNT               (32000)
#endif


/*=====================DEFINE  SECTION  END=======================*/

/*=====================VARIABLE  SECTION  START===================*/
#if CONFIG_PM_LV_TIME_COST_DEBUG
static uint64_t s_wakeup_lv_tick_step[PM_LV_STEP_MAX] = {0};
#endif

#if CONFIG_PM_LV_WDT_PROTECTION
static wifi_event_t pm_wifi_connect_state = EVENT_WIFI_STA_DISCONNECTED;
#endif
/*=====================VARIABLE  SECTION  END=====================*/

/*================FUNCTION DECLARATION  SECTION  START============*/


/*================FUNCTION DECLARATION  SECTION  END===============*/
uint64_t pm_cp1_aon_rtc_counter_get()
{
	volatile uint32_t val = REG_READ(PM_AON_RTC_CNT_VAL_L_ADDR);
	volatile uint32_t val_hi = REG_READ(PM_AON_RTC_CNT_VAL_H_ADDR);

	while (REG_READ(PM_AON_RTC_CNT_VAL_L_ADDR) != val
		|| REG_READ(PM_AON_RTC_CNT_VAL_H_ADDR) != val_hi)
	{
		val = REG_READ(PM_AON_RTC_CNT_VAL_L_ADDR);
		val_hi = REG_READ(PM_AON_RTC_CNT_VAL_H_ADDR);
	}
	return (((uint64_t)(val_hi) << 32) + val);
}


void pm_printf_current_temperature(void)
{
#if CONFIG_TEMP_DETECT
	float temp;

	bk_sensor_get_current_temperature(&temp);
	BK_LOGD(NULL,"current chip temperature about %.2f\r\n",temp);
#endif
}

bk_err_t bk_pm_cpu_freq_dump()
{
	uint32_t value_8 = 0;
	uint32_t value_4 = 0;
	uint32_t value_5 = 0;
	uint32_t cp0_div = 0;
	uint32_t cp1_div = 0;
	value_8 = REG_READ(PM_SYS_REG_0x8);
	value_4 = REG_READ(PM_SYS_REG_0x4);
	value_5 = REG_READ(PM_SYS_REG_0x5);

	if(((value_4>>4)&0x1) == 0x1)
	{
		cp0_div = (value_8&0xF)+1;
	}
	else
	{
		cp0_div = ((value_8&0xF)+1)*2;
	}

	if(((value_5>>4)&0x1) == 0x1)
	{
		cp1_div = (value_8&0xF)+1;
	}
	else
	{
		cp1_div = ((value_8&0xF)+1)*2;
	}

	switch((value_8 >> PM_CPU_SRC_POS)&PM_CPU_SRC_MASK)
	{
		case PM_CPU_SRC_480M:
			LOGI("Curfreq CPU0:(480/%d)M,CPU1/CPU2:(480/%d)M\r\n",cp0_div,cp1_div);
			break;
		case PM_CPU_SRC_320M:
			LOGI("Curfreq:CPU0:(320/%d)M,CPU1/CPU2:(320/%d)M\r\n",cp0_div,cp1_div);
			break;
		default:
			break;
	}
	LOGI("Freq_reg:0x%x,0x%x,0x%x\r\n",value_8,value_4,value_5);
	return BK_OK;
}

/*Why redefine the function:*/
/*The low-voltage code needs to be placed in the ITCM and cannot be in the flash. The existing interface code is large in ITCM,
and putting all of it in the ITCM will occupy more ITCM/iram space.
Additionally, this is only test code that will only be allowed to run after a command is issued to start it*/
#if CONFIG_PM_LV_TIME_COST_DEBUG
__attribute__((section(".iram"))) uint64_t pm_rtc_cur_tick_get()
{
	#if CONFIG_AON_RTC
	volatile uint32_t val = REG_READ(PM_AON_RTC_CNT_VAL_L_ADDR);
	volatile uint32_t val_hi = REG_READ(PM_AON_RTC_CNT_VAL_H_ADDR);
	while (REG_READ(PM_AON_RTC_CNT_VAL_L_ADDR) != val
		|| REG_READ(PM_AON_RTC_CNT_VAL_H_ADDR) != val_hi)
	{
		val = REG_READ(PM_AON_RTC_CNT_VAL_L_ADDR);
		val_hi = REG_READ(PM_AON_RTC_CNT_VAL_H_ADDR);
	}
	return (((uint64_t)(val_hi) << 32) + val);
	#else
	return BK_OK;
	#endif
}
__attribute__((section(".iram")))bk_err_t pm_wakeup_lv_rtc_tick_clear()
{
	for(int i = PM_LV_WAKEUP_STEP_0; i <= PM_LV_WAKEUP_STEP_5; i++)
	{
		s_wakeup_lv_tick_step[i] = 0;
	}
	return BK_OK;
}
__attribute__((section(".iram"))) bk_err_t pm_enter_lv_rtc_tick_clear()
{
	for(int i = PM_LV_ENTER_STEP_0; i <= PM_LV_ENTER_STEP_5; i++)
	{
		s_wakeup_lv_tick_step[i] = 0;
	}
	return BK_OK;
}
__attribute__((section(".itcm_sec_code"))) bk_err_t pm_lv_rtc_tick_set(pm_lv_step_e step,uint64_t tick)
{
	if(step < PM_LV_STEP_MAX)
	{
		s_wakeup_lv_tick_step[step] = tick;
	}
	return BK_OK;
}
uint64_t pm_lv_rtc_tick_get(pm_lv_step_e step)
{
	if(step < PM_LV_STEP_MAX)
	{
		return s_wakeup_lv_tick_step[step];
	}
	return 0;
}

 uint64_t pm_lv_rtc_interval_get(pm_lv_step_e step)
{
	uint32_t tick_count = 0.0;
	uint64_t interval = 0;
	if(step > PM_LV_STEP_MAX)
	{
		return 0;
	}
	uint32_t val = REG_READ(PM_AON_PMU_LPO_ADDR);
	if((val&0x3) == PM_LPO_SRC_X32K)
	{
		tick_count = PM_LPO_EX32K_TICK_COUNT;
	}
	else
	{
		tick_count = PM_LPO_ROSC_TICK_COUNT;
	}
	if(step == PM_LV_ENTER_STEP_0)
	{
		interval = (uint64_t)s_wakeup_lv_tick_step[step];
	}
	else
	{
		interval = (uint64_t)(((s_wakeup_lv_tick_step[step] - s_wakeup_lv_tick_step[step-1])*1000000)/tick_count);
	}
	return interval;
}
#endif

#if CONFIG_PM_LV_WDT_PROTECTION
wifi_event_t bk_pm_wifi_event_state()
{
	return pm_wifi_connect_state;
}
static bk_err_t pm_wifi_event_cb(void *arg, event_module_t event_module,int event_id, void *event_data)
{
	switch (event_id)
	{
		case EVENT_WIFI_STA_CONNECTED:
			pm_wifi_connect_state = EVENT_WIFI_STA_CONNECTED;
			break;

		case EVENT_WIFI_STA_DISCONNECTED:
			pm_wifi_connect_state = EVENT_WIFI_STA_DISCONNECTED;
			break;

		default:
			break;
	}

	return BK_OK;
}

bk_err_t pm_wifi_event_init()
{
	bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, pm_wifi_event_cb, NULL);
	return BK_OK;
}
#endif

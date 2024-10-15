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

/*=====================DEFINE  SECTION  START=====================*/

#define PM_CP1_SOC_AON_RTC_REG_BASE          (SOC_AON_RTC_REG_BASE)
#define PM_AON_RTC_CNT_VAL_L_OFFSET          (0x3*4)
#define PM_AON_RTC_CNT_VAL_H_OFFSET          (0xa*4)

#define PM_AON_RTC_CNT_VAL_L_ADDR            (PM_CP1_SOC_AON_RTC_REG_BASE + PM_AON_RTC_CNT_VAL_L_OFFSET)
#define PM_AON_RTC_CNT_VAL_H_ADDR            (PM_CP1_SOC_AON_RTC_REG_BASE + PM_AON_RTC_CNT_VAL_H_OFFSET)

/*=====================DEFINE  SECTION  END=======================*/

/*=====================VARIABLE  SECTION  START===================*/



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
	os_printf("current chip temperature about %.2f\r\n",temp);
#endif
}
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
#include <os/mem.h>
#include <stdlib.h>
#include <stdio.h>
#include "string.h"

#include "sys_driver.h"
#include "sys/time.h"

#include <driver/aon_rtc.h>
#include "aon_rtc_driver_64bit.h"

/* 
 * NOTES: System entery deepsleep or reboot, the aon rtc time should reserved.
 * 1.When enter deep sleep, the DTCM power is lost,so have to save time to flash.
 * 2.Write time to flash takes about 200us~3ms, When reboot system,the easy flash API call rtos_get_semaphore in ISR cause assert
 * 3.When reboot system,DTCM doesn't loss power,so can save time in DTCM.
 */
#if CONFIG_AON_RTC_KEEP_TIME_SUPPORT
#include <modules/pm.h>
#include "reset_reason.h"
#if CONFIG_EASY_FLASH
#include "bk_ef.h"
#endif
#endif
#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
static SPINLOCK_SECTION volatile spinlock_t rtc_spin_lock = SPIN_LOCK_INIT;
#endif // CONFIG_FREERTOS_SMP
#define AONRTC_GET_SET_TIME_RTC_ID AON_RTC_ID_1
static int64_t s_boot_time_us = 0;	//timeofday value

#ifdef CONFIG_EXTERN_32K
static uint32_t s_aon_rtc_clock_freq = AON_RTC_EXTERN_32K_CLOCK_FREQ;
#else
static uint32_t s_aon_rtc_clock_freq = AON_RTC_DEFAULT_CLOCK_FREQ;
#endif
static uint64_t s_time_base_us = 0;
static uint64_t s_time_base_tick = 0;

static inline uint32_t rtc_enter_critical()
{
       uint32_t flags = rtos_disable_int();

#ifdef CONFIG_FREERTOS_SMP
       spin_lock(&rtc_spin_lock);
#endif // CONFIG_FREERTOS_SMP

       return flags;
}

static inline void rtc_exit_critical(uint32_t flags)
{
#ifdef CONFIG_FREERTOS_SMP
       spin_unlock(&rtc_spin_lock);
#endif // CONFIG_FREERTOS_SMP

       rtos_enable_int(flags);
}

__IRAM_SEC float bk_rtc_get_ms_tick_count(void) {
	return (float)s_aon_rtc_clock_freq/1000;
}

uint32_t bk_rtc_get_clock_freq(void) {
	return s_aon_rtc_clock_freq;
}

static uint64_t rtc_tick_to_us(uint64_t rtc_tick)
{
	if(s_aon_rtc_clock_freq == AON_RTC_EXTERN_32K_CLOCK_FREQ)
	{
		return ((rtc_tick * 125LL * 125LL) >> 9); // rtc_tick * 1000 * 1000 / 32768;
	}
	else if(s_aon_rtc_clock_freq == AON_RTC_DEFAULT_CLOCK_FREQ)
	{
		return ((rtc_tick * 125LL) >> 2);   // rtc_tick * 1000 * 1000 / 32000;
	}
	else if(s_aon_rtc_clock_freq != 0)
	{
		return (rtc_tick * 1000LL * 1000LL) / s_aon_rtc_clock_freq;
	}

	return -1;
}

static uint64_t rtc_tick_to_ms(uint64_t rtc_tick)
{
	if(s_aon_rtc_clock_freq == AON_RTC_EXTERN_32K_CLOCK_FREQ)
	{
		return ((rtc_tick * 125) >> 12); // rtc_tick * 1000 / 32768;
	}
	else if(s_aon_rtc_clock_freq == AON_RTC_DEFAULT_CLOCK_FREQ)
	{
		return (rtc_tick >> 5);   // rtc_tick * 1000 / 32000;
	}
	else if(s_aon_rtc_clock_freq != 0)
	{
		return (rtc_tick * 1000LL) / s_aon_rtc_clock_freq;
	}

	return -1;
}

static uint64_t rtc_tick_to_s(uint64_t rtc_tick)
{
	if(s_aon_rtc_clock_freq == AON_RTC_EXTERN_32K_CLOCK_FREQ)
	{
		return ((rtc_tick) >> 15); // rtc_tick / 32768;
	}
	else if(s_aon_rtc_clock_freq == AON_RTC_DEFAULT_CLOCK_FREQ)
	{
		return (rtc_tick >> 8) / 125LL;   // rtc_tick / 32000;
	}
	else if(s_aon_rtc_clock_freq != 0)
	{
		return rtc_tick / s_aon_rtc_clock_freq;
	}

	return -1;
}

 __IRAM_SEC uint64_t bk_aon_rtc_get_us(void) {
	uint64_t time_tick = bk_aon_rtc_get_current_tick(AONRTC_GET_SET_TIME_RTC_ID);
	uint64_t time_diff = rtc_tick_to_us(time_tick - s_time_base_tick);  // *1000LL/bk_rtc_get_ms_tick_count();
    uint64_t time_us = s_time_base_us + time_diff;
    return  time_us;
}

 __IRAM_SEC uint64_t bk_aon_rtc_get_ms(void) {
	uint64_t time_tick = bk_aon_rtc_get_current_tick(AONRTC_GET_SET_TIME_RTC_ID);
	uint64_t time_diff = rtc_tick_to_ms(time_tick - s_time_base_tick);
    uint64_t time_ms = s_time_base_us + time_diff;
    return  time_ms;
}

#if CONFIG_AON_RTC_KEEP_TIME_SUPPORT
#define AONRTC_DEEPSLEEP_KEEPTIME_EF_KEYNUM 2
/*
 * WARNING:
 * This function is only called by AON RTC driver when set timeofday!!!
 * WORKAROUND:
 * save the time value to flash.
 * after system wakeup,restore the baked time.
 */
static bk_err_t aon_rtc_bake_timeofday()
{
#if CONFIG_EASY_FLASH
	struct timeval rtc_keep_time = {0, 0};
	rtc_keep_time.tv_sec = s_boot_time_us/1000000;
	rtc_keep_time.tv_usec = s_boot_time_us%1000000;
	//bake up current time to flash
	char *rtc_keep_time_key[2] = {"rtc_tv_sec", "rtc_tv_usec"};
	uint32_t ret = 0;

	for(int key_index = 0; key_index < AONRTC_DEEPSLEEP_KEEPTIME_EF_KEYNUM; key_index++)
	{
		if(key_index == 0)
			ret = bk_set_env_enhance(rtc_keep_time_key[key_index], (const void *)&rtc_keep_time.tv_sec, sizeof(rtc_keep_time.tv_sec));
		else if(key_index == 1)
			ret = bk_set_env_enhance(rtc_keep_time_key[key_index], (const void *)&rtc_keep_time.tv_usec, sizeof(rtc_keep_time.tv_usec));

		AON_RTC_LOGV("%s, sec=%d, usec=%d, ret=%d.\n",__func__, (uint32_t)rtc_keep_time.tv_sec, (uint32_t)rtc_keep_time.tv_usec, ret);
	}
#else
	AON_RTC_LOGD("TODO:Please save the time to none lost memory\r\n", __func__);
#endif
	return BK_OK;
}

static void aon_rtc_get_timeofday(struct timeval *time_p)
{
	AON_RTC_LOGV("%s[+]\r\n", __func__);

#if CONFIG_EASY_FLASH
	char *rtc_keep_time_key[2] = {"rtc_tv_sec", "rtc_tv_usec"};

	for(int key_index = 0; key_index < AONRTC_DEEPSLEEP_KEEPTIME_EF_KEYNUM; key_index++){
		if(key_index == 0)
			bk_get_env_enhance(rtc_keep_time_key[key_index], &(time_p->tv_sec), sizeof(time_p->tv_sec));
		else if(key_index == 1)
			bk_get_env_enhance(rtc_keep_time_key[key_index], &(time_p->tv_usec), sizeof(time_p->tv_usec));
	}
	AON_RTC_LOGV("%s tv_sec:%d tv_usec:%d\n",__func__, (uint32_t)time_p->tv_sec, (uint32_t)time_p->tv_usec);
#else
	AON_RTC_LOGD("TODO:%s\n", __func__);
#endif

}

void aon_rtc_update_boot_time()
{
	struct timeval time = {0, 0};
	aon_rtc_get_timeofday(&time);
	s_boot_time_us = ((uint64_t)time.tv_sec)*1000000LL+time.tv_usec;
}
#endif


static uint64_t aon_rtc_counter_get_tick()
{
	volatile uint32_t val = REG_READ(AON_RTC_CNT_VAL_L_ADDR);
	volatile uint32_t val_hi = REG_READ(AON_RTC_CNT_VAL_H_ADDR);

	while (REG_READ(AON_RTC_CNT_VAL_L_ADDR) != val
		|| REG_READ(AON_RTC_CNT_VAL_H_ADDR) != val_hi)
	{
		val = REG_READ(AON_RTC_CNT_VAL_L_ADDR);
		val_hi = REG_READ(AON_RTC_CNT_VAL_H_ADDR);
	}
	return (((uint64_t)(val_hi) << 32) + val);
}

__IRAM_SEC uint64_t bk_aon_rtc_get_current_tick(aon_rtc_id_t id)
{
	if(id >= AON_RTC_ID_MAX)
	{
		AON_RTC_LOGW("%s:id=%d\r\n", __func__, id);
		return 0;
	}

	return (aon_rtc_counter_get_tick());
}

bk_err_t bk_rtc_gettimeofday(struct timeval *tv, void *ptz)
{
    (void)ptz;

    if(tv!=NULL)
    {
        uint64_t uCurTimeUs = s_boot_time_us + bk_aon_rtc_get_us();

        tv->tv_sec=uCurTimeUs/1000000;
        tv->tv_usec=uCurTimeUs%1000000;
        AON_RTC_LOGV("s_boot_time_us:h=0x%x,l=0x%x \r\n", s_boot_time_us>>32, (uint32_t)s_boot_time_us);
        AON_RTC_LOGV("%s sec=%d,us:%d\n", __func__, tv->tv_sec, tv->tv_usec);
    } else
        AON_RTC_LOGW("%s tv is null \r\n",__func__);

    return BK_OK;
}

bk_err_t bk_rtc_settimeofday(const struct timeval *tv,const struct timezone *tz)
{
    (void)tz;
    if(tv)
    {
        uint64_t setTimeUs = ((uint64_t)tv->tv_sec)*1000000LL + tv->tv_usec ;
        uint64_t getCurTimeUs = bk_aon_rtc_get_us();

        s_boot_time_us = setTimeUs - getCurTimeUs;
        AON_RTC_LOGV("%s:sec=%d us=%d\n", __func__, tv->tv_sec, tv->tv_usec);
        AON_RTC_LOGV("get us:h=0x%x,l=0x%x\n", getCurTimeUs>>32, (uint32_t)getCurTimeUs);

#if CONFIG_AON_RTC_KEEP_TIME_SUPPORT
        aon_rtc_bake_timeofday();
#endif
    }

    return BK_OK;
}



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

#pragma once

#include <driver/aon_rtc_types.h>
#include "sys/time.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Get AON RTC current tick with 64 bits
 *	       AON RTC uses 32 Bits counter with 32K clock, the max time is about 36.4 hours.
 *         The 32K clock really frequency is 32768 HZ(External 32K XTL) or 32000 HZ(Internel ROSC).
 *         Set upper interrupt as the 0xFFFFFFFF ticks as one round.
 * @id: register to which aon_rtc id
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
uint64_t bk_aon_rtc_get_current_tick(aon_rtc_id_t id);

/**
 * @brief     Get time of day
 *
 * @tv: Define parameters according to "structure timeval"
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_rtc_gettimeofday(struct timeval *tv, void *ptz);

/**
 * @brief     Set time of day
 *
 * @tv: Define parameters according to "structure timeval"
 * @tz: Define parameters according to "structure timezone"
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_rtc_settimeofday(const struct timeval *tv,const struct timezone *tz);

/**
 * @brief   Get aon rtc ticks of per ms
 *
 *
 * @return
 *    - aon rtc ticks of per ms
 *        |- EXTERN 32K: 32.768
 *        |- Default:    32.000
 */
float bk_rtc_get_ms_tick_count(void);


/**
 * @brief   Get aon rtc clock freq
 *
 *
 * @return
 *    - aon rtc clock freq
 *        |- EXTERN 32K: 32768
 *        |- Default:    32000
 */
uint32_t bk_rtc_get_clock_freq(void);

/**
 * @brief  Get AON RTC current us with 64 bits
 *         AON RTC uses 32 Bits counter with 32K clock, the max time is about 36.4 hours.
 *         The 32K clock really frequency is 32768 HZ(External 32K XTL) or 32000 HZ(Internel ROSC).
 *         Set upper interrupt as the 0xFFFFFFFF ticks as one round.
 * @id: use AON_RTC_ID_1
 *
 * @return
 *    - AON RTC current us with 64 bits with about +30us/-30us presision
 *
 */
uint64_t bk_aon_rtc_get_us(void);

/**
 * @brief  Get AON RTC current ms with 64 bits
 *         AON RTC uses 32 Bits counter with 32K clock, the max time is about 36.4 hours.
 *         The 32K clock really frequency is 32768 HZ(External 32K XTL) or 32000 HZ(Internel ROSC).
 *         Set upper interrupt as the 0xFFFFFFFF ticks as one round.
 * @id: use AON_RTC_ID_1
 *
 * @return
 *    - AON RTC current us with 64 bits with about +30us/-30us presision
 *
 */
uint64_t bk_aon_rtc_get_ms(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif




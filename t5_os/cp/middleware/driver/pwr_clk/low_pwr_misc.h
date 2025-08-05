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

#pragma once
#include <driver/aon_rtc.h>

/**
 * @brief set the rtc tick at the begin of startup
 *
 * @attention
 * - This API is used to set the rtc tick at the begin of startup.
 *
 * @param
 * -time_tick:rtc tick
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_low_pwr_misc_startup_rtc_tick_set(uint64_t time_tick);
/**
 * @brief get the time interval from the startup(unit:us)
 *
 * @attention
 * - This API is used to get the time interval from the startup(unit:us).
 *
 * @param
 * -time_interval[output]:time interval(unit:us)
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_low_pwr_misc_get_time_interval_from_startup(uint32_t* time_interval);
/**
 * @brief enter deepsleep ,wakeup source using rtc
 *
 * @attention
 * - This API is used to enter deepsleep ,wakeup source using rtc(unit:ms).
 *
 * @param
 * -time_interval[input]:time interval(unit:ms)
 * -callback[input]:register callback
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_low_pwr_misc_rtc_enter_deepsleep(uint32_t time_interval , aon_rtc_isr_t callback);
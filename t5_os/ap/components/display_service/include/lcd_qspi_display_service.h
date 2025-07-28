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

#include <driver/lcd_types.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief     Start the qspi lcd display task
 *
 * @param device the struct of lcd device
 *
 * @return
 *    - None
 */
void bk_lcd_qspi_disp_task_start(const lcd_device_t *device);

/**
 * @brief     Stop the qspi lcd display task
 *
 * @param
 *    - None
 *
 * @return
 *    - None
 */
void bk_lcd_qspi_disp_task_stop(void);

/**
 * @brief     Display a frame data
 *
 * @param frame the address of frame data
 *
 * @return
 *    - None
 */
void bk_lcd_qspi_display(uint32_t frame);


#ifdef __cplusplus
}
#endif



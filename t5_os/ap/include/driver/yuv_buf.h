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

#include <common/bk_include.h>
#include <driver/yuv_buf_types.h>
#include <driver/jpeg_enc_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* @brief Overview about this API header
 *
 */


/**
 * @brief     Init the yuv buf driver
 *
 * This API init the resoure common:
 *   - Init yuv buf register
 *   - register yuv buf isr
 *
 * @attention 1. This API should be called before any other jpeg APIs.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_yuv_buf_driver_init(void);

/**
 * @brief     Denit the yuv buf driver
 *
 * This API Denit the resoure common:
 *   - Soft reset yuv buf register
 *   - deregister yuv buf isr
 *
 * @attention 1. This API should be the last one api been called.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_yuv_buf_driver_deinit(void);

/**
 * @brief     Init the yuv buf
 *
 * This API init the yuv buf
 *  - vote cpu to 480MHz
 *  - vote video power on
 *  - set yuv_buf and h264 clk enable
 *  - enable yuv buf system isr
 *  - according work mode, config release register
 *
 * Usage example:
 *
 *      yuv_buf_config_t init_config = {
 *                          .work_mode = xxx, // work mode(yuv/jpeg/h264)
 *                          .mclk_div = 2,    // mclk div value
 *                          .x_pixel = 80,    // image resolution of width: 80 * 8 = 640
 *                          .y_pixel = 60,    // image resolution of height: 60 * 8 = 480
 *                          .{
 *                              .yuv_format = YUV_FORMAT_YUYV, // sensor output data format
 *                              .vsync = SYNC_HIGH_LEVEL,      // sensor vsync active level
 *                              .hsync = SYNC_HIGH_LEVEL,      // sensor hsync active level
 *                           }
 *                          .base_addr = xxx, // sensor output data temp save address
 *                          .emr_base_addr = xxx, // encode module read data address
 *                      };
 *
 *
 * @param config yuv buf parameter settings
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_yuv_buf_init(const yuv_buf_config_t *config);

/**
 * @brief     Deinit yuv buf
 *
 * This API stop yuv buf, disable clk, devote cpu clk, devote video power. Soft reset yuv buf register to init status.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_yuv_buf_deinit(void);

/**
 * @brief     set yuv buf input resolution
 *
 * This API will set before yuv_buf enable
 *
 * @param width image width
 * @param height image height
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_yuv_buf_set_frame_resolution(uint32_t width, uint32_t height);

/**
 * @brief     config yuv buf resize function
 *
 * This API use to resize sensor output resolution
 *
 * @param config resize_config
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_yuv_buf_set_resize(const yuv_buf_resize_config_t *config);

/**
 * @brief     enable yuv buf module
 *
 * This API use to enable yuv buf module
 *
 * @param work_mode mode:yuv/jpeg/h264
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_yuv_buf_start(yuv_mode_t work_mode);

/**
 * @brief     disable yuv buf module
 *
 * This API use to diable yuv buf module
 *
 * @param work_mode mode:yuv/jpeg/h264
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_yuv_buf_stop(yuv_mode_t work_mode);

/**
 * @brief     restart encode
 *
 * This API use to trriger encode function by 16line pertime
 *
 * @attention 1. This API only for encode by line(no sensor mode)
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_yuv_buf_rencode_start(void);

/**
 * @brief     config yuv buf no sensor mode
 *
 * This API use to config yuv buf register
 *
 * @attention 1. This API only for no sensor mode
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_yuv_buf_enable_nosensor_encode_mode(void);

/**
 * @brief     set save sensor output data addr
 *
 * This API use to config yuv buf register
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_yuv_buf_set_em_base_addr(uint32_t em_base_addr);

/**
 * @brief     get save sensor output data addr
 *
 * This API use to get yuv buf register
 *
 * @return
 *    - address
 */
uint32_t bk_yuv_buf_get_em_base_addr(void);

/**
 * @brief     set bus read data to encode addr
 *
 * This API use to config yuv buf register
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_yuv_buf_set_emr_base_addr(uint32_t emr_base_addr);

/**
 * @brief     get bus read data to encode addr
 *
 * This API use to get yuv buf register
 *
 * @return
 *    - address
 */
uint32_t bk_yuv_buf_get_emr_base_addr(void);

/**
 * @brief     register yuv buf isr callback function
 *
 * This API use to get yuv buf register
 *
 * @param type_id isr type
 * @param isr isr callback function
 * @param param isr param
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_yuv_buf_register_isr(yuv_buf_isr_type_t type_id, yuv_buf_isr_t isr, void *param);

/**
 * @brief     unregister yuv buf isr callback function
 *
 * This API use to get yuv buf register
 *
 * @param type_id isr type
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_yuv_buf_unregister_isr(yuv_buf_isr_type_t type_id);

/**
 * @brief     init partial display function
 *
 * This API use to config yuv buf register
 *
 * @param offset_config partial display config
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_yuv_buf_init_partial_display(const yuv_buf_partial_offset_config_t *offset_config);

/**
 * @brief     deinit partial display function
 *
 * This API use to disable partial display function
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_yuv_buf_deinit_partial_display(void);

/**
 * @brief     set mclk div
 *
 * This API use to config yuv buf register
 *
 * @param div mclk div
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_yuv_buf_set_mclk_div(mclk_div_t div);

/**
 * @brief     soft reset yuv buf
 *
 * This API use to soft reset yuv buf
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_yuv_buf_soft_reset(void);


#ifdef __cplusplus
}
#endif


// Copyright 2022-2023 Beken
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
//

#pragma once

#include <common/bk_include.h>
#include <driver/h264_types.h>
#include <driver/media_types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief     init h264 driver
 *
 * This API use to init h264 driver register and register system isr
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_h264_driver_init(void);

/**
 * @brief     deinit h264 driver
 *
 * This API use to deinit h264 driver register and deregister system isr
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_h264_driver_deinit(void);

/**
 * @brief     Init the h264 encoder
 *
 * This API init the h264 encoder
 *  - vote cpu to 480MHz
 *  - vote video power on
 *  - set h264 clk enable
 *  - enable h264 system isr
 *  - according image resolution, config release register
 *
 * @param width input image width
 * @param height input image height
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_h264_init(uint16_t width, uint16_t height);

/**
 * @brief Deinitialize the H264 module
 *
 * This API deinitialize the H264 module
 *  - vote cpu to default
 *  - vote video power off
 *  - disable h264 clk
 *  - disable h264 system isr
 *
 * @return
 *    - BK_OK: Success
 *    - others: Other errors
 */
bk_err_t bk_h264_deinit(void);

/**
 * @brief config h264 encode ratio
 *
 * This API modify h264 register to config encode ratio, you can call anytime
 *
 * @param config compress ratio config
 *    - profile_idc: profile idc
 *    - p_frame_cnt: p frame count
 *    - qp: qp
 *    - num_imb_bits: num imb bits
 *    - num_pmb_bits: num pmb bits
 *
 * @return
 *    - BK_OK: Success
 *    - others: Other errors
 */
bk_err_t bk_h264_set_base_config(compress_ratio_t *config);

/**
 * @brief get h264 encode base config
 *
 * This API get h264 encode base config, you can call anytime
 *
 * @param config h264 base config
 *    - profile_idc: profile idc
 *    - p_frame_cnt: p frame count
 *    - qp: qp
 *    - num_imb_bits: num imb bits
 *    - num_pmb_bits: num pmb bits
 *
 * @return
 *    - BK_OK: Success
 *    - others: Other errors
 */
bk_err_t bk_h264_get_h264_base_config(h264_base_config_t *config);

/**
 * @brief set h264 encode p frame count
 *
 * This API can set h264 encode p frame count
 *
 * @param pframe_number p frame number
 *
 * @attention if you use h264 pipeline, suggest modify it by CONFIG_H264_P_FRAME_CNT
 * @return
 *    - BK_OK: Success
 *    - others: Other errors
 */
bk_err_t bk_h264_set_pframe_num(uint32_t pframe_number);

/**
 * @brief get h264 encode p frame count
 *
 * This API get h264 encode p frame count
 *
 * @return
 *    - p frame number
 */
uint32_t bk_h264_get_pframe_num(void);

/**
 * @brief set h264 encode qp
 *
 * This API can set h264 encode qp
 *
 * @param qp qp config
 *    - qp: qp
 *    - num_imb_bits: num imb bits
 *    - num_pmb_bits: num pmb bits
 *
 * @return
 *    - BK_OK: Success
 *    - others: Other errors
 */
bk_err_t bk_h264_set_qp(h264_qp_t *qp);

/**
 * @brief set h264 encode qp
 *
 * This API can set h264 encode qp
 *
 * @param imb_bits imb bits
 * @param pmb_bits pmb bits
 *
 * @return
 *    - BK_OK: Success
 *    - others: Other errors
 */
bk_err_t bk_h264_set_quality(uint32_t imb_bits, uint32_t pmb_bits);

/**
 * @brief enable h264 encode
 *
 * This API enable h264 encode
 *
 * @return
 *    - BK_OK: Success
 *    - others: Other errors
 */
bk_err_t bk_h264_encode_enable(void);

/**
 * @brief disable h264 encode
 *
 * This API disable h264 encode
 *
 * @return
 *    - BK_OK: Success
 *    - others: Other errors
 */
bk_err_t bk_h264_encode_disable(void);

/**
 * @brief soft reset h264
 *
 * This API soft reset h264, you can call this to reset encode logical, can regenrate idr frame
 *
 * @return
 *    - BK_OK: Success
 *    - others: Other errors
 */
bk_err_t bk_h264_soft_reset(void);

/**
 * @brief config reset h264 driver
 *
 * This API config reset h264 driver, reset all register to init status
 *
 * @attention this api will reconfig h264 all register
 *
 * @return
 *    - BK_OK: Success
 *    - others: Other errors
 */
bk_err_t bk_h264_config_reset(void);

/**
 * @brief get h264 encode fifo addr
 *
 * This API get h264 encode fifo addr
 *
 * @param fifo_addr h264 encode fifo addr
 *
 * @return
 *    - BK_OK: Success
 *    - others: Other errors
 */
bk_err_t bk_h264_get_fifo_addr(uint32_t *fifo_addr);

/**
 * @brief register h264 isr
 *
 * This API register h264 isr
 *
 * @param type_id isr type
 * @param isr isr callback function
 * @param param isr param
 *
 * @return
 *    - BK_OK: Success
 *    - others: Other errors
 */
bk_err_t bk_h264_register_isr(h264_isr_type_t type_id, h264_isr_t isr, void *param);

/**
 * @brief unregister h264 isr
 *
 * This API unregister h264 isr
 *
 * @param type_id isr type
 *
 * @return
 *    - BK_OK: Success
 *    - others: Other errors
 */
bk_err_t bk_h264_unregister_isr(h264_isr_type_t type_id);

/**
 * @brief get h264 encode count
 *
 * This API get h264 encode count
 *
 * @attention output unit is word(4 bytes)
 *
 * @return
 *    - encode count
 */
uint32_t bk_h264_get_encode_count(void);

/**
 * @brief h264 encode lcd dma copy
 *
 * This API h264 encode lcd dma copy
 *
 * @param out output addr
 * @param in input addr
 * @param len copy len
 * @param cpy_chnl dma channel
 *
 * @return
 *    - BK_OK: Success
 *    - others: Other errors
 */
bk_err_t bk_h264_enc_lcd_dma_cpy(void *out, const void *in, uint32_t len, dma_id_t cpy_chnl);

/**
 * @brief h264 updata encode fps
 *
 * This API h264 updata encode fps, this fps will release h264 decode fps
 *
 * @param fps fps
 *
 * @return
 *    - BK_OK: Success
 *    - others: Other errors
 */
bk_err_t bk_h264_updata_encode_fps(uint32_t fps);

/**
 * @brief h264 module global softreset
 *
 * This API will use for resetting h264 module
 *
 * @param enable 0:disable, 1:enable
 *
 * @attenation this api must be called twice to complete the reset: first enable(1), then disable(0)
 *
 * @return
 *    - BK_OK: Success
 *    - others: Other errors.
 */
bk_err_t bk_h264_global_soft_reset(uint8_t enable);

#ifdef __cplusplus
}
#endif

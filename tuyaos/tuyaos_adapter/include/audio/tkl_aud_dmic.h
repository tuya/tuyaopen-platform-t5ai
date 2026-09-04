/**
 * @file tkl_aud_dmic.h
 * @brief
 *
 * @copyright Copyright (c) 2025 Tuya Inc. All Rights Reserved.
 *
 * Permission is hereby granted, to any person obtaining a copy of this software and
 * associated documentation files (the "Software"), Under the premise of complying
 * with the license of the third-party open source software contained in the software,
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software.
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 */

#ifndef __TKL_AUD_DMIC_H__
#define __TKL_AUD_DMIC_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/* Public macros -------------------------------------------------------------*/

/* Public typedefs -----------------------------------------------------------*/
typedef struct {
    TUYA_AUDIO_DMIC_CHAN_E chan;
    uint8_t vol;
    uint8_t sample_bits;            // T5 SMP, only support 16bits
    uint32_t sample_rate;
    uint32_t frame_time_ms;
    TKL_AUD_INPUT_CB upper_cb;
    void *args;
} TKL_AUD_DMIC_CFG_T;

/* Public variables ----------------------------------------------------------*/

/* Public function prototypes ------------------------------------------------*/

/**
 * @brief Initialize audio digital (DMIC) input
 * @param[in] port Digital audio port number
 * @param[in] config DMIC configuration
 * @return OPRT_OK on success, error code on failure
 */
OPERATE_RET tkl_aud_dmic_init(TUYA_AUDIO_DMIC_PORT_E port, TKL_AUD_DMIC_CFG_T *config);

/**
 * @brief Start audio digital (DMIC) capture
 * @param[in] port Digital audio port number
 * @return OPRT_OK on success, error code on failure
 */
OPERATE_RET tkl_aud_dmic_start(TUYA_AUDIO_DMIC_PORT_E port);

/**
 * @brief Set audio digital (DMIC) gain
 * @param[in] port Digital audio port number
 * @param[in] gain Digital gain value
 * @return OPRT_OK on success, error code on failure
 */
OPERATE_RET tkl_aud_dmic_set_vol(TUYA_AUDIO_DMIC_PORT_E port, uint32_t gain);

/**
 * @brief Stop audio digital (DMIC) capture
 * @param[in] port Digital audio port number
 * @return OPRT_OK on success, error code on failure
 */
OPERATE_RET tkl_aud_dmic_stop(TUYA_AUDIO_DMIC_PORT_E port);

/**
 * @brief Deinitialize audio digital (DMIC) input
 * @param[in] port Digital audio port number
 * @return none
 */
void tkl_aud_dmic_deinit(TUYA_AUDIO_DMIC_PORT_E port);

#ifdef __cplusplus
}
#endif

#endif /*__TKL_AUD_DMIC_H__ */


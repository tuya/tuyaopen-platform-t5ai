/**
 * @file tkl_aud_dac.h
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

#ifndef __TKL_AUD_DAC_H__
#define __TKL_AUD_DAC_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/* Public macros -------------------------------------------------------------*/

/* Public typedefs -----------------------------------------------------------*/
typedef void (*TKL_AUD_DAC_FRAME_CB)(TUYA_AUDIO_DAC_FRAME_EVT_E event, void *args);

typedef struct {
    uint8_t volume;
    uint8_t  chan_num;      // now, only one speaker
    uint8_t sample_bits;    // T5 SMP, only support 16bits
    uint32_t sample_rate;
    uint32_t frame_time_ms;
    TKL_AUD_DAC_FRAME_CB frame_cb;
    void              *args;
}TKL_AUD_DAC_CFG_T;

/* Public variables ----------------------------------------------------------*/

/* Public function prototypes ------------------------------------------------*/
/**
 * @brief Initialize the DAC playback context and hardware resources.
 * @param[in] config DAC configuration used to set sample rate, frame size,
 *                   initial volume, and frame callback.
 * @param[in] port DAC output port. This parameter is ignored on T5.
 * @return OPRT_OK on success, other error codes on failure.
 * @note The current platform only supports 16-bit PCM output and forces the
 *       playback channel count to one onboard speaker.
 * @note This function must be called before `tkl_aud_dac_start()`.
 */
OPERATE_RET tkl_aud_dac_init(TUYA_AUDIO_DAC_PORT_E port, TKL_AUD_DAC_CFG_T *config);

/**
 * @brief Deinitialize the DAC driver instance.
 * @param[in] port DAC output port. This parameter is ignored on T5.
 * @return OPRT_OK on success, other error codes on failure.
 * @note If playback is still running, this function stops it first and then
 *       releases DMA, DAC, callback, and power-management resources.
 */
OPERATE_RET tkl_aud_dac_deinit(TUYA_AUDIO_DAC_PORT_E port);

/**
 * @brief Start DAC playback.
 * @param[in] port DAC output port. This parameter is ignored on T5.
 * @return OPRT_OK on success, other error codes on failure.
 * @note The driver pre-fills the DMA ring buffer with silence before enabling
 *       DMA and DAC output to reduce pop noise on startup.
 * @note Calling this function after playback has already started returns
 *       success without reinitializing the hardware.
 */
OPERATE_RET tkl_aud_dac_start(TUYA_AUDIO_DAC_PORT_E port);

/**
 * @brief Stop DAC playback.
 * @param[in] port DAC output port. This parameter is ignored on T5.
 * @return OPRT_OK on success, other error codes on failure.
 * @note This function stops DMA transfer first, mutes the DAC output, and
 *       then stops the DAC hardware.
 * @note Calling this function after playback has already stopped returns
 *       success without additional hardware operations.
 */
OPERATE_RET tkl_aud_dac_stop(TUYA_AUDIO_DAC_PORT_E port);

/**
 * @brief Update the DAC output volume.
 * @param[in] port DAC output port. This parameter is ignored on T5.
 * @param[in] volume Logical volume value in the range `0x00` to `0x3F`.
 * @return OPRT_OK on success, other error codes on failure.
 * @note The input volume is remapped to a non-linear hardware gain curve
 *       before calling the underlying DAC gain API.
 * @note A volume value of `0` mutes the DAC output. Non-zero values unmute it.
 */
OPERATE_RET tkl_aud_dac_set_volume(TUYA_AUDIO_DAC_PORT_E port, uint32_t volume);

/**
 * @brief Write PCM data into the DAC DMA ring buffer.
 * @param[in] port DAC output port. This parameter is ignored on T5.
 * @param[in] buffer Pointer to the PCM data buffer.
 * @param[in] len PCM data length in bytes.
 * @return OPRT_OK on success, `OPRT_OS_ADAPTER_DAC_BUSY` when the ring buffer
 *         does not have enough free space, or other error codes on failure.
 * @note The DAC must already be in the started state before this function is
 *       called.
 * @note Data is copied into the internal DMA ring buffer, so the caller must
 *       ensure that enough free space is available for the requested length.
 */
OPERATE_RET tkl_aud_dac_write(TUYA_AUDIO_DAC_PORT_E port, uint8_t *buffer, uint32_t len);

/**
 * @brief Get the configured DAC frame size in bytes.
 * @param[in] port DAC output port. This parameter is ignored on T5.
 * @return Frame size in bytes, or `0` if the DAC driver is not initialized.
 */
uint32_t tkl_aud_dac_get_frame_size(TUYA_AUDIO_DAC_PORT_E port);


#ifdef __cplusplus
}
#endif

#endif /*__TKL_AUD_DAC_H__ */


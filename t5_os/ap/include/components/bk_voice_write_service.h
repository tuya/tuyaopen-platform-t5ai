#pragma once

#include <components/bk_voice_write_service_types.h>

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

/**
 * @brief      Create a voice write handle to write speech data to voice service.
 *
 * @param[in]      cfg  The voice write configuration.
 *
 * @return         The voice write handle.
 *                 - Not NULL: Success.
 *                 - NULL: Failed.
 */
voice_write_handle_t bk_voice_write_init(voice_write_cfg_t *cfg);

/**
 * @brief      Destroy a voice write handle.
 *
 * @param[in]      voice_write_handle  The voice write handle.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_voice_write_deinit(voice_write_handle_t voice_write_handle);

/**
 * @brief      Start a voice write.
 *
 * @param[in]      voice_write_handle  The voice write handle.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_voice_write_start(voice_write_handle_t voice_write_handle);

/**
 * @brief      Stop a voice write.
 *
 * @param[in]      voice_write_handle  The voice write handle.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_voice_write_stop(voice_write_handle_t voice_write_handle);

/**
 * @brief      Write speech data to voice service.
 *
 * @param[in]      voice_write_handle  The voice write handle.
 * @param[in]      buffer              The speech data buffer.
 * @param[in]      len                 The speech data length.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_voice_write_frame_data(voice_write_handle_t voice_write_handle, char *buffer, uint32_t len);

#ifdef  __cplusplus
}
#endif//__cplusplus

/**
 * @}
 */
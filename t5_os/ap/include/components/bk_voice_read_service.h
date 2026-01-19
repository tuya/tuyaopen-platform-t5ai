#pragma once

#include <components/bk_voice_read_service_types.h>

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

/**
 * @brief      Create a voice read handle to read mic data from voice service.
 *
 * @param[in]      cfg  The voice read configuration.
 *
 * @return         The voice read handle.
 *                 - Not NULL: Success.
 *                 - NULL: Failed.
 */
voice_read_handle_t bk_voice_read_init(voice_read_cfg_t *cfg);

/**
 * @brief      Destroy a voice read handle.
 *
 * @param[in]      voice_read_handle  The voice read handle.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_voice_read_deinit(voice_read_handle_t voice_read_handle);

/**
 * @brief      Start a voice read.
 *
 * @param[in]      voice_read_handle  The voice read handle.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_voice_read_start(voice_read_handle_t voice_read_handle);

/**
 * @brief      Stop a voice read.
 *
 * @param[in]      voice_read_handle  The voice read handle.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_voice_read_stop(voice_read_handle_t voice_read_handle);


#ifdef  __cplusplus
}
#endif//__cplusplus

/**
 * @}
 */
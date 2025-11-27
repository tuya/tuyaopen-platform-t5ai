#pragma once

#include <components/bk_voice_service_types.h>

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

/**
 * @brief      Create a voice call handle.
 *
 * @param[in]      cfg  The voice call configuration.
 *
 * @return         The voice handle.
 *                 - Not NULL: Success.
 *                 - NULL: Failed.
 */
voice_handle_t bk_voice_init(voice_cfg_t *cfg);

/**
 * @brief      Destroy a voice call handle.
 *
 * @param[in]      voice_handle  The voice call handle.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_voice_deinit(voice_handle_t voice_handle);

/**
 * @brief      Start a voice call.
 *
 * @param[in]      voice_handle  The voice call handle.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_voice_start(voice_handle_t voice_handle);

/**
 * @brief      Stop a voice call.
 *
 * @param[in]      voice_handle  The voice call handle.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_voice_stop(voice_handle_t voice_handle);

/**
 * @brief      Read microphone data.
 *
 * @param[in]      voice_handle  The voice call handle.
 * @param[out]     buffer        The buffer to store microphone data.
 * @param[in]      size          The size of the buffer.
 *
 * @return         The actual number of bytes read.
 *                 - Greater than 0: The number of bytes successfully read.
 *                 - Less than or equal to 0: Failed.
 */
int bk_voice_read_mic_data(voice_handle_t voice_handle, char *buffer, uint32_t size);

/**
 * @brief      Write data to the speaker.
 *
 * @param[in]      voice_handle  The voice call handle.
 * @param[in]      buffer        The buffer containing the data to be written.
 * @param[in]      size          The size of the data to be written.
 *
 * @return         The actual number of bytes written.
 *                 - Greater than 0: The number of bytes successfully written.
 *                 - Less than or equal to 0: Failed.
 */
int bk_voice_write_spk_data(voice_handle_t voice_handle, char *buffer, uint32_t size);

/**
 * @brief      Handle voice events.
 *
 * @param[in]      event_handle  The voice event handling function.
 * @param[in]      event         The voice event type.
 * @param[in]      param         The event parameter.
 * @param[in]      args          Additional parameters.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_voice_event_handle(voice_event_handle event_handle, vioce_evt_t event, void *param, void *args);

/**
 * @brief      Get the status of a voice call.
 *
 * @param[in]      voice_handle  The voice call handle.
 * @param[out]     status        The pointer to store the voice status.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_voice_get_status(voice_handle_t voice_handle, voice_sta_t *status);

int bk_voice_get_mic_str(voice_handle_t voice_handle, voice_cfg_t *cfg);

/**
 * @brief      Get the speaker element handle of the voice call.
 *
 * @param[in]      voice_handle  The voice call handle.
 *
 * @return         The speaker element handle.
 *                 - Not NULL: Success.
 *                 - NULL: Failed.
 */
audio_element_handle_t bk_voice_get_spk_element(voice_handle_t voice_handle);

#if 0
int bk_voice_abort_read_mic_data(voice_handle_t voice_handle);
int bk_voice_abort_write_spk_data(voice_handle_t voice_handle);
#endif

void bk_voice_cal_vad_buf_size(voice_cfg_t *cfg, voice_handle_t voice_handle);

/**
 * @brief      Get the microphone stream handle.
 *
 * @param[in]      voice_handle  The voice call handle.
 * @param[out]     mic_str       The pointer to store the microphone stream handle.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_voice_get_micstr(voice_handle_t voice_handle, audio_element_handle_t *mic_str);

/**
 * @brief      Get the microphone type.
 *
 * @param[in]      voice_handle  The voice call handle.
 * @param[out]     mic_type      The pointer to store the microphone type.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_voice_get_micstr_type(voice_handle_t voice_handle, mic_type_t *mic_type);

/**
 * @brief      Get the speaker stream handle.
 *
 * @param[in]      voice_handle  The voice call handle.
 * @param[out]     spk_str       The pointer to store the speaker stream handle.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_voice_get_spkstr(voice_handle_t voice_handle, audio_element_handle_t *spk_str);

/**
 * @brief      Get the speaker type.
 *
 * @param[in]      voice_handle  The voice call handle.
 * @param[out]     spk_type      The pointer to store the speaker type.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_voice_get_spkstr_type(voice_handle_t voice_handle, spk_type_t *spk_type);

/**
 * @brief      Get the AEC algorithm element handle.
 *
 * @param[in]      voice_handle  The voice call handle.
 * @param[out]     aec_alg       The pointer to store the AEC algorithm element handle.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_voice_get_aec_alg(voice_handle_t voice_handle, audio_element_handle_t *aec_alg);

#if CONFIG_VOICE_SERVICE_EQ
/**
 * @brief      Get the EQ algorithm element handle.
 *
 * @param[in]      voice_handle  The voice call handle.
 * @param[out]     eq_alg        The pointer to store the EQ algorithm element handle.
 *
 * @return         Error code.
 *                 - 0: Success.
 *                 - Non-zero: Failed.
 */
bk_err_t bk_voice_get_eq_alg(voice_handle_t voice_handle, audio_element_handle_t *eq_alg);
#endif

#ifdef  __cplusplus
}
#endif//__cplusplus

/**
 * @}
 */
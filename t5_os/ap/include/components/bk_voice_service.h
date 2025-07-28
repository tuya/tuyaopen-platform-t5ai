#pragma once

#include <components/bk_voice_service_types.h>

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus


voice_handle_t bk_voice_init(voice_cfg_t *cfg);
bk_err_t bk_voice_deinit(voice_handle_t voice_handle);
bk_err_t bk_voice_start(voice_handle_t voice_handle);
bk_err_t bk_voice_stop(voice_handle_t voice_handle);
int bk_voice_read_mic_data(voice_handle_t voice_handle, char *buffer, uint32_t size);
int bk_voice_write_spk_data(voice_handle_t voice_handle, char *buffer, uint32_t size);
bk_err_t bk_voice_event_handle(voice_event_handle event_handle, vioce_evt_t event, void *param, void *args);
bk_err_t bk_voice_get_status(voice_handle_t voice_handle, voice_sta_t *status);
#if 0
int bk_voice_abort_read_mic_data(voice_handle_t voice_handle);
int bk_voice_abort_write_spk_data(voice_handle_t voice_handle);
#endif

#ifdef  __cplusplus
}
#endif//__cplusplus

/**
 * @}
 */
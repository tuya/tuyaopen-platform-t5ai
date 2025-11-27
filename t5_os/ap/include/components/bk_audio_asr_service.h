#pragma once

#include <components/bk_audio_asr_service_types.h>

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

asr_handle_t bk_asr_create(asr_cfg_t *cfg);
bk_err_t bk_asr_init(asr_cfg_t *cfg, asr_handle_t asr_handle);
bk_err_t bk_asr_init_with_mic(asr_cfg_t *cfg, asr_handle_t asr_handle);
bk_err_t bk_asr_deinit(asr_handle_t asr_handle);
bk_err_t bk_asr_start(asr_handle_t asr_handle);
bk_err_t bk_asr_stop(asr_handle_t asr_handle);
bk_err_t bk_asr_event_handle(asr_event_handle event_handle, asr_evt_t event, void *param, void *args);
bk_err_t bk_asr_get_status(asr_handle_t asr_handle, asr_sta_t *status);
int bk_aud_asr_read_mic_data(asr_handle_t asr_handle, char *buffer, uint32_t size);

#ifdef  __cplusplus
}
#endif//__cplusplus

/**
 * @}
 */
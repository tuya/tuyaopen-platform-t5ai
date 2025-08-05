#pragma once

#include <components/bk_voice_write_service_types.h>

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus


voice_write_handle_t bk_voice_write_init(voice_write_cfg_t *cfg);
bk_err_t bk_voice_write_deinit(voice_write_handle_t voice_write_handle);
bk_err_t bk_voice_write_start(voice_write_handle_t voice_write_handle);
bk_err_t bk_voice_write_stop(voice_write_handle_t voice_write_handle);
bk_err_t bk_voice_write_frame_data(voice_write_handle_t voice_write_handle, char *buffer, uint32_t len);

#ifdef  __cplusplus
}
#endif//__cplusplus

/**
 * @}
 */
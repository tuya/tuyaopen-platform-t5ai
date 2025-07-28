#pragma once

#include <components/bk_voice_read_service_types.h>

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus


voice_read_handle_t bk_voice_read_init(voice_read_cfg_t *cfg);
bk_err_t bk_voice_read_deinit(voice_read_handle_t voice_read_handle);
bk_err_t bk_voice_read_start(voice_read_handle_t voice_read_handle);
bk_err_t bk_voice_read_stop(voice_read_handle_t voice_read_handle);


#ifdef  __cplusplus
}
#endif//__cplusplus

/**
 * @}
 */
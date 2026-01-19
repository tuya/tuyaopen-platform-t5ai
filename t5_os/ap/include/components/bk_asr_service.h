#pragma once

#include <components/bk_asr_service_types.h>

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus


aud_asr_handle_t bk_aud_asr_init(aud_asr_cfg_t *cfg);
bk_err_t bk_aud_asr_deinit(aud_asr_handle_t aud_asr_handle);
bk_err_t bk_aud_asr_start(aud_asr_handle_t aud_asr_handle);
bk_err_t bk_aud_asr_stop(aud_asr_handle_t aud_asr_handle);


#ifdef  __cplusplus
}
#endif//__cplusplus

/**
 * @}
 */
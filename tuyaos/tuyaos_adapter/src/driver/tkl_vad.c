/**
 * @file tkl_vad.c
 * @version 0.1
 * @date 2025-04-16
 */

#include "tkl_audio.h"
#include "tkl_memory.h"
#include "tkl_queue.h"
#include "tkl_thread.h"
#include "tkl_system.h"
#include "tkl_vad.h"

#include "tkl_output.h"

/***********************************************************
************************macro define************************
***********************************************************/
#define AEC_VAD_FRAME_SIZE (640) // 20ms 16kHz 16bit mono

/***********************************************************
***********************typedef define***********************
***********************************************************/

/***********************************************************
********************function declaration********************
***********************************************************/
extern void *speex_aes_create(int framesize);
extern void speex_aes_destory(void *obj);
extern int speex_aes_process(void *obj, short *mic, short *ref, short *aec);
extern float speex_get_param(void *obj, float *out, short *linearout);

extern void *rnn_vad_create();
extern void rnn_vad_destroy(void *obj);
extern void rnn_vad_start(void *obj);
extern void rnn_vad_stop(void *obj);
extern float rnn_vad_process(void *obj, short *x);
extern void rnn_vad_set_callback(void *obj, float threshold);
/***********************************************************
***********************variable define**********************
***********************************************************/
static void *__s_speex_aec_handle = NULL;
static void *__s_rnn_vad_handle = NULL;
static uint16_t *__s_linearaec = NULL;
static uint32_t __s_frame_size = 0;
static TKL_VAD_STATUS_T __s_aec_vad_flag = TKL_VAD_STATUS_NONE;

/***********************************************************
***********************function define**********************
***********************************************************/
static OPERATE_RET __tkl_aec_vad_process(int16_t *mic_data, int16_t *ref_data, int16_t *out_data)
{
    OPERATE_RET rt = OPRT_OK;

    if (mic_data == NULL || ref_data == NULL || out_data == NULL) {
        tkl_log_output("Invalid parameter: mic_data, ref_data, or out_data is NULL\r\n");
        return OPRT_INVALID_PARM;
    }

    if (__s_speex_aec_handle) {
        speex_aes_process(__s_speex_aec_handle, (short *)mic_data, (short *)ref_data, (short *)out_data);
    }

    if (__s_speex_aec_handle && __s_rnn_vad_handle && __s_linearaec) {
        int has_vad = (int)rnn_vad_process(__s_rnn_vad_handle, (short *)out_data);
        if (has_vad && __s_aec_vad_flag != TKL_VAD_STATUS_SPEECH) {
            // tkl_log_output("################ [vad start] ################\r\n");
            __s_aec_vad_flag = TKL_VAD_STATUS_SPEECH;
        } else if (!has_vad && __s_aec_vad_flag != TKL_VAD_STATUS_NONE) {
            // tkl_log_output("################ [vad stop] ################\r\n");
            __s_aec_vad_flag = TKL_VAD_STATUS_NONE;
        }

        speex_get_param(__s_speex_aec_handle, NULL, (short *)__s_linearaec);
    }

    return rt;
}

OPERATE_RET tkl_vad_set_threshold(TKL_AUDIO_VAD_THRESHOLD_E level)
{
    if (__s_rnn_vad_handle) {
        switch (level) {
        case TKL_AUDIO_VAD_HIGH:
            rnn_vad_set_callback(__s_rnn_vad_handle, -40);
            break;
        case TKL_AUDIO_VAD_MID:
            rnn_vad_set_callback(__s_rnn_vad_handle, -60);
            break;
        case TKL_AUDIO_VAD_LOW:
            rnn_vad_set_callback(__s_rnn_vad_handle, -80);
            break;
        default:
            break;
        }

        return OPRT_OK;
    }

    return OPRT_RESOURCE_NOT_READY;
}

OPERATE_RET tkl_vad_init(TKL_VAD_CONFIG_T *config)
{
    OPERATE_RET rt = OPRT_OK;

    if (NULL == config) {
        return OPRT_INVALID_PARM;
    }

    if (__s_speex_aec_handle == NULL) {
        __s_speex_aec_handle = speex_aes_create(AEC_VAD_FRAME_SIZE / 2);
        if (__s_speex_aec_handle == NULL) {
            tkl_log_output("__s_speex_aec_handle create failed\r\n");
            goto __err_exit;
        }
    }

    if (__s_rnn_vad_handle == NULL) {
        __s_rnn_vad_handle = rnn_vad_create();
        tkl_vad_set_threshold(TKL_AUDIO_VAD_MID);
        if (__s_rnn_vad_handle == NULL) {
            tkl_log_output("__s_rnn_vad_handle create failed\r\n");
            goto __err_exit;
        }
    }

    if (__s_linearaec == NULL) {
#ifdef ENABLE_EXT_RAM
        __s_linearaec = tkl_system_psram_malloc(AEC_VAD_FRAME_SIZE * 2);
        if (NULL == __s_linearaec) {
            tkl_log_output("__s_linearaec psram malloc failed\r\n");
            goto __err_exit;
        }
#else
        __s_linearaec = tkl_system_malloc(AEC_VAD_FRAME_SIZE * 2);
        if (NULL == __s_linearaec) {
            tkl_log_output("__s_linearaec malloc failed\r\n");
            goto __err_exit;
        }
#endif
    }

    __s_frame_size = AEC_VAD_FRAME_SIZE;
    tkl_ai_set_vad_aec_algorithm(__tkl_aec_vad_process);

    return OPRT_OK;

__err_exit:
    tkl_vad_deinit();
    return rt;
}

OPERATE_RET tkl_vad_feed(uint8_t *data, uint32_t len)
{
    // Nothing to do
    // T5 vad feed is called in _aec_v3_algorithm_process() function in
    // platform/T5AI/t5_os/ap/components/bk_audio/audio_algorithms/aec_v3_algorithm/aec_v3_algorithm.c

    return OPRT_OK;
}

TKL_VAD_STATUS_T tkl_vad_get_status(void)
{
    return __s_aec_vad_flag;
}

OPERATE_RET tkl_vad_start(void)
{
    __s_aec_vad_flag = TKL_VAD_STATUS_NONE;
    if (__s_rnn_vad_handle) {
        rnn_vad_start(__s_rnn_vad_handle);
    }

    return OPRT_OK;
}

OPERATE_RET tkl_vad_stop(void)
{
    __s_aec_vad_flag = TKL_VAD_STATUS_NONE;
    if (__s_rnn_vad_handle) {
        rnn_vad_stop(__s_rnn_vad_handle);
    }

    return OPRT_OK;
}

OPERATE_RET tkl_vad_deinit(void)
{
    tkl_ai_set_vad_aec_algorithm(NULL);

    if (__s_linearaec) {
#ifdef ENABLE_EXT_RAM
        tkl_system_psram_free(__s_linearaec);
#else
        tkl_system_free(__s_linearaec);
#endif
        __s_linearaec = NULL;
    }

    if (__s_rnn_vad_handle) {
        rnn_vad_destroy(__s_rnn_vad_handle);
        __s_rnn_vad_handle = NULL;
    }

    if (__s_speex_aec_handle) {
        speex_aes_destory(__s_speex_aec_handle);
        __s_speex_aec_handle = NULL;
    }

    __s_frame_size = 0;

    return OPRT_OK;
}
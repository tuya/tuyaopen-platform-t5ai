/*
 * tkl_audio.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */
#include "aud_intf.h"
#include "tkl_audio.h"

extern void tuya_multimedia_power_on(void);
static aud_intf_drv_setup_t aud_intf_drv_setup = DEFAULT_AUD_INTF_DRV_SETUP_CONFIG();
static aud_intf_voc_setup_t aud_voc_setup = DEFAULT_AUD_INTF_VOC_SETUP_CONFIG();
static aud_intf_work_mode_t aud_work_mode = AUD_INTF_WORK_MODE_NULL;

TKL_FRAME_PUT_CB user_audio_cb = NULL;

static int __audio_send_callback(unsigned char *data, unsigned int len)
{
    if (user_audio_cb != NULL) {
        TKL_AUDIO_FRAME_INFO_T frame;
        frame.pbuf = data;
        frame.buf_size = len;
        user_audio_cb(&frame);
    }

    return len;
}

static int __send_mic_data_to_spk(uint8_t *data, unsigned int len)
{
    bk_err_t ret = BK_OK;

    int16_t *pcm_data = (int16_t *)data;
    uint8_t *g711a_data = NULL;
    g711a_data = os_malloc(len/2);

    /* pcm -> g711a */
    for (int i = 0; i < len/2; i++) {
        g711a_data[i] = linear2alaw(pcm_data[i]);
    }

    /* g711a -> pcm */
    for (int i = 0; i< len/2; i++) {
        pcm_data[i] = alaw2linear(g711a_data[i]);
    }

    /* write a fram speaker data to speaker_ring_buff */
    ret = bk_aud_intf_write_spk_data((uint8 *)pcm_data, len);
    if (ret != BK_OK) {
        os_printf("write mic spk data fail \r\n");
        return ret;
    }

    os_free(g711a_data);
    return len;
}


static void __audio_connect_state_cb_handle(uint8_t state)
{
	os_printf("[--%s--] state: %d \n", __func__, state);
}

static int __audio_send_mic_data_to_spk(uint8_t *data, unsigned int len)
{
    bk_err_t ret = BK_OK;

    int16_t *pcm_data = (int16_t *)data;
    uint8_t *g711a_data = NULL;
    g711a_data = os_malloc(len/2);

    /* pcm -> g711a */
    for (int i = 0; i < len/2; i++) {
        g711a_data[i] = linear2alaw(pcm_data[i]);
    }

    /* g711a -> pcm */
    for (int i = 0; i< len/2; i++) {
        pcm_data[i] = alaw2linear(g711a_data[i]);
    }

    /* write a fram speaker data to speaker_ring_buff */
    ret = bk_aud_intf_write_spk_data((uint8 *)pcm_data, len);
    if (ret != BK_OK) {
        os_printf("write mic spk data fail \r\n");
        return ret;
    }

    os_free(g711a_data);
    return len;
}

/**
* @brief ai init
*
* @param[in] pconfig: audio config
* @param[in] count: count of pconfig
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_init(TKL_AUDIO_CONFIG_T *pconfig, int32_t count)
{
    int ret;

    if (pconfig == NULL)
        return OPRT_INVALID_PARM;

    tuya_multimedia_power_on();
    //aud_voc_setup.aec_enable = true;

    //aud_voc_setup.data_type = AUD_INTF_VOC_DATA_TYPE_G711A;
    aud_voc_setup.data_type = AUD_INTF_VOC_DATA_TYPE_PCM;
    aud_voc_setup.spk_mode = AUD_DAC_WORK_MODE_SIGNAL_END;
    //aud_voc_setup.mic_en = AUD_INTF_VOC_MIC_OPEN;
    //aud_voc_setup.spk_en = AUD_INTF_VOC_SPK_OPEN;

    // TODO BOARD AUDIO ACORDING TO PARAMETER
    aud_voc_setup.mic_type = AUD_INTF_MIC_TYPE_UAC;
    aud_voc_setup.spk_type = AUD_INTF_SPK_TYPE_UAC;

    // TODO sample rate set
    aud_voc_setup.samp_rate = 8000;

    aud_intf_drv_setup.aud_intf_tx_mic_data = __audio_send_callback;
    //aud_intf_drv_setup.aud_intf_tx_mic_data = __send_mic_data_to_spk;
    //aud_intf_drv_setup.aud_intf_rx_spk_data = aud_write_sd_data_to_spk;
    ret = bk_aud_intf_drv_init(&aud_intf_drv_setup);
    if (ret != BK_OK) {
        bk_printf("bk_aud_intf_drv_init fail, ret:%d\n", ret);
        goto error;
    }

    aud_work_mode = AUD_INTF_WORK_MODE_VOICE;
    ret = bk_aud_intf_set_mode(aud_work_mode);
    if (ret != BK_OK) {
        bk_printf("bk_aud_intf_set_mode fail, ret:%d\n", ret);
        goto error;
    }

    /* uac recover connection */
    if (aud_voc_setup.mic_type == AUD_INTF_MIC_TYPE_UAC) {
        ret = bk_aud_intf_register_uac_connect_state_cb(__audio_connect_state_cb_handle);
        if (ret != BK_OK) {
            bk_printf("bk_aud_intf_register_uac_connect_state_cb fail, ret:%d\n", ret);
            goto error;
        }

        ret = bk_aud_intf_uac_auto_connect_ctrl(true);
        if (ret != BK_OK) {
            bk_printf("aud_tras_uac_auto_connect_ctrl fail, ret:%d\n", ret);
            goto error;
        }
    }

    ret = bk_aud_intf_voc_init(aud_voc_setup);
    if (ret != BK_OK) {
        bk_printf("bk_aud_intf_voc_init fail, ret:%d\n", ret);
        goto error;
    }

    if (pconfig->put_cb != NULL) {
        // delay 500ms, 不上报开始500ms的数据，初始化阶段声音可能存在失真杂音
        tkl_system_sleep(500);
        user_audio_cb = pconfig->put_cb;
    }

    return OPRT_OK;

error:
    bk_aud_intf_voc_deinit();
    aud_work_mode = AUD_INTF_WORK_MODE_NULL;
    bk_aud_intf_set_mode(AUD_INTF_WORK_MODE_NULL);
    bk_aud_intf_drv_deinit();
    return OPRT_COM_ERROR;
}

/**
* @brief ai start
*
* @param[in] card: card number
* @param[in] chn: channel number
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_start(int32_t card, TKL_AI_CHN_E chn)
{
    int ret;
    if(aud_work_mode == AUD_INTF_WORK_MODE_NULL) {
        bk_printf("bk_aud_intf_voc_start fail, not init ,aud_work_mode:%d\n", aud_work_mode);
        return OPRT_COM_ERROR;
    }
    ret = bk_aud_intf_voc_start();
    if (ret != BK_OK) {
        bk_printf("bk_aud_intf_voc_start fail, ret:%d\n", ret);
        return OPRT_COM_ERROR;
    }
    return OPRT_OK;
}


/**
* @brief ai set mic volume
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[in] vol: mic volume,[0, 100]
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_set_vol(int32_t card, TKL_AI_CHN_E chn, int32_t vol)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief ai get frame
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[out] pframe: audio frame, pframe->pbuf allocated by upper layer application
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_get_frame(int32_t card, TKL_AI_CHN_E chn, TKL_AUDIO_FRAME_INFO_T *pframe)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief ai set vqe param
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[in] type: vqe type
* @param[in] pparam: vqe param
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_set_vqe(int32_t card, TKL_AI_CHN_E chn, TKL_AUDIO_VQE_TYPE_E type, TKL_AUDIO_VQE_PARAM_T *pparam)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief ai get vqe param
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[in] type: vqe type
* @param[out] pparam: vqe param
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_get_vqe(int32_t card, TKL_AI_CHN_E chn, TKL_AUDIO_VQE_TYPE_E type, TKL_AUDIO_VQE_PARAM_T *pparam)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief ai stop
*
* @param[in] card: card number
* @param[in] chn: channel number
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_stop(int32_t card, TKL_AI_CHN_E chn)
{
    int ret;
    if(aud_work_mode == AUD_INTF_WORK_MODE_NULL) {
        bk_printf("bk_aud_intf_voc_start fail, not init ,aud_work_mode:%d\n", aud_work_mode);
        return OPRT_COM_ERROR;
    }
    ret = bk_aud_intf_voc_stop();
    if (ret != BK_OK) {
        bk_printf("bk_aud_intf_voc_start fail, ret:%d\n", ret);
        return OPRT_COM_ERROR;
    }
    return OPRT_OK;
}


/**
* @brief ai uninit
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_uninit(void)
{
    bk_aud_intf_voc_deinit();
    aud_work_mode = AUD_INTF_WORK_MODE_NULL;
    bk_aud_intf_set_mode(AUD_INTF_WORK_MODE_NULL);
    bk_aud_intf_drv_deinit();
    bk_printf("------ [%s %d]\r\n", __func__, __LINE__);
    return OPRT_OK;
}


/**
* @brief ao init
*
* @param[in] pconfig: audio config
* @param[in] count: config count
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ao_init(TKL_AUDIO_CONFIG_T *pconfig, int32_t count, void **handle)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief ao start
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[out] handle: handle of start
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ao_start(int32_t card, TKL_AO_CHN_E chn, void *handle)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief ao set volume
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[in] vol: mic volume,[0, 100]
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ao_set_vol(int32_t card, TKL_AO_CHN_E chn, void *handle, int32_t vol)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief ao get volume
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[in] vol: mic volume,[0, 100]
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ao_get_vol(int32_t card, TKL_AO_CHN_E chn, void *handle, int32_t *vol)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief ao output frame
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[in] handle: handle of start
* @param[in] pframe: output frame
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ao_put_frame(int32_t card, TKL_AO_CHN_E chn, void *handle, TKL_AUDIO_FRAME_INFO_T *pframe)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief ao stop
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[in] handle: handle of start
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ao_stop(int32_t card, TKL_AO_CHN_E chn, void *handle)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief ao uninit
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ao_uninit(void *handle)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief audio input detect start
*
* @param[in] card: card number
* @param[in] type: detect type
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_detect_start(int32_t card, TKL_MEDIA_DETECT_TYPE_E type)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief audio input detect stop
*
* @param[in] card: card number
* @param[in] type: detect type
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_detect_stop(int32_t card, TKL_MEDIA_DETECT_TYPE_E type)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief audio detect get result
*
* @param[in] card: card number
* @param[in] type: detect type
* @param[out] presult: audio detect result
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_detect_get_result(int32_t card, TKL_MEDIA_DETECT_TYPE_E type, TKL_AUDIO_DETECT_RESULT_T *presult)
{
    return OPRT_NOT_SUPPORTED;
}




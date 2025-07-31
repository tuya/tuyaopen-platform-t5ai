/*
 * tkl_audio.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */
#include "tkl_audio.h"
#include "tkl_gpio.h"
#include "tkl_semaphore.h"
#include "tkl_system.h"
#ifdef CONFIG_VOICE_SERVICE
#include "driver/aud_adc.h"
#include "driver/aud_dac.h"
#include <components/bk_voice_read_service.h>
#include <components/bk_voice_read_service_types.h>
#include <components/bk_voice_service.h>
#include <components/bk_voice_service_types.h>
#include <components/bk_voice_write_service.h>
#include <components/bk_voice_write_service_types.h>

// 驱动内部使用的是4
#define DRIVER_SPEAK_FIFO_FRAME_NUM 4
#define MAX_SEM_AUDIO            (8)
#define CHANNEL_NUM              (2)
#define TIME_SAMPLE_MS           (20)
#define MS_PER_SEC               (1000)

extern void tuya_multimedia_power_on(void);

static voice_handle_t g_voice_handle = NULL;
static voice_read_handle_t g_voice_read_handle = NULL;
typedef struct 
{
    UINT32_T mic_gain;
    UINT32_T spk_gain;
}TKL_AUDIO_GAIN_T;

static TKL_AUDIO_GAIN_T g_audio_gain = {0};
static TKL_FRAME_PUT_CB user_mic_cb = NULL;
static TKL_FRAME_SPK_CB user_spk_cb = NULL;
static INT32_T board_spk_gpio;
static INT32_T board_spk_gpio_polarity;


extern void *tkl_system_psram_malloc(size_t size);
extern void tkl_system_psram_free(void *ptr);

typedef struct {
    bool audio_init;
    bool audio_start;
    bool audio_status;
} AUIDO_INIT_T;

typedef struct {
    TKL_AUDIO_TYPE_E card;
    UINT32_T samp_rate;
    UINT_T aec_enable;
} AUIDO_MIC_T;

static AUIDO_MIC_T s_audio_mic = {0};
static AUIDO_INIT_T s_audio_init = {0};

int voice_read_callback(unsigned char *data, unsigned int len, void *args)
{
    TKL_AUDIO_FRAME_INFO_T pframe = {0};
    pframe.buf_size = len;
    pframe.used_size = len;
    pframe.pbuf = data;
    if (user_mic_cb)
    {
        user_mic_cb(&pframe);
    }
    return len;
}

int tkl_ai_status(void) 
{
    int status = 0;
    bk_voice_get_status(g_voice_handle, (voice_sta_t*)&status); 
    bk_printf("cur status %d\n",status);
    return status;
}
/**
 * @brief ai init
 *
 * @param[in] pconfig: audio config
 * @param[in] count: count of pconfig
 *
 * @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
 */
OPERATE_RET tkl_ai_init(TKL_AUDIO_CONFIG_T *pconfig, INT32_T count)
{
    UINT_T level = 0;
    UINT32_T sample_rate = TKL_AUDIO_SAMPLE_8K;
    uac_mic_stream_cfg_t uac_mic_cfg = UAC_MIC_STREAM_CFG_DEFAULT();
    onboard_mic_stream_cfg_t onboard_mic_cfg = ONBOARD_MIC_ADC_STREAM_CFG_DEFAULT();
    voice_cfg_t voice_cfg = {0};
    os_memset(&voice_cfg, 0, sizeof(voice_cfg_t));
    bk_printf("audio trace: %s %d\r\n", __func__, __LINE__);
    if (pconfig == NULL)
        return OPRT_INVALID_PARM;
    switch (pconfig->sample) {
        case TKL_AUDIO_SAMPLE_8K:
            sample_rate = TKL_AUDIO_SAMPLE_8K;
            break;

        case TKL_AUDIO_SAMPLE_16K:
            sample_rate = TKL_AUDIO_SAMPLE_16K;
            break;

        default:
            sample_rate = TKL_AUDIO_SAMPLE_8K;
            break;
    }
    if (pconfig->card == TKL_AUDIO_TYPE_UAC) {
        voice_cfg.mic_type = MIC_TYPE_UAC;
        uac_mic_cfg.bits = pconfig->datebits;
        uac_mic_cfg.samp_rate = sample_rate;
        /* one farme size, 20ms */
        uac_mic_cfg.frame_size =
            sample_rate * CHANNEL_NUM * TIME_SAMPLE_MS / MS_PER_SEC; // one frame size(20ms)
        uac_mic_cfg.out_block_size = uac_mic_cfg.frame_size;
        uac_mic_cfg.out_block_num = 2;
        memcpy(&voice_cfg.mic_cfg.uac_mic_cfg, &uac_mic_cfg,
               sizeof(uac_mic_stream_cfg_t));
        bk_printf("audio trace: %s %d\r\n", __func__, __LINE__);
    } else {
        voice_cfg.mic_type = MIC_TYPE_ONBOARD;
        if (pconfig->mic_volume)
        {
            onboard_mic_cfg.adc_cfg.dig_gain = (uint32_t)(pconfig->mic_volume * 0x3F / 100);
        }
        onboard_mic_cfg.adc_cfg.bits = pconfig->datebits;
        onboard_mic_cfg.adc_cfg.sample_rate = sample_rate;
        /* one farme size, 20ms */
        onboard_mic_cfg.frame_size =
            sample_rate * CHANNEL_NUM * TIME_SAMPLE_MS / MS_PER_SEC; // one frame size(20ms)
        onboard_mic_cfg.out_block_size = uac_mic_cfg.frame_size;

        memcpy(&voice_cfg.mic_cfg.onboard_mic_cfg, &onboard_mic_cfg,
               sizeof(onboard_mic_stream_cfg_t));
        bk_printf("audio trace: %s %d\r\n", __func__, __LINE__);
    }
    if (pconfig->enable) {
        if ((pconfig->sample != TKL_AUDIO_SAMPLE_8K) && (pconfig->sample != TKL_AUDIO_SAMPLE_16K)) {
            bk_printf("voice only support 8k/16k sample, set value %d\r\n",
                      pconfig->sample);
            return -1;
        }
    }

    if ((pconfig->spk_sample == 0) || (pconfig->enable)) {
        aec_algorithm_cfg_t aec_alg_cfg = DEFAULT_AEC_ALGORITHM_CONFIG();
        // enable置1,表示语音对讲，此时因AEC需要，mic跟spk的
        // 采样率需要相等
        voice_cfg.aec_en = true;
        aec_alg_cfg.out_block_num = 1;
        aec_alg_cfg.aec_cfg.fs = sample_rate;
        voice_cfg.aec_cfg.aec_alg_cfg = aec_alg_cfg;
    } else {
        // 否则直接使用用户传入的采样率
        sample_rate = pconfig->spk_sample? pconfig->spk_sample : TKL_AUDIO_SAMPLE_8K;
        voice_cfg.aec_en = false;
        voice_cfg.aec_cfg.reserve = 0;
    }
    switch (pconfig->codectype) {
        case TKL_CODEC_AUDIO_G711A:
        case TKL_CODEC_AUDIO_G711U: {
            /* g711 encoder config */
            g711_encoder_cfg_t g711_encoder_cfg = DEFAULT_G711_ENCODER_CONFIG();
            voice_cfg.enc_cfg.g711_enc_cfg = g711_encoder_cfg;
            if (pconfig->codectype == TKL_CODEC_AUDIO_G711A) {
                voice_cfg.enc_type = AUDIO_ENC_TYPE_G711A;
                voice_cfg.enc_cfg.g711_enc_cfg.enc_mode = G711_ENC_MODE_A_LOW;
            } else {
                voice_cfg.enc_type = AUDIO_ENC_TYPE_G711U;
                voice_cfg.enc_cfg.g711_enc_cfg.enc_mode = G711_ENC_MODE_U_LOW;
            }
            voice_cfg.enc_cfg.g711_enc_cfg.buf_sz =
                sample_rate * CHANNEL_NUM * TIME_SAMPLE_MS / MS_PER_SEC; // one frame size(20ms)
            voice_cfg.enc_cfg.g711_enc_cfg.out_block_size =
                voice_cfg.enc_cfg.g711_enc_cfg.buf_sz >> 1;
            /* config raw_read input buffer */
            voice_cfg.read_pool_size =
                voice_cfg.enc_cfg.g711_enc_cfg.out_block_size;

            /* g711 decoder config */
            g711_decoder_cfg_t g711_decoder_cfg = DEFAULT_G711_DECODER_CONFIG();
            voice_cfg.dec_cfg.g711_dec_cfg = g711_decoder_cfg;
            if (pconfig->codectype == TKL_CODEC_AUDIO_G711A) {
                voice_cfg.dec_type = AUDIO_DEC_TYPE_G711A;
                voice_cfg.dec_cfg.g711_dec_cfg.dec_mode = G711_DEC_MODE_A_LOW;
            } else {
                voice_cfg.dec_type = AUDIO_DEC_TYPE_G711U;
                voice_cfg.dec_cfg.g711_dec_cfg.dec_mode = G711_DEC_MODE_U_LOW;
            }
            voice_cfg.dec_cfg.g711_dec_cfg.out_block_size =
                sample_rate * CHANNEL_NUM * TIME_SAMPLE_MS / MS_PER_SEC; // one frame size(20ms)
            voice_cfg.dec_cfg.g711_dec_cfg.buf_sz =
                voice_cfg.dec_cfg.g711_dec_cfg.out_block_size >> 1;
            /* config raw_write output buffer */
            voice_cfg.write_pool_size = voice_cfg.dec_cfg.g711_dec_cfg.buf_sz;
        } break;

        case TKL_CODEC_AUDIO_PCM: {
            /* pcm encoder config */
            voice_cfg.enc_type = AUDIO_ENC_TYPE_PCM;
            voice_cfg.enc_cfg.pcm_enc_cfg = 0; // not used
            voice_cfg.dec_type = AUDIO_DEC_TYPE_PCM;
            voice_cfg.dec_cfg.pcm_dec_cfg = 0; // not used

            /* config raw_read input buffer and raw_write output buffer */
            voice_cfg.read_pool_size =
                sample_rate * CHANNEL_NUM * TIME_SAMPLE_MS / MS_PER_SEC; // one frame size(20ms)
            voice_cfg.write_pool_size =
                sample_rate * CHANNEL_NUM * TIME_SAMPLE_MS / MS_PER_SEC; // one frame size(20ms)
        } break;

        default: {
            bk_printf("not support encoder format\n");
            goto error;
        } break;
    }

    voice_cfg.spk_type = SPK_TYPE_ONBOARD;
    if (voice_cfg.spk_type == SPK_TYPE_ONBOARD) {
        onboard_speaker_stream_cfg_t onboard_spk_cfg =
            ONBOARD_SPEAKER_STREAM_CFG_DEFAULT();
        if (pconfig->spk_volume)
        {
            uint32_t new_vol;
            if (pconfig->spk_volume <= 30) {
                // 0-30% 映射到 0-50%
                pconfig->spk_volume = pconfig->spk_volume * 50 / 30;
            } else {
                // 30-100% 映射到 50-70%
                pconfig->spk_volume = 50 + (pconfig->spk_volume - 30) * 20 / 70;
            }
            pconfig->spk_volume = (uint32_t)(pconfig->spk_volume * 0x3F / 100) ;
            onboard_spk_cfg.dig_gain = (uint32_t)pconfig->spk_volume;
        }
        onboard_spk_cfg.bits = pconfig->datebits;
        onboard_spk_cfg.sample_rate = sample_rate;
        /* one farme size, 20ms */
        onboard_spk_cfg.frame_size =
            sample_rate * CHANNEL_NUM * TIME_SAMPLE_MS / MS_PER_SEC; // one frame size(20ms)

        voice_cfg.spk_cfg.onboard_spk_cfg = onboard_spk_cfg;
        if (voice_cfg.aec_en) {
            onboard_spk_cfg.multi_out_port_num = 1;
        } else {
            onboard_spk_cfg.multi_out_port_num = 0;
        }
        voice_cfg.spk_cfg.onboard_spk_cfg = onboard_spk_cfg;
    } else {
        uac_speaker_stream_cfg_t uac_spk_cfg = UAC_SPEAKER_STREAM_CFG_DEFAULT();
        if (pconfig->spk_volume)
        {
            uac_spk_cfg.volume = pconfig->spk_volume;
        }

        uac_spk_cfg.bits = pconfig->datebits;
        uac_spk_cfg.samp_rate = sample_rate;
        /* one farme size, 20ms */
        uac_spk_cfg.frame_size = sample_rate * CHANNEL_NUM * TIME_SAMPLE_MS / MS_PER_SEC;
        if (voice_cfg.aec_en) {
            uac_spk_cfg.multi_out_port_num = 1;
        } else {
            uac_spk_cfg.multi_out_port_num = 0;
        }
        voice_cfg.spk_cfg.uac_spk_cfg = uac_spk_cfg;
    }

    voice_cfg.event_handle = NULL;
    voice_cfg.args = NULL;

    g_voice_handle = bk_voice_init(&voice_cfg);
    if (g_voice_handle == NULL) {
        bk_printf("bk_voice_init fail\n");
        goto error;
    }

    if (pconfig->spk_gpio < 56) {
        TUYA_GPIO_BASE_CFG_T cfg;
        cfg.direct = TUYA_GPIO_OUTPUT;
        if (pconfig->spk_gpio_polarity == 0) {
            cfg.mode = TUYA_GPIO_PULLUP;
            cfg.level = TUYA_GPIO_LEVEL_HIGH;
        } else if (pconfig->spk_gpio_polarity == 1) {
            cfg.mode = TUYA_GPIO_PULLDOWN;
            cfg.level = TUYA_GPIO_LEVEL_LOW;
        }
        board_spk_gpio = pconfig->spk_gpio;
        board_spk_gpio_polarity = pconfig->spk_gpio_polarity;
        tkl_gpio_init(pconfig->spk_gpio, &cfg);

    }

    if (pconfig->put_cb != NULL) {
        // delay 500ms, 不上报开始500ms的数据，初始化阶段声音可能存在失真杂音
        // tkl_system_sleep(500);
        user_mic_cb = pconfig->put_cb;
        voice_read_cfg_t voice_read_cfg = VOICE_READ_CFG_DEFAULT();
        voice_read_cfg.voice_handle = g_voice_handle;
        voice_read_cfg.max_read_size = sample_rate * CHANNEL_NUM * TIME_SAMPLE_MS / MS_PER_SEC; //one frame size(20ms)
        voice_read_cfg.voice_read_callback = voice_read_callback;
        voice_read_cfg.args = NULL;
        voice_read_cfg.mem_type = AUDIO_MEM_TYPE_PSRAM;
        g_voice_read_handle = bk_voice_read_init(&voice_read_cfg);
        if (!g_voice_read_handle)
        {
            bk_printf("%s, %d, voice read init fail\n", __func__, __LINE__);
            goto error;
        }
    }
    if (pconfig->spk_cb != NULL)
    {
        bk_printf("%s, %d, TODO voice spk cb fail\n", __func__, __LINE__);
    }

    if (BK_OK != bk_voice_start(g_voice_handle)) {
        bk_printf("%s, %d, voice start fail\n", __func__, __LINE__);
        goto error;
    }
    //执行时间短，使用自旋锁避免调度提高效率
    level = tkl_system_enter_critical();
    s_audio_mic.aec_enable = pconfig->enable;
    s_audio_mic.samp_rate = sample_rate;
    s_audio_mic.card = pconfig->card;
    s_audio_init.audio_status = true;
    s_audio_init.audio_init = true;
    s_audio_init.audio_start = true;
    tkl_system_exit_critical(level);
    return OPRT_OK;

error:
    if (g_voice_handle) {
        bk_voice_stop(g_voice_handle);
    }

    if (g_voice_read_handle) {
        bk_voice_read_deinit(g_voice_read_handle);
    }

    if (g_voice_handle) {
        bk_voice_deinit(g_voice_handle);
    }

    level = tkl_system_enter_critical();
    g_voice_handle = NULL;

    s_audio_mic.card = TKL_AUDIO_TYPE_UAC;
    s_audio_mic.aec_enable = 0;
    s_audio_mic.samp_rate = 0;
    s_audio_init.audio_status = false;
    s_audio_init.audio_init = false;
    tkl_system_exit_critical(level);
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
OPERATE_RET tkl_ai_start(INT32_T card, TKL_AI_CHN_E chn)
{
    UINT32_T level = 0;
    if (tkl_ai_status() == VOICE_STA_NONE) {
        bk_printf("tkl_ai_start fail, not init\n");
        return OPRT_COM_ERROR;
    }
    if (BK_OK != bk_voice_start(g_voice_handle)) {
        bk_printf("%s, %d, voice read start fail\n", __func__, __LINE__);
    }
    if (g_voice_read_handle)
    {
        if (BK_OK != bk_voice_read_start(g_voice_read_handle)) {
            bk_printf("%s, %d, voice read start fail\n", __func__, __LINE__);
        }
    }
    level = tkl_system_enter_critical();
    s_audio_init.audio_start = true;
    tkl_system_exit_critical(level);

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
OPERATE_RET tkl_ai_set_vol(INT32_T card, TKL_AI_CHN_E chn, INT32_T vol)
{
    if (!s_audio_init.audio_init || !s_audio_init.audio_start) {
        return OPRT_RESOURCE_NOT_READY;
    }

    g_audio_gain.mic_gain = vol;
    uint32_t volume = 0;
    volume =(uint32_t)(vol * 0x3F / 100);

    return bk_aud_adc_set_gain(volume);
}

/**
 * @brief ai get frame
 *
 * @param[in] card: card number
 * @param[in] chn: channel number
 * @param[out] pframe: audio frame, pframe->pbuf allocated by upper layer
 * application
 *
 * @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
 */
OPERATE_RET tkl_ai_get_frame(INT32_T card, TKL_AI_CHN_E chn,
                             TKL_AUDIO_FRAME_INFO_T *pframe)
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
OPERATE_RET tkl_ai_set_vqe(INT32_T card, TKL_AI_CHN_E chn,
                           TKL_AUDIO_VQE_TYPE_E type,
                           TKL_AUDIO_VQE_PARAM_T *pparam)
{
    OPERATE_RET ret = OPRT_NOT_SUPPORTED;

    return ret;
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
OPERATE_RET tkl_ai_get_vqe(INT32_T card, TKL_AI_CHN_E chn,
                           TKL_AUDIO_VQE_TYPE_E type,
                           TKL_AUDIO_VQE_PARAM_T *pparam)
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
OPERATE_RET tkl_ai_stop(INT32_T card, TKL_AI_CHN_E chn)
{
    int ret;
    UINT32_T level = 0;
    if (!s_audio_init.audio_init || !s_audio_init.audio_start) {
        return OPRT_OK;
    }
    if (tkl_ai_status() == VOICE_STA_NONE) {
        bk_printf("tkl_ai_start fail, not init\n");
        return OPRT_COM_ERROR;
    }
    if (g_voice_read_handle)
    {
        if (BK_OK != bk_voice_read_stop(g_voice_read_handle)) {
            bk_printf("%s, %d, voice read start fail\n", __func__, __LINE__);
        }
    }

    ret = bk_voice_stop(g_voice_handle);
    if (ret != BK_OK) {
        bk_printf("bk_voice_read_stop fail, ret:%d\n", ret);
        return OPRT_COM_ERROR;
    }
    level = tkl_system_enter_critical();
    s_audio_init.audio_start = false;
    tkl_system_exit_critical(level);
    return OPRT_OK;
}

/**
 * @brief ai uninit
 *
 * @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
 */
OPERATE_RET tkl_ai_uninit(VOID)
{
    int ret = OPRT_OK;
    UINT_T level = 0;

    if (!s_audio_init.audio_init) {
        return OPRT_OK;
    }

    if (g_voice_handle) {
        bk_voice_stop(g_voice_handle);
    }

    if (g_voice_handle) {
        bk_voice_deinit(g_voice_handle);
    }

    level = tkl_system_enter_critical();
    g_voice_handle = NULL;

    s_audio_mic.card = TKL_AUDIO_TYPE_UAC;
    s_audio_mic.aec_enable = 0;
    s_audio_mic.samp_rate = 0;
    s_audio_init.audio_status = false;
    s_audio_init.audio_init = false;
    tkl_system_exit_critical(level);
    return ret;
}

/**
 * @param[in] count: config count
 *
 * @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
 */
OPERATE_RET tkl_ao_init(TKL_AUDIO_CONFIG_T *pconfig, INT32_T count,
                        VOID **handle)
{
    int ret = OPRT_NOT_SUPPORTED;
    if (s_audio_init.audio_init) {
        *handle = (void *)1;
        return OPRT_OK;
    }

    return ret;
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
OPERATE_RET tkl_ao_start(INT32_T card, TKL_AO_CHN_E chn, VOID *handle)
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
OPERATE_RET tkl_ao_set_vol(INT32_T card, TKL_AO_CHN_E chn, VOID *handle,
                           INT32_T vol)
{
    if (!s_audio_init.audio_init || !s_audio_init.audio_start) {
        return OPRT_RESOURCE_NOT_READY;
    }
    g_audio_gain.spk_gain = vol;
    uint32_t volume = 0;
    if (card == TKL_AUDIO_TYPE_BOARD) {
        // 重新映射音量范围
        uint32_t new_vol;
        if (vol <= 30) {
            // 0-30% 映射到 0-50%
            vol = vol * 50 / 30;
        } else {
            // 30-100% 映射到 50-70%
            vol = 50 + (vol - 30) * 20 / 70;
        }
        volume =(uint32_t)(vol * 0x3F / 100) ;
        if (volume == 0)
        {
            bk_aud_dac_mute();
        }
        else
        {
            bk_aud_dac_unmute();
        }
    }
    else {
        volume = (vol);
    }

    return bk_aud_dac_set_gain(volume);
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
OPERATE_RET tkl_ao_get_vol(INT32_T card, TKL_AO_CHN_E chn, VOID *handle,
                           INT32_T *vol)
{
    if (!s_audio_init.audio_init || !s_audio_init.audio_start) {
        return OPRT_RESOURCE_NOT_READY;
    }
    *vol = g_audio_gain.spk_gain;
    return OPRT_OK;
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
OPERATE_RET tkl_ao_put_frame(INT32_T card, TKL_AO_CHN_E chn, VOID *handle,
                             TKL_AUDIO_FRAME_INFO_T *pframe)
{
    OPERATE_RET ret;
    if (pframe == NULL) {
        return OPRT_INVALID_PARM;
    }
    if (!s_audio_init.audio_init || !s_audio_init.audio_start) {
        return OPRT_RESOURCE_NOT_READY;
    }
    ret = bk_voice_write_spk_data(
        g_voice_handle, (char *)(pframe->pbuf),
        pframe->used_size);
    if (ret == BK_ERR_BUSY) {
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief clear speaker buffer
 *
 * @param[in] card: card number
 * @param[in] chn: channel number
 *
 * @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
 */
OPERATE_RET tkl_ao_clear_buffer(INT32_T card, TKL_AO_CHN_E chn)
{
    return OPRT_OK;
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
OPERATE_RET tkl_ao_stop(INT32_T card, TKL_AO_CHN_E chn, VOID *handle)
{
    UINT32_T level = 0;
    int ret = OPRT_NOT_SUPPORTED;
    if (!s_audio_init.audio_start) {
        return OPRT_OK;
    }

    if (tkl_ai_status() == VOICE_STA_NONE) {
        bk_printf("tkl_ai_start fail, not init\n");
        return OPRT_COM_ERROR;
    }

    if (card != TKL_AUDIO_TYPE_BOARD) {
        if (s_audio_init.audio_start) {
            ret = bk_voice_stop(g_voice_handle);
            if (ret != BK_OK) {
                os_printf("audio intf spk stop fail, ret:%d \r\n", ret);
                return ret;
            }
            level = tkl_system_enter_critical();
            s_audio_init.audio_start = false;
            tkl_system_exit_critical(level);
        }
        ret = OPRT_OK;
    }
    return ret;
}

/**
 * @brief ao uninit
 *
 * @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
 */
OPERATE_RET tkl_ao_uninit(VOID *handle)
{
    UINT32_T level = 0;
    if (!s_audio_init.audio_init) {
        return OPRT_COM_ERROR;
    }
    int ret = OPRT_OK;
    if (s_audio_mic.card == TKL_AUDIO_TYPE_BOARD) {
        // ret = bk_aud_intf_mic_deinit();
        ret = bk_voice_deinit(g_voice_handle);
        if (ret != BK_OK) {
            os_printf("bk_aud_intf_spk_deinit fail, ret:%d \r\n", ret);
            return ret;
        }
    }
    level = tkl_system_enter_critical();
    s_audio_init.audio_init = false;
    s_audio_init.audio_start = false;
    tkl_system_exit_critical(level);
    return ret;
}

/**
 * @brief audio input detect start
 *
 * @param[in] card: card number
 * @param[in] type: detect type
 *
 * @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
 */
OPERATE_RET tkl_ai_detect_start(INT32_T card, TKL_MEDIA_DETECT_TYPE_E type)
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
OPERATE_RET tkl_ai_detect_stop(INT32_T card, TKL_MEDIA_DETECT_TYPE_E type)
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
OPERATE_RET tkl_ai_detect_get_result(INT32_T card, TKL_MEDIA_DETECT_TYPE_E type,
                                     TKL_AUDIO_DETECT_RESULT_T *presult)
{
    return OPRT_NOT_SUPPORTED;
}
#endif

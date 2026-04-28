#include "tkl_mic.h"
#include "sys_driver.h"
#include "tuya_cloud_types.h"
#include "tuya_error_code.h"
#ifdef CONFIG_ADK_ONBOARD_MIC_STREAM
#include "components/bk_audio/audio_pipeline/audio_pipeline.h"
#include "components/bk_audio/audio_streams/onboard_mic_stream.h"
#include "components/bk_audio/audio_streams/raw_stream.h"
#include <components/bk_audio/audio_encoders/g711_encoder.h>
#include <components/bk_audio/audio_streams/uac_mic_stream.h>
#else
#include "audio_record.h"
#endif

#define CHANNEL_NUM (2)
#define TIME_SAMPLE_MS (20)
#define MS_PER_SEC (1000)
#define DEFAULT_SAMPLE_RATE 16000
#define DEFAULT_FRAME_SIZE 640
#define DEFAULT_POOL_SIZE (DEFAULT_FRAME_SIZE * 2)

static uint32_t m_mic_gain = 0;
#ifdef CONFIG_ADK_ONBOARD_MIC_STREAM
static audio_element_handle_t m_mic_encode = NULL;
static audio_pipeline_handle_t m_mic_pipeline = NULL;
static audio_element_handle_t m_mic_stream = NULL;
static audio_element_handle_t m_raw_mic_stream = NULL;
#else
static audio_record_t *m_audio_record = NULL;
#endif
extern void bk_printf(const char *fmt, ...);

/**
 * @brief tuya kernel mic init
 *
 * TODO
 *
 * @return OPRT_OK on success. Others on error, please refer to
 * tuya_error_code.h
 */
OPERATE_RET tkl_mic_init(TKL_MIC_CFG_T *config)
{
    if (config == NULL) {
        return OPRT_INVALID_PARM;
    }

    uint32_t sample_rate =
        config->sample_rate ? config->sample_rate : DEFAULT_SAMPLE_RATE;
    uint32_t frame_size =
        sample_rate * CHANNEL_NUM * TIME_SAMPLE_MS / MS_PER_SEC;
    ;
#ifdef CONFIG_ADK_ONBOARD_MIC_STREAM
    raw_stream_cfg_t raw_read_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_read_cfg.type = AUDIO_STREAM_READER;
    m_raw_mic_stream = raw_stream_init(&raw_read_cfg);
    if (m_raw_mic_stream == NULL) {
        bk_printf("init raw stream fail\r\n");
        return OPRT_COM_ERROR;
    }

    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    m_mic_pipeline = audio_pipeline_init(&pipeline_cfg);
    if (m_mic_pipeline == NULL) {
        bk_printf("init mic stream fail\r\n");
        goto err;
    }

    switch (config->codectype) {
        case TKL_CODEC_MIC_G711A:
        case TKL_CODEC_MIC_G711U: {
            /* g711 encoder config */
            g711_encoder_cfg_t g711_encoder_cfg = DEFAULT_G711_ENCODER_CONFIG();
            if (config->codectype == TKL_CODEC_MIC_G711A) {
                g711_encoder_cfg.enc_mode = G711_ENC_MODE_A_LOW;
            } else {
                g711_encoder_cfg.enc_mode = G711_ENC_MODE_U_LOW;
            }
            g711_encoder_cfg.buf_sz = sample_rate * CHANNEL_NUM *
                                      TIME_SAMPLE_MS /
                                      MS_PER_SEC; // one frame size(20ms)
            g711_encoder_cfg.out_block_size = g711_encoder_cfg.buf_sz >> 1;
            m_mic_encode = g711_encoder_init(&g711_encoder_cfg);
            break;
        }

        default:
            break;
    }
    if (config->card == TKL_MIC_TYPE_BOARD) {
        onboard_mic_stream_cfg_t onboard_mic_cfg =
            ONBOARD_MIC_ADC_STREAM_CFG_DEFAULT();
        onboard_mic_cfg.adc_cfg.chl_num = config->chl_num;
        onboard_mic_cfg.adc_cfg.sample_rate = sample_rate;
        onboard_mic_cfg.adc_cfg.dig_gain = config->volume;
        onboard_mic_cfg.adc_cfg.bits = config->datebits;
        onboard_mic_cfg.frame_size = frame_size;
        onboard_mic_cfg.out_block_size = frame_size;
        onboard_mic_cfg.out_block_num = 2;
        m_mic_stream = onboard_mic_stream_init(&onboard_mic_cfg);
        if (m_mic_stream == NULL) {
            bk_printf("init mic stream fail\r\n");
            goto err;
        }

    }
#ifdef CONFIG_ADK_UAC_MIC_STREAM
    else {
        uac_mic_stream_cfg_t uac_mic_cfg = UAC_MIC_STREAM_CFG_DEFAULT();
        uac_mic_cfg.samp_rate = sample_rate;
        /* one farme size, 20ms */
        uac_mic_cfg.frame_size = frame_size;
        uac_mic_cfg.out_block_size = frame_size;
        uac_mic_cfg.out_block_num = 2;
        m_mic_stream = uac_mic_stream_init(&uac_mic_cfg);
        if (m_mic_stream == NULL) {
            bk_printf("init mic stream fail\r\n");
            goto err;
        }
    }
#endif
    if (BK_OK !=
        audio_pipeline_register(m_mic_pipeline, m_raw_mic_stream, "raw_mic")) {
        bk_printf("register element fail, %d \n", __LINE__);
        goto err;
    }

    if (BK_OK != audio_pipeline_register(m_mic_pipeline, m_mic_stream, "mic")) {
        bk_printf("register element fail, %d \n", __LINE__);
        goto err;
    }

    switch (config->codectype) {
        case TKL_CODEC_MIC_G711A:
        case TKL_CODEC_MIC_G711U: {
            audio_pipeline_register(m_mic_pipeline, m_mic_encode, "encode");
            if (BK_OK != audio_pipeline_link(
                             m_mic_pipeline,
                             (const char *[]){"mic", "encode", "raw_mic"}, 3)) {
                bk_printf("m_mic_pipeline link fail, %d \n", __LINE__);
                goto err;
            }
        } break;
        default: {
            if (BK_OK != audio_pipeline_link(m_mic_pipeline,
                                             (const char *[]){"mic", "raw_mic"},
                                             2)) {
                bk_printf("m_mic_pipeline link fail, %d \n", __LINE__);
                goto err;
            }
        } break;
    }

    if (BK_OK != audio_pipeline_run(m_mic_pipeline)) {
        bk_printf("m_mic_pipeline run fail, %d \n", __LINE__);
        goto err;
    }
    return OPRT_OK;
err:
    if (BK_OK != audio_pipeline_stop(m_mic_pipeline)) {
        bk_printf("m_mic_pipeline stop fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_pipeline_wait_for_stop(m_mic_pipeline)) {
        bk_printf("m_mic_pipeline wait stop fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_pipeline_terminate(m_mic_pipeline)) {
        bk_printf("pipeline terminate fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_pipeline_unregister(m_mic_pipeline, m_mic_stream)) {
        bk_printf("pipeline terminate fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_pipeline_unregister(m_mic_pipeline, m_raw_mic_stream)) {
        bk_printf("pipeline terminate fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_pipeline_deinit(m_mic_pipeline)) {
        bk_printf("pipeline terminate fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_element_deinit(m_mic_stream)) {
        bk_printf("element deinit fail, %d \n", __LINE__);
    }

    if (BK_OK != audio_element_deinit(m_raw_mic_stream)) {
        bk_printf("element deinit fail, %d \n", __LINE__);
    }

    return OPRT_COM_ERROR;
#else
    uint32_t pool_size = frame_size * 2;
    audio_record_cfg_t mic_config = DEFAULT_AUDIO_RECORD_CONFIG();
    mic_config.bitsPerSample = config->datebits;
    mic_config.adc_gain = config->volume;
    mic_config.sampRate = sample_rate;
    mic_config.frame_size = frame_size;
    mic_config.pool_size = pool_size;
    mic_config.nChans = config->chl_num;
    m_audio_record = audio_record_create(AUDIO_RECORD_ONBOARD_MIC, &mic_config);
    if (m_audio_record == NULL) {
        bk_printf("init mic stream fail\r\n");
        return OPRT_COM_ERROR;
    }
    return OPRT_OK;
#endif
}

int32_t tkl_mic_start(void)
{
    int32_t ret = OPRT_OK;
#ifdef CONFIG_ADK_ONBOARD_MIC_STREAM
    if (m_mic_pipeline) {
        ret = audio_pipeline_run(m_mic_pipeline);
    }
#else
    if (m_audio_record) {
        if (BK_OK != audio_record_open(m_audio_record)) {
            bk_printf("open audio record fail\n");
            return OPRT_COM_ERROR;
        }
    }
#endif
    bk_printf("start mic record %d\r\n", ret);
    return ret;
}

int32_t tkl_mic_pause(void)
{
    int32_t ret = OPRT_OK;
#ifdef CONFIG_ADK_ONBOARD_MIC_STREAM
    if (m_mic_pipeline) {
        ret = audio_pipeline_pause(m_mic_pipeline);
    }
#else
    if (m_audio_record) {
        ret = audio_record_control(m_audio_record, AUDIO_RECORD_PAUSE);
        return OPRT_COM_ERROR;
    }
#endif
    bk_printf("pause mic record %d\r\n", ret);
    return ret;
}

int32_t tkl_mic_resume(void)
{
    int32_t ret = OPRT_OK;
#ifdef CONFIG_ADK_ONBOARD_MIC_STREAM
    if (m_mic_pipeline) {
        ret = audio_pipeline_resume(m_mic_pipeline);
    }
#else
    if (m_audio_record) {
        ret = audio_record_control(m_audio_record, AUDIO_RECORD_RESUME);
    }
#endif
    bk_printf("resume mic record %d\r\n", ret);
    return ret;
}

int32_t tkl_mic_set_gain(int32_t gain)
{
    int32_t ret = OPRT_OK;
    if (gain > 100)
    {
        return OPRT_INVALID_PARM;
    }
#ifdef CONFIG_ADK_ONBOARD_MIC_STREAM
    if (m_mic_stream) {
        ret = onboard_mic_stream_set_digital_gain(m_mic_stream, gain);
    }
#else
    if (m_audio_record) {
        ret = audio_play_set_adc_gain(m_audio_record, gain);
    }
#endif
    if(ret == OPRT_OK)
    {
        m_mic_gain = gain;
    }
    bk_printf("set mic gain %d\r\n", ret);
    return ret;
}

int32_t tkl_mic_get_gain(void)
{
    int32_t ret = OPRT_OK;
    int32_t gain = 0;

#ifdef CONFIG_ADK_ONBOARD_MIC_STREAM
    if (m_mic_stream) {
        ret = onboard_mic_stream_get_digital_gain(m_mic_stream, (uint8_t *)&gain);
        ret = gain;
    }
#else
    if (m_audio_record) {
        ret = m_mic_gain;
    }
#endif
    bk_printf("set mic gain %d\r\n", ret);
    return ret;
}

int32_t tkl_mic_read(uint8_t *data, uint32_t len)
{
    int32_t ret = OPRT_OK;
#ifdef CONFIG_ADK_ONBOARD_MIC_STREAM
    if (m_raw_mic_stream) {
        ret = raw_stream_read(m_raw_mic_stream, (char *)data, len);
    }
#else
    if (m_audio_record) {
        ret = audio_record_read_data(m_audio_record, (char *)data, len);
    }
#endif
    return ret;
}

int32_t tkl_mic_stop(void)
{
    int32_t ret = OPRT_OK;
#ifdef CONFIG_ADK_ONBOARD_MIC_STREAM
    if (m_mic_pipeline) {
        if (BK_OK != audio_pipeline_stop(m_mic_pipeline)) {
            bk_printf("m_mic_pipeline stop fail, %d \n", __LINE__);
            return OPRT_COM_ERROR;
        }
        if (BK_OK != audio_pipeline_wait_for_stop(m_mic_pipeline)) {
            bk_printf("m_mic_pipeline wait stop fail, %d \n", __LINE__);
            return OPRT_COM_ERROR;
        }
    }
#else
    if (m_audio_record) {
        ret = audio_record_close(m_audio_record);
    }
#endif
    bk_printf("start mic record %d\r\n", ret);
    return ret;
}

void tkl_mic_deinit(void)
{
#ifdef CONFIG_ADK_ONBOARD_MIC_STREAM
    if (BK_OK != audio_pipeline_stop(m_mic_pipeline)) {
        bk_printf("m_mic_pipeline stop fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_pipeline_wait_for_stop(m_mic_pipeline)) {
        bk_printf("m_mic_pipeline wait stop fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_pipeline_terminate(m_mic_pipeline)) {
        bk_printf("pipeline terminate fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_pipeline_unregister(m_mic_pipeline, m_mic_stream)) {
        bk_printf("pipeline terminate fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_pipeline_unregister(m_mic_pipeline, m_raw_mic_stream)) {
        bk_printf("pipeline terminate fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_pipeline_deinit(m_mic_pipeline)) {
        bk_printf("pipeline terminate fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_element_deinit(m_mic_stream)) {
        bk_printf("element deinit fail, %d \n", __LINE__);
    }

    if (BK_OK != audio_element_deinit(m_raw_mic_stream)) {
        bk_printf("element deinit fail, %d \n", __LINE__);
    }
#else
    if (m_audio_record) {
        audio_record_destroy(m_audio_record);
    }
#endif
    bk_printf("stop mic record \r\n");
}
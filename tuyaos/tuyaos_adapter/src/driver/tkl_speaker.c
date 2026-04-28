#include "tkl_speaker.h"
#include "sys_driver.h"
#include "tkl_gpio.h"
#include "tuya_cloud_types.h"
#include "tuya_error_code.h"
#ifdef CONFIG_ADK_ONBOARD_SPEAKER_STREAM
#include "components/bk_audio/audio_pipeline/audio_pipeline.h"
#include "components/bk_audio/audio_streams/onboard_speaker_stream.h"
#include "components/bk_audio/audio_streams/raw_stream.h"
#include <components/bk_audio/audio_decoders/g711_decoder.h>
#include <components/bk_audio/audio_streams/uac_speaker_stream.h>
#else
#include "audio_play.h"
#endif

#define CHANNEL_NUM (2)
#define TIME_SAMPLE_MS (20)
#define MS_PER_SEC (1000)
#define DEFAULT_SAMPLE_RATE 16000
#define DEFAULT_FRAME_SIZE 640
#define DEFAULT_POOL_SIZE (DEFAULT_FRAME_SIZE * 2)

static uint32_t m_play_gain = 0;
#ifdef CONFIG_ADK_ONBOARD_SPEAKER_STREAM
static audio_element_handle_t m_spk_decode = NULL;
static audio_pipeline_handle_t m_spk_pipeline = NULL;
static audio_element_handle_t m_spk_stream = NULL;
static audio_element_handle_t m_raw_spk_stream = NULL;
#else
static audio_play_t *m_audio_play = NULL;
#endif
extern void bk_printf(const char *fmt, ...);

/**
 * @brief tuya kernel speak init
 *
 * TODO
 * @param[in] func: ADD Callback function
 *
 * @return OPRT_OK on success. Others on error, please refer to
 * tuya_error_code.h
 */
OPERATE_RET tkl_speaker_init(TKL_SPK_CFG_T *config)
{
    if (config == NULL) {
        return OPRT_INVALID_PARM;
    }

    uint32_t sample_rate =
        config->sample_rate ? config->sample_rate : DEFAULT_SAMPLE_RATE;
    uint32_t frame_size =
        sample_rate * CHANNEL_NUM * TIME_SAMPLE_MS / MS_PER_SEC;
    if (config->spk_gpio < 56) {
        TUYA_GPIO_BASE_CFG_T cfg;
        cfg.direct = TUYA_GPIO_OUTPUT;
        if (config->spk_gpio_polarity == 0) {
            cfg.mode = TUYA_GPIO_PULLUP;
            cfg.level = TUYA_GPIO_LEVEL_HIGH;
        } else if (config->spk_gpio_polarity == 1) {
            cfg.mode = TUYA_GPIO_PULLDOWN;
            cfg.level = TUYA_GPIO_LEVEL_LOW;
        }

        tkl_gpio_init(config->spk_gpio, &cfg);
    }

#ifdef CONFIG_ADK_ONBOARD_SPEAKER_STREAM
    raw_stream_cfg_t raw_write_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_write_cfg.type = AUDIO_STREAM_WRITER;
    m_raw_spk_stream = raw_stream_init(&raw_write_cfg);
    if (m_raw_spk_stream == NULL) {
        bk_printf("init raw stream fail\r\n");
        return OPRT_COM_ERROR;
    }

    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    m_spk_pipeline = audio_pipeline_init(&pipeline_cfg);
    if (m_spk_pipeline == NULL) {
        bk_printf("init speak stream fail\r\n");
        goto err;
    }

    switch (config->codectype) {
        case TKL_CODEC_SPK_G711A:
        case TKL_CODEC_SPK_G711U: {
            /* g711 decoder config */
            g711_decoder_cfg_t g711_decoder_cfg = DEFAULT_G711_DECODER_CONFIG();
            if (config->codectype == TKL_CODEC_SPK_G711A) {
                g711_decoder_cfg.dec_mode = G711_DEC_MODE_A_LOW;
            } else {
                g711_decoder_cfg.dec_mode = G711_DEC_MODE_U_LOW;
            }
            g711_decoder_cfg.out_block_size =
                sample_rate * CHANNEL_NUM * TIME_SAMPLE_MS /
                MS_PER_SEC; // one frame size(20ms)
            g711_decoder_cfg.buf_sz = g711_decoder_cfg.out_block_size >> 1;
            m_spk_decode = g711_decoder_init(&g711_decoder_cfg);
        } break;
        default:
            break;
    }
    if (config->card == TKL_SPK_TYPE_BOARD) {
        onboard_speaker_stream_cfg_t onboard_spk_cfg =
            ONBOARD_SPEAKER_STREAM_CFG_DEFAULT();
        onboard_spk_cfg.bits = config->datebits;
        onboard_spk_cfg.chl_num = config->chl_num;
        onboard_spk_cfg.dig_gain = config->volume;
        onboard_spk_cfg.sample_rate = sample_rate;
        onboard_spk_cfg.frame_size = frame_size;
        bk_printf("init speak stream %d %d %d %d %d \r\n", config->datebits,
                  config->chl_num, config->volume, sample_rate, frame_size);
        m_spk_stream = onboard_speaker_stream_init(&onboard_spk_cfg);
        if (m_spk_stream == NULL) {
            bk_printf("init speak stream fail\r\n");
            goto err;
        }
    }
#ifdef CONFIG_ADK_UAC_MIC_STREAM
    else {
        uac_speaker_stream_cfg_t uac_spk_cfg = UAC_SPEAKER_STREAM_CFG_DEFAULT();
        uac_spk_cfg.bits = config->datebits;
        uac_spk_cfg.chl_num = config->chl_num;
        uac_spk_cfg.volume = config->volume;
        uac_spk_cfg.samp_rate = sample_rate;
        /* one farme size, 20ms */
        uac_spk_cfg.frame_size = frame_size;
        m_spk_stream = uac_speaker_stream_init(&uac_spk_cfg);
        if (m_spk_stream == NULL) {
            bk_printf("init speak stream fail\r\n");
            goto err;
        }
    }
#endif

    if (BK_OK !=
        audio_pipeline_register(m_spk_pipeline, m_raw_spk_stream, "raw_spk")) {
        bk_printf("register element fail, %d \n", __LINE__);
        goto err;
    }
    if (BK_OK !=
        audio_pipeline_register(m_spk_pipeline, m_spk_stream, "speaker")) {
        bk_printf("register element fail, %d \n", __LINE__);
        goto err;
    }

    switch (config->codectype) {
        case TKL_CODEC_SPK_G711A:
        case TKL_CODEC_SPK_G711U: {
            audio_pipeline_register(m_spk_pipeline, m_spk_decode, "decode");
            if (BK_OK != audio_pipeline_link(
                             m_spk_pipeline,
                             (const char *[]){"raw_spk", "decode", "speaker"},
                             3)) {
                bk_printf("m_spk_pipeline link fail, %d \n", __LINE__);
                goto err;
            }
        } break;
        default: {
            if (BK_OK != audio_pipeline_link(
                             m_spk_pipeline,
                             (const char *[]){"raw_spk", "speaker"}, 2)) {
                bk_printf("m_spk_pipeline link fail, %d \n", __LINE__);
                goto err;
            }
        } break;
    }

    if (BK_OK != audio_pipeline_run(m_spk_pipeline)) {
        bk_printf("m_spk_pipeline run fail, %d \n", __LINE__);
        goto err;
    }

    return OPRT_OK;
err:
    if (BK_OK != audio_pipeline_stop(m_spk_pipeline)) {
        bk_printf("m_spk_pipeline stop fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_pipeline_wait_for_stop(m_spk_pipeline)) {
        bk_printf("m_spk_pipeline wait stop fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_pipeline_terminate(m_spk_pipeline)) {
        bk_printf("pipeline terminate fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_pipeline_unregister(m_spk_pipeline, m_spk_stream)) {
        bk_printf("pipeline terminate fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_pipeline_unregister(m_spk_pipeline, m_raw_spk_stream)) {
        bk_printf("pipeline terminate fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_pipeline_deinit(m_spk_pipeline)) {
        bk_printf("pipeline terminate fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_element_deinit(m_spk_stream)) {
        bk_printf("element deinit fail, %d \n", __LINE__);
    }

    if (BK_OK != audio_element_deinit(m_raw_spk_stream)) {
        bk_printf("element deinit fail, %d \n", __LINE__);
    }
    return OPRT_COM_ERROR;
#else
    uint32_t pool_size = frame_size * 2;
    audio_play_cfg_t play_config = DEFAULT_AUDIO_PLAY_CONFIG();
    play_config.bitsPerSample = config->datebits;
    play_config.sampRate = sample_rate;
    play_config.volume = config->volume;
    play_config.frame_size = frame_size;
    play_config.pool_size = pool_size;
    play_config.nChans = config->chl_num;
    m_audio_play = audio_play_create(AUDIO_PLAY_ONBOARD_SPEAKER, &play_config);
    if (m_audio_play == NULL) {
        bk_printf("init speak stream fail\r\n");
        return OPRT_COM_ERROR;
    }
    bk_printf("init speak complete \r\n");
    return OPRT_OK;
#endif
}

int32_t tkl_speaker_start(void)
{
    int32_t ret = OPRT_OK;
#ifdef CONFIG_ADK_ONBOARD_SPEAKER_STREAM
    if (m_spk_pipeline) {
        ret = audio_pipeline_run(m_spk_pipeline);
    }
#else
    if (m_audio_play) {
        if (BK_OK != audio_play_open(m_audio_play)) {
            bk_printf("open audio play fail\n");
            return OPRT_COM_ERROR;
        }
    }
#endif
    bk_printf("start speak play %d\r\n", ret);
    return ret;
}

int32_t tkl_speaker_pause(void)
{
    int32_t ret = OPRT_OK;
#ifdef CONFIG_ADK_ONBOARD_SPEAKER_STREAM
    if (m_spk_pipeline) {
        ret = audio_pipeline_pause(m_spk_pipeline);
    }
#else
    if (m_audio_play) {
        ret = audio_play_control(m_audio_play, AUDIO_PLAY_PAUSE);
    }
#endif
    bk_printf("pause speak play %d\r\n", ret);
    return ret;
}

int32_t tkl_speaker_resume(void)
{
    int32_t ret = OPRT_OK;
#ifdef CONFIG_ADK_ONBOARD_SPEAKER_STREAM
    if (m_spk_pipeline) {
        ret = audio_pipeline_resume(m_spk_pipeline);
    }
#else
    if (m_audio_play) {
        ret = audio_play_control(m_audio_play, AUDIO_PLAY_RESUME);
    }
#endif
    bk_printf("resume speak play %d\r\n", ret);
    return ret;
}

int32_t tkl_speaker_set_gain(int32_t gain)
{
    int32_t ret = OPRT_OK;
    if (gain > 100)
    {
        return OPRT_INVALID_PARM;
    }

#ifdef CONFIG_ADK_ONBOARD_SPEAKER_STREAM
    if (m_spk_stream) {
        ret = onboard_speaker_stream_set_digital_gain(m_spk_stream, gain);
    }
#else
    if (m_audio_play) {
        ret = audio_play_set_volume(m_audio_play, gain);
    }
#endif
    if(ret == OPRT_OK)
    {
        m_play_gain = gain;
    }
    bk_printf("set speak gain %d\r\n", ret);
    return ret;
}

int32_t tkl_speaker_get_gain(void)
{
    int32_t gain = 0;
    int32_t ret = OPRT_OK;
#ifdef CONFIG_ADK_ONBOARD_SPEAKER_STREAM
    if (m_spk_stream) {
        ret = onboard_speaker_stream_get_digital_gain(m_spk_stream, (uint8_t *)&gain);
        ret = gain;
    }
#else
    if (m_audio_play) {
        ret = m_play_gain;
    }
#endif
    bk_printf("set speak gain %d\r\n", ret);
    return ret;
}

int32_t tkl_speaker_write(uint8_t *data, uint32_t len)
{
    int32_t ret = OPRT_OK;
#ifdef CONFIG_ADK_ONBOARD_SPEAKER_STREAM
    if (m_raw_spk_stream) {
        ret = raw_stream_write(m_raw_spk_stream, (char *)data, len);
    }
#else
    if (m_audio_play) {
        ret = audio_play_write_data(m_audio_play, (char *)data, len);
    }
#endif
    return ret;
}

int32_t tkl_speaker_stop(void)
{
    int32_t ret = OPRT_OK;
#ifdef CONFIG_ADK_ONBOARD_SPEAKER_STREAM
    if (m_spk_pipeline) {
        if (BK_OK != audio_pipeline_stop(m_spk_pipeline)) {
            bk_printf("m_spk_pipeline stop fail, %d \n", __LINE__);
            return OPRT_COM_ERROR;
        }
        if (BK_OK != audio_pipeline_wait_for_stop(m_spk_pipeline)) {
            bk_printf("m_spk_pipeline wait stop fail, %d \n", __LINE__);
            return OPRT_COM_ERROR;
        }
    }
#else
    if (m_audio_play) {
        audio_play_close(m_audio_play);
    }
#endif
    bk_printf("start speak play %d\r\n", ret);
    return ret;
}

void tkl_speaker_deinit(void)
{
#ifdef CONFIG_ADK_ONBOARD_SPEAKER_STREAM
    if (BK_OK != audio_pipeline_stop(m_spk_pipeline)) {
        bk_printf("m_spk_pipeline stop fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_pipeline_wait_for_stop(m_spk_pipeline)) {
        bk_printf("m_spk_pipeline wait stop fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_pipeline_terminate(m_spk_pipeline)) {
        bk_printf("pipeline terminate fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_pipeline_unregister(m_spk_pipeline, m_spk_stream)) {
        bk_printf("pipeline terminate fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_pipeline_unregister(m_spk_pipeline, m_raw_spk_stream)) {
        bk_printf("pipeline terminate fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_pipeline_deinit(m_spk_pipeline)) {
        bk_printf("pipeline terminate fail, %d \n", __LINE__);
    }
    if (BK_OK != audio_element_deinit(m_spk_stream)) {
        bk_printf("element deinit fail, %d \n", __LINE__);
    }

    if (BK_OK != audio_element_deinit(m_raw_spk_stream)) {
        bk_printf("element deinit fail, %d \n", __LINE__);
    }
#else
    if (m_audio_play) {
        audio_play_destroy(m_audio_play);
    }
#endif
    bk_printf("stop speak play \r\n");
}

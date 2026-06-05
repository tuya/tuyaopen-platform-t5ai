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

// 驱动内部使用的是2
#define DRIVER_SPEAK_FIFO_FRAME_NUM 2
#define MAX_SEM_AUDIO            (8)
#define CHANNEL_NUM              (2)
#define TIME_SAMPLE_MS           (20)
#define MS_PER_SEC               (1000)

#define FRAME_SIZE_PER_20MS(x)  (x * CHANNEL_NUM * TIME_SAMPLE_MS / MS_PER_SEC)

#define TKL_AUDIO_CONFIG_DUMP   0

extern void tuya_multimedia_power_on(void);

static voice_handle_t g_voice_handle = NULL;
static voice_read_handle_t g_voice_read_handle = NULL;
static voice_write_handle_t g_voice_write_handle = NULL;
typedef struct
{
    uint32_t mic_gain;
    uint32_t spk_gain;
}TKL_AUDIO_GAIN_T;

static TKL_AUDIO_GAIN_T g_audio_gain = {0};
static TKL_FRAME_PUT_CB user_mic_cb = NULL;
static TKL_FRAME_SPK_CB user_spk_cb = NULL;
static int32_t board_spk_gpio = 56;
static int32_t board_spk_gpio_polarity = 0;     // 喇叭静音时候电平
static int32_t tkl_bk_vad_enable = 1;

extern void *tkl_system_psram_malloc(size_t size);
extern void tkl_system_psram_free(void *ptr);

typedef struct {
    bool audio_init;
    bool audio_start;
    bool audio_status;
} AUIDO_INIT_T;

typedef struct {
    TKL_AUDIO_TYPE_E card;
    uint32_t samp_rate;
    uint32_t aec_enable;
} AUIDO_MIC_T;

static AUIDO_MIC_T s_audio_mic = {0};
static AUIDO_INIT_T s_audio_init = {0};

int voice_read_callback(unsigned char *data, unsigned int len, void *args)
{
    TKL_AUDIO_FRAME_INFO_T pframe = {0};
    pframe.buf_size = len;
    pframe.used_size = len;
    pframe.pbuf = (char *)data;
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
    return status;
}

static void __tkl_ai_error_cleanup(void)
{
    if (board_spk_gpio < 56) {
        tkl_gpio_write(board_spk_gpio, board_spk_gpio_polarity);
    }

    if (g_voice_read_handle) {
        bk_voice_read_stop(g_voice_read_handle);
    }

    if (g_voice_write_handle) {
        bk_voice_write_stop(g_voice_write_handle);
    }

    if (g_voice_handle) {
        bk_voice_stop(g_voice_handle);
    }

    if (g_voice_read_handle) {
        bk_voice_read_deinit(g_voice_read_handle);
    }

    if (g_voice_write_handle) {
        bk_voice_write_deinit(g_voice_write_handle);
    }

    if (g_voice_handle) {
        bk_voice_deinit(g_voice_handle);
    }

    uint32_t level = tkl_system_enter_critical();

    g_voice_read_handle = NULL;
    g_voice_write_handle = NULL;
    g_voice_handle = NULL;

    s_audio_mic.card = TKL_AUDIO_TYPE_UAC;
    s_audio_mic.aec_enable = 0;
    s_audio_mic.samp_rate = 0;
    s_audio_init.audio_status = false;
    s_audio_init.audio_init = false;
    tkl_system_exit_critical(level);

    bk_printf("tkl audio init error, clean resource complete\r\n");
}

// for test
// mic测试时候，如果使能原厂vad，一段时间检测不到声音后，
// 底层会停止上应用层输出数据
void tkl_ai_disable_vendor_vad(void)
{
    tkl_bk_vad_enable = 0;
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
    uint32_t level = 0;
    uint32_t sample_rate = TKL_AUDIO_SAMPLE_8K;
    uint32_t frame_size = 320;
    uac_mic_stream_cfg_t uac_mic_cfg = UAC_MIC_STREAM_CFG_DEFAULT();
    onboard_mic_stream_cfg_t onboard_mic_cfg = ONBOARD_MIC_ADC_STREAM_CFG_DEFAULT();
    onboard_dual_dmic_mic_stream_cfg_t onboard_dual_mic_cfg = DEFAULT_ONBOARD_DUAL_DMIC_STREAM_CONFIG();
    voice_cfg_t voice_cfg = {0};
    os_memset(&voice_cfg, 0, sizeof(voice_cfg_t));
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

    frame_size = FRAME_SIZE_PER_20MS(sample_rate);
    bk_printf("--- audio trace, init, sample: %d, frame size: %d\r\n", sample_rate, frame_size);

    if (pconfig->card == TKL_AUDIO_TYPE_UAC) {
        voice_cfg.mic_type = MIC_TYPE_UAC;
        uac_mic_cfg.bits = pconfig->datebits;
        uac_mic_cfg.samp_rate = sample_rate;
        /* one farme size, 20ms */
        uac_mic_cfg.frame_size = frame_size;
        uac_mic_cfg.out_block_size = uac_mic_cfg.frame_size;
        uac_mic_cfg.out_block_num = 2;
        memcpy(&voice_cfg.mic_cfg.uac_mic_cfg, &uac_mic_cfg,
               sizeof(uac_mic_stream_cfg_t));
    } else if (pconfig->card == TKL_AUDIO_TYPE_DUAL) {
        voice_cfg.mic_type = MIC_TYPE_ONBOARD_DUAL_DMIC_MIC;
        if (pconfig->mic_volume) {
            onboard_mic_cfg.adc_cfg.dig_gain = (uint32_t)(pconfig->mic_volume * 0x3F / 100);
        }

        onboard_dual_mic_cfg.adc_cfg.bits = pconfig->datebits;
        if (sample_rate == 16000 || sample_rate == 8000) {
            onboard_dual_mic_cfg.adc_cfg.sample_rate = sample_rate;
        } else {
            bk_printf("only support 8K/16k samplerate\r\n");
        }

        /* one farme size, 20ms */
        onboard_dual_mic_cfg.frame_size = frame_size;
        onboard_dual_mic_cfg.out_block_size = onboard_dual_mic_cfg.frame_size;
        onboard_dual_mic_cfg.adc_cfg.clk_src = AUD_CLK_APLL;
        memcpy(&voice_cfg.mic_cfg.onboard_dual_dmic_mic_cfg, &onboard_dual_mic_cfg,
               sizeof(onboard_dual_dmic_mic_stream_cfg_t));
    } else {
        voice_cfg.mic_type = MIC_TYPE_ONBOARD;
        // if (pconfig->mic_volume) {
        //     onboard_mic_cfg.adc_cfg.ana_gain = 0x3f; // (uint32_t)(pconfig->mic_volume * 0x3F / 100);
        // }
        onboard_mic_cfg.adc_cfg.bits = pconfig->datebits;
        onboard_mic_cfg.adc_cfg.sample_rate = sample_rate;
        /* one farme size, 20ms */
        onboard_mic_cfg.frame_size = frame_size;
        // onboard_mic_cfg.out_block_size = uac_mic_cfg.frame_size;
        onboard_mic_cfg.out_block_size = onboard_mic_cfg.frame_size;
        if (pconfig->enable) {
            onboard_mic_cfg.adc_cfg.chl_num = 2;
        } else {
            onboard_mic_cfg.adc_cfg.chl_num = 1;
        }

        onboard_mic_cfg.task_prio = 2;
        onboard_mic_cfg.task_stack = 4096;

        memcpy(&voice_cfg.mic_cfg.onboard_mic_cfg, &onboard_mic_cfg,
               sizeof(onboard_mic_stream_cfg_t));

#if TKL_AUDIO_CONFIG_DUMP
        bk_printf("--- audio mic config:\r\n");
        bk_printf("    onboard_mic_cfg.adc_cfg.chl_num:     0x%x\r\n", onboard_mic_cfg.adc_cfg.chl_num);
        bk_printf("    onboard_mic_cfg.adc_cfg.bits:        0x%x\r\n", onboard_mic_cfg.adc_cfg.bits);
        bk_printf("    onboard_mic_cfg.adc_cfg.sample_rate: 0x%x\r\n", onboard_mic_cfg.adc_cfg.sample_rate);
        bk_printf("    onboard_mic_cfg.adc_cfg.dig_gain:    0x%x\r\n", onboard_mic_cfg.adc_cfg.dig_gain);
        bk_printf("    onboard_mic_cfg.adc_cfg.ana_gain:    0x%x\r\n", onboard_mic_cfg.adc_cfg.ana_gain);
        bk_printf("    onboard_mic_cfg.adc_cfg.mode:        0x%x\r\n", onboard_mic_cfg.adc_cfg.mode);
        bk_printf("    onboard_mic_cfg.adc_cfg.clk_src:     0x%x\r\n", onboard_mic_cfg.adc_cfg.clk_src);
        bk_printf("    onboard_mic_cfg.frame_size:          0x%x\r\n", onboard_mic_cfg.frame_size);
        bk_printf("    onboard_mic_cfg.out_block_size:      0x%x\r\n", onboard_mic_cfg.out_block_size);
        bk_printf("    onboard_mic_cfg.out_block_num:       0x%x\r\n", onboard_mic_cfg.out_block_num);
        bk_printf("    onboard_mic_cfg.multi_out_port_num:  0x%x\r\n", onboard_mic_cfg.multi_out_port_num);
        bk_printf("    onboard_mic_cfg.task_stack:          0x%x\r\n", onboard_mic_cfg.task_stack);
        bk_printf("    onboard_mic_cfg.task_core:           0x%x\r\n", onboard_mic_cfg.task_core);
        bk_printf("    onboard_mic_cfg.task_prio:           0x%x\r\n", onboard_mic_cfg.task_prio);
#endif
    }
    if (pconfig->enable) {
        if ((pconfig->sample != TKL_AUDIO_SAMPLE_8K) && (pconfig->sample != TKL_AUDIO_SAMPLE_16K)) {
            bk_printf("voice only support 8k/16k sample, set value %d\r\n",
                      pconfig->sample);
            return -1;
        }
    }

    if ((pconfig->spk_sample == 0) || (pconfig->enable)) {
        // enable置1,表示语音对讲，此时因AEC需要，mic跟spk的
        // 采样率需要相等
        voice_cfg.aec_en = 1;

        // aec_algorithm_cfg_t aec_alg_cfg = DEFAULT_AEC_ALGORITHM_CONFIG();
        // aec_alg_cfg.out_block_num = 1;
        // aec_alg_cfg.aec_cfg.fs = sample_rate;

        aec_v3_algorithm_cfg_t aec_v3_alg_cfg = DEFAULT_AEC_V3_ALGORITHM_CONFIG();
        aec_v3_alg_cfg.out_block_num = 1;
        aec_v3_alg_cfg.aec_cfg.fs = sample_rate;
        if (pconfig->card == TKL_AUDIO_TYPE_DUAL) {
            aec_v3_alg_cfg.dual_ch = 1;
        }
        aec_v3_alg_cfg.vad_cfg.vad_enable = tkl_bk_vad_enable;
        if(aec_v3_alg_cfg.vad_cfg.vad_enable) {
            voice_cfg.enc_common.frame_in_ms = TIME_SAMPLE_MS;
            voice_cfg.enc_common.frame_in_size = frame_size;
        }
        aec_v3_alg_cfg.aec_cfg.drc = 0x0a;
        aec_v3_alg_cfg.aec_cfg.voice_vol = 0x0d;
        aec_v3_alg_cfg.aec_cfg.mode = AEC_MODE_HARDWARE;

        aec_v3_alg_cfg.aec_cfg.dual_perp = DUAL_CH_90_DEGREE; //DUAL_CH_0_DEGREE;
        aec_v3_alg_cfg.aec_cfg.ec_only_output = 1;

        // voice_cfg.aec_cfg.aec_alg_cfg = aec_alg_cfg;
        voice_cfg.aec_cfg.aec_alg_cfg = aec_v3_alg_cfg;

#if TKL_AUDIO_CONFIG_DUMP
        bk_printf("--- audio aec enable\r\n");
        bk_printf("    aec_v3_alg_cfg.task_stack                    0x%x\r\n", aec_v3_alg_cfg.task_stack);
        bk_printf("    aec_v3_alg_cfg.task_core                     0x%x\r\n", aec_v3_alg_cfg.task_core);
        bk_printf("    aec_v3_alg_cfg.task_prio                     0x%x\r\n", aec_v3_alg_cfg.task_prio);
        bk_printf("    aec_v3_alg_cfg.aec_cfg.mode                  0x%x\r\n", aec_v3_alg_cfg.aec_cfg.mode);
        bk_printf("    aec_v3_alg_cfg.aec_cfg.fs                    0x%x\r\n", aec_v3_alg_cfg.aec_cfg.fs);
        bk_printf("    aec_v3_alg_cfg.aec_cfg.init_flags            0x%x\r\n", aec_v3_alg_cfg.aec_cfg.init_flags);
        bk_printf("    aec_v3_alg_cfg.aec_cfg.delay_points          0x%x\r\n", aec_v3_alg_cfg.aec_cfg.delay_points);
        bk_printf("    aec_v3_alg_cfg.aec_cfg.ec_depth              0x%x\r\n", aec_v3_alg_cfg.aec_cfg.ec_depth);
        bk_printf("    aec_v3_alg_cfg.aec_cfg.ref_scale             0x%x\r\n", aec_v3_alg_cfg.aec_cfg.ref_scale);
        bk_printf("    aec_v3_alg_cfg.aec_cfg.voice_vol             0x%x\r\n", aec_v3_alg_cfg.aec_cfg.voice_vol);
        bk_printf("    aec_v3_alg_cfg.aec_cfg.TxRxThr               0x%x\r\n", aec_v3_alg_cfg.aec_cfg.TxRxThr);
        bk_printf("    aec_v3_alg_cfg.aec_cfg.TxRxFlr               0x%x\r\n", aec_v3_alg_cfg.aec_cfg.TxRxFlr);
        bk_printf("    aec_v3_alg_cfg.aec_cfg.ns_type               0x%x\r\n", aec_v3_alg_cfg.aec_cfg.ns_type);
        bk_printf("    aec_v3_alg_cfg.aec_cfg.ns_filter             0x%x\r\n", aec_v3_alg_cfg.aec_cfg.ns_filter);
        bk_printf("    aec_v3_alg_cfg.aec_cfg.ns_level              0x%x\r\n", aec_v3_alg_cfg.aec_cfg.ns_level);
        bk_printf("    aec_v3_alg_cfg.aec_cfg.ns_para               0x%x\r\n", aec_v3_alg_cfg.aec_cfg.ns_para);
        bk_printf("    aec_v3_alg_cfg.aec_cfg.drc                   0x%x\r\n", aec_v3_alg_cfg.aec_cfg.drc);
        bk_printf("    aec_v3_alg_cfg.aec_cfg.ec_filter             0x%x\r\n", aec_v3_alg_cfg.aec_cfg.ec_filter);
        bk_printf("    aec_v3_alg_cfg.vad_cfg.vad_enable            0x%x\r\n", aec_v3_alg_cfg.vad_cfg.vad_enable);
        bk_printf("    aec_v3_alg_cfg.vad_cfg.vad_start_threshold   0x%x\r\n", aec_v3_alg_cfg.vad_cfg.vad_start_threshold);
        bk_printf("    aec_v3_alg_cfg.vad_cfg.vad_stop_threshold    0x%x\r\n", aec_v3_alg_cfg.vad_cfg.vad_stop_threshold);
        bk_printf("    aec_v3_alg_cfg.vad_cfg.vad_silence_threshold 0x%x\r\n", aec_v3_alg_cfg.vad_cfg.vad_silence_threshold);
        bk_printf("    aec_v3_alg_cfg.vad_cfg.vad_eng_threshold     0x%x\r\n", aec_v3_alg_cfg.vad_cfg.vad_eng_threshold);
        bk_printf("    aec_v3_alg_cfg.vad_cfg.vad_bad_frame         0x%x\r\n", aec_v3_alg_cfg.vad_cfg.vad_bad_frame);
        bk_printf("    aec_v3_alg_cfg.vad_cfg.vad_buf_size          0x%x\r\n", aec_v3_alg_cfg.vad_cfg.vad_buf_size);
        bk_printf("    aec_v3_alg_cfg.vad_cfg.vad_frame_size        0x%x\r\n", aec_v3_alg_cfg.vad_cfg.vad_frame_size);
        bk_printf("    aec_v3_alg_cfg.out_block_size                0x%x\r\n", aec_v3_alg_cfg.out_block_size);
        bk_printf("    aec_v3_alg_cfg.out_block_num                 0x%x\r\n", aec_v3_alg_cfg.out_block_num);
        bk_printf("    aec_v3_alg_cfg.multi_out_port_num            0x%x\r\n", aec_v3_alg_cfg.multi_out_port_num);
        bk_printf("    aec_v3_alg_cfg.dual_ch                       0x%x\r\n", aec_v3_alg_cfg.dual_ch);
#endif
    } else {
        // 否则直接使用用户传入的采样率
        sample_rate = pconfig->spk_sample? pconfig->spk_sample : TKL_AUDIO_SAMPLE_8K;
        voice_cfg.aec_en = false;
        voice_cfg.aec_cfg.reserve = 0;
        bk_printf("--- audio aec disable\r\n");
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
            voice_cfg.enc_cfg.g711_enc_cfg.buf_sz = frame_size;
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

            voice_cfg.dec_cfg.g711_dec_cfg.out_block_size = frame_size;
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
            voice_cfg.read_pool_size = frame_size;
            voice_cfg.write_pool_size = frame_size;
        } break;

        default: {
            bk_printf("not support encoder format\n");
            goto error;
        } break;
    }

#if TKL_AUDIO_CONFIG_DUMP
    bk_printf("    voice_cfg.enc_en                    0x%x\r\n", voice_cfg.enc_en);
    bk_printf("    voice_cfg.enc_type                  0x%x\r\n", voice_cfg.enc_type);
    bk_printf("    voice_cfg.enc_common.samp_rate      0x%x\r\n", voice_cfg.enc_common.samp_rate);
    bk_printf("    voice_cfg.enc_common.bitrate        0x%x\r\n", voice_cfg.enc_common.bitrate);
    bk_printf("    voice_cfg.enc_common.frame_in_size  0x%x\r\n", voice_cfg.enc_common.frame_in_size);
    bk_printf("    voice_cfg.enc_common.frame_out_size 0x%x\r\n", voice_cfg.enc_common.frame_out_size);
    bk_printf("    voice_cfg.enc_common.bits           0x%x\r\n", voice_cfg.enc_common.bits);
    bk_printf("    voice_cfg.enc_common.frame_in_ms    0x%x\r\n", voice_cfg.enc_common.frame_in_ms);
    bk_printf("    voice_cfg.enc_common.vbr            0x%x\r\n", voice_cfg.enc_common.vbr);
    bk_printf("    voice_cfg.enc_common.channels       0x%x\r\n", voice_cfg.enc_common.channels);
    bk_printf("    voice_cfg.read_pool_size            0x%x\r\n", voice_cfg.read_pool_size);
    bk_printf("    voice_cfg.write_pool_size           0x%x\r\n", voice_cfg.write_pool_size);
#endif
    voice_cfg.spk_type = SPK_TYPE_ONBOARD;
    if (voice_cfg.spk_type == SPK_TYPE_ONBOARD) {
        onboard_speaker_stream_cfg_t onboard_spk_cfg =
            ONBOARD_SPEAKER_STREAM_CFG_DEFAULT();
        onboard_spk_cfg.task_prio = 1;
        if (pconfig->spk_volume) {
            if (pconfig->spk_volume <= 30) {
                // 0-30% 映射到 0-50%
                pconfig->spk_volume = pconfig->spk_volume * 50 / 30;
            } else {
                // 30-100% 映射到 50-70%
                pconfig->spk_volume = 50 + (pconfig->spk_volume - 30) * 20 / 70;
            }
            pconfig->spk_volume = (uint32_t)(pconfig->spk_volume * 0x3F / 100) ;
            // onboard_spk_cfg.dig_gain = (uint32_t)pconfig->spk_volume;
            // onboard_spk_cfg.ana_gain = 50;
        }
        onboard_spk_cfg.bits = pconfig->datebits;
        onboard_spk_cfg.sample_rate = sample_rate;
        /* one farme size, 20ms */
        onboard_spk_cfg.frame_size = frame_size;
        onboard_spk_cfg.task_stack = 4096;

        // voice_cfg.spk_cfg.onboard_spk_cfg = onboard_spk_cfg;
        if (voice_cfg.aec_en) {
            onboard_spk_cfg.multi_out_port_num = 1;
        } else {
            onboard_spk_cfg.multi_out_port_num = 0;
        }

        if (pconfig->card == TKL_AUDIO_TYPE_DUAL)
        {
            onboard_spk_cfg.clk_src = AUD_CLK_APLL;
        }

        voice_cfg.spk_cfg.onboard_spk_cfg = onboard_spk_cfg;

#if TKL_AUDIO_CONFIG_DUMP
        bk_printf("--- audio spk init\r\n");
        bk_printf("    onboard_spk_cfg.chl_num            0x%x\r\n", onboard_spk_cfg.chl_num);
        bk_printf("    onboard_spk_cfg.sample_rate        0x%x\r\n", onboard_spk_cfg.sample_rate);
        bk_printf("    onboard_spk_cfg.dig_gain           0x%x\r\n", onboard_spk_cfg.dig_gain);
        bk_printf("    onboard_spk_cfg.ana_gain           0x%x\r\n", onboard_spk_cfg.ana_gain);
        bk_printf("    onboard_spk_cfg.work_mode          0x%x\r\n", onboard_spk_cfg.work_mode);
        bk_printf("    onboard_spk_cfg.bits               0x%x\r\n", onboard_spk_cfg.bits);
        bk_printf("    onboard_spk_cfg.clk_src            0x%x\r\n", onboard_spk_cfg.clk_src);
        bk_printf("    onboard_spk_cfg.multi_in_port_num  0x%x\r\n", onboard_spk_cfg.multi_in_port_num);
        bk_printf("    onboard_spk_cfg.multi_out_port_num 0x%x\r\n", onboard_spk_cfg.multi_out_port_num);
        bk_printf("    onboard_spk_cfg.frame_size         0x%x\r\n", onboard_spk_cfg.frame_size);
        bk_printf("    onboard_spk_cfg.pool_length        0x%x\r\n", onboard_spk_cfg.pool_length);
        bk_printf("    onboard_spk_cfg.pool_play_thold    0x%x\r\n", onboard_spk_cfg.pool_play_thold);
        bk_printf("    onboard_spk_cfg.pool_pause_thold   0x%x\r\n", onboard_spk_cfg.pool_pause_thold);
        bk_printf("    onboard_spk_cfg.pa_ctrl_en         0x%x\r\n", onboard_spk_cfg.pa_ctrl_en);
        bk_printf("    onboard_spk_cfg.pa_ctrl_gpio       0x%x\r\n", onboard_spk_cfg.pa_ctrl_gpio);
        bk_printf("    onboard_spk_cfg.pa_on_level        0x%x\r\n", onboard_spk_cfg.pa_on_level);
        bk_printf("    onboard_spk_cfg.pa_on_delay        0x%x\r\n", onboard_spk_cfg.pa_on_delay);
        bk_printf("    onboard_spk_cfg.pa_off_delay       0x%x\r\n", onboard_spk_cfg.pa_off_delay);
        bk_printf("    onboard_spk_cfg.task_stack         0x%x\r\n", onboard_spk_cfg.task_stack);
        bk_printf("    onboard_spk_cfg.task_core          0x%x\r\n", onboard_spk_cfg.task_core);
        bk_printf("    onboard_spk_cfg.task_prio          0x%x\r\n", onboard_spk_cfg.task_prio);
#endif
    } else {
        uac_speaker_stream_cfg_t uac_spk_cfg = UAC_SPEAKER_STREAM_CFG_DEFAULT();
        if (pconfig->spk_volume) {
            uac_spk_cfg.volume = pconfig->spk_volume;
        }

        uac_spk_cfg.bits = pconfig->datebits;
        uac_spk_cfg.samp_rate = sample_rate;
        /* one farme size, 20ms */
        uac_spk_cfg.frame_size = frame_size;
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
        voice_read_cfg.max_read_size = frame_size;
        voice_read_cfg.voice_read_callback = voice_read_callback;
        voice_read_cfg.args = NULL;
        voice_read_cfg.mem_type = AUDIO_MEM_TYPE_PSRAM;
        voice_read_cfg.task_stack = 4096;
        g_voice_read_handle = bk_voice_read_init(&voice_read_cfg);
        if (!g_voice_read_handle) {
            bk_printf("%s, %d, voice read init fail\n", __func__, __LINE__);
            goto error;
        }
    }

    voice_write_cfg_t voice_write_cfg = VOICE_WRITE_CFG_DEFAULT();
    voice_write_cfg.voice_handle = g_voice_handle;
    voice_write_cfg.mem_type = AUDIO_MEM_TYPE_PSRAM;
    voice_write_cfg.node_size = 32768;
    g_voice_write_handle = bk_voice_write_init(&voice_write_cfg);
    if (!g_voice_write_handle) {
        bk_printf("%s, %d, voice write init fail\n", __func__, __LINE__);
        goto error;
    }


    audio_element_handle_t mic_str = NULL;
    mic_type_t mic_type = MIC_TYPE_INVALID;

    bk_voice_get_micstr(g_voice_handle, &mic_str);
    bk_voice_get_micstr_type(g_voice_handle, &mic_type);
    if (mic_str && mic_type == MIC_TYPE_ONBOARD)
    {
        onboard_mic_stream_set_analog_gain(mic_str, 0x08);
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

    bk_printf("audio init complete, sample: %d\r\n", s_audio_mic.samp_rate);

    return OPRT_OK;

error:
    __tkl_ai_error_cleanup();
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
    uint32_t level = 0;
    if (tkl_ai_status() == VOICE_STA_NONE) {
        bk_printf("tkl_ai_start fail, not init\n");
        return OPRT_COM_ERROR;
    }

    if (g_voice_handle) {
        if (BK_OK != bk_voice_start(g_voice_handle)) {
            bk_printf("%s, %d, voice start fail\n", __func__, __LINE__);
            goto __start_err;
        }
    }

    if (g_voice_read_handle) {
        if (BK_OK != bk_voice_read_start(g_voice_read_handle)) {
            bk_printf("%s, %d, voice read start fail\n", __func__, __LINE__);
            goto __start_err;
        }
    }

    if (g_voice_write_handle) {
        if (BK_OK != bk_voice_write_start(g_voice_write_handle)) {
            bk_printf("%s, %d, voice read start fail\n", __func__, __LINE__);
            goto __start_err;
        }
    }

    level = tkl_system_enter_critical();
    s_audio_init.audio_start = true;
    tkl_system_exit_critical(level);

    return OPRT_OK;

__start_err:
    __tkl_ai_error_cleanup();
    return OPRT_COM_ERROR;
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
    if (!s_audio_init.audio_init || !s_audio_init.audio_start) {
        return OPRT_RESOURCE_NOT_READY;
    }

    TUYA_GPIO_LEVEL_E level = 0;

    // 如果音量为0，关闭PA，不输出信号到喇叭
    if (vol == 0)
        level = board_spk_gpio_polarity;
    else
        level = (board_spk_gpio_polarity == 0)? 1: 0;

    tkl_gpio_write(board_spk_gpio, level);

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
OPERATE_RET tkl_ai_get_frame(int32_t card, TKL_AI_CHN_E chn,
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
OPERATE_RET tkl_ai_set_vqe(int32_t card, TKL_AI_CHN_E chn,
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
OPERATE_RET tkl_ai_get_vqe(int32_t card, TKL_AI_CHN_E chn,
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
OPERATE_RET tkl_ai_stop(int32_t card, TKL_AI_CHN_E chn)
{
    uint32_t level = 0;
    if (!s_audio_init.audio_init || !s_audio_init.audio_start) {
        return OPRT_OK;
    }
    if (tkl_ai_status() == VOICE_STA_NONE) {
        bk_printf("tkl_ai_start fail, not init\n");
        return OPRT_COM_ERROR;
    }
    if (g_voice_read_handle) {
        if (BK_OK != bk_voice_read_stop(g_voice_read_handle)) {
            bk_printf("%s, %d, voice read start fail\n", __func__, __LINE__);
        }
    }

    if (board_spk_gpio < 56) {
        tkl_gpio_write(board_spk_gpio, board_spk_gpio_polarity);
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
OPERATE_RET tkl_ai_uninit(void)
{
    int ret = OPRT_OK;
    uint32_t level = 0;

    if (!s_audio_init.audio_init) {
        return OPRT_OK;
    }

    __tkl_ai_error_cleanup();

    return ret;
}

/**
 * @param[in] count: config count
 *
 * @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
 */
OPERATE_RET tkl_ao_init(TKL_AUDIO_CONFIG_T *pconfig, int32_t count,
                        void **handle)
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
OPERATE_RET tkl_ao_start(int32_t card, TKL_AO_CHN_E chn, void *handle)
{

    if (g_voice_write_handle) {
        if (BK_OK != bk_voice_write_start(g_voice_write_handle)) {
            bk_printf("%s, %d, voice read start fail\n", __func__, __LINE__);
            return OPRT_COM_ERROR;
        }
    }
    return OPRT_OK;
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
OPERATE_RET tkl_ao_set_vol(int32_t card, TKL_AO_CHN_E chn, void *handle,
                           int32_t vol)
{
    if (!s_audio_init.audio_init || !s_audio_init.audio_start) {
        return OPRT_RESOURCE_NOT_READY;
    }
    g_audio_gain.spk_gain = vol;
    uint32_t volume = 0;
    if (card == TKL_AUDIO_TYPE_BOARD) {
        // 重新映射音量范围
        if (vol <= 30) {
            // 0-30% 映射到 0-50%
            vol = vol * 50 / 30;
        } else {
            // 30-100% 映射到 50-70%
            vol = 50 + (vol - 30) * 20 / 70;
        }
        volume =(uint32_t)(vol * 0x3F / 100) ;
        if (volume == 0) {
            bk_aud_dac_mute();
        } else {
            bk_aud_dac_unmute();
        }
    } else {
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
OPERATE_RET tkl_ao_get_vol(int32_t card, TKL_AO_CHN_E chn, void *handle,
                           int32_t *vol)
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
OPERATE_RET tkl_ao_put_frame(int32_t card, TKL_AO_CHN_E chn, void *handle,
                             TKL_AUDIO_FRAME_INFO_T *pframe)
{
    OPERATE_RET ret;
    if (pframe == NULL) {
        return OPRT_INVALID_PARM;
    }
    if (!s_audio_init.audio_init || !s_audio_init.audio_start) {
        return OPRT_RESOURCE_NOT_READY;
    }
    int offset = 0; // 偏移量
    int remaining_size = pframe->used_size; // 剩余数据大小
    int chunk_size = 0;

    if (remaining_size < 0) {
        return -1;
    }

    int spk_ringbuf_size = FRAME_SIZE_PER_20MS(s_audio_mic.samp_rate) * DRIVER_SPEAK_FIFO_FRAME_NUM;
                                            //
    // bk_printf("rate: %d, spk_ringbuf_size: %d, output size:%d\r\n",
    //         s_audio_mic.samp_rate, spk_ringbuf_size, remaining_size);

    // 分段调用 bk_aud_intf_write_spk_data
    while (remaining_size > 0) {
        // 计算当前要写入的数据大小
        chunk_size = (remaining_size > spk_ringbuf_size) ? spk_ringbuf_size : remaining_size;

write_spk_retry:
        ret = bk_voice_write_frame_data(g_voice_write_handle, (char *)(pframe->pbuf + offset), chunk_size);
        // ret = bk_voice_write_spk_data(g_voice_handle, (char *)(pframe->pbuf + offset), chunk_size);
        if (ret == 0) {
            tkl_system_sleep(20);
            goto write_spk_retry;
        }

        if (ret < 0) {
            os_printf("audio intf spk semaphore wait fail, ret:%d \r\n", ret);
            return ret;
        }
        // 更新偏移量和剩余大小
        offset += chunk_size;
        remaining_size -= chunk_size;
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
OPERATE_RET tkl_ao_clear_buffer(int32_t card, TKL_AO_CHN_E chn)
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
OPERATE_RET tkl_ao_stop(int32_t card, TKL_AO_CHN_E chn, void *handle)
{
    uint32_t level = 0;
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
            if (g_voice_write_handle) {
                ret = bk_voice_write_stop(g_voice_write_handle);
                if (ret != BK_OK) {
                    os_printf("audio intf spk stop fail, ret:%d \r\n", ret);
                    return ret;
                }
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
OPERATE_RET tkl_ao_uninit(void *handle)
{
    uint32_t level = 0;
    if (!s_audio_init.audio_init) {
        return OPRT_COM_ERROR;
    }
    int ret = OPRT_OK;
    if (s_audio_mic.card == TKL_AUDIO_TYPE_BOARD) {
        if (g_voice_write_handle) {
            bk_voice_write_stop(g_voice_write_handle);
        }
        if (g_voice_write_handle) {
            bk_voice_write_deinit(g_voice_write_handle);
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
OPERATE_RET tkl_ai_detect_get_result(int32_t card, TKL_MEDIA_DETECT_TYPE_E type,
                                     TKL_AUDIO_DETECT_RESULT_T *presult)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_ai_set_vad_aec_algorithm(aec_vad_process_fun user_process_fun)
{
    extern bk_err_t aec_v3_algorithm_set_user_process(aec_vad_process_fun user_process_fun);
    aec_v3_algorithm_set_user_process(user_process_fun);
}
#endif

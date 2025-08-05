#pragma once

#include <components/bk_audio/audio_algorithms/aec_algorithm.h>
#include <components/bk_audio/audio_algorithms/aec_v3_algorithm.h>
#include <components/bk_audio/audio_streams/raw_stream.h>
#include <components/bk_audio/audio_streams/onboard_mic_stream.h>
#include <components/bk_audio/audio_streams/onboard_dual_dmic_mic_stream.h>
#include <components/bk_audio/audio_streams/onboard_speaker_stream.h>
#include <components/bk_audio/audio_encoders/g711_encoder.h>
#include <components/bk_audio/audio_decoders/g711_decoder.h>
#include <components/bk_audio/audio_encoders/aac_encoder.h>
#include <components/bk_audio/audio_decoders/aac_decoder.h>
#include <components/bk_audio/audio_streams/uac_mic_stream.h>
#include <components/bk_audio/audio_streams/uac_speaker_stream.h>

#include "avdk_types.h"


#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

/**
 * voice status
 */
typedef enum
{
    VOICE_STA_NONE = 0,
    VOICE_STA_IDLE,
    VOICE_STA_RUNNING,
    VOICE_STA_STOPPING,
    VOICE_STA_STOPED,
} voice_sta_t;

/**
 * voice event
 */
typedef enum
{
    VOC_EVT_NONE = 0,
    VOC_EVT_MIC_NOT_SUPPORT,
    VOC_EVT_SPK_NOT_SUPPORT,
    VOC_EVT_ERROR_UNKNOW,
    VOC_EVT_STOP,
} vioce_evt_t;


typedef bk_err_t (*voice_event_handle)(vioce_evt_t, void *, void *);

typedef struct voice *voice_handle_t;

typedef struct
{
    uint32_t                samp_rate;
    uint32_t                bitrate;
    uint32_t                frame_in_size;
    uint32_t                frame_out_size;
    uint8_t                 bits;
    uint8_t                 frame_in_ms;
    uint8_t                 vbr;
    uint8_t                 channels;
}audio_codec_common_t;

typedef struct
{
    mic_type_t              mic_type;
    union
    {
        onboard_mic_stream_cfg_t            onboard_mic_cfg;
        onboard_dual_dmic_mic_stream_cfg_t  onboard_dual_dmic_mic_cfg;
        uac_mic_stream_cfg_t                uac_mic_cfg;
    } mic_cfg;

    bool                    aec_en;
    uint8_t                 aec_ver;
    union
    {
        aec_algorithm_cfg_t    aec_alg_cfg;
        aec_v3_algorithm_cfg_t aec_v3_alg_cfg;
        uint8_t                reserve;
    } aec_cfg;
    
    bool                    enc_en;
    audio_enc_type_t        enc_type;
    audio_codec_common_t    enc_common;
    union
    {
        g711_encoder_cfg_t    g711_enc_cfg;
#if CONFIG_VOICE_SERVICE_AAC_ENCODER
        aac_encoder_cfg_t     aac_enc_cfg;
#endif
        uint8_t               pcm_enc_cfg;
    } enc_cfg;

    uint32_t                read_pool_size;     /*!< the size(byte) of pool save mic data that has been encode */
    uint32_t                write_pool_size;     /*!< the size(byte) of pool save speaker data that has not been decode */

    bool                    dec_en;
    audio_dec_type_t        dec_type;
    audio_codec_common_t    dec_common;
    union
    {
        g711_decoder_cfg_t    g711_dec_cfg;
#if CONFIG_VOICE_SERVICE_AAC_DECODER
        aac_decoder_cfg_t     aac_dec_cfg;
#endif
        uint8_t               pcm_dec_cfg;
    } dec_cfg;
    spk_type_t              spk_type;
    union
    {
        onboard_speaker_stream_cfg_t    onboard_spk_cfg;
        uac_speaker_stream_cfg_t        uac_spk_cfg;
    } spk_cfg;

    voice_event_handle      event_handle;   /*!< voice event handle callback */
    void *                  args;           /*!< the parameter of event_handle func */
} voice_cfg_t;

/* voice call through onboard mic and onboard speaker
 * mic: onboard mic
 * speaker: onboard speaker
 * AEC: ON
 * sample rate: 8000Hz
 * encoder: g711a
 * decoder: g711a
 */
#define VOICE_BY_ONBOARD_MIC_SPK_CFG_DEFAULT() {                \
    .mic_type = MIC_TYPE_ONBOARD,                               \
    .mic_cfg.onboard_mic_cfg = {                                \
        .adc_cfg = {                                            \
           .chl_num = 1,                                        \
           .bits = 16,                                          \
           .sample_rate = 8000,                                 \
           .dig_gain = 0x28,                                    \
           .ana_gain = 0x8,                                     \
           .mode = AUD_ADC_MODE_DIFFEN,                         \
           .clk_src = AUD_CLK_XTAL,                             \
        },                                                      \
        .frame_size = 320,                                      \
        .out_block_size = 320,                                  \
        .out_block_num = 2,                                     \
        .multi_out_port_num = 0,                                \
        .task_stack = ONBOARD_MIC_STREAM_TASK_STACK,            \
        .task_core = ONBOARD_MIC_STREAM_TASK_CORE,              \
        .task_prio = ONBOARD_SPEAKER_STREAM_TASK_PRIO,          \
    },                                                          \
    .aec_en = true,                                             \
    .aec_ver = 1,                                               \
    .aec_cfg.aec_alg_cfg = {                                    \
        .task_stack = AEC_ALGORITHM_TASK_STACK,                 \
        .task_core = AEC_ALGORITHM_TASK_CORE,                   \
        .task_prio = AEC_ALGORITHM_TASK_PRIO,                   \
        .aec_cfg = {                                            \
            .mode = AEC_MODE_SOFTWARE,                          \
            .fs = AEC_ALGORITHM_FS,                             \
            .delay_points = AEC_DELAY_POINTS,                   \
            .ec_depth = AEC_ALGORITHM_EC_DEPTH,                 \
            .TxRxThr = AEC_ALGORITHM_TXRXTHR,                   \
            .TxRxFlr = AEC_ALGORITHM_TXRXFLR,                   \
            .ref_scale = AEC_ALGORITHM_REF_SCALE,               \
            .ns_level = AEC_ALGORITHM_NS_LEVEL,                 \
            .ns_para = AEC_ALGORITHM_NS_PARA,                   \
        },                                                      \
        .out_block_num = 1,                                     \
        .multi_out_port_num = 0,                                \
    },                                                          \
    .enc_en = true,                                             \
    .enc_type = AUDIO_ENC_TYPE_G711A,                           \
    .enc_cfg.g711_enc_cfg = {                                   \
        .buf_sz = G711_ENCODER_BUFFER_SIZE,                     \
        .out_block_size = 160,                                  \
        .out_block_num = 1,                                     \
        .task_stack = G711_ENCODER_TASK_STACK,                  \
        .task_core = G711_ENCODER_TASK_CORE,                    \
        .task_prio = G711_ENCODER_TASK_PRIO,                    \
        .enc_mode = G711_ENC_MODE_A_LOW,                        \
    },                                                          \
    .read_pool_size = 160,                                      \
    .write_pool_size = 320,                                     \
    .dec_en = true,                                             \
    .dec_type = AUDIO_DEC_TYPE_G711A,                           \
    .dec_cfg.g711_dec_cfg = {                                   \
        .buf_sz = G711_DECODER_BUFFER_SIZE,                     \
        .out_block_size = 320,                                  \
        .out_block_num = 1,                                     \
        .task_stack = G711_DECODER_TASK_STACK,                  \
        .task_core = G711_DECODER_TASK_CORE,                    \
        .task_prio = G711_DECODER_TASK_PRIO,                    \
        .dec_mode = G711_DEC_MODE_A_LOW,                        \
    },                                                          \
    .spk_type = SPK_TYPE_ONBOARD,                               \
    .spk_cfg.onboard_spk_cfg = {                                \
        .chl_num = 1,                                           \
        .sample_rate = 8000,                                    \
        .dig_gain = 0x2d,                                       \
        .ana_gain = 0x07,                                       \
        .work_mode = AUD_DAC_WORK_MODE_DIFFEN,                  \
        .bits = 16,                                             \
        .clk_src = AUD_CLK_XTAL,                                \
        .multi_out_port_num = 1,                                \
        .frame_size = 320,                                      \
        .pool_length = 0,                                       \
        .pool_play_thold = 0,                                   \
        .pool_pause_thold = 0,                                  \
        .task_stack = ONBOARD_SPEAKER_STREAM_TASK_STACK,        \
        .task_core = ONBOARD_SPEAKER_STREAM_TASK_CORE,          \
        .task_prio = ONBOARD_SPEAKER_STREAM_TASK_PRIO,          \
    },                                                          \
    .event_handle = NULL,                                       \
    .args = NULL,                                               \
}

/* voice call through uac mic and uac speaker
 * mic: uac mic
 * speaker: uac speaker
 * AEC: ON
 * sample rate: 8000Hz
 * encoder: g711a
 * decoder: g711a
 */
#define VOICE_BY_UAC_MIC_SPK_CFG_DEFAULT() {                   \
    .mic_type = MIC_TYPE_UAC,                                  \
    .mic_cfg.uac_mic_cfg = {                                   \
        .port_index = USB_HUB_PORT_1,                          \
        .format = AUDIO_FORMAT_PCM,                            \
        .chl_num = 1,                                          \
        .bits = 16,                                            \
        .samp_rate = 8000,                                     \
        .frame_size = 320,                                     \
        .out_block_size = 320,                                 \
        .out_block_num = 1,                                    \
        .auto_connect = true,                                  \
        .multi_out_port_num = 0,                               \
        .task_stack = UAC_MIC_STREAM_TASK_STACK,               \
        .task_core = UAC_MIC_STREAM_TASK_CORE,                 \
        .task_prio = UAC_MIC_STREAM_TASK_PRIO,                 \
    },                                                         \
    .aec_en = true,                                            \
    .aec_ver = 1,                                              \
    .aec_cfg.aec_alg_cfg = {                                   \
        .task_stack = AEC_ALGORITHM_TASK_STACK,                \
        .task_core = AEC_ALGORITHM_TASK_CORE,                  \
        .task_prio = AEC_ALGORITHM_TASK_PRIO,                  \
        .aec_cfg = {                                           \
            .mode = AEC_MODE_SOFTWARE,                         \
            .fs = AEC_ALGORITHM_FS,                            \
            .delay_points = 517,                               \
            .ec_depth = AEC_ALGORITHM_EC_DEPTH,                \
            .TxRxThr = AEC_ALGORITHM_TXRXTHR,                  \
            .TxRxFlr = AEC_ALGORITHM_TXRXFLR,                  \
            .ref_scale = AEC_ALGORITHM_REF_SCALE,              \
            .ns_level = AEC_ALGORITHM_NS_LEVEL,                \
            .ns_para = AEC_ALGORITHM_NS_PARA,                  \
        },                                                     \
        .out_block_num = 1,                                    \
        .multi_out_port_num = 0,                               \
    },                                                         \
    .enc_en = true,                                            \
    .enc_type = AUDIO_ENC_TYPE_G711A,                          \
    .enc_cfg.g711_enc_cfg = {                                  \
        .buf_sz = G711_ENCODER_BUFFER_SIZE,                    \
        .out_block_size = 160,                                 \
        .out_block_num = 1,                                    \
        .task_stack = G711_ENCODER_TASK_STACK,                 \
        .task_core = G711_ENCODER_TASK_CORE,                   \
        .task_prio = G711_ENCODER_TASK_PRIO,                   \
        .enc_mode = G711_ENC_MODE_A_LOW,                       \
    },                                                         \
    .read_pool_size = 160,                                     \
    .write_pool_size = 320,                                    \
    .dec_en = true,                                            \
    .dec_type = AUDIO_DEC_TYPE_G711A,                          \
    .dec_cfg.g711_dec_cfg = {                                  \
        .buf_sz = G711_DECODER_BUFFER_SIZE,                    \
        .out_block_size = 320,                                 \
        .out_block_num = 1,                                    \
        .task_stack = G711_DECODER_TASK_STACK,                 \
        .task_core = G711_DECODER_TASK_CORE,                   \
        .task_prio = G711_DECODER_TASK_PRIO,                   \
        .dec_mode = G711_DEC_MODE_A_LOW,                       \
    },                                                         \
    .spk_type = SPK_TYPE_UAC,                                  \
    .spk_cfg.uac_spk_cfg = {                                   \
        .port_index = USB_HUB_PORT_1,                          \
        .format = AUDIO_FORMAT_PCM,                            \
        .chl_num = 1,                                          \
        .bits = 16,                                            \
        .samp_rate = 8000,                                     \
        .frame_size = 320,                                     \
        .volume = 5,                                           \
        .auto_connect = true,                                  \
        .multi_out_port_num = 1,                               \
        .pool_size = 0,                                        \
        .pool_play_thold = 0,                                  \
        .pool_pause_thold = 0,                                 \
        .task_stack = UAC_SPEAKER_STREAM_TASK_STACK,           \
        .task_core = UAC_SPEAKER_STREAM_TASK_CORE,             \
        .task_prio = UAC_SPEAKER_STREAM_TASK_PRIO,             \
    },                                                         \
    .event_handle = NULL,                                      \
    .args = NULL,                                              \
}

#if CONFIG_VOICE_SERVICE_AAC_ENCODER && CONFIG_VOICE_SERVICE_AAC_DECODER
/* voice call through onboard mic and onboard speaker
 * mic: onboard mic
 * speaker: onboard speaker
 * AEC: ON
 * sample rate: 8000Hz
 * encoder: aac
 * decoder: aac
 */
#define VOICE_BY_ONBOARD_MIC_SPK_AAC_CFG_DEFAULT() {                \
        .mic_type = MIC_TYPE_ONBOARD,                               \
        .mic_cfg.onboard_mic_cfg = {                                \
            .adc_cfg = {                                            \
                .sample_rate = 8000,                                \
                .adc_samp_edge = AUD_ADC_SAMP_EDGE_RISING,          \
                .clk_src = AUD_CLK_APLL,                            \
                .chl_cfg = {                                        \
                    {                                               \
                        .dig_gain = 0x4000,                         \
                        .ana_gain = 0x07,                           \
                        .adc_mode = AUD_ADC_MODE_DIFFEN,            \
                        .bits = 16,                                 \
                    },                                              \
                    {                                               \
                        .dig_gain = 0x4000,                         \
                        .ana_gain = 0x07,                           \
                        .adc_mode = AUD_ADC_MODE_DIFFEN,            \
                        .bits = 16,                                 \
                    },                                              \
                    {                                               \
                        .dig_gain = 0x4000,                         \
                        .ana_gain = 0x07,                           \
                        .adc_mode = AUD_ADC_MODE_DIFFEN,            \
                        .bits = 16,                                 \
                    },                                              \
                },                                                  \
            },                                                      \
            .chl_num = 1,                                           \
            .frame_size = 320,                                      \
            .out_block_size = 320,                                  \
            .out_block_num = 2,                                     \
            .multi_out_port_num = 0,                                \
            .task_stack = ONBOARD_MIC_STREAM_TASK_STACK,            \
            .task_core = ONBOARD_MIC_STREAM_TASK_CORE,              \
            .task_prio = ONBOARD_SPEAKER_STREAM_TASK_PRIO,          \
        },                                                          \
        .aec_en = true,                                             \
        .aec_ver = 1,                                               \
        .aec_cfg.aec_alg_cfg = {                                    \
            .task_stack = AEC_ALGORITHM_TASK_STACK,                 \
            .task_core = AEC_ALGORITHM_TASK_CORE,                   \
            .task_prio = AEC_ALGORITHM_TASK_PRIO,                   \
            .aec_cfg = {                                            \
                .mode = AEC_MODE_SOFTWARE,                          \
                .fs = AEC_ALGORITHM_FS,                             \
                .delay_points = AEC_DELAY_POINTS,                   \
                .ec_depth = AEC_ALGORITHM_EC_DEPTH,                 \
                .TxRxThr = AEC_ALGORITHM_TXRXTHR,                   \
                .TxRxFlr = AEC_ALGORITHM_TXRXFLR,                   \
                .ref_scale = AEC_ALGORITHM_REF_SCALE,               \
                .ns_level = AEC_ALGORITHM_NS_LEVEL,                 \
                .ns_para = AEC_ALGORITHM_NS_PARA,                   \
            },                                                      \
            .out_block_num = 1,                                     \
            .multi_out_port_num = 0,                                \
        },                                                          \
        .enc_en = true,                                             \
        .enc_type = AUDIO_ENC_TYPE_AAC,                             \
        .enc_cfg.aac_enc_cfg = {                                    \
            .chl_num            = AAC_ENCODER_CHL_NUM,              \
            .samp_rate          = AAC_ENCODER_SAMP_RATE,            \
            .bits               = AAC_ENCODER_BITS,                 \
            .modules            = AAC_ENCODER_MODULES,              \
            .aot                = AAC_ENCODER_AOT,                  \
            .bitrate            = AAC_ENCODER_BITRATE,              \
            .bitrate_mode       = AAC_ENCODER_BITRATE_MODE,         \
            .sbr_mode           = AAC_ENCODER_SBR_MODE,             \
            .granule_length     = AAC_ENCODER_GRANULE_LENGTH,       \
            .chl_order          = AAC_ENCODER_CHL_ORDER,            \
            .afterburner_en     = AAC_ENCODER_AFTERBURNER_EN,       \
            .transport_type     = AAC_ENCODER_TRANSPORT_TYPE,       \
            .buffer_len         = AAC_ENCODER_BUFFER_LEN,           \
            .in_pool_len        = AAC_ENCODER_IN_POOL_LEN,          \
            .out_buffer_len     = AAC_ENCODER_OUT_BUFFER_LEN,       \
            .out_rb_size        = AAC_ENCODER_OUT_RB_SIZE,          \
            .task_stack         = AAC_ENCODER_TASK_STACK,           \
            .task_core          = AAC_ENCODER_TASK_CORE,            \
            .task_prio          = AAC_ENCODER_TASK_PRIO,            \
        },                                                          \
        .read_pool_size = AAC_ENCODER_OUT_RB_SIZE,                  \
        .write_pool_size = (AAC_DECODER_MAIN_BUFF_SIZE * 2),        \
        .dec_en = true,                                             \
        .dec_type = AUDIO_DEC_TYPE_AAC,                             \
        .dec_cfg.aac_dec_cfg = {                                    \
            .main_buff_size     = AAC_DECODER_MAIN_BUFF_SIZE,       \
            .out_pcm_buff_size  = AAC_DECODER_OUT_PCM_BUFF_SIZE,    \
            .out_rb_size        = AAC_DECODER_OUT_PCM_BUFF_SIZE,    \
            .task_stack         = AAC_DECODER_TASK_STACK,           \
            .task_core          = AAC_DECODER_TASK_CORE,            \
            .task_prio          = AAC_DECODER_TASK_PRIO,            \
        },                                                          \
        .spk_type = SPK_TYPE_ONBOARD,                               \
        .spk_cfg.onboard_spk_cfg = {                                \
            .chl_num = 1,                                           \
            .dac_source = AUD_DAC_SOURCE_CALL,                      \
            .dac_source_gain = 0x10000000,                          \
            .sample_rate = 8000,                                    \
            .dig_gain = 0x7000000,                                  \
            .work_mode = AUD_DAC_WORK_MODE_DIFFEN,                  \
            .bits = 16,                                             \
            .clk_src = AUD_CLK_APLL,                                \
            .multi_out_port_num = 1,                                \
            .frame_size = 1920,                                     \
            .pool_length = 0,                                       \
            .pool_play_thold = 0,                                   \
            .pool_pause_thold = 0,                                  \
            .task_stack = ONBOARD_SPEAKER_STREAM_TASK_STACK,        \
            .task_core = ONBOARD_SPEAKER_STREAM_TASK_CORE,          \
            .task_prio = ONBOARD_SPEAKER_STREAM_TASK_PRIO,          \
        },                                                          \
        .event_handle = NULL,                                       \
        .args = NULL,                                               \
    }
#endif  //CONFIG_VOICE_SERVICE_AAC_ENCODER && CONFIG_VOICE_SERVICE_AAC_DECODER

/* voice call through onboard dual dmic and onboard speaker
 * mic: onboard dual dmic
 * speaker: onboard speaker
 * AEC: ON
 * sample rate: 16000Hz
 * encoder: g711a
 * decoder: g711a
 */
#define VOICE_BY_ONBOARD_DUAL_DMIC_MIC_SPK_CFG_DEFAULT() {      \
    .mic_type = MIC_TYPE_ONBOARD_DUAL_DMIC_MIC,                 \
    .mic_cfg.onboard_dual_dmic_mic_cfg = {                      \
        .adc_cfg = {                                            \
           .chl_num = 1,                                        \
           .bits = 16,                                          \
           .sample_rate = 16000,                                \
           .dig_gain = 0x28,                                    \
           .ana_gain = 0x8,                                     \
           .mode = AUD_ADC_MODE_DIFFEN,                         \
           .clk_src = AUD_CLK_APLL,                             \
        },                                                      \
        .frame_size = 640,                                      \
        .out_block_size = 640,                                  \
        .out_block_num = 2,                                     \
        .multi_out_port_num = 0,                                \
        .task_stack = ONBOARD_DUAL_DMIC_MIC_STREAM_TASK_STACK,  \
        .task_core = ONBOARD_DUAL_DMIC_MIC_STREAM_TASK_CORE,    \
        .task_prio = ONBOARD_SPEAKER_STREAM_TASK_PRIO,          \
        .ref_mode = 0,                                          \
        .dual_dmic = 1,                                         \
        .dual_dmic_sgl_out = 0,                                 \
    },                                                          \
    .aec_en = true,                                             \
    .aec_ver = 3,                                               \
    .aec_cfg.aec_v3_alg_cfg = {                                 \
        .task_stack = AEC_V3_ALGORITHM_TASK_STACK,              \
        .task_core = AEC_V3_ALGORITHM_TASK_CORE,                \
        .task_prio = AEC_V3_ALGORITHM_TASK_PRIO,                \
        .aec_cfg = {                                            \
            .mode = AEC_V3_MODE_SOFTWARE,                       \
            .fs = AEC_V3_ALGORITHM_FS,                          \
            .init_flags = AEC_V3_ALGORITHM_INIT_FLAG,           \
            .delay_points = AEC_V3_DELAY_POINTS,                \
            .ec_depth = AEC_V3_ALGORITHM_EC_DEPTH,              \
            .ref_scale = AEC_V3_ALGORITHM_REF_SCALE,            \
            .voice_vol = AEC_V3_ALGORITHM_VOL,                  \
            .ns_type = NS_AI,                                   \
            .ns_filter = AEC_V3_ALGORITHM_NS_FILTER,            \
            .ns_level = AEC_V3_ALGORITHM_NS_LEVEL,              \
            .ns_para = AEC_V3_ALGORITHM_NS_PARA,                \
            .drc = AEC_V3_ALGORITHM_DRC,                        \
            .ec_filter = AEC_V3_ALGORITHM_EC_FILTER,            \
        },                                                      \
        .vad_cfg = {                                            \
            .vad_enable = 1,                                    \
            .vad_start_threshold = 480,                         \
            .vad_stop_threshold = 960,                          \
            .vad_silence_threshold = 320,                       \
            .vad_eng_threshold =2000,                           \
            .vad_bad_frame = AEC_V3_VAD_BAD_FRAME_NUM,          \
            .vad_buf_size = 15360,                              \
            .vad_frame_size = 640,                              \
        },                                                      \
        .out_block_size = 640,                                  \
        .out_block_num = AEC_V3_ALGORITHM_OUT_BLOCK_NUM,        \
        .multi_out_port_num = 0,                                \
        .dual_ch = 1,                                           \
    },                                                          \
    .enc_en = true,                                             \
    .enc_type = AUDIO_ENC_TYPE_G711A,                           \
    .enc_cfg.g711_enc_cfg = {                                   \
        .buf_sz = G711_ENCODER_BUFFER_SIZE,                     \
        .out_block_size = 320,                                  \
        .out_block_num = 1,                                     \
        .task_stack = G711_ENCODER_TASK_STACK,                  \
        .task_core = G711_ENCODER_TASK_CORE,                    \
        .task_prio = G711_ENCODER_TASK_PRIO,                    \
        .enc_mode = G711_ENC_MODE_A_LOW,                        \
    },                                                          \
    .read_pool_size = 320,                                      \
    .write_pool_size = 640,                                     \
    .dec_en = true,                                             \
    .dec_type = AUDIO_DEC_TYPE_G711A,                           \
    .dec_cfg.g711_dec_cfg = {                                   \
        .buf_sz = G711_DECODER_BUFFER_SIZE,                     \
        .out_block_size = 640,                                  \
        .out_block_num = 1,                                     \
        .task_stack = G711_DECODER_TASK_STACK,                  \
        .task_core = G711_DECODER_TASK_CORE,                    \
        .task_prio = G711_DECODER_TASK_PRIO,                    \
        .dec_mode = G711_DEC_MODE_A_LOW,                        \
    },                                                          \
    .spk_type = SPK_TYPE_ONBOARD,                               \
    .spk_cfg.onboard_spk_cfg = {                                \
        .chl_num = 1,                                           \
        .sample_rate = 16000,                                   \
        .dig_gain = 0x2d,                                       \
        .ana_gain = 0x07,                                       \
        .work_mode = AUD_DAC_WORK_MODE_DIFFEN,                  \
        .bits = 16,                                             \
        .clk_src = AUD_CLK_APLL,                                \
        .multi_out_port_num = 1,                                \
        .frame_size = 640,                                      \
        .pool_length = 0,                                       \
        .pool_play_thold = 0,                                   \
        .pool_pause_thold = 0,                                  \
        .task_stack = ONBOARD_SPEAKER_STREAM_TASK_STACK,        \
        .task_core = ONBOARD_SPEAKER_STREAM_TASK_CORE,          \
        .task_prio = ONBOARD_SPEAKER_STREAM_TASK_PRIO,          \
    },                                                          \
    .event_handle = NULL,                                       \
    .args = NULL,                                               \
}

#ifdef  __cplusplus
}
#endif//__cplusplus

/**
 * @}
 */
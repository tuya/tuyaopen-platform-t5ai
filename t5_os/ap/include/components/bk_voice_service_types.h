#pragma once

#include <components/bk_audio/audio_algorithms/aec_v3_algorithm.h>
#include <components/bk_audio/audio_algorithms/eq_algorithm.h>
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
#include <components/bk_audio/audio_encoders/g722_encoder.h>
#include <components/bk_audio/audio_decoders/g722_decoder.h>
#include <components/bk_audio/audio_encoders/opus_enc.h>
#include <components/bk_audio/audio_decoders/opus_dec.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>


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
    union
    {
        aec_v3_algorithm_cfg_t aec_alg_cfg;
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
#if CONFIG_VOICE_SERVICE_G722_ENCODER
        g722_encoder_cfg_t    g722_enc_cfg;
#endif
#if CONFIG_VOICE_SERVICE_OPUS_ENCODER
        opus_enc_cfg_t        opus_enc_cfg;
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
#if CONFIG_VOICE_SERVICE_G722_DECODER
        g722_decoder_cfg_t    g722_dec_cfg;
#endif
#if CONFIG_VOICE_SERVICE_OPUS_DECODER
        opus_dec_cfg_t        opus_dec_cfg;
#endif
        uint8_t               pcm_dec_cfg;
    } dec_cfg;
    spk_type_t              spk_type;
    union
    {
        onboard_speaker_stream_cfg_t    onboard_spk_cfg;
        uac_speaker_stream_cfg_t        uac_spk_cfg;
    } spk_cfg;
#if CONFIG_VOICE_SERVICE_EQ
    bool                    eq_en;
    union
    {
        eq_algorithm_cfg_t  eq_alg_cfg;
    } eq_cfg;
#endif

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
#define VOICE_BY_ONBOARD_MIC_SPK_CFG_DEFAULT() DEFAULT_VOICE_BY_ONBOARD_MIC_SPK_CONFIG()

 #define DEFAULT_VOICE_BY_ONBOARD_MIC_SPK_CONFIG() {            \
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
    .aec_cfg.aec_alg_cfg = {                                    \
        .task_stack = AEC_V3_ALGORITHM_TASK_STACK,              \
        .task_core = AEC_V3_ALGORITHM_TASK_CORE,                \
        .task_prio = AEC_V3_ALGORITHM_TASK_PRIO,                \
        .aec_cfg = {                                            \
            .mode = AEC_MODE_SOFTWARE,                          \
            .fs = 8000,                                         \
            .init_flags = AEC_V3_ALGORITHM_INIT_FLAG,           \
            .delay_points = AEC_V3_DELAY_POINTS,                \
            .ec_depth = AEC_V3_ALGORITHM_EC_DEPTH,              \
            .ref_scale = AEC_V3_ALGORITHM_REF_SCALE,            \
            .voice_vol = AEC_V3_ALGORITHM_VOL,                  \
            .ns_type = NS_TRADITION,                            \
            .ns_filter = AEC_V3_ALGORITHM_NS_FILTER,            \
            .ns_level = AEC_V3_ALGORITHM_NS_LEVEL,              \
            .ns_para = AEC_V3_ALGORITHM_NS_PARA,                \
            .drc = AEC_V3_ALGORITHM_DRC,                        \
            .ec_filter = AEC_V3_ALGORITHM_EC_FILTER,            \
        },                                                      \
        .vad_cfg = {                                            \
            .vad_enable = 0,                                    \
            .vad_start_threshold = 480,                         \
            .vad_stop_threshold = 960,                          \
            .vad_silence_threshold = 320,                       \
            .vad_eng_threshold =2000,                           \
            .vad_bad_frame = AEC_V3_VAD_BAD_FRAME_NUM,          \
            .vad_buf_size = 15360,                              \
            .vad_frame_size = 320,                              \
        },                                                      \
        .out_block_size = 320,                                  \
        .out_block_num = AEC_V3_ALGORITHM_OUT_BLOCK_NUM,        \
        .multi_out_port_num = 0,                                \
        .dual_ch = 0,                                           \
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
        .multi_in_port_num = 0,                                 \
        .multi_out_port_num = 1,                                \
        .frame_size = 320,                                      \
        .pool_length = 0,                                       \
        .pool_play_thold = 0,                                   \
        .pool_pause_thold = 0,                                  \
        .pa_ctrl_en = false,                                    \
        .pa_ctrl_gpio = 0,                                      \
        .pa_on_level = 0,                                       \
        .pa_on_delay = 0,                                       \
        .pa_off_delay = 0,                                      \
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
#define VOICE_BY_UAC_MIC_SPK_CFG_DEFAULT() DEFAULT_VOICE_BY_UAC_MIC_SPK_CONFIG() 

 #define DEFAULT_VOICE_BY_UAC_MIC_SPK_CONFIG() {               \
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
    .aec_cfg.aec_alg_cfg = {                                   \
        .task_stack = AEC_V3_ALGORITHM_TASK_STACK,             \
        .task_core = AEC_V3_ALGORITHM_TASK_CORE,               \
        .task_prio = AEC_V3_ALGORITHM_TASK_PRIO,               \
        .aec_cfg = {                                           \
            .mode = AEC_MODE_SOFTWARE,                         \
            .fs = 8000,                                        \
            .init_flags = AEC_V3_ALGORITHM_INIT_FLAG,          \
            .delay_points = AEC_V3_DELAY_POINTS,               \
            .ec_depth = AEC_V3_ALGORITHM_EC_DEPTH,             \
            .ref_scale = AEC_V3_ALGORITHM_REF_SCALE,           \
            .voice_vol = AEC_V3_ALGORITHM_VOL,                 \
            .ns_type = NS_TRADITION,                           \
            .ns_filter = AEC_V3_ALGORITHM_NS_FILTER,           \
            .ns_level = AEC_V3_ALGORITHM_NS_LEVEL,             \
            .ns_para = AEC_V3_ALGORITHM_NS_PARA,               \
            .drc = AEC_V3_ALGORITHM_DRC,                       \
            .ec_filter = AEC_V3_ALGORITHM_EC_FILTER,           \
        },                                                     \
        .vad_cfg = {                                           \
            .vad_enable = 0,                                   \
            .vad_start_threshold = 480,                        \
            .vad_stop_threshold = 960,                         \
            .vad_silence_threshold = 320,                      \
            .vad_eng_threshold =2000,                          \
            .vad_bad_frame = AEC_V3_VAD_BAD_FRAME_NUM,         \
            .vad_buf_size = 15360,                             \
            .vad_frame_size = 320,                             \
        },                                                     \
        .out_block_size = 320,                                 \
        .out_block_num = AEC_V3_ALGORITHM_OUT_BLOCK_NUM,       \
        .multi_out_port_num = 0,                               \
        .dual_ch = 0,                                          \
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
#define VOICE_BY_ONBOARD_MIC_SPK_AAC_CFG_DEFAULT() DEFAULT_VOICE_BY_ONBOARD_MIC_SPK_AAC_CONFIG() 

 #define DEFAULT_VOICE_BY_ONBOARD_MIC_SPK_AAC_CONFIG() {            \
        .mic_type = MIC_TYPE_ONBOARD,                               \
        .mic_cfg.onboard_mic_cfg = {                                \
            .adc_cfg = {                                            \
                .chl_num = 1,                                       \
                .bits = 16,                                         \
                .sample_rate = 8000,                                \
                .dig_gain = 0x28,                                   \
                .ana_gain = 0x8,                                    \
                .mode = AUD_ADC_MODE_DIFFEN,                        \
                .clk_src = AUD_CLK_XTAL,                            \
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
        .aec_cfg.aec_alg_cfg = {                                    \
            .task_stack = AEC_V3_ALGORITHM_TASK_STACK,              \
            .task_core = AEC_V3_ALGORITHM_TASK_CORE,                \
            .task_prio = AEC_V3_ALGORITHM_TASK_PRIO,                \
            .aec_cfg = {                                            \
                .mode = AEC_MODE_SOFTWARE,                          \
                .fs = 8000,                                         \
                .init_flags = AEC_V3_ALGORITHM_INIT_FLAG,           \
                .delay_points = AEC_V3_DELAY_POINTS,                \
                .ec_depth = AEC_V3_ALGORITHM_EC_DEPTH,              \
                .ref_scale = AEC_V3_ALGORITHM_REF_SCALE,            \
                .voice_vol = AEC_V3_ALGORITHM_VOL,                  \
                .ns_type = NS_TRADITION,                            \
                .ns_filter = AEC_V3_ALGORITHM_NS_FILTER,            \
                .ns_level = AEC_V3_ALGORITHM_NS_LEVEL,              \
                .ns_para = AEC_V3_ALGORITHM_NS_PARA,                \
                .drc = AEC_V3_ALGORITHM_DRC,                        \
                .ec_filter = AEC_V3_ALGORITHM_EC_FILTER,            \
            },                                                      \
            .vad_cfg = {                                            \
                .vad_enable = 0,                                    \
                .vad_start_threshold = 480,                         \
                .vad_stop_threshold = 960,                          \
                .vad_silence_threshold = 320,                       \
                .vad_eng_threshold =2000,                           \
                .vad_bad_frame = AEC_V3_VAD_BAD_FRAME_NUM,          \
                .vad_buf_size = 15360,                              \
                .vad_frame_size = 320,                              \
            },                                                      \
            .out_block_size = 320,                                  \
            .out_block_num = AEC_V3_ALGORITHM_OUT_BLOCK_NUM,        \
            .multi_out_port_num = 0,                                \
            .dual_ch = 0,                                           \
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
            .sample_rate = 8000,                                    \
            .dig_gain = 0x2d,                                       \
            .ana_gain = 0x07,                                       \
            .work_mode = AUD_DAC_WORK_MODE_DIFFEN,                  \
            .bits = 16,                                             \
            .clk_src = AUD_CLK_XTAL,                                \
            .multi_in_port_num = 0,                                 \
            .multi_out_port_num = 1,                                \
            .frame_size = 320,                                      \
            .pool_length = 0,                                       \
            .pool_play_thold = 0,                                   \
            .pool_pause_thold = 0,                                  \
            .pa_ctrl_en = false,                                    \
            .pa_ctrl_gpio = 0,                                      \
            .pa_on_level = 0,                                       \
            .pa_on_delay = 0,                                       \
            .pa_off_delay = 0,                                      \
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
#define VOICE_BY_ONBOARD_DUAL_DMIC_MIC_SPK_CFG_DEFAULT() DEFAULT_VOICE_BY_ONBOARD_DUAL_DMIC_MIC_SPK_CONFIG() 

 #define DEFAULT_VOICE_BY_ONBOARD_DUAL_DMIC_MIC_SPK_CONFIG() {  \
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
    .aec_cfg.aec_alg_cfg = {                                    \
        .task_stack = AEC_V3_ALGORITHM_TASK_STACK,              \
        .task_core = AEC_V3_ALGORITHM_TASK_CORE,                \
        .task_prio = AEC_V3_ALGORITHM_TASK_PRIO,                \
        .aec_cfg = {                                            \
            .mode = AEC_MODE_SOFTWARE,                          \
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
        .pa_ctrl_en = false,                                    \
        .pa_ctrl_gpio = 0,                                      \
        .pa_on_level = 0,                                       \
        .pa_on_delay = 0,                                       \
        .pa_off_delay = 0,                                      \
        .task_stack = ONBOARD_SPEAKER_STREAM_TASK_STACK,        \
        .task_core = ONBOARD_SPEAKER_STREAM_TASK_CORE,          \
        .task_prio = ONBOARD_SPEAKER_STREAM_TASK_PRIO,          \
    },                                                          \
    .event_handle = NULL,                                       \
    .args = NULL,                                               \
}

#if CONFIG_VOICE_SERVICE_G722_ENCODER && CONFIG_VOICE_SERVICE_G722_DECODER
/* voice call through onboard mic and onboard speaker
 * mic: onboard mic
 * speaker: onboard speaker
 * AEC: ON
 * sample rate: 16000Hz
 * encoder: G722
 * decoder: G722
 */
#define DEFAULT_VOICE_BY_ONBOARD_MIC_SPK_G722_CONFIG() {            \
        .mic_type = MIC_TYPE_ONBOARD,                               \
        .mic_cfg.onboard_mic_cfg = {                                \
            .adc_cfg = {                                            \
                .chl_num = 1,                                       \
                .bits = 16,                                         \
                .sample_rate = 16000,                               \
                .dig_gain = 0x28,                                   \
                .ana_gain = 0x8,                                    \
                .mode = AUD_ADC_MODE_DIFFEN,                        \
                .clk_src = AUD_CLK_XTAL,                            \
            },                                                      \
            .frame_size = 640,                                      \
            .out_block_size = 640,                                  \
            .out_block_num = 2,                                     \
            .multi_out_port_num = 0,                                \
            .task_stack = ONBOARD_MIC_STREAM_TASK_STACK,            \
            .task_core = ONBOARD_MIC_STREAM_TASK_CORE,              \
            .task_prio = ONBOARD_SPEAKER_STREAM_TASK_PRIO,          \
        },                                                          \
        .aec_en = true,                                             \
        .aec_cfg.aec_alg_cfg = {                                    \
            .task_stack = AEC_V3_ALGORITHM_TASK_STACK,              \
            .task_core = AEC_V3_ALGORITHM_TASK_CORE,                \
            .task_prio = AEC_V3_ALGORITHM_TASK_PRIO,                \
            .aec_cfg = {                                            \
                .mode = AEC_MODE_SOFTWARE,                          \
                .fs = AEC_V3_ALGORITHM_FS,                          \
                .init_flags = AEC_V3_ALGORITHM_INIT_FLAG,           \
                .delay_points = AEC_V3_DELAY_POINTS,                \
                .ec_depth = AEC_V3_ALGORITHM_EC_DEPTH,              \
                .ref_scale = AEC_V3_ALGORITHM_REF_SCALE,            \
                .voice_vol = AEC_V3_ALGORITHM_VOL,                  \
                .ns_type = NS_TRADITION,                            \
                .ns_filter = AEC_V3_ALGORITHM_NS_FILTER,            \
                .ns_level = AEC_V3_ALGORITHM_NS_LEVEL,              \
                .ns_para = AEC_V3_ALGORITHM_NS_PARA,                \
                .drc = AEC_V3_ALGORITHM_DRC,                        \
                .ec_filter = AEC_V3_ALGORITHM_EC_FILTER,            \
            },                                                      \
            .vad_cfg = {                                            \
                .vad_enable = 0,                                    \
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
            .dual_ch = 0,                                           \
        },                                                          \
        .enc_en = true,                                             \
        .enc_type = AUDIO_ENC_TYPE_G722,                            \
        .enc_cfg.g722_enc_cfg = {                                   \
            .buf_sz             = G722_ENCODER_BUFFER_SIZE,         \
            .out_block_size     = G722_ENCODER_OUT_BLOCK_SIZE,      \
            .out_block_num      = G722_ENCODER_OUT_BLOCK_NUM,       \
            .task_stack         = G722_ENCODER_TASK_STACK,          \
            .task_core          = G722_ENCODER_TASK_CORE,           \
            .task_prio          = G722_ENCODER_TASK_PRIO,           \
            .enc_rate           = G722_ENC_RATE_64000,              \
            .options            = 0,                                \
        },                                                          \
        .read_pool_size = 160,                                      \
        .write_pool_size = 320,                                     \
        .dec_en = true,                                             \
        .dec_type = AUDIO_DEC_TYPE_G722,                            \
        .dec_cfg.g722_dec_cfg = {                                   \
            .buf_sz             = G722_DECODER_BUFFER_SIZE,         \
            .out_block_size     = G722_DECODER_OUT_BLOCK_SIZE,      \
            .out_block_num      = G722_DECODER_OUT_BLOCK_NUM,       \
            .task_stack         = G722_DECODER_TASK_STACK,          \
            .task_core          = G722_DECODER_TASK_CORE,           \
            .task_prio          = G722_DECODER_TASK_PRIO,           \
            .rate               = G722_DEC_RATE_64000,              \
            .options            = G722_DEC_OPTION_NONE,             \
        },                                                          \
        .spk_type = SPK_TYPE_ONBOARD,                               \
        .spk_cfg.onboard_spk_cfg = {                                \
            .chl_num = 1,                                           \
            .sample_rate = 16000,                                   \
            .dig_gain = 0x2d,                                       \
            .ana_gain = 0x07,                                       \
            .work_mode = AUD_DAC_WORK_MODE_DIFFEN,                  \
            .bits = 16,                                             \
            .clk_src = AUD_CLK_XTAL,                                \
            .multi_in_port_num = 0,                                 \
            .multi_out_port_num = 1,                                \
            .frame_size = 640,                                      \
            .pool_length = 0,                                       \
            .pool_play_thold = 0,                                   \
            .pool_pause_thold = 0,                                  \
            .pa_ctrl_en = false,                                    \
            .pa_ctrl_gpio = 0,                                      \
            .pa_on_level = 0,                                       \
            .pa_on_delay = 0,                                       \
            .pa_off_delay = 0,                                      \
            .task_stack = ONBOARD_SPEAKER_STREAM_TASK_STACK,        \
            .task_core = ONBOARD_SPEAKER_STREAM_TASK_CORE,          \
            .task_prio = ONBOARD_SPEAKER_STREAM_TASK_PRIO,          \
        },                                                          \
        .event_handle = NULL,                                       \
        .args = NULL,                                               \
    }
#endif  //CONFIG_VOICE_SERVICE_G722_ENCODER && CONFIG_VOICE_SERVICE_G722_DECODER

/* voice call through onboard mic and onboard speaker,EQ on
 * mic: onboard mic
 * speaker: onboard speaker
 * AEC: ON
 * sample rate: 8000Hz
 * encoder: g711a
 * decoder: g711a
 * EQ: ON
 */
#if CONFIG_VOICE_SERVICE_EQ
#define VOICE_BY_ONBOARD_MIC_SPK_EQ_CFG_DEFAULT() DEFAULT_VOICE_BY_ONBOARD_MIC_SPK_EQ_CONFIG() 

#define DEFAULT_VOICE_BY_ONBOARD_MIC_SPK_EQ_CONFIG() {          \
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
    .aec_cfg.aec_alg_cfg = {                                    \
        .task_stack = AEC_V3_ALGORITHM_TASK_STACK,              \
        .task_core = AEC_V3_ALGORITHM_TASK_CORE,                \
        .task_prio = AEC_V3_ALGORITHM_TASK_PRIO,                \
        .aec_cfg = {                                            \
            .mode = AEC_MODE_SOFTWARE,                          \
            .fs = 8000,                                         \
            .init_flags = AEC_V3_ALGORITHM_INIT_FLAG,           \
            .delay_points = AEC_V3_DELAY_POINTS,                \
            .ec_depth = AEC_V3_ALGORITHM_EC_DEPTH,              \
            .ref_scale = AEC_V3_ALGORITHM_REF_SCALE,            \
            .voice_vol = AEC_V3_ALGORITHM_VOL,                  \
            .ns_type = NS_TRADITION,                            \
            .ns_filter = AEC_V3_ALGORITHM_NS_FILTER,            \
            .ns_level = AEC_V3_ALGORITHM_NS_LEVEL,              \
            .ns_para = AEC_V3_ALGORITHM_NS_PARA,                \
            .drc = AEC_V3_ALGORITHM_DRC,                        \
            .ec_filter = AEC_V3_ALGORITHM_EC_FILTER,            \
        },                                                      \
        .vad_cfg = {                                            \
            .vad_enable = 0,                                    \
            .vad_start_threshold = 480,                         \
            .vad_stop_threshold = 960,                          \
            .vad_silence_threshold = 320,                       \
            .vad_eng_threshold =2000,                           \
            .vad_bad_frame = AEC_V3_VAD_BAD_FRAME_NUM,          \
            .vad_buf_size = 15360,                              \
            .vad_frame_size = 320,                              \
        },                                                      \
        .out_block_size = 320,                                  \
        .out_block_num = AEC_V3_ALGORITHM_OUT_BLOCK_NUM,        \
        .multi_out_port_num = 0,                                \
        .dual_ch = 0,                                           \
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
        .multi_in_port_num = 0,                                 \
        .multi_out_port_num = 1,                                \
        .frame_size = 320,                                      \
        .pool_length = 0,                                       \
        .pool_play_thold = 0,                                   \
        .pool_pause_thold = 0,                                  \
        .pa_ctrl_en = false,                                    \
        .pa_ctrl_gpio = 0,                                      \
        .pa_on_level = 0,                                       \
        .pa_on_delay = 0,                                       \
        .pa_off_delay = 0,                                      \
        .task_stack = ONBOARD_SPEAKER_STREAM_TASK_STACK,        \
        .task_core = ONBOARD_SPEAKER_STREAM_TASK_CORE,          \
        .task_prio = ONBOARD_SPEAKER_STREAM_TASK_PRIO,          \
    },                                                          \
    .eq_en = true,                                              \
    .eq_cfg.eq_alg_cfg = {                                      \
    .task_stack            = EQ_ALGORITHM_TASK_STACK,           \
    .task_core             = EQ_ALGORITHM_TASK_CORE,            \
    .task_prio             = EQ_ALGORITHM_TASK_PRIO,            \
    .eq_cal_para = {                                            \
        .eq_en = 1,                                             \
        .filters = 2,                                           \
        .globle_gain = EQGLOBALGAIN,                            \
        .eq_para[0].a[0] = -EQ0A0,                              \
        .eq_para[0].a[1] = -EQ0A1,                              \
        .eq_para[0].b[0] = EQ0B0,                               \
        .eq_para[0].b[1] = EQ0B1,                               \
        .eq_para[0].b[2] = EQ0B2,                               \
        .eq_para[1].a[0] = -EQ1A0,                              \
        .eq_para[1].a[1] = -EQ1A1,                              \
        .eq_para[1].b[0] = EQ1B0,                               \
        .eq_para[1].b[1] = EQ1B1,                               \
        .eq_para[1].b[2] = EQ1B2,                               \
        .eq_load.f_gain     = EQFGAIN,                          \
        .eq_load.samplerate = EQSAMP,                           \
        .eq_load.eq_load_para[0].freq   = EQ0FREQ,              \
        .eq_load.eq_load_para[0].gain   = EQ0GAIN,              \
        .eq_load.eq_load_para[0].q_val  = EQ0QVAL,              \
        .eq_load.eq_load_para[0].type   = EQ0FTYPE,             \
        .eq_load.eq_load_para[0].enable  = EQ0,                 \
        .eq_load.eq_load_para[1].freq   = EQ1FREQ,              \
        .eq_load.eq_load_para[1].gain   = EQ1GAIN,              \
        .eq_load.eq_load_para[1].q_val  = EQ1QVAL,              \
        .eq_load.eq_load_para[1].type   = EQ1FTYPE,             \
        .eq_load.eq_load_para[1].enable  = EQ1,                 \
    },                                                          \
    .eq_chl_num            = 1,                                 \
    .eq_frame_size         = EQFRAMESIZE,                       \
    .out_block_num         = 2,                                 \
    .multi_out_port_num    = 0,                                 \
    },                                                          \
    .event_handle = NULL,                                       \
    .args = NULL,                                               \
}
#endif //CONFIG_VOICE_SERVICE_EQ

#if CONFIG_VOICE_SERVICE_OPUS_ENCODER && CONFIG_VOICE_SERVICE_OPUS_DECODER
    /* voice call through onboard mic and onboard speaker
     * mic: onboard mic
     * speaker: onboard speaker
     * AEC: ON
     * sample rate: 16000Hz
     * encoder: OPUS
     * decoder: OPUS
     */
#define DEFAULT_VOICE_BY_ONBOARD_MIC_SPK_OPUS_CONFIG() {            \
        .mic_type = MIC_TYPE_ONBOARD,                               \
        .mic_cfg.onboard_mic_cfg = {                                \
            .adc_cfg = {                                            \
                .chl_num = 1,                                       \
                .bits = 16,                                         \
                .sample_rate = 16000,                               \
                .dig_gain = 0x28,                                   \
                .ana_gain = 0x8,                                    \
                .mode = AUD_ADC_MODE_DIFFEN,                        \
                .clk_src = AUD_CLK_XTAL,                            \
            },                                                      \
            .frame_size = 640,                                      \
            .out_block_size = 640,                                  \
            .out_block_num = 2,                                     \
            .multi_out_port_num = 0,                                \
            .task_stack = ONBOARD_MIC_STREAM_TASK_STACK,            \
            .task_core = ONBOARD_MIC_STREAM_TASK_CORE,              \
            .task_prio = ONBOARD_SPEAKER_STREAM_TASK_PRIO,          \
        },                                                          \
        .aec_en = true,                                             \
        .aec_cfg.aec_alg_cfg = {                                    \
            .task_stack = AEC_V3_ALGORITHM_TASK_STACK,              \
            .task_core = AEC_V3_ALGORITHM_TASK_CORE,                \
            .task_prio = AEC_V3_ALGORITHM_TASK_PRIO,                \
            .aec_cfg = {                                            \
                .mode = AEC_MODE_SOFTWARE,                          \
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
                .vad_enable = 0,                                    \
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
            .dual_ch = 0,                                           \
        },                                                          \
        .enc_en = true,                                             \
        .enc_type = AUDIO_ENC_TYPE_OPUS,                            \
        .enc_cfg.opus_enc_cfg = {                                   \
                .buf_sz             = OPUS_ENC_BUFFER_SIZE,         \
                .out_block_size     = OPUS_ENC_OUT_BLOCK_SIZE,      \
                .out_block_num      = OPUS_ENC_OUT_BLOCK_NUM,       \
                .task_stack         = OPUS_ENC_TASK_STACK,          \
                .task_core          = OPUS_ENC_TASK_CORE,           \
                .task_prio          = OPUS_ENC_TASK_PRIO,           \
                .enc_mode           = OPUS_ENC_MODE_AUDIO,          \
                .sample_rate        = OPUS_ENC_SAMPLE_RATE,         \
                .channels           = 1,                            \
                .bitrate            = OPUS_ENC_BITRATE,             \
                .frame_samples_per_channel = 320,                   \
        },                                                          \
        .read_pool_size = 160,                                      \
        .write_pool_size = 320,                                     \
        .dec_en = true,                                             \
        .dec_type = AUDIO_DEC_TYPE_OPUS,                            \
        .dec_cfg.opus_dec_cfg = {                                   \
            .buf_sz             = OPUS_DEC_BUFFER_SIZE,             \
            .out_block_size     = OPUS_DEC_OUT_BLOCK_SIZE,          \
            .out_block_num      = OPUS_DEC_OUT_BLOCK_NUM,           \
            .task_stack         = OPUS_DEC_TASK_STACK,              \
            .task_core          = OPUS_DEC_TASK_CORE,               \
            .task_prio          = OPUS_DEC_TASK_PRIO,               \
            .sample_rate        = OPUS_DEC_SAMPLE_RATE,             \
            .channels           = 1,                                \
        },                                                          \
        .spk_type = SPK_TYPE_ONBOARD,                               \
        .spk_cfg.onboard_spk_cfg = {                                \
            .chl_num = 1,                                           \
            .sample_rate = 16000,                                   \
            .dig_gain = 0x2d,                                       \
            .ana_gain = 0x07,                                       \
            .work_mode = AUD_DAC_WORK_MODE_DIFFEN,                  \
            .bits = 16,                                             \
            .clk_src = AUD_CLK_XTAL,                                \
            .multi_in_port_num = 0,                                 \
            .multi_out_port_num = 1,                                \
            .frame_size = 640,                                      \
            .pool_length = 0,                                       \
            .pool_play_thold = 0,                                   \
            .pool_pause_thold = 0,                                  \
            .pa_ctrl_en = false,                                    \
            .pa_ctrl_gpio = 0,                                      \
            .pa_on_level = 0,                                       \
            .pa_on_delay = 0,                                       \
            .pa_off_delay = 0,                                      \
            .task_stack = ONBOARD_SPEAKER_STREAM_TASK_STACK,        \
            .task_core = ONBOARD_SPEAKER_STREAM_TASK_CORE,          \
            .task_prio = ONBOARD_SPEAKER_STREAM_TASK_PRIO,          \
        },                                                          \
        .event_handle = NULL,                                       \
        .args = NULL,                                               \
    }
#endif  //CONFIG_VOICE_SERVICE_OPUS_ENCODER && CONFIG_VOICE_SERVICE_OPUS_DECODER


#ifdef  __cplusplus
}
#endif//__cplusplus

/**
 * @}
 */
#pragma once
#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#ifndef UNUSED_ATTR
#define UNUSED_ATTR __attribute__((unused))
#endif

#define INVALID_DEVICE          (NULL)


typedef void *device_object_t;



/**************** audio interface mic ****************/

/* audio interface mic type */
typedef enum
{
    MIC_TYPE_ONBOARD = 1,
    MIC_TYPE_UAC,
    MIC_TYPE_ONBOARD_DUAL_DMIC_MIC,
} mic_type_t;

/**************** audio interface speaker ****************/

/* audio interface mic type */
typedef enum
{
    SPK_TYPE_ONBOARD = 1,
    SPK_TYPE_UAC,
} spk_type_t;

typedef enum
{
    AUDIO_ENC_TYPE_PCM = 1,
    AUDIO_ENC_TYPE_G711A,
    AUDIO_ENC_TYPE_G711U,
    AUDIO_ENC_TYPE_AAC,
    AUDIO_ENC_TYPE_ADPCM,
    AUDIO_ENC_TYPE_LC3,
    AUDIO_ENC_TYPE_OPUS,
    AUDIO_ENC_TYPE_G722
} audio_enc_type_t;

typedef enum
{
    AUDIO_DEC_TYPE_PCM = 1,
    AUDIO_DEC_TYPE_G711A,
    AUDIO_DEC_TYPE_G711U,
    AUDIO_DEC_TYPE_AAC,
    AUDIO_DEC_TYPE_ADPCM,
    AUDIO_DEC_TYPE_LC3,
    AUDIO_DEC_TYPE_MP3,
    AUDIO_DEC_TYPE_OPUS,
    AUDIO_DEC_TYPE_G722
} audio_dec_type_t;

typedef enum
{
    AUDIO_MEM_TYPE_SRAM = 1,
    AUDIO_MEM_TYPE_PSRAM,
    AUDIO_MEM_TYPE_AUDIO_HEAP,
} audio_mem_type_t;

#ifdef  __cplusplus
}
#endif//__cplusplus

/**
 * @}
 */
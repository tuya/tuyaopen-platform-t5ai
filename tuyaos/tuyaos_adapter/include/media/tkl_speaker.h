 /**
 * @file tkl_audio.h
 * @brief Common process - adapter the audio api
 * @version 0.1
 * @date 2021-11-04
 *
 * @copyright Copyright 2019-2021 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TKL_SPEAKER_H__
#define __TKL_SPEAKER_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
    extern "C" {
#endif

typedef enum
{
    TKL_SPK_DATABITS_8 = 8,
    TKL_SPK_DATABITS_16 = 16,
    TKL_SPK_DATABITS_MAX = 0xFF
}TKL_SPK_DATABITS_E;

typedef enum
{
    TKL_SPK_TYPE_UAC = 0,
    TKL_SPK_TYPE_BOARD,
}TKL_SPK_TYPE_E;

typedef enum
{
    TKL_CODEC_SPK_ADPCM,
    TKL_CODEC_SPK_PCM,
    TKL_CODEC_SPK_AAC_RAW,
    TKL_CODEC_SPK_AAC_ADTS,
    TKL_CODEC_SPK_AAC_LATM,
    TKL_CODEC_SPK_G711U,
    TKL_CODEC_SPK_G711A,
    TKL_CODEC_SPK_G726,
    TKL_CODEC_SPK_SPEEX,
    TKL_CODEC_SPK_MP3,
    TKL_CODEC_SPK_MAX = 199,
}TKL_SPK_CODEC_TYPE_E;

typedef struct {
    uint8_t chl_num;        //dac通道
    uint8_t volume;        //音量
    uint8_t spk_gpio;        //功放
    uint8_t spk_gpio_polarity;        //功放
    uint32_t sample_rate;   // 采样率
    TKL_SPK_DATABITS_E    datebits;                    // datebit
    TKL_SPK_TYPE_E  card;                   // codec type
    TKL_SPK_CODEC_TYPE_E  codectype;                   // codec type
} TKL_SPK_CFG_T;

/**
* @brief speaker init
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_speaker_init(TKL_SPK_CFG_T *config);

/**
* @brief speaker start
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
int32_t tkl_speaker_start(void);

/**
* @brief speaker stop
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
int32_t tkl_speaker_stop(void);

/**
* @brief speaker pause
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
int32_t tkl_speaker_pause(void);

/**
* @brief speaker set vqe param
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[in] type: vqe type
* @param[in] pparam: vqe param
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
int32_t tkl_speaker_resume(void);

/**
* @brief speaker set gain
*
* @param[in] gain: gain value
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
int32_t tkl_speaker_set_gain(int32_t gain);

/**
* @brief speaker stop
*
* @param[in] data: address of buffer
* @param[in] len: length of buffer
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
int32_t tkl_speaker_write(uint8_t *data, uint32_t len);

/**
* @brief speaker uninit
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
int32_t tkl_speaker_stop(void);

/**
* @brief speaker deinit
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
void tkl_speaker_deinit(void);

#ifdef __cplusplus
}
#endif

#endif

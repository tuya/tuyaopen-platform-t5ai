 /**
 * @file tkl_audio.h
 * @brief Common process - adapter the audio api
 * @version 0.1
 * @date 2021-11-04
 *
 * @copyright Copyright 2019-2021 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TKL_MIC_H__
#define __TKL_MIC_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
    extern "C" {
#endif
typedef enum
{
    TKL_MIC_DATABITS_8 = 8,
    TKL_MIC_DATABITS_16 = 16,
    TKL_MIC_DATABITS_MAX = 0xFF
}TKL_MIC_DATABITS_E;

typedef enum
{
    TKL_MIC_TYPE_UAC = 0,
    TKL_MIC_TYPE_BOARD,
}TKL_MIC_TYPE_E;

typedef enum
{
    TKL_CODEC_MIC_ADPCM,
    TKL_CODEC_MIC_PCM,
    TKL_CODEC_MIC_AAC_RAW,
    TKL_CODEC_MIC_AAC_ADTS,
    TKL_CODEC_MIC_AAC_LATM,
    TKL_CODEC_MIC_G711U,
    TKL_CODEC_MIC_G711A,
    TKL_CODEC_MIC_G726,
    TKL_CODEC_MIC_SPEEX,
    TKL_CODEC_MIC_MP3,
    TKL_CODEC_MIC_MAX = 199,
}TKL_MIC_CODEC_TYPE_E;

typedef struct {
    UINT8_T chl_num;        //adc通道
    UINT8_T volume;        //音量
    UINT32_T sample_rate;   // 采样率
    TKL_MIC_DATABITS_E    datebits;                    // datebit
    TKL_MIC_TYPE_E  card;                   // codec type
    TKL_MIC_CODEC_TYPE_E  codectype;                   // codec type
} TKL_MIC_CFG_T;

/**
* @brief mic init
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_mic_init(TKL_MIC_CFG_T *config);

/**
* @brief mic start
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
INT32_T tkl_mic_start(VOID_T);

/**
* @brief mic stop
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
INT32_T tkl_mic_stop(VOID_T);

/**
* @brief mic get frame
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[out] pframe: audio frame, pframe->pbuf allocated by upper layer application
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
INT32_T tkl_mic_pause(VOID_T);

/**
* @brief mic set vqe param
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[in] type: vqe type
* @param[in] pparam: vqe param
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
INT32_T tkl_mic_resume(VOID_T);

/**
* @brief mic set gain
*
* @param[in] gain: gain value
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
INT32_T tkl_mic_set_gain(INT32_T gain);

/**
* @brief mic stop
*
* @param[in] data: address of buffer
* @param[in] len: length of buffer
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
INT32_T tkl_mic_read(UINT8_T *data, UINT32_T len);

/**
* @brief mic uninit
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
INT32_T tkl_mic_stop(VOID_T);

/**
* @brief mic deinit
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
VOID_T tkl_mic_deinit(VOID_T);

#ifdef __cplusplus
}
#endif

#endif

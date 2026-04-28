/**
* @file tkl_jpeg_codec.h
* @brief Common process - adapter the jpeg codec api
* @version 0.1
* @date 2025-08-13
*
* @copyright Copyright 2021-2022 Tuya Inc. All Rights Reserved.
*
*/

#ifndef __TKL_JPEG_CODEC_H__
#define __TKL_JPEG_CODEC_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    JPEG_DEC_OUT_YUV422 = 0,
    JPEG_DEC_OUT_RGB565,
    JPEG_DEC_OUT_RGB888,
} JPEG_DEC_OUT_FMT;

typedef struct
{
    TUYA_FRAME_FMT_E    out_fmt;
    uint16_t            out_width;
    uint16_t            out_height;
    uint32_t            in_size;
} TKL_JPEG_CODEC_INFO_T;

/**
 * @brief jpeg codec init
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_jpeg_codec_init();

/**
 * @brief jpeg codec deinit
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_jpeg_codec_deinit();

/**
* @brief jpeg img info get
* 
* @param[in]  in_frame : image source
* @param[in out] out_frame: image dest
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_jpeg_codec_img_info_get(uint8_t *jpeg_buf, uint32_t jpeg_size, TKL_JPEG_CODEC_INFO_T *jpeg_info);

/**
* @brief codec convert
* 
* @param[in]  in_frame : image source
* @param[in out] out_frame: image dest
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_jpeg_codec_convert(uint8_t *src_buf, uint8_t *dst_buf, TKL_JPEG_CODEC_INFO_T *jpeg_codec_info, JPEG_DEC_OUT_FMT out_fmt);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __TKL_JPEG_CODEC_H__

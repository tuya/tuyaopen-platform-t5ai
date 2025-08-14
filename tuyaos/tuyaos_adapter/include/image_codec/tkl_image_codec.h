/**
* @file tkl_image_codec.h
* @version 0.1
* @date 2025-07-01
*
* @copyright Copyright 2021-2025 Tuya Inc. All Rights Reserved.
*
*/

#ifndef __TKL_IMAGE_CODEC_H__
#define __TKL_IMAGE_CODEC_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    TKL_IMAGE_CODEC_TYPE_H264 = 0,
    TKL_IMAGE_CODEC_TYPE_JPEG,
    TKL_IMAGE_CODEC_TYPE_DMA2D,
    TKL_IMAGE_CODEC_TYPE_MAX,
} TKL_IMAGE_CODEC_INIT_E;

typedef enum {
    TKL_CODEC_HARDWARE = 0,
    TKL_CODEC_H264,
    TKL_CODEC_JPEG,
    TKL_CODEC_YUV420,
    TKL_CODEC_YUV422,
    TKL_CODEC_YUV444,
    TKL_CODEC_RGB888,
    TKL_CODEC_RGB565,
    TKL_CODEC_RGB666,
    TKL_CODEC_MAX = 0xFF,
} TKL_IMAGE_CODEC_TYPE_E;

typedef enum {
    TKL_IMAGE_EVT_TRANS_COMPLETE = 0,
    TKL_IMAGE_EVT_TRANS_ERROR,
    TKL_IMAGE_EVT_CONFIG_TIMEOUT,
    TKL_IMAGE_EVT_MAX,
} TKL_IMAGE_CODEC_EVT_E;

typedef void (*TKL_IMAGE_CODEC_EVT_CB)(TKL_IMAGE_CODEC_EVT_E event);

typedef struct {
    TKL_IMAGE_CODEC_TYPE_E type;                       // [in] : type of pbuf 
    uint8_t     *pbuf;                                 // [in out] : frame buffer
    uint32_t     size;                                 // [in out] : buffer size
    uint16_t     width;                                // [in out] : buffer wide
    uint16_t     height;                               // [in out] : buffer height
    uint16_t     xpos;                                 // [out] : buffer x position
    uint16_t     ypos;                                 // [out] : buffer y position
} TKL_CODEC_FRAME_T;

typedef struct {
    uint32_t     width;                                // [in] : buffer wide
    uint32_t     height;                               // [in] : buffer height
} TUYA_H264_CODEC_INIT_T;

typedef struct
{
    uint32_t     width;                                // [in] : buffer wide
    uint32_t     height;                               // [in] : buffer height
} TUYA_JPEG_CODEC_INIT_T;

typedef struct
{
    TKL_IMAGE_CODEC_INIT_E  type;                // [in] : init type
    TKL_IMAGE_CODEC_EVT_CB  cb;                  // [in] : event callback
    union {
        TUYA_H264_CODEC_INIT_T      h264;        /* h264 init */
        TUYA_JPEG_CODEC_INIT_T      jpeg;        /* jpeg init */
    } init_para;
} TKL_CODEC_BASE_CFG_T;

typedef struct
{
	uint16_t width;     /**< coded width */
	uint16_t height;    /**< coded height */
	uint32_t size;	    /**< coded size needed*/
} TKL_IMAGE_CODEC_DES_T;

/****** Image Codec Irq Event *****/
typedef enum {
    TUYA_IMAGE_CODEC_EVENT_TRANSFER_COMPLETE = 0,
    TUYA_IMAGE_CODEC_EVENT_TRANSFER_BUFFER_FULL        = 1,  ///< fifo is full
    TUYA_IMAGE_CODEC_EVENT_TRANSFER_ERROR              = 2,  ///< transfer error
} TUYA_CODEC_IRQ_EVT_E;

typedef VOID_T (*TUYA_IMAGE_CODEC_IRQ_CB)(TKL_IMAGE_CODEC_TYPE_E type, TUYA_CODEC_IRQ_EVT_E event, VOID_T *arg);

typedef struct {
    TUYA_IMAGE_CODEC_IRQ_CB     cb;
    VOID_T                      *arg;
} TUYA_IMAGE_CODEC_IRQ_T;

/**
* @brief image codec init
* 
* @param[in] pconfig: image config
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_image_codec_init(TKL_CODEC_BASE_CFG_T *para);

/**
* @brief image codec deinit
* 
* @param[in] pconfig: image config
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_image_codec_deinit(TKL_IMAGE_CODEC_INIT_E type);

/**
* @brief codec convert
* 
* @param[in]  in_frame : image source
* @param[in out] out_frame: image dest
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_image_codec_convert(TKL_CODEC_FRAME_T *in_frame, TKL_CODEC_FRAME_T *out_frame);

/**
* @brief hardware memcopy
* 
* @param[in]  in_frame : image source
* @param[in out] out_frame: image dest
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_image_codec_memcopy(TKL_CODEC_FRAME_T *in_frame, TKL_CODEC_FRAME_T *out_frame);

/**
* @brief image codec get info
* 
* @param[in] frame_buf: image buf
* @param[out] result   : image info
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_image_codec_get_info(TKL_CODEC_FRAME_T *frame_buf, TKL_IMAGE_CODEC_DES_T *result);

/**
 * @brief image codec irq init
 * NOTE: call this API will not enable interrupt
 * 
 * @param[in] cfg:  image codec irq config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_image_codec_irq_init(TKL_IMAGE_CODEC_INIT_E type, const TUYA_IMAGE_CODEC_IRQ_T *cfg);

/**
 * @brief image codec irq enable
 * 
 * @param[in] para: image codec para
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_image_codec_irq_enable(TKL_IMAGE_CODEC_INIT_E handle);

/**
 * @brief spi irq disable
 * 
 * @param[in] para: image codec para
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_image_codec_irq_disable(TKL_IMAGE_CODEC_INIT_E handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

/**
* @file tkl_dma2d.h
* @brief Common process - adapter the flash api
* @version 0.1
* @date 2025-08-13
*
* @copyright Copyright 2021-2022 Tuya Inc. All Rights Reserved.
*
*/

#ifndef __TKL_DMA2D_H__
#define __TKL_DMA2D_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief dma2d interrupt mode
 */
typedef enum {
    TUYA_DMA2D_TRANS_COMPLETE_ISR,
    TUYA_DMA2D_TRANS_ERROR_ISR,
} TUYA_DMA2D_IRQ_E;

typedef struct
{
    UINT16_T      x_axis;
    UINT16_T      y_axis;
}TKL_DMA2D_POINT_T;

typedef struct
{
    TUYA_FRAME_FMT_E type;                           // [in] : type of pbuf 
    uint8_t     *pbuf;                                // [in out] : frame buffer
    UINT16_T     width;                                // [in] : buffer wide
    UINT16_T     height;                               // [in] : buffer height
    TKL_DMA2D_POINT_T  axis;                         // [in] : coordinates
    UINT16_T     width_cp;                             // [in] : buffer wide
    UINT16_T     height_cp;                            // [in] : buffer height
}TKL_DMA2D_FRAME_INFO_T;

typedef VOID_T (*TUYA_DMA2D_IRQ_CB)(TUYA_DMA2D_IRQ_E type, VOID_T *args);

/**
 * @brief dma2d interrupt config
 */
typedef struct {
    TUYA_DMA2D_IRQ_CB     cb;
    VOID_T              *arg;
} TUYA_DMA2D_BASE_CFG_T;

/**
 * @brief dma2d init
 * 
 * @param[in] cfg: dma2d config
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_dma2d_init(CONST TUYA_DMA2D_BASE_CFG_T *cfg);

/**
 * @brief dma2d deinit
 * 
 * @param[in] cfg: dma2d config
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_dma2d_deinit();

/**
* @brief codec convert
* 
* @param[in]  in_frame : image source
* @param[in out] out_frame: image dest
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_dma2d_convert(TKL_DMA2D_FRAME_INFO_T *src, TKL_DMA2D_FRAME_INFO_T *dst);

/**
* @brief hardware memcopy
* 
* @param[in]  src : image source
* @param[out] dst: image dest
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/

OPERATE_RET tkl_dma2d_memcpy(TKL_DMA2D_FRAME_INFO_T *src, TKL_DMA2D_FRAME_INFO_T *dst);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // __TKL_DMA2D_H__

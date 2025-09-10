/**
* @file tkl_dvp.h
* @brief Common process - camera dvp process
* @version 0.1
* @date 2025-06-09
*
* @copyright Copyright 2021-2025 Tuya Inc. All Rights Reserved.
*
*/
#ifndef __TKL_DVP_H__
#define __TKL_DVP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_types.h"

typedef TUYA_DVP_FRAME_MANAGE_T *(*DVP_FRAME_ASSIGN_CB)(TUYA_FRAME_FMT_E fmt);
typedef OPERATE_RET (*DVP_FRAME_POST_CB)(TUYA_DVP_FRAME_MANAGE_T *dvp_frame);

/**
 * @brief tkl_dvp_init
 * 
 * @param[in] cfg: camera device cfg
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_dvp_init(TUYA_DVP_CFG_T *dvp_cfg, UINT32_T clk);

/**
 * @brief tkl_dvp_deinit
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_dvp_deinit(void);

/**
 * @brief tkl_dvp_frame_assign_cb_register
 *
 * @param[in] func: get idle dvp frame callback
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_dvp_frame_assign_cb_register(DVP_FRAME_ASSIGN_CB func);

/**
 * @brief tkl_dvp_frame_post_register_callback
 *
 * @param[in] func: capture module output frame ready to callback func
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_dvp_frame_post_cb_register(DVP_FRAME_POST_CB func);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
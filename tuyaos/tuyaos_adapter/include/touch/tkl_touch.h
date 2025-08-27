/**
* @file tkl_touch.h
* @brief tkl touch api
* @version 0.1
* @date 2021-08-06
*
* @copyright Copyright 2021-2022 Tuya Inc. All Rights Reserved.
*
*/
#ifndef __TKL_TOUCH_H__
#define __TKL_TOUCH_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Touch channel count definition
#define TUYA_TOUCH_CHANNEL_MAX 16

// Touch detection threshold definitions
typedef enum {
    TUYA_TOUCH_DETECT_THRESHOLD_0,  // Threshold 0
    TUYA_TOUCH_DETECT_THRESHOLD_1,  // Threshold 1
    TUYA_TOUCH_DETECT_THRESHOLD_2,  // Threshold 2
    TUYA_TOUCH_DETECT_THRESHOLD_3,  // Threshold 3
    TUYA_TOUCH_DETECT_THRESHOLD_4,  // Threshold 4
    TUYA_TOUCH_DETECT_THRESHOLD_5,  // Threshold 5
    TUYA_TOUCH_DETECT_THRESHOLD_6,  // Threshold 6
} TUYA_TOUCH_DETECT_THRESHOLD_E;

// Touch detection range definitions
typedef enum {
    TUYA_TOUCH_DETECT_RANGE_8PF,    // 8PF range
    TUYA_TOUCH_DETECT_RANGE_12PF,   // 12PF range
    TUYA_TOUCH_DETECT_RANGE_19PF,   // 19PF range
    TUYA_TOUCH_DETECT_RANGE_27PF,   // 27PF range
} TUYA_TOUCH_DETECT_RANGE_E;

// Touch sensitivity level definitions
typedef enum {
    TUYA_TOUCH_SENSITIVITY_LEVEL_0, // Sensitivity level 0
    TUYA_TOUCH_SENSITIVITY_LEVEL_1, // Sensitivity level 1
    TUYA_TOUCH_SENSITIVITY_LEVEL_2, // Sensitivity level 2
    TUYA_TOUCH_SENSITIVITY_LEVEL_3, // Sensitivity level 3
} TUYA_TOUCH_SENSITIVITY_LEVEL_E;

// Touch event types
typedef enum {
    TUYA_TOUCH_EVENT_UP,           // Touch release event
    TUYA_TOUCH_EVENT_DOWN,         // Touch press event
    TUYA_TOUCH_EVENT_LONG_PRESS,   // Long press event
} TUYA_TOUCH_EVENT_E;

// Touch configuration structure
typedef struct {
    TUYA_TOUCH_SENSITIVITY_LEVEL_E sensitivity_level;  // Sensitivity level
    TUYA_TOUCH_DETECT_THRESHOLD_E  detect_threshold;   // Detection threshold
    TUYA_TOUCH_DETECT_RANGE_E      detect_range;       // Detection range
} TUYA_TOUCH_CONFIG_T;

// Touch event callback function type
typedef VOID (*TUYA_TOUCH_CALLBACK)(UINT32_T channel, TUYA_TOUCH_EVENT_E event, VOID *arg);

/**
 * @brief Initialize touch functionality
 * 
 * @param[in] channel_mask Touch channel mask, each bit represents a channel
 * 
 * @return OPRT_OK: Success, others: Failure
 */
OPERATE_RET tkl_touch_init(UINT32_T channel_mask, TUYA_TOUCH_CONFIG_T *cfg);

/**
 * @brief Deinitialize touch functionality
 * 
 * @param[in] channel_mask Touch channel mask, each bit represents a channel
 * 
 * @return OPRT_OK: Success, others: Failure
 */
OPERATE_RET tkl_touch_deinit(UINT32_T channel_mask);

/**
 * @brief Get channel detection range
 * 
 * @param[in] channel Channel number
 * @param[out] detect_range Detection range
 * 
 * @return OPRT_OK: Success, others: Failure
 */
OPERATE_RET tkl_touch_get_channel_detect_range(UINT32_T channel, TUYA_TOUCH_DETECT_RANGE_E *detect_range);

/**
 * @brief Get single channel status
 * 
 * @param[in] touch_id Channel number
 * 
 * @return UINT8_T Channel status 0: Not touched, 1: Touched
 */
UINT8_T tkl_touch_get_single_channel_status(UINT8_T touch_id);

/**
 * @brief Register touch event callback function
 * 
 * @param[in] channel_mask Touch channel mask, each bit represents a channel
 * @param[in] callback Callback function pointer
 * @param[in] arg Callback function argument
 * 
 * @return OPRT_OK: Success, others: Failure
 */
OPERATE_RET tkl_touch_register_callback(UINT32_T channel_mask, TUYA_TOUCH_CALLBACK callback, VOID *arg);

/**
 * @brief Get calibration value
 * 
 * @param[out] value Calibration value, maximum 0x1FF
 * 
 * @return OPRT_OK: Success, others: Failure
 */
OPERATE_RET tkl_touch_get_single_calibration_value(UINT32_T channel_mask, float *value);

/**
 * @brief Enable touch channels
 * 
 * @param[in] channel_mask Touch channel mask, each bit represents a channel
 * 
 * @return OPRT_OK: Success, others: Failure
 */
OPERATE_RET tkl_touch_enable(UINT32_T channel_mask);

/**
 * @brief Disable touch channels
 * 
 * @param[in] channel_mask Touch channel mask, each bit represents a channel
 * 
 * @return OPRT_OK: Success, others: Failure
 */
OPERATE_RET tkl_touch_disable(UINT32_T channel_mask);

/**
 * @brief Clear interrupt status
 * 
 * @param[in] channel_mask Touch channel mask, each bit represents a channel
 * 
 * @return OPRT_OK: Success, others: Failure
 */
OPERATE_RET tkl_touch_clear_interrupt(UINT32_T channel_mask);

/**
 * @brief Enable interrupt
 * 
 * @param[in] channel_mask Touch channel mask, each bit represents a channel
 * @param[in] enable TRUE: Enable, FALSE: Disable
 * 
 * @return OPRT_OK: Success, others: Failure
 */
OPERATE_RET tkl_touch_interrupt_enable(UINT32_T channel_mask, BOOL_T enable);

void cli_touch_single_channel_calib_mode_test_cmd();
void cli_touch_single_channel_manul_mode_test_cmd();
void cli_touch_multi_channel_scan_mode_test_cmd();
void cli_touch_adc_mode_test_cmd();
void cli_touch_multi_channel_cyclic_calib_test_cmd();
void cli_touch_single_channel_multi_calib_test_cmd();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
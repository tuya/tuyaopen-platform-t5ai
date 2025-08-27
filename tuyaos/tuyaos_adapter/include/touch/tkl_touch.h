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

// 触摸通道数量定义
#define TUYA_TOUCH_CHANNEL_MAX 16

// 触摸检测阈值定义
typedef enum {
    TUYA_TOUCH_DETECT_THRESHOLD_0,  // 阈值0
    TUYA_TOUCH_DETECT_THRESHOLD_1,  // 阈值1
    TUYA_TOUCH_DETECT_THRESHOLD_2,  // 阈值2
    TUYA_TOUCH_DETECT_THRESHOLD_3,  // 阈值3
    TUYA_TOUCH_DETECT_THRESHOLD_4,  // 阈值4
    TUYA_TOUCH_DETECT_THRESHOLD_5,  // 阈值5
    TUYA_TOUCH_DETECT_THRESHOLD_6,  // 阈值6
} TUYA_TOUCH_DETECT_THRESHOLD_E;

// 触摸检测范围定义
typedef enum {
    TUYA_TOUCH_DETECT_RANGE_8PF,    // 8PF范围
    TUYA_TOUCH_DETECT_RANGE_12PF,   // 12PF范围
    TUYA_TOUCH_DETECT_RANGE_19PF,   // 19PF范围
    TUYA_TOUCH_DETECT_RANGE_27PF,   // 27PF范围
} TUYA_TOUCH_DETECT_RANGE_E;

// 触摸灵敏度等级定义
typedef enum {
    TUYA_TOUCH_SENSITIVITY_LEVEL_0, // 灵敏度等级0
    TUYA_TOUCH_SENSITIVITY_LEVEL_1, // 灵敏度等级1
    TUYA_TOUCH_SENSITIVITY_LEVEL_2, // 灵敏度等级2
    TUYA_TOUCH_SENSITIVITY_LEVEL_3, // 灵敏度等级3
} TUYA_TOUCH_SENSITIVITY_LEVEL_E;

// 触摸事件类型
typedef enum {
    TUYA_TOUCH_EVENT_UP,           // 触摸释放事件
    TUYA_TOUCH_EVENT_DOWN,            // 触摸按下事件
    TUYA_TOUCH_EVENT_LONG_PRESS,    // 长按事件
} TUYA_TOUCH_EVENT_E;

// 触摸配置结构体
typedef struct {
    TUYA_TOUCH_SENSITIVITY_LEVEL_E sensitivity_level;  // 灵敏度等级
    TUYA_TOUCH_DETECT_THRESHOLD_E  detect_threshold;   // 检测阈值
    TUYA_TOUCH_DETECT_RANGE_E      detect_range;       // 检测范围
} TUYA_TOUCH_CONFIG_T;

// 触摸事件回调函数类型
typedef VOID (*TUYA_TOUCH_CALLBACK)(UINT32_T channel, TUYA_TOUCH_EVENT_E event, VOID *arg);

/**
 * @brief 初始化触摸功能
 * 
 * @param[in] channel_mask 触摸通道掩码，每一位代表一个通道
 * 
 * @return OPRT_OK: 成功，其他: 失败
 */
OPERATE_RET tkl_touch_init(UINT32_T channel_mask, TUYA_TOUCH_CONFIG_T *cfg);

/**
 * @brief 反初始化触摸功能
 * 
 * @param[in] channel_mask 触摸通道掩码，每一位代表一个通道
 * 
 * @return OPRT_OK: 成功，其他: 失败
 */
OPERATE_RET tkl_touch_deinit(UINT32_T channel_mask);

/**
 * @brief 获取通道检测范围
 * 
 * @param[in] channel 通道号
 * @param[out] detect_range 检测范围
 * 
 * @return OPRT_OK: 成功，其他: 失败
 */
OPERATE_RET tkl_touch_get_channel_detect_range(UINT32_T channel, TUYA_TOUCH_DETECT_RANGE_E *detect_range);

/**
 * @brief 获取单个通道状态
 * 
 * @param[in] touch_id 通道号
 * 
 * @return UINT8_T 通道状态 0: 未触摸, 1: 触摸
 */
UINT8_T tkl_touch_get_single_channel_status(UINT8_T touch_id);

/**
 * @brief 注册触摸事件回调函数
 * 
 * @param[in] channel_mask 触摸通道掩码，每一位代表一个通道
 * @param[in] callback 回调函数指针
 * @param[in] arg 回调函数参数
 * 
 * @return OPRT_OK: 成功，其他: 失败
 */
OPERATE_RET tkl_touch_register_callback(UINT32_T channel_mask, TUYA_TOUCH_CALLBACK callback, VOID *arg);

/**
 * @brief 获取电容校准值
 * 
 * @param[out] value 校准值，最大 0x1FF
 * 
 * @return OPRT_OK: 成功，其他: 失败
 */
OPERATE_RET tkl_touch_get_single_calibration_value(UINT32_T channel_mask, float *value);

/**
 * @brief 启用触摸通道
 * 
 * @param[in] channel_mask 触摸通道掩码，每一位代表一个通道
 * 
 * @return OPRT_OK: 成功，其他: 失败
 */
OPERATE_RET tkl_touch_enable(UINT32_T channel_mask);

/**
 * @brief 禁用触摸通道
 * 
 * @param[in] channel_mask 触摸通道掩码，每一位代表一个通道
 * 
 * @return OPRT_OK: 成功，其他: 失败
 */
OPERATE_RET tkl_touch_disable(UINT32_T channel_mask);

/**
 * @brief 清除中断状态
 * 
 * @param[in] channel_mask 触摸通道掩码，每一位代表一个通道
 * 
 * @return OPRT_OK: 成功，其他: 失败
 */
OPERATE_RET tkl_touch_clear_interrupt(UINT32_T channel_mask);

/**
 * @brief 使能中断
 * 
 * @param[in] channel_mask 触摸通道掩码，每一位代表一个通道
 * @param[in] enable TRUE: 启用, FALSE: 禁用
 * 
 * @return OPRT_OK: 成功，其他: 失败
 */
OPERATE_RET tkl_touch_interrupt_enable(UINT32_T channel_mask, BOOL_T enable);

#if 0
void cli_touch_single_channel_calib_mode_test_cmd();
void cli_touch_single_channel_manul_mode_test_cmd();
void cli_touch_multi_channel_scan_mode_test_cmd();
void cli_touch_adc_mode_test_cmd();
void cli_touch_multi_channel_cyclic_calib_test_cmd();
void cli_touch_single_channel_multi_calib_test_cmd();
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
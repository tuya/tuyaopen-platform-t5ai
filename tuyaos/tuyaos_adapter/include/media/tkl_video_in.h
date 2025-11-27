 /**
 * @file tkl_video_in.h
 * @brief video input
 * @version 0.1
 * @date 2021-11-04
 *
 * @copyright Copyright 2019-2021 Tuya Inc. All Rights Reserved.
 *
 * Video input configuration: for sensor, ISP or VI attribute configuration,
 * such as image flip, anti flicker, compensation mode, profile, etc.
 */

#ifndef __TKL_VIDEO_IN_H__
#define __TKL_VIDEO_IN_H__

#include "tuya_cloud_types.h"
#include "tkl_media.h"
#include "tkl_isp.h"

#ifdef __cplusplus
    extern "C" {
#endif

#define TKL_VI_MD_ROI_RECT_MAX 4
#define TKL_VI_PERI_POINT_MAX  8

typedef enum {
    TKL_VI_ROTATE_NONE = 0, /**< no rotate */
    TKL_VI_ROTATE_90, /**< Image rotaged 90 degress*/
    TKL_VI_ROTATE_180, /**< No support yet, reserved for the future*/
    TKL_VI_ROTATE_270, /**< Image rotaged 270 degress*/
} TKL_VI_ROTATE_E;

typedef struct
{
    // 1个IIC+host 对应一个video设备   hardware_port[0] + host_port[0] 构造成1个video设备
    UINT32_T * hardware_port;          // 存放iic
    UINT32_T * host_port;              // 存放host dvp or mipi
    UINT32_T   haraware_source_num;    // 一共有多少对 iic + host

    // 视频内存池的分配方法 在RTOS系统有片外的PSRAM 所以需要支持这个
    void (*v_mem_free)(void *);
    void *(*v_mem_malloc)(UINT32_T size);

}TKL_VI_HARDWARE_SOURCE_T;

typedef struct
{
    UINT32_T page;
    UINT32_T reg_index;
    UINT32_T val;
}TKL_VI_SENSOR_REG_CONFIG_T;

typedef enum
{
    TKL_VI_SENSOR_TYPE_IIC,
    TKL_VI_SENSOR_TYPE_SPI,
}TKL_VI_SENSOR_CBUS_TYPE_E;

typedef struct
{
    CHAR_T conf[128];                                             // isp conf path
    CHAR_T conf1[128];                                            // isp conf1 path
    CHAR_T conf2[128];                                            // isp conf2 path
    CHAR_T conf3[128];                                            // isp conf3 path
    CHAR_T version[32];                                           // version num
    CHAR_T name[16];                                              // isp sensor num
    TKL_ISP_DN_SWITCH_CONFIG_T isp_dn_switch_config;              // ADN switch config
    TKL_VI_SENSOR_CBUS_TYPE_E  sensor_type;                       // sensor control bus type
    INT32_T              addr;                                    // sensor address
    INT32_T              width;                                   // sensor width
    INT32_T              height;                                  // sensor height
    TKL_ISP_FIG_STYLE_CONFIG_T isp_fig_style_day;                 // isp fig style in day
    TKL_ISP_FIG_STYLE_CONFIG_T isp_fig_style_night;               // isp fig style in night
    INT32_T              fps;                                     // sensor fps
}TKL_VI_ISP_CONFIG_T;

typedef enum
{
    TKL_VI_CAMERA_TYPE_DVP,
    TKL_VI_CAMERA_TYPE_UVC,
    TKL_VI_CAMERA_TYPE_NET,
}TKL_VI_CAMERA_TYPE_E;

typedef struct {
    TUYA_GPIO_NUM_E     clk;
    TUYA_GPIO_NUM_E     sda;
} TKL_VI_DVP_CONF_IO_T;

typedef struct
{
    TKL_VI_CAMERA_TYPE_E    camera_type;
    TKL_MEDIA_CODEC_TYPE_E  fmt;
    INT32_T                 width;
    INT32_T                 height;
    INT32_T                 fps;
    TKL_VI_ROTATE_E         rotate;
    TUYA_GPIO_NUM_E         power_pin;
    TUYA_GPIO_LEVEL_E       active_level;
    TKL_VI_DVP_CONF_IO_T    i2c;
} TKL_VI_CAMERA_CONF_T;

typedef enum
{
    TKL_VI_EXT_CONF_CAMERA,
    TKL_VI_EXT_CONF_INVALID,
}TKL_VI_EXT_CONFIG_E;

typedef struct
{
    TKL_VI_EXT_CONFIG_E type;
    union {
        TKL_VI_CAMERA_CONF_T camera;
        uint8_t buffer[64];
    };
}TKL_VI_EXT_CONFIG_T;

typedef struct
{
    INT32_T              enable;                                  // 1,enable,0,disable
    TKL_VI_CHN_E         chn;                                     // video input channel
    INT32_T              mirror;                                  // mirror defaults
    INT32_T              filp;                                    // filp defaults
    TKL_VI_ISP_CONFIG_T  isp;                                     // isp config
    VOID * pdata;                                                 // reserver data
}TKL_VI_CONFIG_T;

typedef struct
{
    FLOAT_T x;                                                     // 矩形框x坐标 | [0.0 - 1.0]
    FLOAT_T y;                                                     // 矩形框y坐标 | [0.0 - 1.0]
    FLOAT_T width;                                                 // 矩形框宽度　| [0.0 - 1.0]
    FLOAT_T height;                                                // 矩形框高度　| [0.0 - 1.0]
} TKL_VI_RECT_T;

typedef struct
{
    INT32_T x;
    INT32_T y;
} TKL_VI_POINT_T;

typedef struct
{
    TKL_VI_RECT_T    draw_rect;                                   // coordinate region
    FLOAT_T          score;                                       // score       | [0.0 - 1.0]
    TKL_MEDIA_DETECT_TYPE_E type;                                 // 目标类型，人形，宠物，火焰等
	UINT32_T         id;                                          // 目标编号
} TKL_VI_DETECT_TARGET_T;

typedef struct
{
    int value;                                                    // 0,无动检，1，检测到动检
    TKL_VI_POINT_T motion_point;                                  // 动检中心点坐标, 矩形中心为（0, 0）
} TKL_VI_MD_RESULT_T;

typedef struct
{
    INT32_T count;
    TKL_VI_DETECT_TARGET_T target[TKL_VI_TARGET_MAX];
    union{
        TKL_VI_MD_RESULT_T md;
    };
    UINT64_T timestamp;                                           // utc time, 单位:ms
} TKL_VI_DETECT_RESULT_T;

typedef struct
{
    UINT32_T roi_count;                                           // 检测有效区域个数
    TKL_VI_RECT_T roi_rect[TKL_VI_MD_ROI_RECT_MAX];               // 区域框
    INT32_T track_enable;                                         // 移动追踪使能
} TKL_VI_MD_PARAM_T;

typedef struct
{
    UINT32_T point_count;                                         // 顶点个数
    TKL_VI_POINT_T point[TKL_VI_PERI_POINT_MAX];                  // 点
} TKL_VI_PERI_PARAM_T;

typedef struct
{
    INT32_T sensitivity;
    union{
        TKL_VI_MD_PARAM_T md;
        TKL_VI_PERI_PARAM_T peri;
    };
} TKL_VI_DETECT_PARAM_T;

typedef struct
{
    CHAR_T         *pmodel;                                       // 检测模型数据指针
    INT32_T         model_len;                                    // 检测模型的数据长度
}TKL_VI_DETECT_CONFIG_T;

/**
* @brief vi init
*
* @param[in] pconfig: vi config
* @param[in] count: count of pconfig
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_init(TKL_VI_CONFIG_T *pconfig, INT32_T count);

/**
* @brief vi set mirror and flip
*
* @param[in] chn: vi chn
* @param[in] flag: mirror and flip value
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_set_mirror_flip(TKL_VI_CHN_E chn, TKL_VI_MIRROR_FLIP_E flag);

/**
* @brief vi get mirror and flip
*
* @param[in] chn: vi chn
* @param[out] flag: mirror and flip value
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_get_mirror_flip(TKL_VI_CHN_E chn, TKL_VI_MIRROR_FLIP_E *flag);

/**
* @brief vi uninit
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_uninit(TKL_VI_CAMERA_TYPE_E device_type);

/**
* @brief  set sensor reg value
*
* @param[in] chn: vi chn
* @param[in] pparam: reg info
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_sensor_reg_set( TKL_VI_CHN_E chn, TKL_VI_SENSOR_REG_CONFIG_T *parg);

/**
* @brief  get sensor reg value
*
* @param[in] chn: vi chn
* @param[in] pparam: reg info
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_sensor_reg_get( TKL_VI_CHN_E chn, TKL_VI_SENSOR_REG_CONFIG_T *parg);

/**
* @brief detect init
*
* @param[in] chn: vi chn
* @param[in] type: detect type
* @param[in] pconfig: config
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_init(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type, TKL_VI_DETECT_CONFIG_T *p_config);

/**
* @brief detect start
*
* @param[in] chn: vi chn
* @param[in] type: detect type
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_start(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type);

/**
* @brief detect stop
*
* @param[in] chn: vi chn
* @param[in] type: detect type
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_stop(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type);

/**
* @brief get detection results
*
* @param[in] chn: vi chn
* @param[in] type: detect type
* @param[out] presult: detection results
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_get_result(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type, TKL_VI_DETECT_RESULT_T *presult);

/**
* @brief set detection param
*
* @param[in] chn: vi chn
* @param[in] type: detect type
* @param[in] pparam: detection param
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_set(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type, TKL_VI_DETECT_PARAM_T *pparam);

/**
* @brief detect uninit
*
* @param[in] chn: vi chn
* @param[in] type: detect type
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_uninit(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type);

/**
* @brief set video power control pin
*
* @param[in] chn: vi chn
* @param[in] type: detect type
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_get_power_info(uint8_t device_type, uint8_t *io, uint8_t *active);

/**
* @brief vi set display lcd handle
*
* @param[in] handle: lcd handle
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_set_lcd(void *handle);
#ifdef __cplusplus
}
#endif

#endif

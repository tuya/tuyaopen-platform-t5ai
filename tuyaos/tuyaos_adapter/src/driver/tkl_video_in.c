/*
 * tkl_video_in.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "media_app.h"
#include "media_evt.h"
#include "tkl_video_in.h"
#include "tkl_semaphore.h"
#include "tuya_cloud_types.h"

#define TKL_VI_SENSOR_DVP       0
#define TKL_VI_SENSOR_UVC       1
#define TKL_VI_SENSOR_MAX       2

// default
static TKL_VI_EXT_CONFIG_T camera_conf[TKL_VI_SENSOR_MAX] = {
    [TKL_VI_SENSOR_DVP] = {
        .type = TKL_VI_EXT_CONF_CAMERA,
        .camera = {
            .camera_type = TKL_VI_CAMERA_TYPE_DVP,
            .width = 864,
            .height = 480,
            .fps = 15,
            .rotate = TKL_VI_ROTATE_NONE,
            .power_pin = TUYA_GPIO_NUM_56,
            .active_level = TUYA_GPIO_LEVEL_HIGH,
            .i2c = {
                .clk = TUYA_GPIO_NUM_56,
                .sda = TUYA_GPIO_NUM_56,
            }
        }
    },
    [TKL_VI_SENSOR_UVC] = {
        .type = TKL_VI_EXT_CONF_CAMERA,
        .camera = {
            .camera_type = TKL_VI_CAMERA_TYPE_UVC,
            .width = 864,
            .height = 480,
            .fps = 15,
            .rotate = TKL_VI_ROTATE_NONE,
            .power_pin = TUYA_GPIO_NUM_56,
            .active_level = TUYA_GPIO_LEVEL_HIGH,
        }
    }
};

static camera_handle_t vi_handle = NULL;
static uint8_t vi_uvc_port = 0;
static uint8_t vi_uvc_status = 0;
static uint8_t vi_dvp_status = 0;

uint8_t tkl_vi_get_status(uint8_t device_type)
{
    if (device_type == UVC_CAMERA) {
        return vi_uvc_status;
    } else if (device_type == DVP_CAMERA) {
        return vi_dvp_status;
    }

    return 0xff;
}

OPERATE_RET tkl_vi_get_power_info(uint8_t device_type, uint8_t *io, uint8_t *active)
{
    if (device_type == UVC_CAMERA) {
        *io = camera_conf[TKL_VI_SENSOR_UVC].camera.power_pin;
        *active = camera_conf[TKL_VI_SENSOR_UVC].camera.active_level;
    } else if (device_type == DVP_CAMERA) {
        *io = camera_conf[TKL_VI_SENSOR_DVP].camera.power_pin;
        *active = camera_conf[TKL_VI_SENSOR_DVP].camera.active_level;
    }

    return 0;
}

int tkl_vi_set_dvp_i2c_pin(uint8_t clk, uint8_t sda)
{
    camera_conf[TKL_VI_SENSOR_DVP].camera.i2c.clk = clk;
    camera_conf[TKL_VI_SENSOR_DVP].camera.i2c.sda = sda;
    bk_printf("set dvp i2c, clk: %d sda: %d\r\n", clk, sda);

    tkl_io_pinmux_config(clk, TUYA_IIC0_SCL);
    tkl_io_pinmux_config(sda, TUYA_IIC0_SDA);

    // dvp used sw i2c0
    TUYA_IIC_BASE_CFG_T cfg = {
        .role = TUYA_IIC_MODE_MASTER,
        .speed = TUYA_IIC_BUS_SPEED_100K,
        .addr_width = TUYA_IIC_ADDRESS_7BIT,
    };
    tkl_i2c_init(TUYA_I2C_NUM_0, &cfg);

    return 0;
}

int tkl_vi_get_dvp_i2c_idx(uint8_t *clk, uint8_t *sda)
{
    return TUYA_I2C_NUM_0;
}

/**
 * @brief vi init
 *
* @param[in] pconfig: vi config
* @param[in] count: count of pconfig
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_init(TKL_VI_CONFIG_T *pconfig, INT32_T count)
{
    OPERATE_RET ret;

    if ((pconfig == NULL) || (pconfig->pdata == NULL)) {
        bk_printf("parameter error\r\n");
        return OPRT_INVALID_PARM;
    }

    void *video_handle = NULL;
    media_camera_device_t device = {0};

    TKL_VI_EXT_CONFIG_T *ext = (TKL_VI_EXT_CONFIG_T *)pconfig->pdata;
    if (ext->type == TKL_VI_EXT_CONF_CAMERA) {
        if (ext->camera.camera_type == TKL_VI_CAMERA_TYPE_UVC) {
            device.type = UVC_CAMERA;
            device.port = 1;
            device.format = IMAGE_MJPEG;
            memcpy(&camera_conf[TKL_VI_SENSOR_UVC], pconfig->pdata, sizeof(TKL_VI_EXT_CONFIG_T));
        } else if (ext->camera.camera_type == TKL_VI_CAMERA_TYPE_DVP) {
            device.type = DVP_CAMERA;
            device.port = 0;
            device.format = IMAGE_YUV | IMAGE_H264;
            memcpy(&camera_conf[TKL_VI_SENSOR_DVP], pconfig->pdata, sizeof(TKL_VI_EXT_CONFIG_T));
        } else {
            bk_printf("not support camera type: %d\r\n", ext->camera.camera_type);
            return OPRT_NOT_SUPPORTED;
        }

        device.rotate = ext->camera.rotate;
        device.width  = ext->camera.width;
        device.height = ext->camera.height;

        if (ext->camera.fps == 10) {
            device.fps = FPS10;
        } else if (ext->camera.fps == 15) {
            device.fps = FPS15;
        } else if (ext->camera.fps == 25) {
            device.fps = FPS25;
        } else if (ext->camera.fps == 30) {
            device.fps = FPS30;
        } else {
            bk_printf("not support camera fps: %d\r\n", ext->camera.fps);
            return OPRT_NOT_SUPPORTED;
        }
    }

    ret = media_app_camera_open(&video_handle, &device);
    if (ret != BK_OK) {
        bk_printf("%s, %d, open failed...\n", __func__, __LINE__);
        return OPRT_COM_ERROR;
    }

    ret = media_app_set_rotate(device.rotate);
    if (ret != OPRT_OK) {
        return ret;
    }

    if (device.type == UVC_CAMERA) {
        vi_uvc_status = 1;
        lcd_jdec_pipeline_open();
    } else if (device.type == DVP_CAMERA) {
        vi_dvp_status = 1;
        img_service_open();
    }

    vi_handle = video_handle;
    vi_uvc_port = device.port;
    return ret;
}

/**
* @brief vi uninit
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_uninit(TKL_VI_CAMERA_TYPE_E device_type)
{
    OPERATE_RET ret;
    camera_handle_t handle = NULL;

    camera_type_t type;
    if (device_type == TKL_VI_CAMERA_TYPE_UVC) {
        type = UVC_CAMERA;
        vi_uvc_status = 2;
        lcd_jdec_pipeline_close();
    } else if (device_type == TKL_VI_CAMERA_TYPE_DVP) {
        type = DVP_CAMERA;
        vi_dvp_status = 2;
        img_service_close();
    }
    handle = bk_camera_handle_node_pop();
    if (handle)
    {
        media_app_camera_close(&handle);
    }
    else
    {
        bk_printf("Pop node failed in %s: %d\r\n", __func__, __LINE__);
        ret = OPRT_COM_ERROR;
    }
    return ret;
}

/**
* @brief vi set mirror and flip
*
* @param[in] chn: vi chn
* @param[in] flag: mirror and flip value
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_set_mirror_flip(TKL_VI_CHN_E chn, TKL_VI_MIRROR_FLIP_E flag)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief vi get mirror and flip
*
* @param[in] chn: vi chn
* @param[out] flag: mirror and flip value
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_get_mirror_flip(TKL_VI_CHN_E chn, TKL_VI_MIRROR_FLIP_E *flag)
{
    return OPRT_NOT_SUPPORTED;
}

/**
* @brief  set sensor reg value
*
* @param[in] chn: vi chn
* @param[in] pparam: reg info
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_sensor_reg_set( TKL_VI_CHN_E chn, TKL_VI_SENSOR_REG_CONFIG_T *parg)
{
    return OPRT_NOT_SUPPORTED;
}

/**
* @brief  get sensor reg value
*
* @param[in] chn: vi chn
* @param[in] pparam: reg info
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_sensor_reg_get( TKL_VI_CHN_E chn, TKL_VI_SENSOR_REG_CONFIG_T *parg)
{
    return OPRT_OK;
}


/**
* @brief detect init
*
* @param[in] chn: vi chn
* @param[in] type: detect type
* @param[in] pconfig: config
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_init(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type, TKL_VI_DETECT_CONFIG_T *p_config)
{
    return OPRT_OK;
}


/**
* @brief detect start
*
* @param[in] chn: vi chn
* @param[in] type: detect type
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_start(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type)
{
    return OPRT_NOT_SUPPORTED;
}

/**
* @brief detect stop
*
* @param[in] chn: vi chn
* @param[in] type: detect type
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_stop(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type)
{
    return OPRT_OK;
}

/**
* @brief get detection results
*
* @param[in] chn: vi chn
* @param[in] type: detect type
* @param[out] presult: detection results
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_get_result(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type, TKL_VI_DETECT_RESULT_T *presult)
{
    return OPRT_NOT_SUPPORTED;
}

/**
* @brief set detection param
*
* @param[in] chn: vi chn
* @param[in] type: detect type
* @param[in] pparam: detection param
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_set(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type, TKL_VI_DETECT_PARAM_T *pparam)
{
    return OPRT_OK;
}

/**
* @brief detect uninit
*
* @param[in] chn: vi chn
* @param[in] type: detect type
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_uninit(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type)
{
    return OPRT_NOT_SUPPORTED;
}




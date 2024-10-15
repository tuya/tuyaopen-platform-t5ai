Multimedia power consumption
=================================

:link_to_translation:`zh_CN:[中文]`

1 Overview of multimedia power consumption
--------------------------------------------

    Based on BK7258 projects, multimedia functions are implemented on CPU1, and each hardware module, such as PSRAM, LCD, H264, USB, etc.,
    has the corresponding clock configuration, power management, etc. In order to achieve lower power consumption, the switch needs to be used symmetrically.

2 Multimedia applications
-------------------------------------

    Multimedia SDK provides a feature-rich and complete interface, for the use of the user, and user does not need to consider the vote of the application of the interface, is the need to make sure that is symmetric call, interface reference address: ``./components/multimedia/include/media_app.h``,
    The description is as follows.

    Enable/disable the camera, need to pay attention to the content of the parameters::

        bk_err_t media_app_camera_open(media_camera_device_t *device);
        bk_err_t media_app_camera_close(camera_type_t type);

    Enable/disable the interface to get images. Different types of images need to be passed different types of parameters, allowing the acquired image data to be processed directly in the callback function (parameter cb), this interface can only be effective after the camera is turned on::

        bk_err_t media_app_register_read_frame_callback(pixel_format_t fmt, frame_cb_t cb);
        bk_err_t media_app_unregister_read_frame_callback(void);

    Enable/disable h264 encoding function, this interface supports JPEG image decoding, image conversion to YUV format, and then H264 encoding output::

        bk_err_t media_app_h264_pipeline_open(void);
        bk_err_t media_app_h264_pipeline_close(void);

    Enable/disable image rotation function, this interface supports JPEG decoded images, rotation at any Angle, and then display on the corresponding LCD screen::

        bk_err_t media_app_lcd_pipline_rotate_open(media_rotate_t rotate);
        bk_err_t media_app_lcd_pipline_rotate_close(void);

    In addition to the above basic functions, it also supports some additional functions, such as when the UVC camera is opened, support registration callback function to obtain UVC enumeration information and connection status, after obtaining the need to call the parameter configuration interface, so that UVC really start.
    Support to set parameters directly in the registered callback function to start UVC::

        bk_err_t media_app_uvc_register_info_notify_cb(uvc_device_info_t cb);
        bk_err_t media_app_set_uvc_device_param(bk_uvc_config_t *config);

    Support in H264 encoding, adjust the compression rate to achieve a better image effect, specific debugging instructions please refer to the H264 encoding document, this interface only after the H264 encoding function is opened::

        bk_err_t media_app_set_compression_ratio(compress_ratio_t * ratio);

    The interface has a corresponding reference case, please refer: ``./components/multimedia/cli/media_cli.c``.

3 Multimedia voting
--------------------------

    All multimedia functions are implemented on CPU1. If the SDK interface is invoked, CPU1 will vote on by default and then process the corresponding process.
    The voting interface is as follows::

        bk_err_t bk_pm_module_vote_boot_cp1_ctrl(pm_boot_cp1_module_name_e module,pm_power_module_state_e power_state);

    The interface supports different modules to vote and power on or off. For details, reference::

        typedef enum
        {
            PM_BOOT_CP1_MODULE_NAME_FFT          = 0,
            PM_BOOT_CP1_MODULE_NAME_AUDP_SBC        ,// 1
            PM_BOOT_CP1_MODULE_NAME_AUDP_AUDIO      ,// 2
            PM_BOOT_CP1_MODULE_NAME_AUDP_I2S        ,// 3
            PM_BOOT_CP1_MODULE_NAME_VIDP_JPEG_EN    ,// 4
            PM_BOOT_CP1_MODULE_NAME_VIDP_JPEG_DE    ,// 5
            PM_BOOT_CP1_MODULE_NAME_VIDP_DMA2D      ,// 6
            PM_BOOT_CP1_MODULE_NAME_VIDP_LCD        ,// 7
            PM_BOOT_CP1_MODULE_NAME_MULTIMEDIA      ,// 8
            PM_BOOT_CP1_MODULE_NAME_APP             ,// 9
            PM_BOOT_CP1_MODULE_NAME_VIDP_ROTATE     ,// 10
            PM_BOOT_CP1_MODULE_NAME_VIDP_SCALE      ,// 11
            PM_BOOT_CP1_MODULE_NAME_GET_MEDIA_MSG   ,// 12
            PM_BOOT_CP1_MODULE_NAME_MAX             ,// attention: MAX value can not exceed 31.
        }pm_boot_cp1_module_name_e;

    The power supply of different modules also needs to be controlled by voting to ensure that the modules share the power domain. When a module needs to be used,
    the power supply needs to be turned on. When all modules do not need to work, the power failure process is carried out. Customers do not need to pay attention
    to it when using it, but they can pay attention to whether the corresponding module is really powered off when entering the low power mode. The voting interface is as follows::

        bk_err_t bk_pm_module_vote_power_ctrl(pm_power_module_name_e module,pm_power_module_state_e power_state);

    The above interface can achieve the switch function of different modules by configuring different parameters.
    The type reference file of the first parameter is: ``./bk_idk/include/modules/pm.h``. The following are the tickets defined by the multimedia module::

        /*----SUB POWER DOMAIN VIDP--------*/
        #define PM_POWER_SUB_MODULE_NAME_VIDP_DMA2D            (POWER_SUB_MODULE_NAME_VIDP_DMA2D)
        #define PM_POWER_SUB_MODULE_NAME_VIDP_YUVBUF           (POWER_SUB_MODULE_NAME_VIDP_YUVBUF)
        #define PM_POWER_SUB_MODULE_NAME_VIDP_JPEG_EN          (POWER_SUB_MODULE_NAME_VIDP_JPEG_EN)
        #define PM_POWER_SUB_MODULE_NAME_VIDP_JPEG_DE          (POWER_SUB_MODULE_NAME_VIDP_JPEG_DE)
        #define PM_POWER_SUB_MODULE_NAME_VIDP_LCD              (POWER_SUB_MODULE_NAME_VIDP_LCD)
        #define PM_POWER_SUB_MODULE_NAME_VIDP_ROTT             (POWER_SUB_MODULE_NAME_VIDP_ROTT)
        #define PM_POWER_SUB_MODULE_NAME_VIDP_SCAL0            (POWER_SUB_MODULE_NAME_VIDP_SCAL0)
        #define PM_POWER_SUB_MODULE_NAME_VIDP_SCAL1            (POWER_SUB_MODULE_NAME_VIDP_SCAL1)
        #define PM_POWER_SUB_MODULE_NAME_VIDP_H264             (POWER_SUB_MODULE_NAME_VIDP_H264)

    When different PASRAM modules are used, they will also vote to ensure normal function and low power consumption. The voting interface is::

        bk_err_t bk_pm_module_vote_psram_ctrl(pm_power_psram_module_name_e module,pm_power_module_state_e power_state);

    The above interface can achieve the control of PASRAM by different modules by configuring different parameters. The enumerated values of different modules are as follows::

        typedef enum
        {
            PM_POWER_PSRAM_MODULE_NAME_FFT       = 0,
            PM_POWER_PSRAM_MODULE_NAME_AUDP_SBC     ,// 1
            PM_POWER_PSRAM_MODULE_NAME_AUDP_AUDIO   ,// 2
            PM_POWER_PSRAM_MODULE_NAME_AUDP_I2S     ,// 3
            PM_POWER_PSRAM_MODULE_NAME_VIDP_JPEG_EN ,// 4
            PM_POWER_PSRAM_MODULE_NAME_VIDP_H264_EN ,// 5
            PM_POWER_PSRAM_MODULE_NAME_VIDP_JPEG_DE ,// 6
            PM_POWER_PSRAM_MODULE_NAME_VIDP_DMA2D   ,// 7
            PM_POWER_PSRAM_MODULE_NAME_VIDP_LCD     ,// 8
            PM_POWER_PSRAM_MODULE_NAME_APP          ,// 9
            PM_POWER_PSRAM_MODULE_NAME_AS_MEM       ,// 10
            PM_POWER_PSRAM_MODULE_NAME_CPU1         ,// 11
            PM_POWER_PSRAM_MODULE_NAME_MEDIA        ,// 12
            PM_POWER_PSRAM_MODULE_NAME_LVGL_CODE_RUN,// 13
            PM_POWER_PSRAM_MODULE_NAME_MAX          ,// attention: MAX value can not exceed 31.
        }pm_power_psram_module_name_e;

4 multimedia low power inspection
------------------------------------

    When it is necessary to enter the low power consumption, all peripherals need to be turned off, including PSRAM, to ensure that all modules are powered off,
    you can send the following command to check::

        pm_debug 8

    By default, for example, when the command is powered on, the output is as follows::

        1. pm video and audio state:0x0 0x0

        2. pm ahpb and bakp state:0x0 0x10001

        3. pm low vol[module:0xffbfefff] [need module:0xffffffff]

        4. pm deepsleep[module:0xc0][need module:0x3c0]

        5. pm power and pmu state[0x200000c8][0x2f1]

        6. Attention the bakp not power down[modulue:0x10001]

        7. pm_psram_ctrl_state:0x0 0x0

        8. pm_cp1_ctrl_state:0x0

        9. pm_cp1_boot_ready:0x0 0xffffffff

        10. pm_module_lv_sleep_state:0x0

    For the multimedia module, pay attention to the first line (indicating the internal power control of each multimedia module, and the corresponding ticket
    is pm_power_module_name_e, which should be 0 when not used) and the seventh line (indicating the power control of PSRAM, because several modules use PARAM.
    Line 8 (indicates that different modules voted for CPU1 to start, and the corresponding vote is pm_boot_cp1_module_name_e, which should be 0 when not applicable).

.. note::

    The above interfaces do not require the user to encapsulate the call, when using multimedia functions, these functions have been encapsulated into the internal logic
    of the switch, only need to ensure that the switch is symmetric when used. In addition, the above command is a check method that depends on the CLI command function.
    CLI functions may not be enabled.

JPEG编码
=================================

:link_to_translation:`en:[English]`

1、功能概述
--------------------

	JPEG编码主要用于将YUV422的数据通过硬件编码，输出JPEG图像数据，并且压缩图像数据量。当前JPEG功能主要用在DVP摄像头，DVP摄像头输出YUV422数据给

芯片，芯片压缩成JPEG图像。也支持将外部YUV422数据，压缩成JPEG图像。由于BK7258有一个专门处理YUV数据的模块YUV_BUF，JPEG模块也需要依赖它。

2、开发资料
--------------------

	当前avdk提供了详细的sdk接口，参考文件路径：``.\bk_idk\include\driver\jpeg_enc.h``，``.\bk_idk\include\driver\jpeg_enc.h``

	为了方便客户使用，重新封装了编码功能，当前只需要开关摄像头，然后针对图像质量，也提供一个api接口去调节，参考文件：``.\components\multimedia\app\media_app.c``.


3、接口说明
-------------------------------

	BK7258支持将DVP摄像头数据的数据编码成JPEG、H264以及同步输出YUV422数据用于LCD显示。

 - 打开DVP摄像头接口::

 	bk_err_t media_app_camera_open(media_camera_device_t *device);

 - 关闭DVP摄像头接口::

 	bk_err_t media_app_camera_close(camera_type_t type);

需要注意传入形参的数据结构
::

	typedef enum
	{
		UNKNOW_CAMERA,
		DVP_CAMERA,    /**< dvp camera */
		UVC_CAMERA,    /**< uvc camera */
		NET_CAMERA,    /**< net camera, use dual chip transfer each other*/
	} camera_type_t;

	typedef enum
	{
		UNKNOW_MODE = 0,
		YUV_MODE,     /**< output yuv data */
		GRAY_MODE,    /**< output gray data */
		JPEG_MODE,    /**< output jpeg data */
		H264_MODE,    /**< output h264 data */
		H265_MODE,    /**< output h265 data */
		JPEG_YUV_MODE,/**< output jpeg & yuv data */
		H264_YUV_MODE,/**< output h264 & yuv data */
	} yuv_mode_t;

	typedef struct {
		camera_type_t  type; /**< camera type */
		yuv_mode_t     mode; /**< work mode */
		pixel_format_t fmt;  /**< camera output data format */
		frame_info_t   info; /**< camera output resolution and fps */
	} media_camera_device_t;

根据上述的数据结构，配置对应的参数，传入到接口中，以达到相应的期望结果。比如当前使用DVP摄像头，则摄像头类型为DVP_CAMERA，如果要同时输出JPEG和YUV数据
则yuv模式设为JPEG_YUV_MODE，如果只需要JPEG图像，则yuv模式设为YUV_MODE。

 - 调整压缩率的接口::

 	bk_err_t media_app_set_compression_ratio(compress_ratio_t *ratio);

根据传入参数的数据结构，是支持JPEG和H264两种方式的压缩率调节，此接口支持在编码过程中调用
::

	typedef struct {
		yuv_mode_t mode;  /**< 需要调节的模式，只支持JPEG_MODE和H264_MODE */
		h264_qp_t qp;     /**< h264编码的初始qp值，qp值越小压缩率越大 */
		uint8_t  enable;  /**< 使能编码调整 */
		uint16_t jpeg_up; /**< 调整jpeg压缩，目标图像输出大小上限，单位byte */
		uint16_t jpeg_low;/**< 调整jpeg压缩，目标图像输出大小上限，单位byte */
		uint16_t imb_bits;/**< h264编码，目标I帧输出大小上限，单位byte */
		uint16_t pmb_bits;/**< h264编码，目标P帧输出大小上限，单位byte */
	} compress_ratio_t;

根据上述的数据结构，配置对应的参数，传入到接口中，以达到相应的期望结果。比如调整JPEG编码压缩率，mode设为JPEG_MODE，jpeg编码过程中，当输出一帧
图像的大小超过jpeg_up，会进一步压缩，使得数据量不超过jpeg_up且不小于jpeg_low。

4、驱动代码
------------------

DVP摄像头驱动代码路径：``.\bk_idk\middleware\camera\dvp_camera.c``.

底层驱动代码路径：``.\bk_idk\middleware\jpeg``, ``.\bk_idk\middleware\yuv_buf``.

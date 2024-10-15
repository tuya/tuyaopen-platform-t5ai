JPEG Encoding
========================

:link_to_translation:`zh_CN:[中文]`

1. Function overview
-----------------------

	JPEG encoding is mainly used to encode YUV422 data through hardware, output JPEG image data, and compress image data. At present, JPEG function is mainly used in DVP camera, DVP camera output YUV422 data to

Chip, chip compression into JPEG image. It also supports compression of external YUV422 data into JPEG images. Since BK7258 has a module, YUV_BUF, that deals specifically with YUV data, the JPEG module also needs to rely on it.

2. Develop materials
-----------------------

	Currently avdk provides a detailed sdk interface, reference file path:``.\bk_idk\include\driver\jpeg_enc.h``,
	``.\bk_idk\include\driver\jpeg_enc.h``.

	In order to facilitate the use of customers, re-packaging the coding function, the current only need to switch the camera,
	and then for the image quality, also provide an api interface to adjust, reference file:``.\components\multimedia\app\media_app.c``.


3. Interface description
-------------------------------

	BK7258 supports the encoding of DVP camera data into JPEG, H264 and synchronous output of YUV422 data for LCD display.

- Open DVP camera interface::

 		bk_err_t media_app_camera_open(media_camera_device_t *device);

- Close DVP camera interface::

 		bk_err_t media_app_camera_close(camera_type_t type);

You need to pay attention to the data structure passed into the parameter
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

According to the above data structure, the corresponding parameters are configured and passed into the interface to achieve
the corresponding expected result. For example, the current use of DVP camera, then the camera type is DVP_CAMERA,
if you want to output JPEG and YUV data, Then the yuv mode is set to JPEG_YUV_MODE, or if only JPEG images are needed,
the yuv mode is set to YUV_MODE.

- Interface for adjusting compression rate ::

		bk_err_t media_app_set_compression_ratio(compress_ratio_t *ratio);

According to the data structure of the passed parameters, the compression rate adjustment is supported in both JPEG and H264,
and this interface supports calling during the encoding process.
::

	typedef struct {
		yuv_mode_t mode;   /**< The mode to be adjusted. Only JPEG_MODE and H264_MODE */ are supported
		h264_qp_t qp;      /**< The initial qp value of the h264 encoding, the smaller the qp value, the larger the compression rate */
		uint8_t  enable;   /**< Enable encoding adjustment */
		uint16_t jpeg_up;  /**< Adjust the upper limit of jpeg compression, target image output size, in byte */
		uint16_t jpeg_low; /**< Adjust the upper limit of jpeg compression, target image output size, in byte */
		uint16_t imb_bits; /**< h264 encoding, upper limit of target I frame output size, unit byte */
		uint16_t pmb_bits; /**< h264 encoding, upper limit of the output size of the target P-frame, unit byte */
	} compress_ratio_t;

According to the above data structure, the corresponding parameters are configured and passed into the interface to achieve
the corresponding expected result. For example, adjust the JPEG encoding compression rate, mode set to JPEG_MODE, jpeg encoding process,
when output a frame. If the size of the image exceeds jpeg_up, it is further compressed so that the amount of data is no more than
jpeg_up and no less than jpeg_low.

4. Driver code
------------------

	DVP camera driver code path：``.\bk_idk\middleware\camera\dvp_camera.c``.

	Underlying driver ocde path：``.\bk_idk\middleware\jpeg``, ``.\bk_idk\middleware\yuv_buf``.
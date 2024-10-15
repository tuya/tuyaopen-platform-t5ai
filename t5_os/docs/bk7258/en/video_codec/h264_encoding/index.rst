H264 Encoding
========================

:link_to_translation:`zh_CN:[中文]`


1. Function overview
---------------------

	h264 coding is mainly used to encode YUV422 data through hardware, output h264 image data, and compress image data

2. Develop materials
----------------------
	Currently avdk provides a detailed sdk interface, refer to the file path:`.\bk_idk\middleware\driver\h264`

	In order to facilitate the use of customers, re-packaging the coding function, the current only need to switch the camera, and then for the image quality, also provide an api interface to adjust, reference file:.\components\multimedia\app\media_app.c '

3. Interface description
-------------------------------

	Debugging description based on h264 image quality: media_app_set_compression_ratio

	Structure parameter description: ``compress_ratio_t``

	- yuv_mode_t mode:		The current mode to be adjusted. Only JPEG and H264 are supported. Select H264
	- h264_qp_t  h264_qp:	The qp value of h264 encode value
	- uint8_t    enable:	enables compression adjustment, which is supported only in JPEG mode
	- uint16_t   jpeg_up:	Upper limit of data JPEG image size, in byte, valid only in JPEG mode
	- uint16_t   jpeg_low:	Lower limit of data JPEG image size, in byte, valid only in JPEG mode
	- uint16_t   imb_bits:	The size of the I frame macro block encoded by H264, in byte. This parameter is valid only in H264 mode. The larger the value, the smaller the compression rate, the higher the image quality and the larger the encoded output image, range [1, 4095].
	- uint16_t   pmb_bits:	The size of the H264 output P-frame macro block, in byte, is valid only in H264 mode. The larger the value, the smaller the compression rate, the higher the image quality, and the larger the encoded output image, range [1, 4095].

	Structure parameter description: ``h264_qp_t``

	- uint8_t init_qp;		The init qp value，range [0, 51];
	- uint8_t i_min_qp;		The minimum qp of I frame encode, range [0, 51];
	- uint8_t i_max_qp;		The maximum qp of I frame encode, range [0, 51];
	- uint8_t p_min_qp;		The minimum qp of P frame encode, range [0, 51];
	- uint8_t p_max_qp;		The maximum qp of P frame encode, range [0, 51];

.. note::

	In addition to paying attention to the valid range of the above values, it is also necessary to pay attention to the maximum value of the corresponding encoding
	must be greater than the minimum value. The image quality can be adjusted through the above interface. It should be noted that improving the image quality will
	undoubtedly increase the amount of data. At present, the maximum output space of a frame of yuv422 data encoding is 64KB.
	The encoded H264 image output is larger than 64K, and there will be instability.

4. Code gear adjust
---------------------------

	The default SDK provides the configuration of h264 encoded gear adjustment, which is defined by the configuration macro: ``CONFIG_H264_QUALITY_LEVEL``, which is described as follows:

	- Value range: [0, 3], define three gears, the values are 1/2/3, corresponding to the h264 compressed image quality from low to high, the clearer the image is.
	- If CONFIG_H264_QUALITY_LEVEL=0, the default value is used instead of the three-gear parameter. The default value reference path: ``.\bk_idk\middleware\soc\bk7258\hal\h264_default_config.h``, you can change the default value to achieve the desired effect.
	- The default SDK gear is defined in the middle gear, CONFIG_H264_QUALITY_LEVEL=2. You can change this value in project config.

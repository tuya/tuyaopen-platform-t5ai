Camera Debug Info
=================================

:link_to_translation:`zh_CN:[中文]`

1 Function Overview
--------------------
	The main function of this project is that after the hardware is connected to the uvc/dvp camera, the data information of each time after MJPEG decoding can be obtained from the serial port, 
	such as the data format after decoding,whether to rotate and so on

2 Project Code Path
-------------------------------------
	Demo path:``./projects/peripheral/uvc`` or ``./projects/peripheral/dvp``

3 Compiling Commands
-------------------------------------
	Compile the command：``make bk7258 PROJECT=peripheral/uvc`` or ``make bk7258 PROJECT=peripheral/dvp``

4 Presentation
-------------------------------------
	Select the corresponding compilation command according to the type of camera used, connect the development board camera and download the program, open the serial port and press the reset button to see the information printing.
	The information mainly includes the MJPEG data obtained by the camera and the data information output after MJPEG decoding.

  	The MJPEG data obtained by the camera includes camera type (camera_type), frame frame_id, frme_length, and frame address.
	MJPEG decoded data ppi, rotation Angle, output data format and big/little endian.

	.. figure:: ../../../_static/camera_mjpeg_decode_info.png
		:align: center
		:alt: camera_mjpeg_decode info
		:figclass: align-center

		Figure 1. Output of the serial port

5 DVP camera debugging
------------------------------------

	All supported DVP camera driver code paths:``./components/bk_peripheral/src/dvp``.

	1. When adding a new camera driver, you first need to refer to the default camera implementation method to write your own driver code.

Refer to the data structure of the camera driver
::

	typedef struct
	{
		char *name;  /**< sensor name */
		media_ppi_t def_ppi;  /**< sensor default resolution */
		frame_fps_t def_fps;  /**< sensor default fps */
		mclk_freq_t  clk;  /**< sensor work clk in config fps and ppi */
		pixel_format_t fmt; /**< sensor input data format */
		sync_level_t vsync; /**< sensor vsync active level  */
		sync_level_t hsync; /**< sensor hsync active level  */
		uint16_t id;  /**< sensor type, sensor_id_t */
		uint16_t address;  /**< sensor write register address by i2c */
		uint16_t fps_cap;  /**< sensor support fps */
		uint16_t ppi_cap;  /**< sensor support resoultions */
		bool (*detect)(void);  /**< auto detect used dvp sensor */
		int (*init)(void);  /**< init dvp sensor */
		int (*set_ppi)(media_ppi_t ppi);  /**< set resolution of sensor */
		int (*set_fps)(frame_fps_t fps);  /**< set fps of sensor */
		int (*power_down)(void);  /**< power down or reset of sensor */
		int (*dump_register)(media_ppi_t ppi);  /**< dump sensor register */
		void (*read_register)(bool enable);  /**< read sensor register when write*/
	} dvp_sensor_config_t;

	Description of some parameters:

	clk: clock input for the camera. The default clock is 24MHz, which needs to be configured according to the camera specification.

	fmt: The format of camera output data to the chip. Currently, only YUV420 is supported, and the sequence needs to be synchronized according to the camera output sequence. The default is YUYV.

	vsync: camera output vsync effective level. When some cameras vsync is low, the output valid data needs to be synchronized with the camera vsync output level. The default high level is valid.

	hsync: effective level of camera output vsync. When vsync of some cameras is low, the output of valid data needs to be synchronized with the hsync output level of the camera. The default high level is valid.

	address: Configure the I2C slave address of the camera register. The parameter needs to be configured according to the camera specification.

	fps_cap: The frame rate table supported by the camera, which needs to be configured with the corresponding register.

	ppi_cap: resolution table supported by camera, need to configure the corresponding register to achieve.

The above parameters are recommended to use the default configuration. If the actual output image is different from the configuration, it is not recommended to modify the driver configuration. It is recommended to modify the register configuration to achieve synchronization between the two.

	2. Configure the driver code and test whether the diagram can be drawn. It is recommended to use the media_app_camera_open() interface, command line, or corresponding apk to test the image transmission.

	- media dvp open 640X480 jpeg | Indicates that the dvp camera is opened, the image resolution is set to 640X480, and the jpeg encoding is output

	See if there is a corresponding log print, and if the number after the jpg is not 0::

		cpu1:media_ui:I(4012):jpg:14[41, 0], h264:0[0], dec:0[0], lcd:0[0], lcd_fps:0[0], lvgl:0[0]

	3. Analysis of common problems:

	- Cannot recognize the dvp camera:

	In this case, first consider whether the power supply of the camera is normal, whether it meets the requirements of the specification, whether the DVDD/IOVDD is normal, and whether the physical connection of the camera is normal;

	If the I2C connection is normal, I2C1, GPIO0, and GPIO1 are used by default

	Whether MCLK is normal can be tested by logos or oscilloscopes

	- resolution error: Resolution error:

	In this case, the resolution of the configuration is inconsistent with the actual output resolution. In some cases, although the software configuration is no problem, the actual output of the camera may be different, such as the configuration of 640X480.

	vsync, hsync and pclk signals are measured by oscilloscope or logic. There are 480 hsync pulses in a vsync and 640X2(1280) pclk pulses in a hsync. These parameters need to be strictly observed

	The vsync/hsync signal effective level of the driver configuration and whether it is consistent with the actual output need to configure the camera register.

	- Coding error

	Whether the YUV422 data sequence of the driver configuration is consistent with the internal register configuration of the camera, it is recommended to be configured as YUYV

	Whether the frame interval time is enough, from the completion of one frame to the beginning of the next frame, it is recommended that the frame interval is not less than 4ms, which can be achieved by configuring the camera register.

	dvp cameras support different encoding modes, refer to the data structure

6 Commissioning the UVC camera
--------------------------------

	UVC belongs to plug and play, it is necessary to pay attention to whether the configured resolution and output format are supported, if a resolution that is not supported by the camera is configured, it cannot be mapped. If only JPEG output is supported, but configured to work with H264 output

You can't draw a picture.

	1. Analysis of common problems:

	- Resolution error: the camera does not support this resolution, there will be a corresponding log reminder;

	- No encoding format: The camera does not support the encoding format you configured;

	- Image screen: need to analyze whether the unpack is normal, some cameras will appear, a frame jpeg image has not been transmitted, and then re-transmit a new frame image, it is recommended to use the protocol analyzer to capture data and refer to the unpack code.

	  Analysis resolution where there is no well compatible:``./bk_idk/middleware/camera/uvc_camera.c``;

	- It is recommended to use the standard transmission mode, that is, the packet header with uvc;
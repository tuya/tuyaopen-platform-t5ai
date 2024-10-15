Camera Debug Info
=================================

:link_to_translation:`en:[English]`

1 功能概述
--------------------
	本工程主要功能是在硬件接入uvc/dvp摄像头之后，可以从串口获得MJPEG解码后每次的数据信息，如解码之后的数据格式，是否旋转等

2 工程代码路径
-------------------------------------
	demo路径：``./projects/peripheral/uvc`` 或 ``./projects/peripheral/dvp``

3 编译命令
-------------------------------------
	编译命令：``make bk7258 PROJECT=peripheral/uvc`` 或 ``make bk7258 PROJECT=peripheral/dvp``

4 演示介绍
-------------------------------------
	根据使用的摄像头类型选择对应的编译命令，将开发板摄像头连接并下载程序，打开串口按下复位键即可看见信息打印。信息主要有摄像头获取到的MJPEG数据以及MJPEG解码之后输出的数据信息。

  	摄像头获取的MJPEG数据信息有摄像头类型（camera_type），读取到的frame的frame_id，frme_length,以及frame地址。
	MJPEG解码之后的数据的ppi，旋转的角度，输出的数据格式以及大小端。

	.. figure:: ../../../_static/camera_mjpeg_decode_info.png
		:align: center
		:alt: camera_mjpeg_decode info
		:figclass: align-center

		Figure 1. 串口输出信息

5 DVP摄像头调试
------------------------------------
	所有支持的DVP摄像头驱动代码路径：``./components/bk_peripheral/src/dvp``。

	1.在增加新的摄像头驱动时，首先需要参考已经默认摄像头实现的方式书写自己的驱动代码。

参考摄像头驱动的数据结构
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

	一些参数的说明：
	clk：摄像头输入的时钟，默认使用24MHz，需要根据摄像头规格书自行配置；
	fmt：摄像头输出数据给芯片的格式，当前只支持YUV420，顺序需要根据摄像头输出顺序同步，默认YUYV；
	vsync：摄像头输出vsync有效电平，有些摄像头vsync为低时，输出有效数据，需要与摄像头vsync输出电平同步，默认高电平有效；
	hsync：摄像头输出vsync有效电平，有些摄像头vsync为低时，输出有效数据，需要与摄像头hsync输出电平同步，默认高电平有效；
	address：配置摄像头寄存器的I2C slave地址，参需要根据摄像头规格书自行配置；
	fps_cap：摄像头支持的帧率表，需要配置对应的寄存器来实现；
	ppi_cap：摄像头支持的分辨率表，需要配置对应的寄存器来实现；

上面的参数建议使用默认配置，如果实际输出的图像与配置有出入，不建议修改驱动配置，建议修改寄存器的配置，来达到两者同步。

	2.配置好驱动代码，尝试测试能否出图。建议使用media_app_camera_open()接口实现，或者命令行，或者相应的apk进行图传测试：

	- media dvp open 640X480 jpeg | 表示打开dvp摄像头，且输出的图像分辨率为640X480，并执行jpeg编码输出

	观察是否有相应的log打印，jpg后面的数字是否不为0::

		cpu1:media_ui:I(4012):jpg:14[41, 0], h264:0[0], dec:0[0], lcd:0[0], lcd_fps:0[0], lvgl:0[0]

	3.常见问题分析：

	- 无法识别dvp摄像头：

		这种情况先考虑摄像头的供电是否正常，是否符合规格书的要求，DVDD/IOVDD是否正常，摄像头物理连接是否正常；

		I2C连接是否正常，默认使用I2C1，及GPIO0和GPIO1

		MCLK是否正常，可以通过逻分或者示波器进行测试

	- 分辨率错误，resolution error:

		这种情况先考虑配置的分辨率与实际输出的分辨率不一致，有些情况下，虽然软件配置没有问题，但是摄像头实际输出可能会有差别，比如配置640X480。

		用示波器或者逻分测量vsync、hsync、pclk信号，一个vsync内部有480个hsync脉冲，一个hsync内部有640X2(1280)个pclk脉冲，这些参数是需要严格遵守的

		驱动配置的vsync/hsync信号有效电平，是否和实际输出一致，这些都需要配置摄像头的寄存器。

	- 编码错误

		驱动配置的YUV422数据顺序是否和摄像头内部寄存器配置的一致，建议都配成YUYV

		帧间距时间是否足够，一帧完成到下一帧开始，建议帧间距不小于4ms，这个可以通过配置摄像头寄存器实现。

		dvp摄像头支持不同的编码模式，参考数据结构：``

6 UVC摄像头调试
------------------

	UVC属于即插即用，需要注意是否支持配置的分辨率，输出格式，如果配置一个摄像头不支持的分辨率，是不能出图的。如果只支持JPEG输出，但是配置让其工作H264输出

是不能出图的。

	1.常见问题分析：

	- 分辨率错误：摄像头不支持这种分辨率，会有相应的log提醒；

	- 不支持编码格式：摄像头不支持你配置的编码格式；

	- 图像花屏：需要分析解包是否正常，有些摄像头会出现，一帧jpeg图像还没传输完成，又重新传输新的一帧图像，建议用协议分析仪抓取数据并参考解包代码，

	分析解析哪里没有做好兼容：``./bk_idk/middleware/camera/uvc_camera.c``；

	- 建议使用标准传输方式，即带有uvc的包头；

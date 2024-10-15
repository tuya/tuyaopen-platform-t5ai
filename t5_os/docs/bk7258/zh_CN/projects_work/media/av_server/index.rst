Double-board intertransfermission(server)
======================================================

:link_to_translation:`en:[English]`

1 功能概述
-------------------------------------
	本工程展示两块BK7258板子之间视频单向传输，音频双向传输的的功能，server端不接入实体摄像头dvp/uvc，且支持当前板子接入LCD屏实时显示client端传输过来的摄像头拍摄画面

2 代码路径
-------------------------------------
	demo路径：``./projects/media/av_client/main/av_server_main.c``

	编译命令：``make bk7258 PROJECT=media/av_server``


3 传输宏控
-------------------------------------
	传输方式

	- CONFIG_AV_DEMO_MODE_UDP:使用UDP连接进行传输(当前默认)

	- CONFIG_AV_DEMO_MODE_TCP:使用TCP连接进行传输

4 参数配置
-------------------------------------
	需要配置的参数涉及：摄像头输出数据格式(JPEG/H264等)，摄像头数据分辨率和帧率，显示屏幕的型号和分辨率，音频的采样率等

	参考:``./projects/media/av_client/main/av_server_main.c``，中结构体:``video_dev``，的参数说明。

.. note::

	当前摄像头的类型不能修改，video_dev.device.type = NET_CAMERA，表示server端接入的是网络摄像头，视频数据是从网络中收到的。

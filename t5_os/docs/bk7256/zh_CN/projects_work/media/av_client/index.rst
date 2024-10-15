Double-board intertransfermission(client)
======================================================

:link_to_translation:`en:[English]`

1 功能概述
-------------------------------------
	本工程展示两块BK7256板子之间视频单向传输，音频双向传输的的功能，client端接入实体摄像头dvp/uvc，且支持当前板子接入LCD屏实时显示摄像头拍摄的画面

2 代码路径
-------------------------------------
	demo路径：``./projects/media/av_client/main/av_client_main.c``

	编译命令：``make bk7256 PROJECT=media/av_client``


3 传输宏控
-------------------------------------
	传输方式

	- CONFIG_AV_DEMO_MODE_UDP:使用UDP连接进行传输(当前默认)

	- CONFIG_AV_DEMO_MODE_TCP:使用TCP连接进行传输

4 参数配置
-------------------------------------
	需要配置的参数涉及：使用摄像头的类型(UVC/DVP)，摄像头输出数据格式(JPEG/H264等)，摄像头数据分辨率和帧率，显示屏幕的型号和分辨率，音频的采样率等

	参考:``./projects/media/av_client/main/av_client_main.c``，中结构体:``video_dev``，的参数说明

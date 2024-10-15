Double-Board Intertransfer Mission (Server)
==============================================

:link_to_translation:`zh_CN:[中文]`

1 Function Overview
---------------------------------------------------------------------------------------------------------------------------
	This project displays two BK7256 plates between the video between the video between the video, the function of the two way transmission of the audio, the Server side does not access the DVP/UVC of the physical camera, and supports the current board connecting the LCD screen to the camera shooting screen transmitted by the CLIENT terminal.

2 code path
---------------------------------------------------------------------------------------------------------------------------
	DEMO path: ``./projects/media/av_client/main/av_server_main.c``

	Compile commands: ``make BK7256 PROJECT=media/av_server``


3 Transfer macro control
---------------------------------------------------------------------------------------------------------------------------
	transfer method

	-CONFIG_AV_DEMO_MODE_UDP: Use UDP connection for transmission (currently default)

	-CONFIG_AV_DEMO_MODE_TCP: Use TCP connection to transfer

4 parameter configuration
---------------------------------------------------------------------------------------------------------------------------
	The parameters that need to be configured are involved: camera output data format (JPEG/H264, etc.), camera data resolution and frame rate, display screen model and resolution, audio sampling rate, etc.

	Reference: ``./projects/media/av_client/main/av_server_main.c``, mid -structure: ``video_dev``, parameter description.

.. note ::

	The type of the current camera cannot be modified. Video_dev.device.type = net_camera means that the server is connected to the network camera, and the video data is received from the network.

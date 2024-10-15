Double-Board Intertransfer Mission (Client)
==============================================

:link_to_translation:`zh_CN:[中文]`

1 Function Overview
---------------------------------------------------------------------------------------------------------------------------
	This project displays two BK7258 boards between the video between the video between the video, the function of the two -way transmission of the audio, the client is connected to the DVP/UVC of the physical camera, and supports the current panel access LCD screen real -time display of the camera shooting screen.

2 code path
---------------------------------------------------------------------------------------------------------------------------
	DEMO path: ``./projects/media/av_client/main/av_client_main.c``

	Compiled command: ``make BK7258 PROJECT = media/av_client``


3 Transfer macro control
---------------------------------------------------------------------------------------------------------------------------
	transfer method

	-CONFIG_AV_DEMO_MODE_UDP: Use UDP connection for transmission (currently default)

	-CONFIG_AV_DEMO_MODE_TCP: Use TCP connection to transfer

4 parameter configuration
---------------------------------------------------------------------------------------------------------------------------
	The parameters that need to be configured are involved: the type of the camera (UVC/DVP), the camera output data format (JPEG/H264, etc.), the camera data resolution and frame rate, the model and resolution of the display screen, the sampling rate of audio, etc.

	Reference: ``./projects/media/av_client/main/av_Client_main.c``, mid -structure: `` video_dev``, parameter description.


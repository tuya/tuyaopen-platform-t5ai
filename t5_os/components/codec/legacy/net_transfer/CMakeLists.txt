
#TODO should move all public include dir to modules
set(incs
	video_work
	net_work
	avi_work
	rtsp_server/include
	)

set(srcs "")

if (CONFIG_VIDEO_AVI)
	list(APPEND srcs
	#avi_work/app_video_store.c
	avi_work/app_jpeg2avi.c
	)
endif()

if (CONFIG_NTP_SYNC_RTC)
	list(APPEND srcs
	time/time.c
	time/ntp.c
	time/app_time_intf.c
	)
endif()

if (CONFIG_APP_DEMO_VIDEO_TRANSFER AND CONFIG_SYS_CPU0)
	list(APPEND srcs
		video_work/video_transfer_tcp.c
		video_work/video_transfer_udp.c
		video_work/video_upd_spd.c
		video_work/video_upd_spd_pub.c
	)
endif()

if (CONFIG_NET_WORK_VIDEO_TRANSFER AND CONFIG_SYS_CPU0)
	list(APPEND srcs
		net_work/video_demo_main.c
		# net_work/video_demo_station.c
		# net_work/video_demo_softap.c
	)
endif()

if (CONFIG_RTSP_SERVER AND CONFIG_SYS_CPU0)
	list(APPEND srcs
		rtsp_server/src/h264_parse.c
		rtsp_server/src/rtsp_handler.c
		rtsp_server/src/rtsp_parse.c
	)
endif()

if (CONFIG_P2P)
	list(APPEND srcs
	net_work/video_demo_p2p.c
	)
endif()

#TODO fix main
armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	REQUIRES bk_wifi bk_cli bk_rtos
	PRIV_REQUIRES bk_common aud_intf lwip_intf_v2_1 avdk_libs multimedia media
	)

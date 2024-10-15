if (CONFIG_AUD_INTF)
if (CONFIG_AUDIO_TRANSFER)
if (NOT (CONFIG_AUD_TRAS_MODE_CPU1 AND ((CONFIG_CPU_CNT EQUAL 1))))

set(incs include)

set(srcs
	#mb/aud_mb.c
	aud/aud_tras_drv.c
	)

if (CONFIG_SYS_CPU0)
	list(APPEND srcs
		core/aud_intf.c
		core/aud_tras.c
	)

if (CONFIG_AUD_INTF_TEST)
	list(APPEND srcs
		demo/aud_intf_demo.c
	)
endif()

if (CONFIG_AUD_TRAS_AEC_DUMP_MODE_UDP)
	list(APPEND srcs
		debug/aud_debug_udp.c
	)
endif()

if (CONFIG_LWIP_V2_1)
	list(APPEND srcs
		debug/aud_debug_tcp.c
	)
endif()

endif()

endif()
endif()
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common lwip_intf_v2_1 avdk_libs)

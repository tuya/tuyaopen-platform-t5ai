set(incs include  .)
set(srcs
    ap_main.c
    )

if (CONFIG_INTEGRATION_DOORBELL)
	list(APPEND srcs
		src/doorbell_core.c
		src/doorbell_boarding.c
		src/doorbell_sdp.c
		src/doorbell_network.c
		src/doorbell_transmission.c
		src/doorbell_udp_service.c
		src/doorbell_tcp_service.c
		src/doorbell_devices.c
		src/doorbell_cmd.c
		src/bk_smart_config.c
	)
endif()

if (CONFIG_INTEGRATION_DOORBELL_CS2)
list(APPEND srcs
	src/doorbell_cs2_service.c
)
endif()

set(priv_req  media_service multimedia wifi_transfer avdk_utils lwip_intf_v2_1 bk_wifi_driver cs2p2p bt)

if ("${ARMINO_SOC}" STREQUAL "bk7258")
list(APPEND priv_req)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}"
)

if (CONFIG_WANSON_SIMD)
target_compile_options(${COMPONENT_LIB} PRIVATE -O3)
endif()

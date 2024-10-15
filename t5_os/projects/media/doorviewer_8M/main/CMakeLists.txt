set(incs . ../../doorviewer/main/include)
set(srcs app_main.c)

if (CONFIG_INTEGRATION_DOORBELL)
if (CONFIG_SYS_CPU0)
list(APPEND srcs
	../../doorviewer/main/src/doorbell_core.c
	../../doorviewer/main/src/doorbell_boarding.c
	../../doorviewer/main/src/doorbell_sdp.c
	../../doorviewer/main/src/doorbell_network.c
	../../doorviewer/main/src/doorbell_transmission.c
	../../doorviewer/main/src/doorbell_udp_service.c
	../../doorviewer/main/src/doorbell_tcp_service.c
	../../doorviewer/main/src/doorbell_devices.c
	../../doorviewer/main/src/doorbell_cmd.c
)
endif()
endif()

if (CONFIG_INTEGRATION_DOORBELL_CS2)
list(APPEND srcs
	../../doorviewer/main/src/doorbell_cs2_service.c
)
endif()

set(priv_req bk_init bt media_service lwip_intf_v2_1 wifi_transfer multimedia cs2p2p)

if (CONFIG_OVERRIDE_FLASH_PARTITION)
	list(APPEND srcs
		vendor_flash.c
	)
endif()

armino_component_register(
	SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES "${priv_req}"
)

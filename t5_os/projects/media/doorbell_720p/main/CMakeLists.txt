set(incs . ../../doorbell/main/include)
set(srcs app_main.c)

if (CONFIG_INTEGRATION_DOORBELL)
if (CONFIG_SYS_CPU0)
list(APPEND srcs
	../../doorbell/main/src/doorbell_core.c
	../../doorbell/main/src/doorbell_boarding.c
	../../doorbell/main/src/doorbell_sdp.c
	../../doorbell/main/src/doorbell_network.c
	../../doorbell/main/src/doorbell_transmission.c
	../../doorbell/main/src/doorbell_udp_service.c
	../../doorbell/main/src/doorbell_tcp_service.c
	../../doorbell/main/src/doorbell_devices.c
	../../doorbell/main/src/doorbell_cmd.c
)
endif()
endif()

if (CONFIG_INTEGRATION_DOORBELL_CS2)
list(APPEND srcs
	../../doorbell/main/src/doorbell_cs2_service.c
)
endif()

set(priv_req bk_init bt media_service lwip_intf_v2_1)


if ("${ARMINO_SOC}" STREQUAL "bk7256") 
list(APPEND priv_req media cs2p2p aud_intf)
endif()

if ("${ARMINO_SOC}" STREQUAL "bk7258")
list(APPEND priv_req multimedia cs2p2p)
endif()

if (CONFIG_OVERRIDE_FLASH_PARTITION)
	list(APPEND srcs
		vendor_flash.c
	)
endif()

armino_component_register(
	SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES "${priv_req}" wifi_transfer
)

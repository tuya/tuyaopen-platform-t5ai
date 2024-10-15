set(incs .
    include)

set(srcs rlk_server_main.c)

if (CONFIG_SYS_CPU0)
list(APPEND srcs
	src/rlk_multimedia_server.c
	src/rlk_control_server.c
	src/rlk_cli_server.c
	src/rlk_server_multimedia_devices.c
	src/rlk_server_multimedia_transmission.c
)
endif()

set(priv_req bk_init media_service cs2p2p)

if ("${ARMINO_SOC}" STREQUAL "bk7256") 
list(APPEND priv_req media  aud_intf)
endif()

if ("${ARMINO_SOC}" STREQUAL "bk7258")
list(APPEND priv_req multimedia)
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
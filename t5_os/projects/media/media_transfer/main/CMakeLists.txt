set(incs . include)
set(srcs app_main.c)

if (CONFIG_SYS_CPU0)
	list(APPEND srcs
		src/rtsp_main.c
	)
endif()

set(priv_req bk_init media_service lwip_intf_v2_1 bk_cli net_transfer multimedia media)

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
	PRIV_REQUIRES "${priv_req}"
)

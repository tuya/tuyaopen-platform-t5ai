set(incs .)
set(srcs app_main.c)

if (CONFIG_SYS_CPU0)
list(APPEND srcs
	lcd_mcu_8080.c
)
endif()


set(priv_req bk_init media_service wifi_transfer)


if ("${ARMINO_SOC}" STREQUAL "bk7256")
list(APPEND priv_req media aud_intf)
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
	PRIV_REQUIRES "${priv_req}"
)


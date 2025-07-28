set(incs)
set(srcs)

if (CONFIG_AT_CMD)
    list(APPEND incs
		include
		)
	list(APPEND srcs
		"src/at_common.c"
        )

	if (CONFIG_WIFI_ENABLE)
		list(APPEND srcs
			"src/wifi_at_command.c")
	endif()

	if (CONFIG_BT)
	if (NOT CONFIG_BTDM_CONTROLLER_ONLY)
		list(APPEND srcs
			"src/bt_at_command.c")
	endif()
	endif()

	if (CONFIG_BLE)
	if (NOT CONFIG_BTDM_CONTROLLER_ONLY)
                list(APPEND srcs
                        "src/ble_at_command.c"
						"src/ble_at_gatt.c"
									)
	endif()
	endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}"
                        REQUIRES bk_common bk_log
                        PRIV_REQUIRES bk_bluetooth bt lwip_intf_v2_1)


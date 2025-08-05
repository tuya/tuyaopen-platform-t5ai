set(incs)
set(srcs)

if (CONFIG_BLE AND NOT CONFIG_BTDM_CONTROLLER_ONLY)
	list(APPEND incs
		ble_boarding
		)

	list(APPEND srcs
		ble_boarding/ble_boarding.c
		ble_boarding/dm_ble_boarding.c
		)
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_common bk_bluetooth psa_mbedtls
	)

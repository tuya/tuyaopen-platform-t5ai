set(incs)
set(srcs)

if (CONFIG_BLUETOOTH)

    list(APPEND srcs
        api/bt_main.c
    )

	if (CONFIG_BTDM_CONTROLLER_ONLY)
		list(APPEND srcs
			"hci/hal_hci_core.c"
			)
		list(APPEND incs
			hci
			)
	endif()#CONFIG_BTDM_CONTROLLER_ONLY
if(CONFIG_AT)
	if(CONFIG_BT)
		if (CONFIG_BT_AT_ENABLE)
			if(NOT CONFIG_BTDM_CONTROLLER_ONLY)
				list(APPEND srcs
					"api/bk_at_bt.c")
			endif()#NOT CONFIG_BTDM_CONTROLLER_ONLY
		endif()#CONFIG_BT_AT_ENABLE
	endif()#CONFIG_BT
	
	if (CONFIG_BLE_AT_ENABLE)
		if(NOT CONFIG_BTDM_CONTROLLER_ONLY)
			list(APPEND srcs
				"api/bk_at_ble.c"
				"api/ble_at_gatt.c")
		endif()#NOT CONFIG_BTDM_CONTROLLER_ONLY
	endif()#CONFIG_BLE_AT_ENABLE

	set(components ../../components)
	list(APPEND incs
		${components}/at_server
		${components}/at_server/_at_server
		${components}/at_server/_at_server_port
	)
endif()#CONFIG_AT
	if(CONFIG_ALI_MQTT)
		list(APPEND incs
			${components}/ali_mqtt/mqtt
			${components}/ali_mqtt/mqtt/utils
		)
	endif()#CONFIG_ALI_MQTT
	set(bk_inc ../../include)
		list(APPEND incs
			${bk_inc}/os
			#${components}/ali_mqtt/mqtt/utils
		)

    if (CONFIG_BT)
        list(APPEND srcs
            api/bt_l2cap_export_api.c
            api/bt_gap_export_api.c
            api/bk_a2dp_api.c
            api/bt_spp_export_api.c
            api/bt_hidd_export_api.c
            api/bt_hidh_export_api.c
            api/bk_hfp_api.c
        )
    endif()

    if (CONFIG_BLE)
        list(APPEND srcs
            api/ble_gap_export_api.c
        )
    endif()

    list(APPEND incs
        include
		include/private
		.
        )

    list(APPEND srcs feat_config/bt_feature_config.c)

	if (CONFIG_SOC_BK7256XX)
		list(APPEND srcs
			soc/bk7256/bluetooth.c
			)
	endif()

	if (CONFIG_SOC_BK7236XX)
		list(APPEND srcs
			soc/bk7236/bluetooth.c
			)
	endif()

	if (CONFIG_SOC_BK7239XX)
		list(APPEND srcs
			soc/bk7239/bluetooth.c
			)
	endif()

	if (CONFIG_SOC_BK7286XX)
		list(APPEND srcs
			soc/bk7286/bluetooth.c
			)
	endif()

    if (CONFIG_BLUETOOTH_SUPPORT_IPC)
        list(APPEND srcs
            ipc/src/bt_ipc_core.c
        )
        list(APPEND incs
            ipc/include
        )
    endif()
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_phy psa_mbedtls
	)

if (CONFIG_BLUETOOTH)
    #host: <libbluetooth_host_ble|libbluetooth_host_dm_(bt|ble|dual)>[_nspe].a
    #controller: libbluetooth_controller_<bt|ble|dual|controller_only>[_<link num>link][_nspe].a

    set(bt_host_dm libbluetooth_host_dm)
    set(bt_host_ble libbluetooth_host_ble)
	set(bt_controller libbluetooth_controller)

    set(bt_host_exist 1)
	set(bt_cp_exist 1)
    set(bt_host_basename)
    set(bt_controller_basename)

    if(CONFIG_BTDM_CONTROLLER_ONLY)
        set(bt_host_exist 0)
    endif()

	if(CONFIG_BTDM_CONTROLLER_ONLY)
		if(CONFIG_BT)
			set(bt_controller_basename ${bt_controller}_controller_only_dual)
		else()
			set(bt_controller_basename ${bt_controller}_controller_only_ble)
		endif()
    elseif(CONFIG_BT AND NOT CONFIG_BLE)
        set(bt_host_basename ${bt_host_dm}_bt)
        set(bt_controller_basename ${bt_controller}_bt)
    elseif(CONFIG_BLE AND NOT CONFIG_BT)
        set(bt_host_basename ${bt_host_ble})
        set(bt_controller_basename ${bt_controller}_ble)
    elseif(CONFIG_BLE AND CONFIG_BT)
        set(bt_host_basename ${bt_host_dm}_dual)
        set(bt_controller_basename ${bt_controller}_dual)
    endif()

    if(CONFIG_BLUETOOTH_RELEASE_CODESIZE AND CONFIG_BLE AND NOT CONFIG_BT)
        set(bt_controller_basename ${bt_controller}_reduce)
        set(bt_host_basename ${bt_host_ble}_reduce)
    else()
        if(CONFIG_BLUETOOTH_BLE_SLAVE_ONLY)
            set(bt_controller_basename ${bt_controller}_ble_cp_slave_only)
			if (NOT CONFIG_BTDM_CONTROLLER_ONLY)
				set(bt_controller_basename ${bt_controller}_ble_slave_only)
				set(bt_host_basename ${bt_host_ble}_slave_only)
			endif()
        endif()
    endif()

    if (CONFIG_TZ AND NOT CONFIG_SPE)
        #set(bt_host_basename ${bt_host_basename}_nspe)
        set(bt_controller_basename ${bt_controller_basename}_nspe)
    endif()

    set(bt_host_basename ${bt_host_basename}.a)
    set(bt_controller_basename ${bt_controller_basename}.a)

    set(bluetooth_elem)

    if(bt_host_exist EQUAL 1)
        list(APPEND bluetooth_elem ${bt_host_basename})
    endif()
	if(bt_cp_exist EQUAL 1)
        list(APPEND bluetooth_elem ${bt_controller_basename})
    endif()

    message("WARINIG bt_host_exist ${bt_host_exist}")
    message("WARINIG bt_host_basename ${bt_host_basename}")
    message("WARINIG bt_controller_basename ${bt_controller_basename}")

	foreach(lib_name ${bluetooth_elem})
		#message("WARINIG lib_name ${ARMINO_PATH}/components/bk_libs/${ARMINO_SOC}/libs/${lib_name}")
		add_prebuilt_library(${lib_name} "${ARMINO_PATH}/components/bk_libs/${ARMINO_SOC}/libs/${lib_name}")
		target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
	endforeach()
endif()

armino_component_register_call_subdirs()

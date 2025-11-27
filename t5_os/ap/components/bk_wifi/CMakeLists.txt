set(incs include include/bk_private)
set(srcs)
#if(CONFIG_WIFI_ENABLE)
list(APPEND srcs
	src/wifi_api.c
	src/wifi_cli.c
	src/wifi_api_ipc.c
	src/phy_client.c
	src/phy_notify.c
	#src/wifi_demo.c
)
#endif()


if(CONFIG_AT)
list(APPEND srcs
	src/wifi_at.c
)
set(components ../../components)
set(IDK_INC ../../include)
list(APPEND incs
	${components}/at_server
	${components}/at_server/_at_server
	${components}/at_server/_at_server_port
	${components}/bk_net/include
	${components}/bk_event
	${components}/bk_wifi_driver
	${IDK_INC}/modules
	)
endif()

if(CONFIG_NET_PARAM)
list(APPEND srcs
	src/net_param.c
)
endif()

armino_component_register(SRCS "${srcs}"
    INCLUDE_DIRS "${incs}"
    PRIV_REQUIRES bk_common lwip_intf_v2_1 uart_debug bk_wifi_driver
    REQUIRES bk_netif)


get_filename_component(config_component_name ${CMAKE_CURRENT_LIST_DIR} NAME)
set(config_component_name ${config_component_name})
armino_component_get_target(target "${config_component_name}")
armino_build_get_property(armino_path ARMINO_PATH)

armino_component_register_call_subdirs()


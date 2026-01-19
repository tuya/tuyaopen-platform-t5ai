set(incs)
set(srcs)

if(CONFIG_BK_NETIF)
	list(APPEND srcs
		bk_netif.c
	)
	list(APPEND incs
		include
	)
if(CONFIG_AT)
           list(APPEND srcs
            bk_at_netif.c
        )
	set(components ../../components)
	list(APPEND incs
		${components}/at_server
		${components}/at_server/_at_server
		${components}/at_server/_at_server_port
	)

endif()#CONFIG_AT
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_common lwip_intf_v2_1 bk_wifi_driver)

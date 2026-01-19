set(incs)
set(srcs)

#set(prev_req )

if(CONFIG_WIFI_VNET_CONTROLLER)
	list(APPEND incs . include)

	list(APPEND srcs
			wdrv_ipc.c
			wdrv_main.c
			wdrv_cntrl.c
			wdrv_tx.c
			wdrv_rx.c
			wdrv_co_list.c
		)
	#list(APPEND prev_req mailbox)
endif()

armino_component_register(SRCS "${srcs}"
		INCLUDE_DIRS "${incs}"
		PRIV_REQUIRES bk_common bk_wifi lwip_intf_v2_1 bk_netif
		)
set(incs)
set(srcs)

set(prev_req bk_common bk_wifi lwip_intf_v2_1 bk_pm app demos)

#if(CONFIG_WIFI_VNET_CONTROLLER)
	list(APPEND incs .)

	list(APPEND srcs
		cif_cntrl.c
		cif_wifi_dp.c
		cif_ps.c
		cif_main.c
		cif_ipc.c
		cif_mem_mgmt.c
		cif_wifi_api.c
		cif_co_list.c
		)
	#list(APPEND prev_req mailbox)
#endif()

armino_component_register(SRCS "${srcs}"
		INCLUDE_DIRS "${incs}"
		PRIV_REQUIRES "${prev_req}"
		)
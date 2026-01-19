set(incs include)
set(srcs)

if (CONFIG_WIFI_TRANSFER)
	list(APPEND srcs
		src/wifi_transfer.c
		src/trans_list.c
		)
endif()

set(priv_req bk_common multimedia)

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES "${priv_req}"
	)
 
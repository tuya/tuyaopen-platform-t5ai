set(incs include)
set(srcs)

set(priv_req bk_common)

if (CONFIG_WIFI_TRANSFER AND CONFIG_SYS_CPU0)
	list(APPEND srcs
		src/wifi_transfer.c
		src/trans_list.c
		)
endif()

if (("${ARMINO_SOC}" STREQUAL "bk7258") OR ("${ARMINO_SOC}" STREQUAL "bk7258_cp1") OR ("${ARMINO_SOC}" STREQUAL "bk7258_cp2"))
    list(APPEND priv_req multimedia)
else()
    list(APPEND priv_req media)
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES "${priv_req}"
	)
 
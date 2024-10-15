set(incs include)
set(srcs)
list(APPEND srcs
	src/bk_phy_adapter.c
	src/bk_rf_adapter.c
	src/bk_phy_api.c
)
armino_component_register(SRCS "${srcs}"
    INCLUDE_DIRS "${incs}"
    PRIV_REQUIRES bk_common bk_bluetooth
    )


if (CONFIG_TZ AND NOT CONFIG_SPE)
	if (CONFIG_BLUETOOTH AND NOT CONFIG_WIFI_ENABLE)
		set (phy_elem libcom_phy_nspe.a)
	elseif (CONFIG_ATE_TEST)
		set (phy_elem libate_phy_nspe.a)
	else()
		set (phy_elem libbk_phy_nspe.a)
	endif()
else()
	if (CONFIG_BLUETOOTH AND NOT CONFIG_WIFI_ENABLE)
		set (phy_elem libcom_phy.a)
	elseif (CONFIG_ATE_TEST)
		set (phy_elem libate_phy.a)
	else()
		set (phy_elem libbk_phy.a)
	endif()
endif()

foreach(lib_name ${phy_elem})
	add_prebuilt_library(${lib_name} "${ARMINO_PATH}/components/bk_libs/${ARMINO_SOC}/libs/${lib_name}")
	target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
endforeach()


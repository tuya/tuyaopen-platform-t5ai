armino_build_get_property(project_dir PROJECT_DIR)
set(incs
	.
	)

if (CONFIG_HTTPC)
set(srcs
	httpc.c
	)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common  lwip_intf_v2_1 psa_mbedtls)

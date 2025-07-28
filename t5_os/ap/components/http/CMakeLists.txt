armino_build_get_property(project_dir PROJECT_DIR)
set(incs
	.
	${project_dir}/main
	)

if (CONFIG_HTTP)
set(srcs
	utils_httpc.c
	utils_net.c
	utils_timer.c
	lite-log.c
	)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common ota lwip_intf_v2_1)

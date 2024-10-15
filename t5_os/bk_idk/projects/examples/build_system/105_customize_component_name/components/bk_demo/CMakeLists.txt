set(incs
	.
	include
	)

set(srcs c1.c)


armino_component_register(INTERNAL_LIB
                        SRCS "${srcs}"
                        INCLUDE_DIRS "${incs}")



set(config_component_name bk_demo_ex)

set(definitions
    CONFIG_FEATUR1_EABLE=1
    CONFIG_FEATUR2_COUNT=999
    )

armino_component_register_ex(INTERNAL_LIB
                        SRCS "${srcs}"
                        INCLUDE_DIRS "${incs}"
                        CONFIG_COMPONENT_NAME "${config_component_name}")

armino_component_get_target(target "${config_component_name}")

target_compile_definitions(${target} PRIVATE ${definitions})

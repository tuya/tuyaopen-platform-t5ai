set(incs .
    include)

set(srcs
	rlk_demo_main.c
	src/rlk_demo_cli.c
	src/rlk_ping.c
	src/rlk_common.c
	)
	
if (CONFIG_OVERRIDE_FLASH_PARTITION)
    list(APPEND srcs
        vendor_flash.c
    )
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")

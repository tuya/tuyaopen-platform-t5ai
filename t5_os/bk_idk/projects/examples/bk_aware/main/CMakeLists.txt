set(incs .)
set(srcs
    app_main.c
	bk_aware_crc.c
	bk_aware_demo.c
    )

if (CONFIG_OVERRIDE_FLASH_PARTITION)
    list(APPEND srcs
        vendor_flash.c
    )
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")

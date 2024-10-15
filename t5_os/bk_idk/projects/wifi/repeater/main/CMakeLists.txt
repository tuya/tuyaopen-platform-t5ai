set(incs .)
set(srcs
    "repeater_main.c"
	"wifi_repeater_cli.c"
    )

if (CONFIG_OVERRIDE_FLASH_PARTITION)
    list(APPEND srcs
        vendor_flash.c
    )
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")


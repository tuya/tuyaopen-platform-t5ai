set(incs .)

set(srcs
	meter_main.c
)

if (CONFIG_LV_USE_DEMO_METER)
list(APPEND srcs
	lv_example_meter.c
)
endif()

if (CONFIG_OVERRIDE_FLASH_PARTITION)
	list(APPEND srcs
		vendor_flash.c
	)
endif()

armino_component_register(
	SRCS "${srcs}"
	INCLUDE_DIRS "${incs}" 
	PRIV_REQUIRES bk_init bk_cli multimedia lvgl media_service
)

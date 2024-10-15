set(incs .)

if (0)
set(srcs
	camera_main.c
)
else()
set(srcs
	app_main.c
)
endif()

if (CONFIG_LV_USE_DEMO_METER)
list(APPEND srcs
	lv_example_meter.c
	lvgl_vfs_init.c
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
	PRIV_REQUIRES media bk_init bk_cli bk_vfs lvgl media_service
)

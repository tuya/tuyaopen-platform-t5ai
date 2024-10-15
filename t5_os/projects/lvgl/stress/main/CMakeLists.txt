set(incs .)

set(srcs
	stress_main.c
)

if (CONFIG_LV_USE_DEMO_STRESS)
list(APPEND srcs
	lv_demo_stress.c
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
	PRIV_REQUIRES media bk_init bk_cli multimedia lvgl media_service
)

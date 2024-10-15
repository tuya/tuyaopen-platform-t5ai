set(incs .)
set(srcs)

list(APPEND srcs
	app_main.c
)

if (CONFIG_OVERRIDE_FLASH_PARTITION)
    list(APPEND srcs
        vendor_flash.c
    )
endif()

if (CONFIG_SYS_CPU0)

	list(APPEND srcs
		agora_rtc_demo.c
		agora_sdk/agora_rtc.c
		audio/audio_transfer.c
		flow_control/fpscc.c
	)

list(APPEND incs
	include
)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")

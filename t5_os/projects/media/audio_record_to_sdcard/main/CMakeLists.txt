set(incs .
		include
	)

set(srcs
		app_main.c
	)

if (CONFIG_ASDF_WORK_CPU0 AND CONFIG_ASDF_FATFS_STREAM AND CONFIG_ASDF_ONBOARD_MIC_STREAM)
	list(APPEND srcs
		audio_record.c
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
	PRIV_REQUIRES media bk_init bk_cli avdk_libs audio_pipeline onboard_mic_stream fatfs_stream)

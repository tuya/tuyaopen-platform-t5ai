set(incs .
		include
	)

set(srcs
		app_main.c
	)

if ((CONFIG_ASDF_WORK_CPU0 AND CONFIG_ASDF_FATFS_STREAM AND CONFIG_ASDF_ONBOARD_SPEAKER_STREAM AND CONFIG_SOC_BK7256XX) OR (CONFIG_ASDF_WORK_CPU1 AND CONFIG_ASDF_FATFS_STREAM AND CONFIG_ASDF_ONBOARD_SPEAKER_STREAM  AND CONFIG_SOC_BK7236XX))
	list(APPEND srcs
		audio_play.c
	)
endif()

set(priv_req bk_init bk_cli avdk_libs audio_pipeline onboard_speaker_stream fatfs_stream mp3_decoder media_service)

if ("${ARMINO_SOC}" STREQUAL "bk7256")
list(APPEND priv_req media)
endif()

if ("${ARMINO_SOC}" STREQUAL "bk7258_cp1")
list(APPEND priv_req multimedia)
endif()

if (CONFIG_OVERRIDE_FLASH_PARTITION)
	list(APPEND srcs
		vendor_flash.c
	)
endif()

armino_component_register(
	SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES "${priv_req}"
)

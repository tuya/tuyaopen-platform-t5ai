set(incs .
		include
	)

set(srcs
		app_main.c
	)

if (CONFIG_WANSON_ASR)
if ((CONFIG_ASDF_WORK_CPU0 AND CONFIG_ASDF_RAW_STREAM AND CONFIG_ASDF_ONBOARD_MIC_STREAM AND CONFIG_SOC_BK7256XX) OR (CONFIG_ASDF_WORK_CPU1 AND CONFIG_ASDF_RAW_STREAM AND CONFIG_ASDF_ONBOARD_MIC_STREAM AND CONFIG_SOC_BK7236XX))
	list(APPEND srcs
		wanson_asr.c
	)
endif()
endif()

set(priv_req bk_init bk_cli avdk_libs audio_pipeline onboard_mic_stream raw_stream wanson)

if ("${ARMINO_SOC}" STREQUAL "bk7256" OR "${ARMINO_SOC}" STREQUAL "bk7256_cp1")
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

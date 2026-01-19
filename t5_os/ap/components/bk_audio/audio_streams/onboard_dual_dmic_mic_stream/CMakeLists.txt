set(incs .)

set(srcs)

if (CONFIG_ADK_ONBOARD_DUAL_DMIC_MIC_STREAM)

list(APPEND srcs
		onboard_dual_dmic_mic_stream.c
	)

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)
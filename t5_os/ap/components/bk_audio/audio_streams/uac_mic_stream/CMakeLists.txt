set(incs .)

set(srcs)

if (CONFIG_ADK_UAC_MIC_STREAM)

list(APPEND srcs
		uac_mic_stream.c
	)

if (CONFIG_ADK_UAC_MIC_STREAM_TEST)
	list(APPEND srcs
		uac_mic_stream_test.c
	)
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)
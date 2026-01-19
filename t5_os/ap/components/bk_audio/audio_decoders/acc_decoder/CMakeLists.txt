set(incs .)

set(srcs)

if (CONFIG_ADK_AAC_DECODER)

list(APPEND srcs
		aac_decoder.c
	)

if (CONFIG_ADK_AAC_DECODER_TEST)
	list(APPEND srcs
		aac_decoder_test.c
	)
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)
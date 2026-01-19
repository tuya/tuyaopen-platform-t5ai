set(incs .)

set(srcs)

if (CONFIG_ADK_AAC_ENCODER)

list(APPEND srcs
		aac_encoder.c
	)

if (CONFIG_ADK_AAC_ENCODER_TEST)
	list(APPEND srcs
		aac_encoder_test.c
	)
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)
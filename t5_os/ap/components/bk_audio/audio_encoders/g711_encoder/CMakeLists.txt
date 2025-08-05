set(incs .)

set(srcs)

if (CONFIG_ADK_G711_ENCODER)

list(APPEND srcs
		g711_encoder.c
	)

if (CONFIG_ADK_G711_ENCODER_TEST)
	list(APPEND srcs
		g711_encoder_test.c
	)
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)
set(incs .)

set(srcs)

if (CONFIG_ADK_G722_DECODER)

list(APPEND srcs
		g722_decoder.c
	)

if (CONFIG_ADK_G722_DECODER_TEST)
	list(APPEND srcs
		g722_decoder_test.c
	)
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)
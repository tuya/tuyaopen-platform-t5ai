set(incs .)

set(srcs)

if (CONFIG_ADK_OPUS_DECODER)

list(APPEND srcs
		opus_dec.c
	)

if (CONFIG_ADK_OPUS_DECODER_TEST)
	list(APPEND srcs
		opus_dec_test.c
	)
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)
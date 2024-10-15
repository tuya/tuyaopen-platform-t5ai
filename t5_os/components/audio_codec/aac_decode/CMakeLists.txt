if (CONFIG_AUDIO_AACDEC_TEST)
set(srcs
	aacdec_demo.c
	)
endif()

armino_component_register(SRCS "${srcs}" PRIV_REQUIRES bk_common aacdec)
if (CONFIG_AAC_ENCODE_TEST)
set(srcs
	aac_encode_demo.c
	)
endif()

armino_component_register(SRCS "${srcs}" PRIV_REQUIRES bk_common avdk_libs)
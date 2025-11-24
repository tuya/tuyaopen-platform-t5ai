set(incs .)

set(srcs)

if (CONFIG_ADK_ARRAY_STREAM)
list(APPEND srcs
		array_stream.c
	)

if (CONFIG_ADK_ARRAY_STREAM_TEST)
	list(APPEND srcs
		array_stream_test.c
	)
endif()
endif()#(CONFIG_ADK_ARRAY_STREAM)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)

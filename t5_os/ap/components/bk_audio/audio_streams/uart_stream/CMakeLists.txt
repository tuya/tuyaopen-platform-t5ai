set(incs .)

set(srcs)

if (CONFIG_ADK_UART_STREAM)

list(APPEND srcs
		uart_stream.c
	)

if (CONFIG_ADK_UART_STREAM_TEST)
	list(APPEND srcs
		uart_stream_test.c
	)
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)
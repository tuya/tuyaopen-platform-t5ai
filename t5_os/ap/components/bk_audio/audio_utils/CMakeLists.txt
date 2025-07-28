set(incs .)

set(srcs)

if (CONFIG_ADK_UART_UTIL)
	list(APPEND srcs
		uart_util.c
	)
endif()

if (CONFIG_ADK_COUNT_UTIL)
	list(APPEND srcs
		count_util.c
	)
endif()

if (CONFIG_ADK_TFCARD_UTIL)
	list(APPEND srcs
		tfcard_util.c
	)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)

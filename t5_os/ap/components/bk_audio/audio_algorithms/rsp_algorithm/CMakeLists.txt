set(incs .)

set(srcs)

if (CONFIG_ADK_RSP_ALGORITHM)

list(APPEND srcs
		rsp_algorithm.c
	)


endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)
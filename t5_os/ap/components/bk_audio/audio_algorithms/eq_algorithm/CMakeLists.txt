set(incs .)

set(srcs)

if (CONFIG_ADK_EQ_ALGORITHM)

list(APPEND srcs
		eq_algorithm.c
	)

if (CONFIG_ADK_EQ_ALGORITHM_TEST)
	list(APPEND srcs
		eq_algorithm_test.c
	)
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)

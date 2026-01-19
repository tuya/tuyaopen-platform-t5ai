set(incs .)

set(srcs)

if (CONFIG_ADK_AGC_ALGORITHM)

list(APPEND srcs
		agc_algorithm.c
	)

if (CONFIG_ADK_AGC_ALGORITHM_TEST)
	list(APPEND srcs
		agc_algorithm_test.c
	)
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)
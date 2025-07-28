set(incs include)
set(srcs
	src/avdk_crc.c
	src/uart_util.c
	src/count_util.c
)

if (CONFIG_FATFS)
	list(APPEND srcs
		src/tfcard_util.c
	)
endif()

set(priv_req )


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

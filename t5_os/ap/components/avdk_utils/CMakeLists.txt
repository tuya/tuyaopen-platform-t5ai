set(incs include)
set(srcs
	src/avdk_crc.c
)

set(priv_req )


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

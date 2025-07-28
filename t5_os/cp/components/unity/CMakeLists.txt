set(incs 
	extras/fixture/src
	src
)
set(srcs
	extras/fixture/src/unity_fixture.c
	src/unity.c
)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)

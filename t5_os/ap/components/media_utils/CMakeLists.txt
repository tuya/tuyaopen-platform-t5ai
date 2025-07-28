set(incs include)
set(srcs
	src/media_utils.c
	src/bk_list_edge.c
)

if (CONFIG_PSRAM)
	list(APPEND srcs
		src/psram_mem_slab.c
	)
endif()

set(priv_req bk_common)


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}" unity include)

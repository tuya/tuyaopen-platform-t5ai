set(incs .)
set(srcs)
if (CONFIG_CJSON_USE)
list(APPEND srcs cJSON.c
)
endif()
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)

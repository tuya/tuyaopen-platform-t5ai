set(incs .)
set(srcs
    ap_main.c
    )

set(priv_req  media_service multimedia)

if ("${ARMINO_SOC}" STREQUAL "bk7258")
list(APPEND priv_req)
endif()


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}"
)

set(incs .)
set(srcs
    ap_main.c
    )

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")

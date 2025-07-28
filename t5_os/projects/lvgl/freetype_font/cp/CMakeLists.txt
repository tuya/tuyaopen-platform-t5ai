set(incs .)
set(srcs
    cp_main.c
    )

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")

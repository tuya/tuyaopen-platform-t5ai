set(incs .)

set(srcs
    ap_main.c
    )


set(priv_req  media_service multimedia)

if ("${ARMINO_SOC}" STREQUAL "bk7258_ap")
list(APPEND priv_req  lvgl display_service bk_cli media_service)
endif()


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}"
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-variable -O3)

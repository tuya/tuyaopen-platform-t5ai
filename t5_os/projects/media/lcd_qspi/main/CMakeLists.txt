set(incs .)
set(srcs
    app_main.c
    )




armino_component_register(
	SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES media bk_init bk_cli)

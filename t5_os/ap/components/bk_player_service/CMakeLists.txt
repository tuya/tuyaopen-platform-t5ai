set(incs include)
set(srcs "")


if (CONFIG_PLAYER_SERVICE)
	list(APPEND srcs
		src/bk_player_service.c
	)
endif()

if (CONFIG_PLAYER_SERVICE_TEST)
	list(APPEND srcs
		cli/player_cli.c
	)
endif()

armino_component_register(
	SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES avdk_utils bk_common include
)

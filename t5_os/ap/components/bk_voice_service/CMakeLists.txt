set(incs include)
set(srcs "")


if (CONFIG_VOICE_READ_SERVICE)
	list(APPEND srcs
		src/bk_voice_read_service.c
	)
endif()

if (CONFIG_VOICE_WRITE_SERVICE)
	list(APPEND srcs
		src/bk_voice_write_service.c
	)
endif()

if (CONFIG_VOICE_SERVICE_TEST)
	list(APPEND srcs
		cli/voice_cli.c
	)
endif()

if (CONFIG_VOICE_SERVICE)
	list(APPEND srcs
		src/bk_voice_service.c
	)
endif()


armino_component_register(
    SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES avdk_utils bk_common include
)


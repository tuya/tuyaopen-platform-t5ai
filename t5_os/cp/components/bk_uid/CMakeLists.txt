set(incs)
set(srcs)

if (CONFIG_GET_UID_ENABLE)
	list(APPEND incs
		.
	)

	list(APPEND srcs
		bk_uid_adaptor.c
		bk_uid.c
	)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common psa_mbedtls)


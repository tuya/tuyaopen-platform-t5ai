set(incs include)
set(srcs "")

if (CONFIG_ASR_SERVICE)
	list(APPEND srcs
		src/bk_asr.c
		src/bk_asr_service.c
	)
endif()

armino_component_register(
    SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES avdk_utils bk_common include wanson
)


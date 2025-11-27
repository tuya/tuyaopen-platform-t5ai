set(srcs "")

if (CONFIG_AUD_PARAM_CTRL)
	list(APPEND srcs
        audio_param_ctrl.c
        audio_dbgtool_intf.c
	)
endif()

armino_component_register(
    SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES avdk_utils bk_common bk_cli include
)


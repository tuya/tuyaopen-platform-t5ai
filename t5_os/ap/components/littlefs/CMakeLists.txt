set(incs "")
set(srcs "")
if(CONFIG_LITTLEFS)
	list(APPEND incs .)
	list(APPEND srcs
		"lfs.c"
		"lfs_util.c"
		"lfs_flashbd.c"
		)
endif()

# Modified by TUYA Start
armino_component_register(SRCS "${srcs}"
		INCLUDE_DIRS "${incs}"
		PRIV_REQUIRES bk_common tuyaos_adapter
		)
# Modified by TUYA End

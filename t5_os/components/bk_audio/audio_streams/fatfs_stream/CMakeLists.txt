set(incs .)

set(srcs)

if (CONFIG_ASDF_FATFS_STREAM)
if ((CONFIG_SYS_CPU0 AND CONFIG_ASDF_WORK_CPU0) OR (CONFIG_SYS_CPU1 AND CONFIG_ASDF_WORK_CPU1))
list(APPEND srcs
		fatfs_stream.c
		fatfs_act.c
	)

if (CONFIG_ASDF_FATFS_STREAM_TEST)
	list(APPEND srcs
		fatfs_stream_test.c
	)
endif()
endif()#((CONFIG_SYS_CPU0 AND CONFIG_ASDF_WORK_CPU0) OR (CONFIG_SYS_CPU1 AND CONFIG_ASDF_WORK_CPU1))
endif()#(CONFIG_ASDF_FATFS_STREAM)

if (CONFIG_SYS_CPU0 AND CONFIG_ASDF_WORK_CPU1 AND CONFIG_FATFS)
	list(APPEND srcs
		fatfs_coprocess.c
	)
endif()

set(priv_req bk_common audio_pipeline)

if ("${ARMINO_SOC}" STREQUAL "bk7256" OR "${ARMINO_SOC}" STREQUAL "bk7256_cp1")
list(APPEND priv_req media)
endif()

if ("${ARMINO_SOC}" STREQUAL "bk7258" OR "${ARMINO_SOC}" STREQUAL "bk7258_cp1")
list(APPEND priv_req multimedia)
endif()


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
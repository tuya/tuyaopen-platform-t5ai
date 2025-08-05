set(incs .)

set(srcs)

if (CONFIG_ADK_FATFS_STREAM)
list(APPEND srcs
		fatfs_stream.c
	)

if (CONFIG_ADK_FATFS_STREAM_TEST)
	list(APPEND srcs
		fatfs_stream_test.c
	)
endif()
endif()#(CONFIG_ADK_FATFS_STREAM)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common bk_vfs)

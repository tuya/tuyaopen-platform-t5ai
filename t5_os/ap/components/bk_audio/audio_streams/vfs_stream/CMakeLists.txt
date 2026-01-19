set(incs .)

set(srcs)

if (CONFIG_ADK_VFS_STREAM)
list(APPEND srcs
		vfs_stream.c
	)

if (CONFIG_ADK_VFS_STREAM_TEST)
	list(APPEND srcs
		vfs_stream_test.c
	)
endif()
endif()#(CONFIG_ADK_VFS_STREAM)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common bk_vfs)

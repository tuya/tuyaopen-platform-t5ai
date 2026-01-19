set(incs include)
set(srcs bk_feature.c bk_crc.c)

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	REQUIRES release bk_startup os_source app bk_event bk_rtos driver bk_log bk_system bk_ps bk_init
	)

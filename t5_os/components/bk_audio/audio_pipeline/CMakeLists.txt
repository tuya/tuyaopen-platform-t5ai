set(incs ./include)

set(srcs)

if ((CONFIG_SYS_CPU1 AND CONFIG_ASDF_WORK_CPU1) OR (CONFIG_SYS_CPU0 AND CONFIG_ASDF_WORK_CPU0))
list(APPEND srcs
		audio_element.c
		audio_event_iface.c
		audio_pipeline.c
		ringbuf.c
		audio_mem.c
	)

if (CONFIG_ASDF_AUTO_TEST_PIPELINE)
	list(APPEND srcs
		auto_test/audio_pipeline_test.c
	)
endif()

if (CONFIG_ASDF_AUTO_TEST_ELEMENT)
	list(APPEND srcs
		auto_test/audio_element_test.c
	)
endif()

if (CONFIG_ASDF_AUTO_TEST_EVENT)
	list(APPEND srcs
		auto_test/audio_event_iface_test.c
	)
endif()
endif()#(CONFIG_SYS_CPU1 AND CONFIG_ASDF_WORK_CPU1)

if ((CONFIG_SYS_CPU0 AND CONFIG_ASDF_WORK_CPU1))
	list(APPEND srcs
		audio_coprocess.c
		audio_mailbox.c
	)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common multimedia fatfs_stream)
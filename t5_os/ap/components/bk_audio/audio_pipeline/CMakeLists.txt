set(incs .)

set(srcs)

if (CONFIG_ADK)
list(APPEND srcs
		audio_element.c
		audio_event_iface.c
		audio_pipeline.c
		ringbuf.c
		framebuf.c
		audio_mem.c
		audio_thread.c
		audio_port.c
		cb_port.c
		rb_port.c
		fb_port.c
	)

if (CONFIG_ADK_AUTO_TEST_PIPELINE)
	list(APPEND srcs
		auto_test/audio_pipeline_test.c
	)
endif()

if (CONFIG_ADK_AUTO_TEST_ELEMENT)
	list(APPEND srcs
		auto_test/audio_element_test.c
	)
endif()

if (CONFIG_ADK_AUTO_TEST_EVENT)
	list(APPEND srcs
		auto_test/audio_event_iface_test.c
	)
endif()
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)

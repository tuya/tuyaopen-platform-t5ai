set(incs .)

set(srcs)

if (CONFIG_ADK_ONBOARD_SPEAKER_STREAM)

list(APPEND srcs
		onboard_speaker_stream.c
	)

if (CONFIG_ADK_ONBOARD_SPEAKER_STREAM_TEST)
	list(APPEND srcs
		onboard_speaker_stream_test.c
	)
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)
set(incs .)

set(srcs)

if (CONFIG_ASDF_RAW_STREAM)

list(APPEND srcs
		raw_stream.c
	)

if (CONFIG_ASDF_RAW_STREAM_TEST)
	list(APPEND srcs
		raw_stream_test.c
	)
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common audio_pipeline onboard_speaker_stream onboard_mic_stream)
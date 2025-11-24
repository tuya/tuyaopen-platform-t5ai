set(incs .)

set(srcs)

if (CONFIG_ADK_WAV_DECODER)

list(APPEND srcs
		wav_decoder.c
	)

if (CONFIG_ADK_WAV_DECODER_TEST)
	list(APPEND srcs
		wav_decoder_test.c
	)
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common array_stream onboard_speaker_stream)
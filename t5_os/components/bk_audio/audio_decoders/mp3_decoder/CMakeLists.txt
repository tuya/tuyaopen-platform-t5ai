set(incs .)

set(srcs)

if (CONFIG_ASDF_MP3_DECODER)

list(APPEND srcs
		mp3_decoder.c
	)

if (CONFIG_ASDF_MP3_DECODER_TEST)
	list(APPEND srcs
		mp3_decoder_test.c
	)
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common audio_pipeline fatfs_stream)
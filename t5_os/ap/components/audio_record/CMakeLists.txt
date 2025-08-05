set(incs include)
set(srcs "")

if (CONFIG_AUDIO_RECORD)
	list(APPEND srcs
		src/audio_record.c
		src/onboard_mic_record.c
		src/ring_buffer.c
	)
endif()

armino_component_register(
    SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_common include avdk_utils
)

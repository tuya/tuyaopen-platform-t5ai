set(incs include)
set(srcs "")

if (CONFIG_AUDIO_PLAY)
	list(APPEND srcs
		src/audio_play.c
		src/onboard_speaker_play.c
		src/ring_buffer.c
	)
endif()

armino_component_register(
    SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_common include avdk_utils
)


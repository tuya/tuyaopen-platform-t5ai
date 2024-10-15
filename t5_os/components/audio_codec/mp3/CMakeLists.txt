if (CONFIG_AUDIO_DAC AND CONFIG_AUDIO_MP3_TEST AND CONFIG_FATFS)

set(incs .
	include)

set(srcs
	mp3_demo.c
	)

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common avdk_libs aud_intf)
set(incs .
    include)

set(srcs
    voice_call_main.c
    )

if (CONFIG_SYS_CPU0)

    list(APPEND srcs
		src/voice_pipeline.c
		src/dual_device_voice_transmission_udp.c
    )
endif()#if (CONFIG_SYS_CPU0)

if (CONFIG_OVERRIDE_FLASH_PARTITION)
    list(APPEND srcs
        vendor_flash.c
    )
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_init lwip_intf_v2_1 audio_pipeline onboard_mic_stream onboard_speaker_stream raw_stream aec_algorithm g711_encoder g711_decoder media_service)
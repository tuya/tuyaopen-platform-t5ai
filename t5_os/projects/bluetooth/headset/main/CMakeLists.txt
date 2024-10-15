set(incs .
    )
set(srcs
    armino_main.c
    )

if (CONFIG_OVERRIDE_FLASH_PARTITION)
    list(APPEND srcs
        vendor_flash.c
    )
endif()

if(CONFIG_BLE)
    list(APPEND incs
        gatt
    )

    list(APPEND srcs
        gatt/dm_gatt.c
        gatt/dm_gattc.c
        gatt/dm_gatts.c
        gatt/dm_gatt_cli.c
        gatt/dm_gatt_connection.c
    )

endif()


if (CONFIG_BT)
	list(APPEND srcs
		a2dp_sink/a2dp_sink_demo.c
		a2dp_sink/ring_buffer_node.c
		a2dp_sink_demo_cli.c
		storage/bluetooth_storage.c
		hfp_hf/hfp_hf_demo.c
        bt_manager.c
		)
endif()

list(APPEND incs
    storage
    )

armino_component_register(
	SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES media bk_init bk_cli media_service aud_intf multimedia raw_stream audio_pipeline onboard_speaker_stream onboard_mic_stream aac_decoder
)

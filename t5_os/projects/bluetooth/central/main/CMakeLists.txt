set(incs .
    )
set(srcs
    armino_main.c
    )

set(priv_req_list )
if (CONFIG_OVERRIDE_FLASH_PARTITION)
    list(APPEND srcs
        vendor_flash.c
    )
endif()

if(CONFIG_SYS_CPU0)
    list(APPEND incs
        a2dp_source
    )

    list(APPEND srcs
        a2dp_source/ring_buffer_particle.c
        a2dp_source/a2dp_source_demo.c
        a2dp_source/a2dp_source_demo_cli.c
        a2dp_source/a2dp_source_demo_avrcp.c
    )

    if(CONFIG_BLE)
        list(APPEND incs
            gatt
			storage
        )

        list(APPEND srcs
            gatt/dm_gatt.c
            gatt/dm_gattc.c
            gatt/dm_gatts.c
            gatt/dm_gatt_cli.c
            gatt/dm_gatt_connection.c
			storage/bluetooth_storage.c
        )

    endif()

    #message(FATAL_ERROR "main")
endif()

if (("${ARMINO_SOC}" STREQUAL "bk7258") OR ("${ARMINO_SOC}" STREQUAL "bk7258_cp1"))
    list(APPEND priv_req_list multimedia media_service)
endif()

list(APPEND priv_req_list bk_init bk_cli)

armino_component_register(
	SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES "${priv_req_list}"
)

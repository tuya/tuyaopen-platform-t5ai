set(incs . ../../../media/doorbell/main/include)

set(srcs
	camera_main.c
)

if (CONFIG_SOC_BK7256)
if (CONFIG_LVGL_DEMO)
list(APPEND srcs
	lv_example_image_show.c
	img_utility.c
)
endif()

if (CONFIG_LV_USE_DEMO_METER)
list(APPEND srcs
	lv_example_meter.c
	lvgl_vfs_init.c
)
endif()
else ()

if (CONFIG_INTEGRATION_DOORBELL)
if (CONFIG_SYS_CPU0)
list(APPEND srcs
	../../../media/doorbell/main/src/doorbell_core.c
	../../../media/doorbell/main/src/doorbell_boarding.c
	../../../media/doorbell/main/src/doorbell_sdp.c
	../../../media/doorbell/main/src/doorbell_network.c
	../../../media/doorbell/main/src/doorbell_transmission.c
	../../../media/doorbell/main/src/doorbell_udp_service.c
	../../../media/doorbell/main/src/doorbell_tcp_service.c
	../../../media/doorbell/main/src/doorbell_devices.c
	../../../media/doorbell/main/src/doorbell_cmd.c
)
endif()
endif()

if (CONFIG_INTEGRATION_DOORBELL_CS2)
list(APPEND srcs
	../../../media/doorbell/main/src/doorbell_cs2_service.c
)
endif()

if (CONFIG_LV_USE_DEMO_METER)
list(APPEND srcs
	bg_img.c
)
endif()
endif()

if (CONFIG_OVERRIDE_FLASH_PARTITION)
    list(APPEND srcs
        vendor_flash.c
    )
endif()

armino_component_register(
	SRCS "${srcs}"
	INCLUDE_DIRS "${incs}" 
	PRIV_REQUIRES media multimedia bk_init bk_cli bk_vfs lvgl media_service bt lwip_intf_v2_1 wifi_transfer cs2p2p
)

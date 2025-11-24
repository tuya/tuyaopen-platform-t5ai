if (CONFIG_MEDIA)
set(incs include
		pipeline
		camera
		cli
	)

set(srcs
		app/media_app.c
		comm/frame_buffer.c
		camera/camera_act.c
		camera/net_camera.c
		cli/media_cli.c
		app/camera_handle_list.c
	)

if (CONFIG_DVP_CAMERA)
	list(APPEND srcs
		camera/dvp.c
	)
endif()

if (CONFIG_USBH_UVC)
		list(APPEND srcs
		camera/uvc.c
	)
endif()

if (CONFIG_WIFI_TRANSFER)
	list(APPEND srcs
		transfer/transfer_act.c
	)
endif()#CONFIG_WIFI_TRANSFER


if (CONFIG_IMAGE_STORAGE)
	list(APPEND srcs
		storage/storage_comm.c
		storage/storage_act.c
	)
endif()#CONFIG_IMAGE_STORAGE

if (CONFIG_LCD)
	list(APPEND srcs
		image_service/img_service.c
	)
endif()


if (CONFIG_LCD_ROTATE OR CONFIG_LCD_HW_PFC)
	list(APPEND srcs
		image_service/bk_rotate.c
	)
endif()

if (CONFIG_LCD_HW_DECODE OR CONFIG_LCD_SW_DECODE)
	list(APPEND srcs
		image_service/bk_decode.c
	)
endif()

if (CONFIG_MEDIA_SCALE)
	list(APPEND srcs
		image_service/bk_scale.c
	)
endif()

if (CONFIG_MEDIA_PIPELINE)
	list(APPEND srcs
		pipeline/jpeg_decode_pipeline.c
		pipeline/lcd_scale_pipeline.c
		pipeline/yuv_rotate_pipeline.c
		pipeline/uvc_pipeline_act.c
		pipeline/jpeg_decode_list.c
		pipeline/jpeg_get_pipeline.c
		pipeline/h264_encode_pipeline.c
	)
endif()

if (CONFIG_MEDIA_BT_AUDIO)
	list(APPEND srcs
		bt/bt_audio_act.c
	)
endif()

if (CONFIG_SOC_SMP)
list(APPEND srcs 
	rotate/sw_rotate.c
	)
endif()
if (CONFIG_JPEG_SW_DECODE_SUPPORT_BY_FRAME)
    list(APPEND srcs
	decode/sw_decode_cp2.c
	decode/sw_decode.c
    )
endif()
endif()

list(APPEND srcs core/media_mem_share.c)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_cli lvgl unity media_utils bk_draw_blend display_service bk_dvp bk_uvc avdk_utils bk_vfs)

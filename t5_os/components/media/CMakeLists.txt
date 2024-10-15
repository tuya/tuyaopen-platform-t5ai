if (CONFIG_SOC_BK7258)
else ()
if (CONFIG_MEDIA)
set(incs include)

if (CONFIG_SYS_CPU0)
	set(srcs
		core/media_major.c
		cli/media_cli.c
		comm/comm_act.c
		comm/frame_buffer.c
		app/media_app.c
		utils/mlist.c
	)

if (CONFIG_DVP_CAMERA OR CONFIG_USB_UVC)
	list(APPEND incs camera)
	list(APPEND srcs
		camera/camera_act.c
	)

if (CONFIG_DVP_CAMERA)
		list(APPEND srcs
		camera/dvp.c
	)
endif()

if (CONFIG_USB_UVC)
		list(APPEND srcs
		camera/uvc.c
	)
endif()

if (CONFIG_WIFI_TRANSFER)
		list(APPEND srcs
		camera/net_camera.c
	)
endif()

endif()#(CONFIG_DVP_CAMERA OR CONFIG_USB_UVC)

if (CONFIG_WIFI_TRANSFER)
	list(APPEND srcs
		transfer/transfer_act.c
	)
endif()

if (CONFIG_IMAGE_STORAGE)
	list(APPEND srcs
		storage/storage_act.c
	)
endif()

if (CONFIG_LCD_MEDIA)
	list(APPEND srcs
		lcd/lcd_act.c
	)
endif()
if (CONFIG_LCD_DMA2D_BLEND OR CONFIG_LCD_FONT_BLEND)
	list(APPEND srcs
		lcd/lcd_blend.c
	)
endif()

if (CONFIG_LCD_ROTATE OR CONFIG_LCD_HW_PFC)
	list(APPEND srcs
		lcd/lcd_rotate.c
	)
endif()

if (CONFIG_LCD_HW_DECODE OR CONFIG_LCD_SW_DECODE)
	list(APPEND srcs
		lcd/lcd_decode.c
	)
endif()
if (CONFIG_LVGL)
	list(APPEND srcs
		lcd/lvgl_draw.c
	)
endif()

#if (CONFIG_AUDIO)
#	list(APPEND srcs
#		aud/aud_api.c
#		aud/aud_act.c
#		aud/adc_ccb.c
#		aud/dac_ccb.c
#	)
#endif()


if ((CONFIG_CPU_CNT GREATER 1))
	list(APPEND srcs
		mb/mailbox_act.c
	)
endif()

else()# NOT CONFIG_SYS_CPU1
	set(srcs
		core/media_minor.c
		cli/media_cli.c
		comm/comm_act.c
		comm/frame_buffer.c
		utils/mlist.c
	)


if (CONFIG_DVP_CAMERA OR CONFIG_USB_UVC)
	list(APPEND incs camera)
	list(APPEND srcs
		camera/camera_act.c
	)

if (CONFIG_DVP_CAMERA)
		list(APPEND srcs
		camera/dvp.c
	)
endif()

if (CONFIG_USB_UVC)
		list(APPEND srcs
		camera/uvc.c
	)
endif()

if (CONFIG_AV_DEMO)
		list(APPEND srcs
		camera/net_camera.c
	)
endif()

endif()#(CONFIG_DVP_CAMERA OR CONFIG_USB_UVC)

if (CONFIG_IMAGE_STORAGE)
	list(APPEND srcs
		storage/storage_act.c
	)
endif()

if (CONFIG_WIFI_TRANSFER)
	list(APPEND srcs
		transfer/transfer_act.c
	)
endif()

#if (CONFIG_AUDIO)
#	list(APPEND srcs
#		aud/aud_api.c
#		aud/aud_act.c
#		aud/adc_ccb.c
#		aud/dac_ccb.c
#	)
#endif()

if (CONFIG_LCD_MEDIA)
	list(APPEND srcs
		lcd/lcd_act.c
	)
endif()


if (CONFIG_LCD_ROTATE OR CONFIG_LCD_HW_PFC)
	list(APPEND srcs
		lcd/lcd_calc.c
		lcd/lcd_rotate.c
	)
endif()

if (CONFIG_LVGL)
	list(APPEND srcs
		lcd/lvgl_draw.c
	)
endif()

if ((CONFIG_CPU_CNT GREATER 1))
	list(APPEND srcs
		mb/mailbox_act.c
	)
endif()

endif()# NOT CONFIG_SYS_CPU1

endif()# CONFIG_MEDIA

endif()
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_cli lvgl face_detection net_transfer avdk_libs video aud_intf media_utils display_service)

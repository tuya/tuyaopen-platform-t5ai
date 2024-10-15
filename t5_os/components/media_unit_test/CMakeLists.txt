set(incs include)
set(srcs "")

set(priv_req bk_common)

if (("${ARMINO_SOC}" STREQUAL "bk7258"))

if (CONFIG_MEDIA_UNIT_TEST)
	list(APPEND srcs
		src/media_unit_test.c
		src/case_utils.c
		src/case_lcd_on_off.c
		src/case_h264_on_off.c
		src/case_pipeline_on_off.c
		src/case_read_frame_on_off.c
		src/case_audio_on_off.c
	)
endif()

list(APPEND priv_req unity include multimedia media_utils)


endif()


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

function(set_local_library_compile_option input_target)
	set(local_compile_option )

	set(TMP_WARNING_LIST)
	list(APPEND TMP_WARNING_LIST
				unused-but-set-variable
				unused-variable
				unused-function
				unused-but-set-variable
				unused-label
				#uninitialized
	)

	foreach(tmp_loopvar IN LISTS TMP_WARNING_LIST)
		list(APPEND local_compile_option -Wno-error=${tmp_loopvar} -Wno-${tmp_loopvar})
	endforeach()

	target_compile_options(${input_target} PRIVATE ${local_compile_option})
endfunction()

set(incs .)
set(srcs
	ap_main.c
	)

if (CONFIG_BT)
	list(APPEND incs
		hfp_hf
		a2dp_sink
		storage
	)

	list(APPEND srcs
		a2dp_sink/a2dp_sink_demo.c
		a2dp_sink/ring_buffer_node.c
		a2dp_sink/mpeg4_latm_dec.c
		a2dp_sink_demo_cli.c
		hfp_hf/hfp_hf_demo.c
		hfp_hf/ring_buffer_particle.c
		bt_manager.c
		storage/bluetooth_storage.c
		)
endif()

set(priv_req  bk_init media_service multimedia bk_cli audio_play audio_record lwip_intf_v2_1)

if ("${ARMINO_SOC}" STREQUAL "bk7258")
list(APPEND priv_req)
endif()


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
get_filename_component(config_component_name ${CMAKE_CURRENT_LIST_DIR} NAME)
armino_component_get_target(target "${config_component_name}")
set_local_library_compile_option(${target})

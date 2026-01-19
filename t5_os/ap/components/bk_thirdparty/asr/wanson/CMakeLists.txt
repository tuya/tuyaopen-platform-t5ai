set(incs )

list(APPEND incs
            include
            bk7258
)

if (CONFIG_WANSON_NEW_LIB_SEG OR CONFIG_WANSON_SIMD)
set(srcs
    tdnn_utils.c
    )
endif()

armino_component_register(INCLUDE_DIRS "${incs}" SRCS "${srcs}")

if (CONFIG_WANSON_ASR)

if(CONFIG_WANSON_NEW_LIB_SEG)

	get_filename_component(lib_name ${CMAKE_CURRENT_SOURCE_DIR}/bk7258/libasrbase_seg.a NAME)
	add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/bk7258/libasrbase_seg.a")
	target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
elseif(CONFIG_WANSON_SIMD)  
  
	get_filename_component(lib_name ${CMAKE_CURRENT_SOURCE_DIR}/bk7258/libasrbase_simd.a NAME)
	add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/bk7258/libasrbase_simd.a")
	target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
else()
	get_filename_component(lib_name ${CMAKE_CURRENT_SOURCE_DIR}/bk7258/libasrbase.a NAME)
	add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/bk7258/libasrbase.a")
	target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
endif()

endif()

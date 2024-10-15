set(incs include)
set(srcs)

if (CONFIG_JPEG_SW_ENCODER_TEST)
    list(APPEND incs "${ARMINO_PATH}/../include/avdk")
    list(APPEND srcs "${CMAKE_CURRENT_SOURCE_DIR}/src/jpeg_sw_enc_test.c")
    list(APPEND srcs "${CMAKE_CURRENT_SOURCE_DIR}/src/time_consumption.c")
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")

if (CONFIG_JPEG_SW_ENCODER_TEST)
    
    set(libs_name )
    if (CONFIG_VIDEO_DTCM)
        if (CONFIG_VIDEO_DTCM_ALL)
            list(APPEND libs_name
                libtjpg_enc_dtcm.a)
        else()
            list(APPEND libs_name
                libtjpg_enc_dtcm_part.a)
        endif()
    else()
        list(APPEND libs_name
            libtjpg_enc_sram.a
            )
    endif()

    foreach(lib_name ${libs_name})
        add_prebuilt_library(${lib_name} "${ARMINO_PATH}/../components/avdk_libs/${ARMINO_SOC}/libs/${lib_name}")
        target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
    endforeach()

endif()






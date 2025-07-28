set(incs
	.
	)

set(srcs
	image_convert.c
	image_convert.h
	)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)


set(libs_name )
if (CONFIG_VIDEO_DTCM)
    list(APPEND libs_name
        libscale_dtcm.a)
    if (VIDEO_DTCM_ALL)
        list(APPEND libs_name
            libtjpg_dtcm.a)
    else()
        list(APPEND libs_name
            libtjpg_dtcm_part.a)
    endif()
else()
    list(APPEND libs_name
        libtjpg_sram.a
        libscale_sram.a
        )
endif()

foreach(lib_name ${libs_name})

    add_prebuilt_library(${lib_name} "${ARMINO_PATH}/components/bk_libs/${ARMINO_SOC}/libs/${lib_name}")
    target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
endforeach()
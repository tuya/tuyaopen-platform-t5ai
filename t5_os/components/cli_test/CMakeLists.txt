set(incs .)
set(srcs)
if(CONFIG_AT)
set(priv_req bk_common avdk_libs at_server)
else()
set(priv_req bk_common avdk_libs at)
endif()


if (CONFIG_DVP_CAMERA_TEST)
    list(APPEND srcs "dvp_test.c")
endif()

if (CONFIG_IDF_TEST)
if(CONFIG_AT)
set(../../bk_idk/components ../../components)
list(APPEND incs
	${components}/at_server
	${components}/at_server/_at_server
	${components}/at_server/_at_server_port
	)

    list(APPEND srcs
        "bk_at_video.c"
        )
else()
    list(APPEND srcs
        "video_at_command.c"
		"idf_test.c"
        )
endif()
endif()
if ("${ARMINO_SOC}" STREQUAL "bk7258")
    list(APPEND priv_req multimedia)
else()
    list(APPEND priv_req media)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

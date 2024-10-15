set(incs include)
set(srcs src/media_service.c
)
set(priv_req bk_common)

if (("${ARMINO_SOC}" STREQUAL "bk7258") OR ("${ARMINO_SOC}" STREQUAL "bk7258_cp1") OR ("${ARMINO_SOC}" STREQUAL "bk7258_cp2"))
    list(APPEND priv_req multimedia)
else()
    list(APPEND priv_req media)
endif()

list(APPEND incs bk_private)
list(APPEND srcs
	osi_wrapper/video_osi_wrapper.c
	osi_wrapper/audio_osi_wrapper.c
)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}" REQUIRES avdk_libs net_transfer media_utils bk_peripheral media_unit_test)

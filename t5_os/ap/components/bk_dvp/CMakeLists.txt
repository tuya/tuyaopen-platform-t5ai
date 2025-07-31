set(incs include)
set(srcs "")
set(priv_req bk_common avdk_utils)

if (CONFIG_DVP_CAMERA)
	list(APPEND srcs
		src/bk_dvp.c
		src/video_common_driver.c
	)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}" media_utils)

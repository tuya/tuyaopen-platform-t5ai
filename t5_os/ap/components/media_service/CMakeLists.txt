set(incs include)
set(srcs src/media_service.c
)
set(priv_req bk_common)

list(APPEND priv_req multimedia)


list(APPEND incs bk_private)
list(APPEND srcs
	osi_wrapper/video_osi_wrapper.c
	osi_wrapper/audio_osi_wrapper.c
)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}" REQUIRES media_utils bk_peripheral 
bk_vfs display_service )

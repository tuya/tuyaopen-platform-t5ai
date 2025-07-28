set(incs)
set(srcs)

if (CONFIG_BLUETOOTH_AP)
    list(APPEND srcs
        bt_main.c
    )

    if (CONFIG_BLUETOOTH_SUPPORT_IPC)
        list(APPEND srcs
            ipc/src/bt_ipc_core.c
        )
        list(APPEND incs
            ipc/include
        )
    endif()
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"  
	)

armino_component_register_call_subdirs()

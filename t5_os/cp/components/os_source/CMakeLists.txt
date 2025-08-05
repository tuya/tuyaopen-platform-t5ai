if(CONFIG_FREERTOS)
	if(CONFIG_FREERTOS_V10)
		set(incs freertos_v10/include)
		set(srcs
			freertos_v10/croutine.c
			freertos_v10/event_groups.c
			freertos_v10/list.c
			freertos_v10/queue.c
			freertos_v10/tasks.c
			freertos_v10/stream_buffer.c
			freertos_v10/timers.c
			freertos_v10/portable/MemMang/heap_4.c
			)
		
		if(CONFIG_ARCH_CM33)
			list(APPEND incs
				freertos_v10/portable/GCC/ARM_CM33_NTZ/non_secure
				)
			
			list(APPEND srcs
				freertos_v10/portable/GCC/ARM_CM33_NTZ/non_secure/port.c
				freertos_v10/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.c
				freertos_v10/portable/Common/mpu_wrappers.c
				)
		endif()
		
		if(CONFIG_FULLY_HOSTED OR CONFIG_MEM_MGMT)
			list(REMOVE_ITEM srcs freertos_v10/portable/MemMang/heap_4.c)
		endif()
	
	endif()

endif()


armino_component_register(SRCS "${srcs}"
		INCLUDE_DIRS "${incs}"
		PRIV_REQUIRES bk_common bk_rtos cmsis driver)


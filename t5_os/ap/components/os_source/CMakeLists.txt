if(CONFIG_FREERTOS)

	if(CONFIG_FREERTOS_SMP)
		set(incs
			freertos_smp_v2p0/FreeRTOS-Kernel/include/freertos
			freertos_smp_v2p0/FreeRTOS-Kernel/include/additions
		)
		set(srcs
			freertos_smp_v2p0/FreeRTOS-Kernel/croutine.c
			freertos_smp_v2p0/FreeRTOS-Kernel/event_groups.c
			freertos_smp_v2p0/FreeRTOS-Kernel/list.c
			freertos_smp_v2p0/FreeRTOS-Kernel/queue.c
			freertos_smp_v2p0/FreeRTOS-Kernel/tasks.c
			freertos_smp_v2p0/FreeRTOS-Kernel/stream_buffer.c
			freertos_smp_v2p0/FreeRTOS-Kernel/timers.c
            # freertos_smp_v2p0/FreeRTOS-Kernel/portable/MemMang/heap_6.c
			freertos_smp_v2p0/FreeRTOS-Kernel/portable/MemMang/heap_4.c
			)

		if(CONFIG_ARCH_CM33)
			list(APPEND incs
				freertos_smp_v2p0/FreeRTOS-Kernel/portable/GCC/ARM_CM33_NTZ/non_secure
				)

			list(APPEND srcs
				freertos_smp_v2p0/FreeRTOS-Kernel/portable/GCC/ARM_CM33_NTZ/non_secure/port.c
				freertos_smp_v2p0/FreeRTOS-Kernel/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.c
				# freertos_smp_v2p0/FreeRTOS-Kernel/portable/Common/mpu_wrappers.c
				)
		endif()

		if(CONFIG_FULLY_HOSTED OR CONFIG_MEM_MGMT)
            # list(REMOVE_ITEM srcs freertos_smp_v2p0/FreeRTOS-Kernel/portable/MemMang/heap_6.c)
			list(REMOVE_ITEM srcs freertos_smp_v2p0/FreeRTOS-Kernel/portable/MemMang/heap_4.c)
		endif()

	endif()

endif()


armino_component_register(SRCS "${srcs}"
		INCLUDE_DIRS "${incs}"
        PRIV_REQUIRES bk_common bk_rtos cmsis driver lwip_intf_v2_1) # Modified by TUYA


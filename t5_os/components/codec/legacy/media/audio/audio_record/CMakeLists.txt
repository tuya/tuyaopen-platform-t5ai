if (CONFIG_AUDIO_TEST AND CONFIG_AUDIO_RISCV_IP_V1_0 AND (CONFIG_CPU_CNT GREATER 1))

if (NOT CONFIG_SYS_CPU1)
set(incs .)

set(srcs
	audio_record_demo.c
	)

endif()

endif()
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)
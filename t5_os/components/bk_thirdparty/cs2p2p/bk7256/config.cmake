set(CS2P2P_LIB_COMPILER_PATH )
string(FIND ${CONFIG_TOOLCHAIN_PATH} "nds32le-elf-mculib-v5f" compiler_type)
#set(compiler_type "-1")

if (CONFIG_CS2_P2P_SERVER)

	if (${compiler_type} STREQUAL "-1")
		set(LIB_CS2_FILE_NAME "nds32le-elf-mculib-v5/libPPCS_API_4.5.3_BK7256-Listen-armino-v1.5.9-LNS-IoTWIFI-cs2pmalloc_3ch-20230630-3.a")
	else()
		set(LIB_CS2_FILE_NAME "nds32le-elf-mculib-v5f/libPPCS_API_4.5.3_BK7256-Listen-armino-v1.5.13-v5f-LNS-IoTWIFI-cs2pmalloc_3ch-20230919.a")
	endif()

endif()

if (CONFIG_CS2_P2P_CLIENT)

	if (${compiler_type} STREQUAL "-1")
		set(LIB_CS2_FILE_NAME "nds32le-elf-mculib-v5/libPPCS_API_4.5.3_BK7256-Connect-armino-v1.5.9-LNS-IoTWIFI-cs2pmalloc_3ch-20230630-3.a")
	else()
		set(LIB_CS2_FILE_NAME "nds32le-elf-mculib-v5f/libPPCS_API_4.5.3_BK7256-Connect-armino-v1.5.13-v5f-LNS-IoTWIFI-cs2pmalloc_3ch-20230919.a")
	endif()

endif()

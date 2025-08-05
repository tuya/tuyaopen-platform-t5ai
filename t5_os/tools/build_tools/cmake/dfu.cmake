# Add DFU build and flashing related targets
#

function(__add_dfu_targets)
    armino_build_get_property(target ARMINO_SOC)
    armino_build_get_property(soc_path SOC_PATH)
    set(armino_tools_path "$ENV{ARMINO_TOOLS_PATH}")
    
    if (EXISTS ${soc_path}/${target}/${target}.defconfig)
        message("Check defconfig: " ${soc_path}/${target}/${target}.defconfig)
    else()
        message(SEND_ERROR "DFU PID unknown for ${target}")
    endif()

    armino_build_get_property(python PYTHON)
    armino_build_get_property(armino_path ARMINO_PATH)
    armino_build_get_property(armino_tools_path ARMINO_TOOLS_PATH)

    add_custom_target(dfu
        COMMAND ${python} ${armino_tools_path}/build_tools/mkdfu.py write
        -o "${CMAKE_CURRENT_BINARY_DIR}/dfu.bin"
        --json "${CMAKE_CURRENT_BINARY_DIR}/flasher_args.json"
        --pid "${dfu_pid}"
        DEPENDS gen_project_binary bootloader
        VERBATIM
        USES_TERMINAL)

    add_custom_target(dfu-list
        COMMAND ${CMAKE_COMMAND}
        -D BEKEN_DFU_LIST="1"
        -P ${armino_tools_path}/build_tools/cmake/run_dfu_util.cmake
        USES_TERMINAL)

    add_custom_target(dfu-flash
        COMMAND ${CMAKE_COMMAND}
        -D BEKEN_DFU_BIN="${CMAKE_CURRENT_BINARY_DIR}/dfu.bin"
        -D BEKEN_DFU_PID="${dfu_pid}"
        -P ${armino_tools_path}/build_tools/cmake/run_dfu_util.cmake
        USES_TERMINAL)
endfunction()

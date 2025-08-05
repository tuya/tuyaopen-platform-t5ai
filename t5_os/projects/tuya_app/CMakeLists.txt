# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENTS_DIRS $ENV{ARMINO_PATH}/../../tuyaos/tuyaos_adapter)

# set(lwip_select $ENV{TUYA_LWIP_STACK_USED})
# if (lwip_select STREQUAL "lwip_tuya")
#     list(APPEND EXTRA_COMPONENTS_DIRS $ENV{ARMINO_PATH}/../../tuyaos/tuyaos_adapter/lwip_intf_v2_1)
# endif()

include($ENV{ARMINO_TOOLS_PATH}/build_tools/cmake/project.cmake)
project(app)

# The project compile options should be set after project command
# ???? armino_build_set_property(C_COMPILE_OPTIONS "-DCFG_LOG_LEVEL=2" APPEND)
armino_build_set_property(COMPILE_OPTIONS "-Wno-error" APPEND)

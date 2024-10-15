# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/components/demos/net/iperf)

include($ENV{ARMINO_PATH}/tools/build_tools/cmake/project.cmake)
project(iperf)

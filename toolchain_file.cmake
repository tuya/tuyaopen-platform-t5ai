
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR Linux)

set(TOOLCHAIN_DIR "${PLATFORM_PATH}/../tools/gcc-arm-none-eabi-10.3-2021.10")
set(TOOLCHAIN_PRE "arm-none-eabi-")

# message(STATUS "[TOP] BOARD_PATH: ${BOARD_PATH}")

execute_process(COMMAND uname -s OUTPUT_VARIABLE OS_NAME ERROR_VARIABLE error)
message(STATUS "OS: ${OS_NAME}")

if (${OS_NAME} MATCHES  "Linux")
    set(CMAKE_AR "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}ar")
    set(CMAKE_C_COMPILER "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}gcc")
    set(CMAKE_CXX_COMPILER "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}g++")
elseif (${OS_NAME} MATCHES  "MINGW*" OR ${OS_NAME} MATCHES "CYGWIN*" OR ${OS_NAME} MATCHES "MSYS*")
    set(CMAKE_AR "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}ar.exe")
    set(CMAKE_C_COMPILER "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}gcc.exe")
    set(CMAKE_CXX_COMPILER "${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PRE}g++.exe")
else()
endif()

SET (CMAKE_C_COMPILER_WORKS 1)
SET (CMAKE_CXX_COMPILER_WORKS 1)

set(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_DIR}/bin)

set(CMAKE_C_FLAGS "-g -Os -std=c99 -nostdlib -Wall -Wno-format -Wno-unknown-pragmas -Wno-address-of-packed-member -ffunction-sections -fsigned-char -fdata-sections -fno-strict-aliasing -ggdb -std=gnu99 -Wno-old-style-declaration -mcpu=cortex-m33+nodsp -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mcmse -end-group")

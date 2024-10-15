cmake_minimum_required (VERSION 3.5)

project(diff2ya)

add_library(${PROJECT_NAME} 
    src/diff2ya_hal.c
    src/diff2ya.c
    src/bspatch.c
    src/xz_read.c
    src/utils_crc32.c
    src/bsdiff_flash.c
    src/ty_bsdiff_adapt.c
    src/ty_bsdiff_adapt_fs.c
    src/tuya_fota_cmd.c
)
add_definitions("-Wall -g -std=c99")
add_definitions(-DLINUX_OTA_EN)

target_include_directories( ${PROJECT_NAME}
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(diff2ya xz)

# Add sub directories
add_subdirectory(xz)
add_subdirectory(examples)

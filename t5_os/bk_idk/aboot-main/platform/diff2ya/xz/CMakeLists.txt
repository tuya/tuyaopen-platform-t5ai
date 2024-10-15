# Set the project name
project (xz)

# Add a library with the above sources
add_library(${PROJECT_NAME} STATIC
    xz_crc32.c
    xz_crc64.c
    xz_dec_bcj.c
    xz_dec_stream.c
    xz_dec_lzma2.c
)

target_include_directories( ${PROJECT_NAME}
    PUBLIC ${PROJECT_SOURCE_DIR}
)
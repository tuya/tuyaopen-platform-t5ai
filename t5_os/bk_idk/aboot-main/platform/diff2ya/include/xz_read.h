#ifndef _XZ_READ_H
#define _XZ_READ_H

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "xz.h"

// #define INBUF_MAX BUFSIZ
#define INBUF_MAX 256
#define DICT_SIZE (1024 * 4)
// xz -e --check=crc32 --lzma2=dict=4KiB new2.bin

typedef struct _nb_file_t {
    void* opaque;
    uint32_t size;
    uint32_t offset;
    uint32_t pos;
    int (*read)(struct _nb_file_t* stream, void* buffer, uint32_t length, void* arg);
} nb_file_t;

typedef struct
{
    struct xz_buf xzbuf;
    struct xz_dec* xzdec;
    nb_file_t stream_in;
    uint8_t inbuf[INBUF_MAX];
    size_t avail_size;
    size_t read_pos;
} xzread_handler_t;

bool xz_init(xzread_handler_t* handler, nb_file_t* stream);
size_t xz_read(xzread_handler_t* handler, uint8_t* buf, size_t len, void* arg);

#endif
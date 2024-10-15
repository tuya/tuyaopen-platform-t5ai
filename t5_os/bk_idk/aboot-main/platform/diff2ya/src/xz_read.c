#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "xz_read.h"

size_t nb_fread(void* ptr, size_t size, size_t nmemb, nb_file_t* stream, void* arg)
{
    return stream->read(stream, ptr, size * nmemb, arg);
}

int nb_fseek(nb_file_t* stream, long int offset, int whence)
{
    if (whence == SEEK_SET) {
        stream->pos = stream->offset + offset;
    } else if (whence == SEEK_CUR) {
        stream->pos = stream->pos + offset;
    } else if (whence == SEEK_END) {
        stream->pos = stream->offset + stream->size - offset;
    }
    return 0;
}

int nb_feof(nb_file_t* stream)
{
    if (stream->pos < stream->size) {
        return 0;
    } else {
        return 1;
    }
}

bool xz_init(xzread_handler_t* handler, nb_file_t* stream)
{
    // xz_crc32_init();
    memset(handler, 0, sizeof(xzread_handler_t));
    handler->xzdec = xz_dec_init(XZ_DYNALLOC, DICT_SIZE);
    if (handler->xzdec == NULL) {
        return false;
    }

    handler->xzbuf.in_pos = 0;
    handler->xzbuf.in_size = 0;

    memcpy(&handler->stream_in, stream, sizeof(nb_file_t));
    return true;
}

size_t xz_read(xzread_handler_t* handler, uint8_t* buf, size_t len, void* arg)
{
    if (handler == NULL || buf == NULL || len == 0) {
        return 0;
    }

    //const char* msg;
    enum xz_ret ret;
    handler->xzbuf.in = handler->inbuf;
    handler->xzbuf.out = buf;
    handler->xzbuf.out_size = len;
    handler->xzbuf.out_pos = 0;

    while (true) {
        if (handler->xzbuf.in_pos == handler->xzbuf.in_size) {
            handler->xzbuf.in_size = nb_fread(handler->inbuf, 1, sizeof(handler->inbuf), &handler->stream_in, arg);
            handler->xzbuf.in_pos = 0;
        }

        ret = xz_dec_run(handler->xzdec, &(handler->xzbuf));
        if (ret == XZ_OK) {
            if (handler->xzbuf.out_pos == len) {
                handler->read_pos += handler->xzbuf.out_pos;
                handler->xzbuf.out_pos = 0;
                return len;
            } else {
                continue;
            }
        }

        switch (ret) {
        case XZ_STREAM_END:
            handler->read_pos += handler->xzbuf.out_pos;
            xz_dec_end(handler->xzdec);
            return (handler->xzbuf.out_pos);
#if 0
        case XZ_MEM_ERROR:
            msg = "Memory allocation failed\n";
            goto error;

        case XZ_MEMLIMIT_ERROR:
            msg = "Memory usage limit reached\n";
            goto error;

        case XZ_FORMAT_ERROR:
            msg = "Not a .xz file\n";
            goto error;

        case XZ_OPTIONS_ERROR:
            msg = "Unsupported options in the .xz headers\n";
            goto error;

        case XZ_DATA_ERROR:
            msg = "XZ_DATA_ERROR\n";
            goto error;

        case XZ_BUF_ERROR:
            msg = "File is corrupt\n";
            goto error;

        default:
            msg = "Bug!\n";
            goto error;
#endif
        default:
            goto error;
        }
    }

error:
    xz_dec_end(handler->xzdec);
    return 0;
}
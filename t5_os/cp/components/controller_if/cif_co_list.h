#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _CIF_CO_LIST_H_
#define _CIF_CO_LIST_H_

#include <stddef.h>
#include <os/os.h>
#include "bk_uart.h"
#include <os/mem.h>

// PbufTrackNode
typedef struct PbufTrackNode {
    void* pbuf_addr;
    const char* file;
    int line;
    struct PbufTrackNode* next;
} PbufTrackNode;

void track_pbuf_free(void* pbuf);
void track_pbuf_alloc(void* pbuf, const char* file, int line);


#define TRACK_PBUF_ALLOC(pbuf) track_pbuf_alloc((void*)(pbuf), __FILE__, __LINE__)
#define TRACK_PBUF_FREE(pbuf)  track_pbuf_free((void*)(pbuf))

void print_unreleased_pbufs(void);
int print_unreleased_pbufs_cnt(void);

#endif // _CO_LIST_H_

#ifdef __cplusplus
}
#endif
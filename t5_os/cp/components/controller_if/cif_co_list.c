#include "cif_co_list.h"

static PbufTrackNode* pbuf_head = NULL;

// Add pbuf record
void track_pbuf_alloc(void* pbuf, const char* file, int line) {
    PbufTrackNode* node = (PbufTrackNode*)os_malloc(sizeof(PbufTrackNode));
    if (!node) {
        BK_LOGD(NULL,"track_pbuf_alloc: malloc failed\n");
        return;
    }
    node->pbuf_addr = pbuf;
    node->file = file;
    node->line = line;
    node->next = pbuf_head;
    pbuf_head = node;
}

// Remove pbuf record
void track_pbuf_free(void* pbuf) {
    PbufTrackNode** curr = &pbuf_head;
    while (*curr) {
        if ((*curr)->pbuf_addr == pbuf) {
            PbufTrackNode* to_delete = *curr;
            *curr = (*curr)->next;
            os_free(to_delete);
            return;
        }
        curr = &(*curr)->next;
    }
    BK_LOGD(NULL,"track_pbuf_free: Warning! pbuf not found in list: %p\n", pbuf);
}

// Print all unreleased pbufs
void print_unreleased_pbufs(void) {
    PbufTrackNode* curr = pbuf_head;
    int count = 0;
    BK_LOGD(NULL,"=== Unreleased pbufs ===\n");
    while (curr) {
        BK_LOGD(NULL,"pbuf: %p allocated at %s:%d\n", curr->pbuf_addr, curr->file, curr->line);
        curr = curr->next;
        count++;
    }
    BK_LOGD(NULL,"Total unreleased pbufs: %d\n", count);
}

// Return unreleased pbuf cnt
int print_unreleased_pbufs_cnt(void) {
    PbufTrackNode* curr = pbuf_head;
    int count = 0;
    //BK_LOGD(NULL,"=== Unreleased pbufs ===\n");
    while (curr) {
        //BK_LOGD(NULL,"pbuf: %p allocated at %s:%d\n", curr->pbuf_addr, curr->file, curr->line);
        curr = curr->next;
        count++;
    }
    return count;
    //BK_LOGD(NULL,"Total unreleased pbufs: %d\n", count);
}
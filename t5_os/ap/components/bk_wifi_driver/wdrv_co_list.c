/**
 ****************************************************************************************
 *
 * @file co_list.c
 *
 * Copyright (C) RivieraWaves 2011-2019
 *
 * @brief List management functions
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup CO_LIST
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
// for mem* functions
#include <string.h>

#include "wdrv_co_list.h"
#include <os/mem.h>
#include <common/bk_include.h>
#include "wdrv_main.h"
// for assertions
//#include "dbg_assert.h"
//#include "wifi_rtos.h"

#define WIFI_GLOBAL_INT_DECLARATION()             //uint32_t wifi_irq_level

#define WIFI_GLOBAL_INT_DISABLE()             //wifi_irq_level = rtos_disable_int()

#define WIFI_GLOBAL_INT_RESTORE()             //rtos_disable_int(wifi_irq_level)

#define ASSERT_ERR

static PbufTrackNode* pbuf_head = NULL;
/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void co_list_init(struct co_list *list)
{
    WIFI_GLOBAL_INT_DECLARATION();

    WIFI_GLOBAL_INT_DISABLE();

    list->first = NULL;
    list->last = NULL;

    WIFI_GLOBAL_INT_RESTORE();
}

void co_list_pool_init(struct co_list *list,
                       void *pool,
                       size_t elmt_size,
                       uint32_t elmt_cnt,
                       void *default_value)
{
    uint32_t i;

    // initialize the free list relative to the pool
    co_list_init(list);

    // Add each element of the pool to this list, and init them one by one
    for (i = 0; i < elmt_cnt; i++)
    {
        if (default_value)
        {
            memcpy(pool, default_value, elmt_size);
        }
        co_list_push_back(list, (struct co_list_hdr *) pool);

        // move to the next pool element
        pool = (void *)((uint8_t *)pool + (uint32_t)elmt_size);
    }
}

void co_list_push_back(struct co_list *list,
                       struct co_list_hdr *list_hdr)
{
    WIFI_GLOBAL_INT_DECLARATION();

    WIFI_GLOBAL_INT_DISABLE();

    // Sanity check
    ASSERT_ERR(list_hdr != NULL);

    // check if list is empty
    if (co_list_is_empty(list))
    {
        // list empty => pushed element is also head
        list->first = list_hdr;
    }
    else
    {
        // list not empty => update next of last
        list->last->next = list_hdr;
    }

    // add element at the end of the list
    list->last = list_hdr;
    list_hdr->next = NULL;

    WIFI_GLOBAL_INT_RESTORE();
}

void co_list_push_front(struct co_list *list,
                        struct co_list_hdr *list_hdr)
{
    WIFI_GLOBAL_INT_DECLARATION();

    WIFI_GLOBAL_INT_DISABLE();

    // Sanity check
    ASSERT_ERR(list_hdr != NULL);

    // check if list is empty
    if (co_list_is_empty(list))
    {
        // list empty => pushed element is also head
        list->last = list_hdr;
    }

    // add element at the beginning of the list
    list_hdr->next = list->first;
    list->first = list_hdr;

    WIFI_GLOBAL_INT_RESTORE();
}

struct co_list_hdr *co_list_pop_front(struct co_list *list)
{
    struct co_list_hdr *element;

    WIFI_GLOBAL_INT_DECLARATION();

    WIFI_GLOBAL_INT_DISABLE();

    // check if list is empty
    element = list->first;
    if (element != NULL)
    {
        // The list isn't empty : extract the first element
        list->first = list->first->next;
        if (list->last == element)
            list->last = NULL;
    }

    WIFI_GLOBAL_INT_RESTORE();

    return element;
}

void co_list_extract(struct co_list *list,
                     struct co_list_hdr *list_hdr)
{
    struct co_list_hdr *scan_list;

    // sanity check
    ASSERT_ERR(list != NULL);

    scan_list = list->first;

    // Check if list is empty or not
    if (scan_list == NULL)
        return;

    // check if searched element is first
    if (scan_list == list_hdr)
    {
        // Extract first element
        list->first = scan_list->next;
        if (list->last == scan_list)
            list->last = NULL;
    }
    else
    {
        // Look for the element in the list
        while ((scan_list->next != NULL) && (scan_list->next != list_hdr))
        {
            scan_list = scan_list->next;
        }

        // Check if element was found in the list
        if (scan_list->next != NULL)
        {
            // check if the removed element is the last in the list
            if (list->last == list_hdr)
            {
                // Last element will be extracted
                list->last = scan_list;
            }
            // Extract the element from the list
            scan_list->next = list_hdr->next;
        }
    }
}

bool co_list_find(struct co_list *list,
                  struct co_list_hdr *list_hdr)
{
    struct co_list_hdr *tmp_list_hdr;

    // Go through the list to find the element
    tmp_list_hdr = list->first;
    while((tmp_list_hdr != list_hdr) && (tmp_list_hdr != NULL))
    {
        tmp_list_hdr = tmp_list_hdr->next;
    }

    return (tmp_list_hdr == list_hdr);
}

uint32_t co_list_cnt(const struct co_list *const list)
{
    uint32_t cnt = 0;
    struct co_list_hdr *elt = co_list_pick(list);

    // Go through the list to count the number of elements
    while (elt != NULL)
    {
        cnt++;
        elt = co_list_next(elt);
    }

    return(cnt);
}

/**
 ****************************************************************************************
 * @brief Insert an element in a sorted list.
 *
 * This primitive use a comparison function from the parameter list to select where the
 * element must be inserted.
 *
 * @param[in]  list     Pointer to the list.
 * @param[in]  element  Pointer to the element to insert.
 * @param[in]  cmp      Comparison function (return true if first element has to be inserted
 *                      before the second one).
 *
 * @return              Pointer to the element found and removed (NULL otherwise).
 ****************************************************************************************
 */
void co_list_insert(struct co_list * const list,
                    struct co_list_hdr * const element,
                    bool (*cmp)(struct co_list_hdr const *elementA,
                                struct co_list_hdr const *elementB))
{
    struct co_list_hdr *prev = NULL;
    struct co_list_hdr *scan = list->first;

    for(;;)
    {
        // scan the list until the end or cmp() returns true
        if (scan)
        {
            if (cmp(element, scan))
            {
                // insert now
                break;
            }
            prev = scan;
            scan = scan->next;
        }
        else
        {
            // end of list
            list->last = element;
            break;
        }
    }

    element->next = scan;

    if (prev)
    {
        // second or more
        prev->next = element;
    }
    else
    {
        // first message
        list->first = element;
    }
}

void co_list_insert_after(struct co_list * const list,
                          struct co_list_hdr * const prev_element,
                          struct co_list_hdr * const element)
{
    struct co_list_hdr *scan = list->first;

    if (prev_element == NULL)
    {
        // Insert the element in front on the list
        co_list_push_front(list, element);
    }
    else
    {
        // Look for prev_element in the list
        while (scan)
        {
            if (scan == prev_element)
            {
                break;
            }

            // Get next element
            scan = scan->next;
        }

        // If prev_element has been found, insert element
        if (scan)
        {
            element->next = prev_element->next;
            prev_element->next = element;

            if (list->last == prev_element)
            {
                list->last = element;
            }
        }
    }
}

void co_list_insert_after_fast(struct co_list * const list,
                               struct co_list_hdr * const prev_element,
                               struct co_list_hdr * const element)
{
    if (prev_element == NULL)
    {
        // Insert the element in front on the list
        co_list_push_front(list, element);
    }
    else
    {
        element->next = prev_element->next;
        prev_element->next = element;

        if (list->last == prev_element)
            list->last = element;
    }
}

void co_list_insert_before(struct co_list * const list,
                           struct co_list_hdr * const next_element,
                           struct co_list_hdr * const element)
{
    if (next_element == NULL)
    {
        // Insert the element at the end of the list
        co_list_push_back(list, element);
    }
    else if (next_element == list->first)
    {
        // Insert the element in front of the list
        co_list_push_front(list, element);
    }
    else
    {
        struct co_list_hdr *scan = list->first;

        // Look for next_element in the list
        while (scan)
        {
            if (scan->next == next_element)
            {
                break;
            }

            // Get next element
            scan = scan->next;
        }

        // Insert element after scan
        if (scan)
        {
            element->next = next_element;
            scan->next = element;
        }
    }
}

void co_list_concat(struct co_list *list1, struct co_list *list2)
{
    // If list2 is empty, don't do anything
    if (list2->first != NULL)
    {
        // Check if list1 is empty
        if (list1->first == NULL)
        {
            // If list1 is empty, list1 becomes list2
            *list1 = *list2;
        }
        else
        {
            // Otherwise, append list2 to list1
            list1->last->next = list2->first;
            list1->last = list2->last;
        }
        // Clear list2
        list2->first = NULL;
    }
}


void co_list_remove(struct co_list *list,
                    struct co_list_hdr *prev_element,
                    struct co_list_hdr *element)
{
    // sanity check
    ASSERT_ERR(list != NULL);
    ASSERT_ERR((prev_element == NULL) || (prev_element->next == element));
    ASSERT_ERR(element != NULL);


    if (prev_element == NULL)
    {
        list->first = element->next;
    }
    else
    {
        prev_element->next = element->next;
    }
    if (list->last == element)
        list->last = prev_element;

    element->next = NULL;
}
// Add pbuf record
void track_pbuf_alloc(void* pbuf, const char* file, int line) {
    PbufTrackNode* node = (PbufTrackNode*)os_malloc(sizeof(PbufTrackNode));
    if (!node) {
        WDRV_LOGE("track_pbuf_alloc: malloc failed\n");
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
    WDRV_LOGE("track_pbuf_free: Warning! pbuf not found in list: %p\n", pbuf);
}

// Print all unreleased pbufs
void print_unreleased_pbufs(void) {
    PbufTrackNode* curr = pbuf_head;
    int count = 0;
    WDRV_LOGE("=== Unreleased pbufs ===\n");
    while (curr) {
        WDRV_LOGE("pbuf: %p allocated at %s:%d\n", curr->pbuf_addr, curr->file, curr->line);
        curr = curr->next;
        count++;
    }
    WDRV_LOGE("Total unreleased pbufs: %d\n", count);
}

// Return unreleased pbuf cnt
int print_unreleased_pbufs_cnt(void) {
    PbufTrackNode* curr = pbuf_head;
    int count = 0;
    //BK_LOGD(NULL, "=== Unreleased pbufs ===\n");
    while (curr) {
        //BK_LOGD(NULL, "pbuf: %p allocated at %s:%d\n", curr->pbuf_addr, curr->file, curr->line);
        curr = curr->next;
        count++;
    }
    return count;
    //BK_LOGD(NULL, "Total unreleased pbufs: %d\n", count);
}
/// @}

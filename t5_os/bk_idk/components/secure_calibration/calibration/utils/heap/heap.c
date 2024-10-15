/*
 * Copyright (C), 2018-2019, Arm Technology (China) Co., Ltd.
 * All rights reserved
 *
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 * License Agreement between Licensee and Arm Technology (China) Co., Ltd
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and,,
 * any derivative work shall retain this copyright notice.
 */

#include "base.h"
#include "heap.h"
#include "memmgmt/heap_4.h"
#include "string.h"

#define ENABLE_DEBUG 0

#if ENABLE_DEBUG
#define  KH_DBG       DBG
#define  KH_INFO      INFO
#else
#define  KH_DBG(_f, ...)
#define  KH_INFO(_f, ...)
#endif
#if CONFIG_MEM_LEAK
#include "mem_leak/mem_leak_check.h"
void kfree_ext(void *ptr);
#endif


/* define the ucHeap here for heap_4.c use */
KERNEL_HEAP_DEFINE(ucHeap, 4, CONFIG_SYSTEM_HEAP_SIZE);

void global_heap_init(void)
{
    prvHeapInit();
}

void *kmalloc(size_t size)
{
    void *p = NULL;

#if ENABLE_DEBUG
    uint32_t lr;
    __asm__ volatile("mov %0, lr" : "=r" (lr)::"memory");
#endif

    p = pvPortMalloc(size);

#if CONFIG_MEM_LEAK
        if (p) {
            if (0 != add_mem_info(p, size))
            {
                kfree_ext(p);
                return NULL;
            }
        }
#endif

#if ENABLE_DEBUG
    KH_DBG("+++++++kmalloc size 0x%x, return: 0x%x, lr 0x%x\n",
            size, (uintptr_t)p, lr);
#endif
    return p;
}

void *kcalloc(size_t nmemb, size_t size)
{
    void *p = NULL;

#if ENABLE_DEBUG
    uint32_t lr;
    __asm__ volatile("mov %0, lr" : "=r" (lr)::"memory");
#endif

    p = pvPortMalloc(size * nmemb);

#if CONFIG_MEM_LEAK
         if (p) {
             if (0 != add_mem_info(p, size))
             {
                 kfree_ext(p);
                 return NULL;
             }
         }
#endif

    if (p) {
        memset(p, 0, size * nmemb);
    }

#if ENABLE_DEBUG
    KH_DBG("+++++++kcalloc size 0x%x, return: 0x%x, lr 0x%x\n",
            size * nmemb, (uintptr_t)p, lr);
#endif
    return p;
}

void kfree(void *ptr)
{
#if ENABLE_DEBUG
    uint32_t lr;
    __asm__ volatile("mov %0, lr" : "=r" (lr)::"memory");
#endif

    KH_DBG("-------kfree 0x%x, lr 0x%08x\n", (uintptr_t)ptr, lr);
    vPortFree(ptr);
#if CONFIG_MEM_LEAK
    if(ptr) {
        remove_mem_info(ptr);
    }
#endif
}


#if CONFIG_MEM_LEAK
void kfree_ext(void *ptr)
{
#if ENABLE_DEBUG
    uint32_t lr;
    __asm__ volatile("mov %0, lr" : "=r" (lr)::"memory");
#endif

    KH_DBG("-------kfree_ext 0x%x, lr 0x%08x\n", (uintptr_t)ptr, lr);
    vPortFree(ptr);
}
#endif


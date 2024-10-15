// Copyright 2022-2023 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <stdlib.h>
#include "string.h"
#include "audio_mem.h"

//#define ENABLE_AUDIO_MEM_TRACE

#define TAG "AUD_MEM"

void *audio_malloc(uint32_t size)
{
    void *data =  NULL;
    data = os_malloc(size);

#ifdef ENABLE_AUDIO_MEM_TRACE
    BK_LOGI(TAG, "malloc:%p, size:%d, called:0x%08x \n", data, size, (intptr_t)__builtin_return_address(0) - 2);
#endif
    return data;
}

void audio_free(void *ptr)
{
    os_free(ptr);
#ifdef ENABLE_AUDIO_MEM_TRACE
    BK_LOGI(TAG, "free:%p, called:0x%08x \n", ptr, (intptr_t)__builtin_return_address(0) - 2);
#endif
}

void *audio_calloc(uint32_t nmemb, uint32_t size)
{
    void *data =  NULL;
//    data = calloc(nmemb, size);
	data = os_malloc(nmemb * size);
	if (data)
		os_memset(data, 0x00, nmemb * size);

#ifdef ENABLE_AUDIO_MEM_TRACE
    BK_LOGI(TAG, "calloc:%p, size:%d, called:0x%08x \n", data, size, (intptr_t)__builtin_return_address(0) - 2);
#endif
    return data;
}

void *audio_realloc(void *ptr, uint32_t size)
{
    void *p = NULL;
	p = os_realloc(ptr, size);
#ifdef ENABLE_AUDIO_MEM_TRACE
    BK_LOGI(TAG, "realloc,new:%p, ptr:%p size:%d, called:0x%08x \n", p, ptr, size, (intptr_t)__builtin_return_address(0) - 2);
#endif
    return p;
}

char *audio_strdup(const char *str)
{
    char *copy = os_malloc(strlen(str) + 1);
    if (copy) {
        strcpy(copy, str);
    }
#ifdef ENABLE_AUDIO_MEM_TRACE
    BK_LOGI(TAG, "strdup:%p, size:%d, called:0x%08x \n", copy, strlen(copy), (intptr_t)__builtin_return_address(0) - 2);
#endif
    return copy;
}

void *audio_calloc_inner(uint32_t n, uint32_t size)
{
#if 0
    void *data =  NULL;
    data = os_calloc(n, size);

#ifdef ENABLE_AUDIO_MEM_TRACE
    BK_LOGI("AUIDO_MEM", "calloc_inner:%p, size:%d, called:0x%08x", data, size, (intptr_t)__builtin_return_address(0) - 2);
#endif
    return data;
#endif
	return NULL;
}

void audio_mem_print(char *tag, int line, const char *func)
{
    BK_LOGI(TAG, "Func:%s, Line:%d, MEM Total:%d Bytes\r\n", func, line, rtos_get_free_heap_size());
}


bool audio_mem_spiram_is_enabled(void)
{
    return false;
}

bool audio_mem_spiram_stack_is_enabled(void)
{
    return false;
}


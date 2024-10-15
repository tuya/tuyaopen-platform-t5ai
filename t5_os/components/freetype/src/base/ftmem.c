#include <os/os.h>
#include <os/mem.h>
#include "freetype/ftmem.h"

void *ft_os_malloc(size_t size)
{
    return psram_malloc(size);
}

void *ft_os_realloc(void *ptr, size_t size)
{
    return bk_psram_realloc(ptr, size);
}

void ft_os_free(void *ptr)
{
    psram_free(ptr);
}

void* ft_os_calloc(int num, unsigned int size)
{
    return psram_malloc(num*size);
}
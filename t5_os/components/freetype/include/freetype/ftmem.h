#ifndef _FTMEM_H_
#define _FTMEM_H_
void *ft_os_malloc(size_t size);
void *ft_os_realloc(void *ptr, size_t size);
void ft_os_free(void *ptr);
void* ft_os_calloc(int num, unsigned int size);
#endif
#include "diff2ya_hal.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

static diff2ya_hal_module_t diff2ya_hal_module = {0};

void diff2ya_hal_register_module(diff2ya_hal_module_t *module)
{
    diff2ya_hal_module.malloc_fn = module->malloc_fn;
    diff2ya_hal_module.free_fn = module->free_fn;
    diff2ya_hal_module.flash_write = module->flash_write;
    diff2ya_hal_module.flash_read = module->flash_read;
}

void* diff2ya_malloc(size_t sz)
{
    return diff2ya_hal_module.malloc_fn(sz);
}

void diff2ya_free(void *ptr)
{
    diff2ya_hal_module.free_fn(ptr);
}

int diff2ya_flash_write(uint32_t pos, uint8_t* buf, uint32_t len)
{
    return diff2ya_hal_module.flash_write(pos, buf, len);
}

int diff2ya_flash_read(uint32_t pos, uint8_t* buf, uint32_t len)
{
    //printf("  read : %x,   len : %d\r\n", pos, len);
    return diff2ya_hal_module.flash_read(pos, buf, len);
}
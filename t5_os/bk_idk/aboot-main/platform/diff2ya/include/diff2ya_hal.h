#ifndef _DIFF2YA_HAL_H_
#define _DIFF2YA_HAL_H_

#include <stdint.h>
#include <stdlib.h>

typedef struct {
	void* (*malloc_fn)(size_t num);
	void (*free_fn)(void *ptr);
    int (*flash_read)(uint32_t pos, uint8_t* buf, uint32_t len);
	int (*flash_write)(uint32_t pos, uint8_t* buf, uint32_t len);
}diff2ya_hal_module_t;

void diff2ya_hal_register_module(diff2ya_hal_module_t *module);

void* diff2ya_malloc(size_t sz);
void diff2ya_free(void *ptr);

int diff2ya_flash_write(uint32_t pos, uint8_t* buf, uint32_t len);
int diff2ya_flash_read(uint32_t pos, uint8_t* buf, uint32_t len);


#endif
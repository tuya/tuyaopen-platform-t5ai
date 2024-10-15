#ifndef _TY_BSDIFF_ADAPT_H_
#define _TY_BSDIFF_ADAPT_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef LINUX_OTA_EN

#if 1
#if 1
#  define ty_adapt_print( format, ... ) printf( format, ## __VA_ARGS__ )
#else
#  define ty_adapt_print( format, ... )
#endif
#endif

#define FILE_FLH_OLD_ADDR           0x10000000
#define FILE_FLH_NEW_ADDR           0x20000000

#define FILE_FLH_LEN_ALL         	0x10000000 //512MB

#define FILE_FLH_PATCH_TYPE_ADDR 	0xA0000000
#define FILE_FLH_BACKUP_TYPE_ADDR   0xB0000000       
#define FILE_FLH_MANAGER_TYPE_ADDR  0xC0000000
#define FILE_FLH_BKUP_TYPE_ADDR     0xD0000000

typedef struct
{
    char old_file[128];
    char new_file[128];
    char patch_file[128];
    char mag_file[16];
    char bak_file[16];
} patch_file_info_t;

#else

// #include "cmsis.h"
#define ty_adapt_print bk_printf

#endif

typedef struct {
    uint32_t blocksz;
    uint32_t manager_addr;
} flash_cfg_t;


flash_cfg_t *ty_adapt_flash_get_cfg(void);
int ty_adapt_flash_erase(uint32_t addr, uint32_t len, void* arg);
int ty_adapt_flash_write(uint32_t addr, uint8_t *buf, uint32_t len, void* arg);
int ty_adapt_flash_read(uint32_t addr, uint8_t *buf, uint32_t len, void* arg);

void *ty_adapt_malloc(uint32_t size);
void ty_adapt_free(void *ptr);
uint32_t ty_adapter_get_free_heap_size(void);

#endif
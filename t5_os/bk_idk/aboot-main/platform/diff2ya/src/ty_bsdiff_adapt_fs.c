
#include "ty_bsdiff_adapt.h"
#include "bsdiff_flash.h"

#ifdef LINUX_OTA_EN


#define BLOCK_SZ 		    4096 * 1

flash_cfg_t cfg = {
    .blocksz = BLOCK_SZ,
    .manager_addr = FILE_FLH_MANAGER_TYPE_ADDR,
};

flash_cfg_t *ty_adapt_flash_get_cfg(void)
{
    return &cfg; 
}

char* _get_file_name(uint32_t type_addr, void* arg)
{
    patch_file_info_t* p_file_info;
    uint32_t type = type_addr & 0xF0000000;

    if (arg == NULL)
        return NULL;

    p_file_info = (patch_file_info_t*)arg;
    switch (type)
    {
    case FILE_FLH_OLD_ADDR:
        return p_file_info->old_file;
        break;

    case FILE_FLH_NEW_ADDR:
        return p_file_info->new_file;
        break;

    case FILE_FLH_PATCH_TYPE_ADDR:
        return p_file_info->patch_file;
        break;

    case FILE_FLH_BACKUP_TYPE_ADDR:
        return p_file_info->bak_file;
        break;

    case FILE_FLH_MANAGER_TYPE_ADDR:
        return p_file_info->mag_file;
        break;
    default:
        break;
    }
    return NULL;
}

int ty_adapt_flash_erase(uint32_t addr, uint32_t len, void* arg)
{
    char* filename = _get_file_name(addr, arg);
    if ((addr == 0xFFFFFFFF) || (NULL == filename)) {
        return -1;
    }
    uint32_t off = addr & 0x0FFFFFFF;

    FILE *fp = fopen(filename, "r+b");
    if (NULL == fp) {
        fp = fopen(filename, "w+");
        if (NULL == fp) {
            ty_adapt_print("ty_adapt_flash_erase-addr: 0x%x, filename:%s err!!!\r\n", addr, filename);
            return -1;
        }
    }
    fseek(fp, off, SEEK_SET);
    
    uint8_t *erasebuf = malloc(len);
    if(NULL == erasebuf)
        return -1;

    memset(erasebuf, 0xff, len);
    fwrite(erasebuf, 1, len, fp);
    fclose(fp);

    free(erasebuf);
    return 0;  
}

int ty_adapt_flash_write(uint32_t addr, uint8_t *buf, uint32_t len, void* arg)
{
    char* filename = _get_file_name(addr, arg);
    if ((addr == 0xFFFFFFFF) || (NULL == filename)) {
        return -1;
    }
    uint32_t off = addr & 0x0FFFFFFF;

    FILE *fp = fopen(filename, "r+b");
    if (NULL == fp) {
        fp = fopen(filename, "w+");
        if (NULL == fp) {
            ty_adapt_print("ty_adapt_flash_write-addr:0x%x, filename:%s err!!!\r\n", addr, filename);
            return -1;
        }
    }
    fseek(fp, off, SEEK_SET);

    fwrite(buf, 1, len, fp);
    fclose(fp);

    return 0;
}

int ty_adapt_flash_read(uint32_t addr, uint8_t *buf, uint32_t len, void* arg)
{
    char* filename = _get_file_name(addr, arg);
    if ((addr == 0xFFFFFFFF) || (NULL == filename)) {
        return -1;
    }
    uint32_t off = addr & 0x0FFFFFFF;

    FILE *fp = fopen(filename, "r+b");
    if (NULL == fp) {
        fp = fopen(filename, "w+");
        if (NULL == fp) {
            ty_adapt_print("ty_adapt_flash_read-addr:%d, filename:%s err!!!\r\n", addr, filename);
            return -1;
        }
    }
    fseek(fp, off, SEEK_SET);

    fread(buf, 1, len, fp);
    fclose(fp);

    return 0;
}

void *ty_adapt_malloc(uint32_t size)
{
    return malloc(size);
}


void ty_adapt_free(void *ptr)
{
    free(ptr);
}

uint32_t ty_adapter_get_free_heap_size(void)
{
    return 0;
}

#endif


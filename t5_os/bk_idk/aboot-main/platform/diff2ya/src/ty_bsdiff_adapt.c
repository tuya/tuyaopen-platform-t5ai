#include "ty_bsdiff_adapt.h"
#include "bsdiff_flash.h"



#ifndef LINUX_OTA_EN

// #include "image/flash.h"
// #include "hal_cache.h"

#define BLOCK_SZ 		    4096*1

#define FLASH_START_ADDR_IMG    0x011000
#define FLASH_START_LEN_IMG     (0x3D8000-0x011000)
#define MAG_FLASH_START_ADDR    0x3D8000
#define MAG_FLASH_LEN           (8*1024)

uint8_t mem_buff[1024*100] = {0};               /**默认100k堆区，各平台可根据实际情况调整*/
#define FLH_MANAGER_ADDR      MAG_FLASH_START_ADDR         /**固定manager地址，如果不指定backup和patch地址，需要将manager放在flash最末尾*/
#define MFLASH                       0

// extern hal_spic_adaptor_t *pglob_spic_adaptor;

flash_cfg_t cfg = {
    .blocksz = BLOCK_SZ,
    .manager_addr = FLH_MANAGER_ADDR,
};

flash_cfg_t *ty_adapt_flash_get_cfg(void)
{
    return &cfg; 
}

int ty_adapt_flash_erase(uint32_t addr, uint32_t len, void* arg)
{
    uint16_t start_sec = (addr/BLOCK_SZ);
    uint16_t end_sec = ((addr+len-1)/BLOCK_SZ);
    uint16_t i = 0;
    uint32_t sector_addr = 0;
    // ty_adapt_print("ty_adapt_flash_erase:addr:0x%x, len:%d\r\n", addr, len);
    for(i = start_sec; i <= end_sec; i++) {
        sector_addr = BLOCK_SZ * i;
        // ty_adapt_print("sector_addr:0x%x\r\n", sector_addr);
        flash_erase_sector(sector_addr);
    }
    return 0;  
}

int ty_adapt_flash_write(uint32_t addr, uint8_t *buf, uint32_t len, void* arg)
{
    flash_write_data(buf, addr, len);
    return 0;
}

int ty_adapt_flash_read(uint32_t addr, uint8_t *buf, uint32_t len, void* arg)
{
    // BL_ERR("addr:0x%x, buf:%p, len:%d\r\n", addr, buf, len);
    flash_read_data(buf, addr, len);
    return 0;
}

void *ty_adapt_malloc(uint32_t size)
{
    static uint8_t heap_init_flg = 0;
    if (!heap_init_flg) {
        heap_init_flg = 1;
        prvHeapInit(mem_buff, sizeof(mem_buff));
    }
    return ty_pvPortMalloc(size);
    // return malloc(size);
}

void ty_adapt_free(void *ptr)
{
    ty_vPortFree(ptr);
    // free(ptr);
}

uint32_t ty_adapter_get_free_heap_size(void)
{
    return xPortGetFreeHeapSize();
    // return 0;
}
#endif

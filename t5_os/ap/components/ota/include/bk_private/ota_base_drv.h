#pragma once
#include "sdkconfig.h"
#include "driver/flash.h"
#include "driver/flash_partition.h"
#include "common/bk_err.h"
#include "CheckSumUtils.h"

enum
{
    F_OTA_DEBUG_LEVEL_ERROR,
    F_OTA_DEBUG_LEVEL_WARNING,
    F_OTA_DEBUG_LEVEL_INFO,
    F_OTA_DEBUG_LEVEL_DEBUG,
    F_OTA_DEBUG_LEVEL_VERBOSE,
};

#define F_OTA_DEBUG_LEVEL F_OTA_DEBUG_LEVEL_DEBUG
#define OTA_LOGE(format, ...) do{if(F_OTA_DEBUG_LEVEL > F_OTA_DEBUG_LEVEL_ERROR)   BK_LOGE("f_ota", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define OTA_LOGW(format, ...) do{if(F_OTA_DEBUG_LEVEL > F_OTA_DEBUG_LEVEL_WARNING) BK_LOGW("f_ota", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define OTA_LOGI(format, ...) do{if(F_OTA_DEBUG_LEVEL > F_OTA_DEBUG_LEVEL_INFO)    BK_LOGI("f_ota", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define OTA_LOGD(format, ...) do{if(F_OTA_DEBUG_LEVEL > F_OTA_DEBUG_LEVEL_DEBUG)   BK_LOGI("f_ota", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define OTA_LOGV(format, ...) do{if(F_OTA_DEBUG_LEVEL > F_OTA_DEBUG_LEVEL_VERBOSE) BK_LOGI("f_ota", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)

#ifndef ERASE_TOUCH_TIMEOUT
#define ERASE_TOUCH_TIMEOUT         (3000)
#endif
#ifndef ERASE_FLASH_TIMEOUT
#define ERASE_FLASH_TIMEOUT         (56)
#endif
#ifndef WRITE_FLASH_TIMEOUT
#define WRITE_FLASH_TIMEOUT         (4)
#endif
#ifndef FLASH_SECTOR_SIZE
#define FLASH_SECTOR_SIZE           (0x1000)
#endif
#define OTA_FLASH_BUFFER_LENGTH     (1024)

#ifndef MIN
#define MIN(x, y)                  (((x) < (y)) ? (x) : (y))
#endif

#define OTA_CHECK_POINTER(ptr) do{ \
    if(ptr == NULL){ \
        return BK_FAIL; \
    } \
}while(0)

#define OTA_MALLOC(ptr, size) do{ \
    ptr = (void*)os_malloc(size); \
    if(ptr == NULL){ \
        OTA_LOGE("%s malloc fail \r\n",ptr); \
        return BK_FAIL; \
    } \
}while(0)

#define OTA_MALLOC_WITHOUT_RETURN(ptr, size) do{ \
    ptr = (void*)os_malloc(size); \
    if(ptr == NULL){ \
        OTA_LOGE("%s malloc fail \r\n",ptr); \
        return ; \
    } \
}while(0)

#define OTA_FREE(ptr) do{ \
    if(ptr != NULL){ \
        os_free(ptr); \
        ptr = NULL; \
    } \
}while(0)

typedef struct __attribute__((packed))
{
    /* data */
    uint8_t  *magic_code;                  /*ota magic code*/
    uint16_t new_sequence_number;          /*the new sequence number*/
    uint16_t curr_sequence_number;         /*the current sequence number*/
    uint16_t wr_last_len ;                 /*the remaining lenth*/
    uint32_t partition_length;             /*the partition length*/
    uint32_t wr_address;                   /*the written address*/
    uint32_t image_size;                   /*the ota image total size*/
    uint32_t received_total_size;          /*receive the total image size*/  
    uint8_t  *wr_buf;                      /*store data from apk*/
    uint8_t  *wr_tmp_buf;                  /*store data from app temporarily*/
    uint8_t  *rd_buf;                      /*store data from falsh */
    bk_logic_partition_t *pt;              /**/
    flash_protect_type_t  protect_type;    /**/
    uint8_t  wr_flash_flag;                /*the write flash flag*/
    CRC32_Context ota_crc;
}f_ota_t;

typedef struct
{
    /* data */
    int (*init)(f_ota_t* ota_ptr);
    int (*wr_flash)(f_ota_t* ota_ptr, uint16_t len);
    int (*data_process)(f_ota_t* ota_ptr,uint16_t len);
    int (*deinit)(f_ota_t* ota_ptr);
    int (*crc)(f_ota_t* ota_ptr,uint32_t in_crc);
}f_ota_func_t;

extern const f_ota_func_t *f_ota_fun_ptr;
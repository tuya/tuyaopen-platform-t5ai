#ifndef _BSDIFF_FLASH_H_
#define _BSDIFF_FLASH_H_


#include <stdint.h>
#include <stdlib.h>
#include "ty_bsdiff_adapt.h"

#define BSDF_MAGIC      0xABCDDCBA
#define SINGLE_PATCH    0x53494E47//单区SING-0x53494E47   双区DOUB-0x444F5542

#define BSDIFF_IDLE  0
#define BSDIFF_RESTORE0  1
#define BSDIFF_RESTORE1  2
#define BSDIFF_FLIT  3


#define BD_OK               0
#define BD_COM_ERR         -1
#define BD_INVALID_PRM     -2
#define BD_MALLOC_ERR      -3
#define BD_CHECK_ERR       -4
#define BD_NO_NEED_OTA     -5

//#define BSDIFF_UNIT        (20 * 1024)

typedef struct {
    uint32_t magic;//0xABCDDCBA
    uint32_t crc32;
    uint32_t patchsz;//patch package size
    uint32_t wrcnt;//timestamp
    uint32_t ctrlid;//state == BSDIFF_RESTORE0/1, progress of recovery
    uint32_t flitoff;//state == BSDIFF_FLIT, progress of flit
    uint8_t blockid;//manage block id
    uint8_t state;
    uint8_t is_confirm_addr;        /*支持固定back_up地址，无特殊需求，这里直接给0，有差分算法自动计算----0:差分算法自动按照从后向前计算地址，1:固定back_up/patch起始地址*/
    uint8_t reserved1[1];
    uint32_t flh_start_addr;        /*flash起始地址*/
    uint32_t flh_len;               /*flash总长度*/
    uint32_t backup_confirm_addr;   /*back_up起始地址固定，不执行从后向前计算*/
    uint32_t patch_confirm_addr;    /*patch起始地址固定，不执行从后向前计算*/
    uint8_t reserved2[20];
} manage_info_t;

/*
|------------------firmware-------------------patch-------|-----backup----|--manage-|
|-------------------len-28K-------------------------------|------20K------|----8K---|
*/
typedef struct {
    flash_cfg_t cfg;
    uint32_t frmaddr;
    uint32_t pthaddr;
    uint32_t bkupaddr;
    uint32_t magaddr;
} flash_info_t;

typedef struct 
{
    manage_info_t mag_info;
    flash_info_t flh_info;
} bsdiff_mag_t;

typedef struct {
    uint32_t magic_ver;
    uint32_t bin_type;          /*单区SING-0x53494E47   双区DOUB-0x444F5542*/
    uint32_t src_crc32;         /*src bin crc32*/
    uint32_t dst_crc32;         /*dst bin crc32*/
    uint32_t src_length;        /*src bin length*/
    uint32_t dst_length;        /*dst bin length*/
    uint32_t bin_offset;        /*bin offset from FOTA Pkg Head*/
    uint32_t bin_start_addr;    /*bin start addr in ROM*/
    uint32_t bin_length;        /*bin length*/
    uint32_t bin_crc32;         /*bin crc32 checksum*/
    uint32_t step;              /*maxlen + step*/
    uint32_t buf_size;          /*buf_size*/
    uint8_t mode;               /*0:forward   1:backward*/
    uint8_t file_num;
    uint16_t ver_supp_min_code; /*ver_supp_min_code*/
    uint8_t reserved2[12];
}diff2ya_header_t;

typedef struct {
	uint32_t old_bin_addr;
	uint32_t new_bin_addr;
	uint32_t patch_bin_addr;
}diff2ya_context_t;

#define bd_malloc  ty_adapt_malloc
#define bd_free    ty_adapt_free


int bsdiff_mag_clear(manage_info_t *mag_info, flash_info_t *flh_info, void* arg);
int update_manage_info(manage_info_t *mag_info, flash_info_t *flh_info, void* arg);
int bsdiff_flash_init(manage_info_t *mag_info, flash_info_t *flh_info, diff2ya_header_t *diff2ya_header, void* arg);
int _bkup_2_frmware(bsdiff_mag_t *mag, uint32_t frmaddr, uint8_t *buf, uint32_t len, uint8_t flag, void* arg);
int _buff_2_bkup_2_frmware(bsdiff_mag_t *mag, uint32_t ctrlid, uint32_t frmaddr, uint8_t *buf, uint32_t len, uint8_t flag, void* arg);
int _flit_data_step(bsdiff_mag_t *mag, uint32_t dst_length, uint32_t fl_offlen, uint32_t buf_size, uint32_t step, void* arg);

#endif


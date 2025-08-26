/**
 * @file ota_head.c
 * @version 0.1
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#include "tuya_cloud_types.h"
#include "tkl_ota.h"
#include "tkl_flash.h"
#include "tkl_memory.h"

/***********************************************************
************************macro define************************
***********************************************************/
#define FLASH_SECTOR_SIZE 4096
#define OTA_HEAD_MAGIC    0x4d4d4d
#define BSDF_MAGIC        0xABCDDCBA

#define EXTEND_ABILITY_FIRMWARE_CRC   1<<5
#define EXTEND_ABILITY_FIRMWARE_NEED_ADD_ATTR2   1<<6
/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef struct {
    uint32_t diff_start_addr;
    uint32_t diff_end_addr;
    uint32_t manage_start_addr;
    uint32_t flash_len;
    uint32_t src_bin_len;
} OTA_FLASH_ADDR_T;

typedef struct {
    uint32_t magic_ver;             //0x4D4D4D
    uint32_t bin_type;            
    uint32_t src_crc32;             /*src bin crc32*/
    uint32_t dst_crc32;             /*dst bin crc32*/
    uint32_t src_length;            /*src bin length*/
    uint32_t dst_length;            /*dst bin length*/
    uint32_t bin_offset;            /*bin offset from FOTA Pkg Head*/
    uint32_t bin_start_addr;        /*bin start addr in ROM*/
    uint32_t patch_length;          /*patch length*/
    uint32_t patch_crc32;           /*patch crc32 checksum*/
    uint32_t step;                  /*maxlen + step*/
    uint32_t buf_size;              /*buf_size*/
    uint8_t  mode;                   /*0/10:forward   1/11:backward    （10/11为支持还原速度优化）*/
    uint8_t  file_index;            
    uint16_t min_supp_ver;          
    uint32_t attr1;                 
    uint32_t attr2;              
    uint32_t extend_len;            
}OTA_HEAD_T;

typedef struct {
    uint32_t extend_type;
    uint32_t ability;		
    uint32_t src_bin_len;
    uint32_t dst_bin_len;
    uint32_t part_b_crc;
    uint32_t diff_flg_len;	
    uint8_t* diff_flg;	    
} OTA_HEAD_EXT_T;

typedef struct {
    uint32_t magic;//0xABCDDCBA
    uint32_t crc32;
    uint32_t patchsz;//patch package size
    uint32_t wrcnt;//timestamp
    uint32_t ctrlid;//state == BSDIFF_RESTORE0/1, progress of recovery
    uint32_t flitoff;//state == BSDIFF_FLIT, progress of flit
    uint8_t blockid;//manage block id
    uint8_t state;
    uint8_t is_confirm_addr;        /*支持固定back_up地址，无特殊需求，这里直接给0，差分算法自动计算----0:差分算法自动按照从后向前计算地址，1:固定back_up/patch起始地址*/
    uint8_t recover_type;                        /*0-普通差分，1-AB分区覆盖下载，2-AB分区还原*/
    uint32_t flh_start_addr;        /*flash起始地址*/
    uint32_t flh_len;               /*flash总长度*/
    uint32_t backup_confirm_addr;   /*back_up起始地址固定，不执行从后向前计算*/
    uint32_t patch_confirm_addr;    /*patch起始地址固定，不执行从后向前计算*/
    uint32_t attr1;                 /*1<<0          AB 分区的时候*/
    uint32_t ability;               /*1<<0|0        AB分区的B区起始地址
                                      1<<1          bk系列32字节加2字节crc      */
    uint32_t part_b_crc;
    uint8_t reserved1[8];
} manage_info_t;

typedef struct {
    uint32_t image_size;
    bool     is_firmware_crc;
    uint32_t part_b_crc; 
    uint32_t offset;
}OTA_FILE_INFO_T;

/***********************************************************
***********************variable define**********************
***********************************************************/
static OTA_FLASH_ADDR_T sg_ota_flash_addr;
static OTA_FILE_INFO_T sg_file_info;
 
/***********************************************************
***********************function define**********************
***********************************************************/
static OPERATE_RET __ota_get_flash_addr(OTA_FLASH_ADDR_T *flash_addr)
{
    TUYA_FLASH_BASE_INFO_T flash_info;

    if (OPRT_OK != tkl_flash_get_one_type_info(TUYA_FLASH_TYPE_APP_BIN, &flash_info)) {
        return OPRT_INVALID_PARM;
    }
    flash_addr->src_bin_len = flash_info.partition[0].size;

    if (OPRT_OK != tkl_flash_get_one_type_info(TUYA_FLASH_TYPE_APP, &flash_info)) {
        return OPRT_INVALID_PARM;
    }
    flash_addr->diff_start_addr = flash_info.partition[0].start_addr;
    flash_addr->flash_len = flash_info.partition[0].size;

    if (OPRT_OK != tkl_flash_get_one_type_info(TUYA_FLASH_TYPE_OTA, &flash_info)) {
        return OPRT_INVALID_PARM;
    }
    flash_addr->manage_start_addr = flash_info.partition[0].start_addr;
    flash_addr->diff_end_addr = flash_addr->manage_start_addr + flash_info.partition[0].size;

    bk_printf("__ota_get_flash_addr: %x, %x, %x, flash_len:%d, bin:%d\r\n", flash_addr->diff_start_addr, \
                                                                            flash_addr->diff_end_addr, \
                                                                            flash_addr->manage_start_addr, \
                                                                            flash_addr->flash_len, \
                                                                            flash_addr->src_bin_len);
    return OPRT_OK;
}


static uint32_t _cal_patch_start_addr(OTA_HEAD_T *head_path)
{
    uint32_t patch_actual_len = head_path->patch_length + sizeof(OTA_HEAD_EXT_T);

    if (patch_actual_len % FLASH_SECTOR_SIZE) {
        patch_actual_len += FLASH_SECTOR_SIZE - patch_actual_len % FLASH_SECTOR_SIZE;
    }

    return (sg_ota_flash_addr.manage_start_addr - patch_actual_len);
}

static uint32_t _check_patch_head(OTA_HEAD_T *head_path, bool is_crc)
{
    //校验path包头标记为是否为差分包
    if ((head_path->magic_ver != OTA_HEAD_MAGIC)) { // || (head_path->bin_type != 0x53494E47)
        bk_printf("path_head magic_ver(0x%x) or bin_type(0x%x) is error\r\n", head_path->magic_ver, head_path->bin_type);
        return -1;
    }

    uint32_t bin_actual_len = (head_path->dst_length > head_path->src_length) ? head_path->dst_length : head_path->src_length;
    if (is_crc) {
        bin_actual_len = (bin_actual_len % 32) ? (bin_actual_len + 32 - bin_actual_len % 32) : bin_actual_len;
        bin_actual_len += bin_actual_len / 32 * 2;  
    }

    if (bin_actual_len % FLASH_SECTOR_SIZE) {
        bin_actual_len += (FLASH_SECTOR_SIZE - bin_actual_len % FLASH_SECTOR_SIZE);
    }

    uint32_t patch_actual_len = head_path->patch_length + sizeof(OTA_HEAD_T);
    if (patch_actual_len % FLASH_SECTOR_SIZE) {
        patch_actual_len += FLASH_SECTOR_SIZE - patch_actual_len % FLASH_SECTOR_SIZE;
    }

    uint32_t occupy_len = bin_actual_len + patch_actual_len + head_path->step + head_path->buf_size + 2 * FLASH_SECTOR_SIZE;
    if (sg_ota_flash_addr.flash_len < occupy_len) {
        bk_printf("overflow flash size flash_len %d, occupy_len %d\r\n", sg_ota_flash_addr.flash_len, occupy_len);
        return -2;
    }

    return 0;
}

OPERATE_RET bk_ota_get_file_init(uint32_t image_size)
{
    OPERATE_RET ret = OPRT_OK;

    ret = __ota_get_flash_addr(&sg_ota_flash_addr);
    if (ret != OPRT_OK) {
        return ret;
    }

    sg_file_info.image_size = image_size;
    sg_file_info.is_firmware_crc = false;
    sg_file_info.part_b_crc = 0;

    return OPRT_OK;
}

OPERATE_RET bk_ota_get_file_header(TUYA_OTA_DATA_T *pack)
{
    OPERATE_RET ret = OPRT_OK;
    OTA_HEAD_T *p_headr = NULL;
    OTA_HEAD_EXT_T *p_header_ext = NULL;
    bool is_crc = false;

    if(NULL == pack) {
        return OPRT_INVALID_PARM;
    }

    if (pack->len < sizeof(OTA_HEAD_T)) {
        return OPRT_INVALID_PARM;
    }

    p_headr = (OTA_HEAD_T *)pack->data;
    if(p_headr->extend_len) {
        p_header_ext = (OTA_HEAD_EXT_T *)(pack->data + sizeof(OTA_HEAD_T));
        if(p_header_ext->ability & EXTEND_ABILITY_FIRMWARE_NEED_ADD_ATTR2) {
            is_crc = true;
        }
        sg_file_info.part_b_crc = p_header_ext->part_b_crc;
    }else {
        is_crc = false;
    }

    ret = _check_patch_head(p_headr, is_crc);
    if (ret != OPRT_OK) {
        return ret;
    }

    pack->start_addr = _cal_patch_start_addr(p_headr);

    sg_file_info.offset = p_headr->attr1; 
    sg_file_info.is_firmware_crc = is_crc;
    
    return OPRT_OK;
}

static uint32_t _cal_crc32(uint8_t *buf, uint32_t len)
{
    uint32_t crc32 = 0;
    uint32_t i;
    for (i = 0; i < len; i++) {
        crc32 += buf[i];
    }

    return crc32;
}

static int32_t __read_manage_block(uint8_t id, uint8_t *buf, uint32_t len)
{
    uint32_t ret = 0;
    if (NULL == buf) {
        return -1;
    }

    if ((id > 1) || (len > FLASH_SECTOR_SIZE)) {
        return -2;
    }

    uint32_t addr = sg_ota_flash_addr.manage_start_addr + id * FLASH_SECTOR_SIZE;
    ret = tkl_flash_read(addr, buf, len);
    if (ret) {
        return -3;
    }

    //check validity
    uint32_t rescrc = 0;
    manage_info_t *mag = (manage_info_t *)buf;

 #if 1
    bk_printf("magic:%x\r\n", mag->magic);
    bk_printf("crc32:%x\r\n", mag->crc32);
    bk_printf("patchsz:%d\r\n", mag->patchsz);
    bk_printf("wrcnt:%d\r\n", mag->wrcnt);
    bk_printf("ctrlid:%d\r\n", mag->ctrlid);
    bk_printf("flitoff:%d\r\n", mag->flitoff);
    bk_printf("blockid:%d\r\n", mag->blockid);
    bk_printf("state:%d\r\n", mag->state);
    bk_printf("reserved1:%d\r\n", mag->recover_type);
    bk_printf("flh_start_addr:%x\r\n", mag->flh_start_addr);
    bk_printf("flh_len:%d\r\n", mag->flh_len);
    bk_printf("backup_confirm_addr:%x\r\n", mag->backup_confirm_addr);
    bk_printf("patch_confirm_addr:%x\r\n", mag->patch_confirm_addr);
    bk_printf("attr1:%x\r\n", mag->attr1);
    bk_printf("ability:%x\r\n", mag->ability);
#endif 

    if ((mag->magic != BSDF_MAGIC) || \
        ((rescrc = _cal_crc32(buf + 8, len - 8)) != mag->crc32) || \
        (mag->blockid != id)) {
        bk_printf("read_manage_block err: [%d-%d]mag->magic=0x%x, rescrc=0x%x, mag->crc32=0x%x.\r\n",
                       id,mag->blockid,mag->magic, rescrc, mag->crc32);
        return -4;
    }

    return 0;
}

static int32_t update_manage_info(manage_info_t *mag_info)
{
    uint32_t ret = 0;

    if ((NULL == mag_info)) {
        return -2;
    }

    if (mag_info->magic != BSDF_MAGIC) {
        return -1;
    }

    mag_info->blockid = mag_info->blockid ? 0 : 1;
    mag_info->wrcnt++;

    uint8_t *buf = (uint8_t *)mag_info;
    buf += 8;
    uint32_t len = sizeof(manage_info_t) - 8;
    mag_info->crc32 = _cal_crc32(buf, len);

    //erase flash
    uint32_t addr = sg_ota_flash_addr.manage_start_addr + mag_info->blockid * FLASH_SECTOR_SIZE;
    ret = tkl_flash_erase(addr, FLASH_SECTOR_SIZE);
    if (ret) {
        goto EXIT;
    }
    //write flash
    buf = (uint8_t *)mag_info;
    ret = tkl_flash_write(addr, buf, sizeof(manage_info_t));
    if (ret) {
        goto EXIT;
    }

EXIT:
    return ret;
}

static OPERATE_RET read_manage_info(manage_info_t *mag_info)
{
    if (NULL == mag_info) {
        return -1;
    }

    manage_info_t mag0, mag1;
    uint8_t *buf0 = (uint8_t *)&mag0;
    uint8_t *buf1 = (uint8_t *)&mag1;

    uint8_t checkerr0 = 0, checkerr1 = 0;
    int ret = __read_manage_block(0, buf0, sizeof(manage_info_t));
    if (ret) {
        if (ret != -4) {
            return ret;
        } else {
            checkerr0 = 1;
        }
    }

    ret = __read_manage_block(1, buf1, sizeof(manage_info_t));
    if (ret) {
        if (ret != -4) {
            return ret;
        } else {
            checkerr1 = 1;
        }
    }

    bk_printf("ota manage info:%d, %d\r\n", checkerr0, checkerr1);
    if (checkerr0) {
        if (checkerr1) {
            return -5;
        } else {
            memcpy((uint8_t *)mag_info, buf1, sizeof(manage_info_t));
        }
    } else {
        if (checkerr1) {
            memcpy((uint8_t *)mag_info, buf0, sizeof(manage_info_t));
        } else {
            manage_info_t *mag0 = (manage_info_t *)buf0;
            manage_info_t *mag1 = (manage_info_t *)buf1;
            if (mag0->wrcnt < mag1->wrcnt) {
                memcpy((uint8_t *)mag_info, buf1, sizeof(manage_info_t));
            } else {
                memcpy((uint8_t *)mag_info, buf0, sizeof(manage_info_t));
            }
        }
    }

    return 0;
}

OPERATE_RET bk_ota_write_manage_info(void)
{
    OPERATE_RET ret = OPRT_OK;
    manage_info_t *p_mag = NULL;

    p_mag = (manage_info_t *)tkl_system_malloc(sizeof(manage_info_t));
    if (NULL == p_mag) {
        return OPRT_MALLOC_FAILED;
    }
    memset(p_mag, 0, sizeof(manage_info_t));

    ret = read_manage_info(p_mag);
    if (ret != OPRT_OK) {
        bk_printf("read_manage_info failed: %d\r\n", ret);
        p_mag->magic = BSDF_MAGIC;
        p_mag->state = 1;
        p_mag->patchsz = sg_file_info.image_size;
        p_mag->flh_start_addr = sg_ota_flash_addr.diff_start_addr;
        p_mag->flh_len = sg_ota_flash_addr.flash_len;
        p_mag->ability= sg_file_info.is_firmware_crc;
        p_mag->attr1 = sg_file_info.offset;
        p_mag->part_b_crc = sg_file_info.part_b_crc;

        uint32_t crc32 = 0, i=0;
        uint8_t *buf = (uint8_t *)p_mag;
        for (i = 8; i < sizeof(manage_info_t); i++) {
            crc32 += buf[i];
        }
        p_mag->crc32 = crc32;

        tkl_flash_erase(sg_ota_flash_addr.manage_start_addr, 2 * FLASH_SECTOR_SIZE);
        tkl_flash_write(sg_ota_flash_addr.manage_start_addr, buf, sizeof(manage_info_t));
    }else {
        p_mag->patchsz = sg_file_info.image_size;
        p_mag->state = 1;
        p_mag->flh_start_addr = sg_ota_flash_addr.diff_start_addr;
        p_mag->flh_len = sg_ota_flash_addr.flash_len;
        p_mag->ability = sg_file_info.is_firmware_crc;
        p_mag->attr1 = sg_file_info.offset;
        p_mag->part_b_crc = sg_file_info.part_b_crc;
        ret = update_manage_info(p_mag);
        bk_printf("update_manage_info failed: %d\r\n", ret);
    }

    tkl_system_free(p_mag);
    p_mag = NULL;

    return OPRT_OK;
}
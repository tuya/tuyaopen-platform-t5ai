/**
 * @file tuyaopen_license.c
 * @brief tuyaopen_license module is used to 
 * @version 0.1
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 */

#include "tuyaopen_license.h"

#include "tkl_memory.h"
#include "cJSON.h"

#include <common/bk_include.h>
#include <driver/otp_types.h>
#include <driver/flash_partition.h>
#include "flash_bypass.h"

/***********************************************************
************************macro define************************
***********************************************************/
// platform/T5AI/t5_os/build/bk7258/tuya_app/bk7258_ap/armino/partitions/_build/_otp.h
#define OTP_CHIP_OPENSDK_ITEM           OTP_TUYAOPEN
// platform/T5AI/t5_os/ap/middleware/boards/bk7258_ap/csv/otp2.csv
#define OTP_CHIP_OPENSDK_SIZE_MAX       (256)

#define OPENSDK_RFCALI_SIZE_MAX         (512)
#define OTP_CHIP_OPENSDK_RFCALI_ITEM    OTP_TUYAOPEN_RFCALI
#define OTP_CHIP_OPENSDK_RFCALI_MAX     OPENSDK_RFCALI_SIZE_MAX

// flash otp
#define OTP_FLASH_SIZE 1024

#define OTP_FLASH_RF_ADDR 0
#define OTP_FLASH_RF_SIZE 512

#define OTP_FLASH_OPENSDK_ADDR (OTP_FLASH_RF_ADDR + OTP_FLASH_RF_SIZE)
#define OTP_FLASH_OPENSDK_SIZE 256

#define OTP_FLASH_RFDATA_SIZE       OPENSDK_RFCALI_SIZE_MAX

#define ENABLE_TUYAOPEN_LICENSE_DEBUG 0

/***********************************************************
***********************typedef define***********************
***********************************************************/


/***********************************************************
********************function declaration********************
***********************************************************/
// platform/T5AI/t5_os/ap/middleware/driver/otp/otp_driver_v1_1.c
extern bk_err_t bk_otp_ahb_update(otp2_id_t item, uint8_t* buf, uint32_t size);
extern bk_err_t bk_otp_ahb_read(otp2_id_t item, uint8_t* buf, uint32_t size);
extern bk_err_t bk_otp_ahb_write_permission(otp2_id_t item, otp_privilege_t permission);
extern otp_privilege_t bk_otp_ahb_read_permission(otp2_id_t item);

/***********************************************************
***********************variable define**********************
***********************************************************/
static otp2_id_t sg_otp_rf_id[] ={
    OTP_RFCALI1,
    OTP_RFCALI2,
    OTP_RFCALI3,
    OTP_RFCALI4
};

#define OTP_CHIP_RF_NUM  (sizeof(sg_otp_rf_id) / sizeof(sg_otp_rf_id[0]))

/***********************************************************
***********************function define**********************
***********************************************************/

static int __chip_otp_write(const uint8_t *data, const uint32_t data_len)
{
    int rt = OPRT_OK;

    bk_err_t bk_ret = BK_OK;
    // Chip efuse/flash/otp write

    if (NULL == data || data_len == 0 || data_len > OTP_CHIP_OPENSDK_SIZE_MAX) {
        return OPRT_INVALID_PARM;
    }

    uint8_t *read_buf = (uint8_t *)tkl_system_malloc(OTP_CHIP_OPENSDK_SIZE_MAX);
    if (read_buf == NULL) {
        bk_printf("malloc failed for read_buf\n");
        return OPRT_MALLOC_FAILED;
    }
    memset(read_buf, 0, OTP_CHIP_OPENSDK_SIZE_MAX);

    /* read chip otp, if all data is 0, then it is empty */
    bk_ret =  bk_otp_ahb_read(OTP_CHIP_OPENSDK_ITEM, read_buf, OTP_CHIP_OPENSDK_SIZE_MAX);
    if (bk_ret != BK_OK) {
        bk_printf("read chip otp failed, ret: %d\n", bk_ret);
        rt = OPRT_COM_ERROR;
        goto __EXIT;
    }

    /* check if chip otp is empty */
    int is_empty = 1;
    for (int i = 0; i < OTP_CHIP_OPENSDK_SIZE_MAX; i++) {
        if (read_buf[i] != 0) {
            is_empty = 0;
            break;
        }
    }
    if (!is_empty) {
        bk_printf("chip otp is not empty, cannot write\n");
        rt = OPRT_COM_ERROR;
        goto __EXIT;
    }

#if (ENABLE_TUYAOPEN_LICENSE_DEBUG == 0)
    /* write chip otp */
    bk_ret = bk_otp_ahb_update(OTP_CHIP_OPENSDK_ITEM, data, data_len);
    if (bk_ret != BK_OK) {
        bk_printf("write chip otp failed, ret: %d\n", bk_ret);
        rt = OPRT_COM_ERROR;
        goto __EXIT;
    }

    /* verify chip otp */
    memset(read_buf, 0, OTP_CHIP_OPENSDK_SIZE_MAX);
    bk_ret =  bk_otp_ahb_read(OTP_CHIP_OPENSDK_ITEM, read_buf, OTP_CHIP_OPENSDK_SIZE_MAX);
    if (bk_ret != BK_OK) {
        bk_printf("read chip otp failed, ret: %d\n", bk_ret);
        rt = OPRT_COM_ERROR;
        goto __EXIT;
    }

    if (memcmp(read_buf, data, data_len) != 0) {
        bk_printf("verify chip otp failed\n");
        rt = OPRT_COM_ERROR;
        goto __EXIT;
    }

    // NOTE: do NOT lock OTP permission here. The Beken OTP permission lock
    // register (rolck) is shared across 32-word (128-byte) groups, so locking
    // OTP_TUYAOPEN (id=5) would also lock OTP_TUYAOPEN_RFCALI (id=6) which
    // shares the word 320~351 permission group. The official CP RF-calibration
    // path also writes OTP without locking, so we follow the same convention.
    // bk_ret = bk_otp_ahb_write_permission(OTP_CHIP_OPENSDK_ITEM, OTP_READ_ONLY);
    // if (bk_ret != BK_OK) {
    //     bk_printf("lock chip otp failed, ret: %d\n", bk_ret);
    //     rt = OPRT_COM_ERROR;
    //     goto __EXIT;
    // }
#endif // ENABLE_TUYAOPEN_LICENSE_DEBUG

__EXIT:
    /* free read buffer */
    tkl_system_free(read_buf);
    read_buf = NULL;

    return rt;
}

static int __chip_otp_license_read(uint8_t **data, uint32_t *data_len)
{
    bk_err_t bk_ret = BK_OK;

    // Input validation
    if (NULL == data || NULL == data_len) {
        return OPRT_INVALID_PARM;
    }

    uint8_t *read_buf = (uint8_t *)tkl_system_malloc(OTP_CHIP_OPENSDK_SIZE_MAX);
    if (read_buf == NULL) {
        bk_printf("malloc failed for read_buf\n");
        return OPRT_MALLOC_FAILED;
    }
    memset(read_buf, 0, OTP_CHIP_OPENSDK_SIZE_MAX);

    // read data from chip otp
    bk_ret = bk_otp_ahb_read(OTP_CHIP_OPENSDK_ITEM, read_buf, OTP_CHIP_OPENSDK_SIZE_MAX);
    if (bk_ret != BK_OK) {
        bk_printf("read chip otp failed, ret: %d\n", bk_ret);
        tkl_system_free(read_buf);
        read_buf = NULL;
        return OPRT_COM_ERROR;
    }

    *data = read_buf;
    *data_len = OTP_CHIP_OPENSDK_SIZE_MAX;

    return OPRT_OK;
}

static int __flash_otp_license_read(uint8_t **data, uint32_t *data_len)
{
    int ret = 0;

    // Input validation
    if (NULL == data || NULL == data_len) {
        return OPRT_INVALID_PARM;
    }

    uint8_t *read_buf = (uint8_t *)tkl_system_malloc(OTP_FLASH_OPENSDK_SIZE);
    if (NULL == read_buf) {
        return OPRT_MALLOC_FAILED;
    }
    memset(read_buf, 0, OTP_FLASH_OPENSDK_SIZE);

    flash_bypass_otp_ctrl_t otp_op = {0};
    otp_op.otp_idx = 1; // "1 or 2 or 3"
    otp_op.addr_offset = OTP_FLASH_OPENSDK_ADDR;
    otp_op.write_len = 0;
    otp_op.write_buf = NULL;
    otp_op.read_len = OTP_FLASH_OPENSDK_SIZE;
    otp_op.read_buf = read_buf;

    ret = flash_bypass_otp_operation(FLASH_BYPASS_OTP_READ, &otp_op);
    if (ret != BK_OK) {
        tkl_system_free(read_buf);
        read_buf = NULL;
        return OPRT_COM_ERROR;
    }

    *data = read_buf;
    *data_len = OTP_FLASH_OPENSDK_SIZE;

    return OPRT_OK;
}

// Back up RF calibration data to OTP
// static int __chip_otp_rf_cali_backup(void)
// {
//     int rt = OPRT_OK;
//     bk_err_t bk_ret = BK_OK;

//     uint32_t addr;
//     uint8_t *dst = NULL;
//     uint32_t size;

//     // RF cali data magic header: ASCII "TLV\0" (0x54 0x4C 0x56 0x00)
//     static const uint8_t rf_cali_magic[4] = {0x54, 0x4C, 0x56, 0x00};

//     int rfcali_stat = bk_wifi_manual_cal_rfcali_status();
//     if (rfcali_stat != BK_OK) {
//         bk_printf("RF calibration status check failed, status: %d\n", rfcali_stat);
//         return OPRT_COM_ERROR;
//     }

//     // Why flash is 512 bytes but OTP_RFCALI1~4 are only 256 bytes each:
//     //
//     // - Flash RF partition (OTP_FLASH_RF_SIZE = 512B) holds the FULL RF cali
//     //   data as a nested TLV (magic 0x00564C54, ~504 bytes): it keeps every
//     //   band/mode calibration entry plus its descriptive structure, in the raw
//     //   uncompressed form produced by the calibration tool.
//     //
//     // - OTP_RFCALI1~4 are 256 bytes because 256B is the OTP physical layout
//     //   granularity (see otp2.csv: offsets 0x40/0x140/0x240/0x340, all 256B
//     //   aligned). But the closed CP library that owns these slots only consumes
//     //   a 140-byte COMPACT format out of each one: a flat power-table array
//     //   with a trailing CRC8 (poly 0x31). The 140-byte form is a lossy,
//     //   converted/compressed private format, NOT the raw flash TLV.
//     //
//     // - Consequence: you cannot copy the 504-byte flash TLV into a 140-byte
//     //   compact slot -- it would overflow and the CRC8 check would fail. That
//     //   is why the backup is written to the dedicated 512-byte archive slot
//     //   OTP_TUYAOPEN_RFCALI, which fits the full flash TLV with no conversion.
//     //
//     // OTP_RFCALI1 ~ OTP_RFCALI4 are owned by that closed CP library, which
//     // auto-iterates them at boot to restore RF data. We only READ them here to
//     // decide whether to skip the backup: a slot counts as "already backed up"
//     // only when it is BOTH locked (read-only) AND holds valid RF cali data
//     // (TLV magic). Permission alone is insufficient -- a slot may be locked
//     // with non-RF content. This read-only probe also replaces the standalone
//     // permission check done below for the archive slot.
//     for (int i = 0; i < OTP_CHIP_RF_NUM; i++) {
//         // otp_privilege_t permission = bk_otp_ahb_read_permission(sg_otp_rf_id[i]);
//         // if (permission == OTP_READ_WRITE) {
//         //     continue; // writable -> nothing backed up in this slot yet
//         // }

//         // slot is locked (read-only). A locked permission alone is not enough
//         // to treat it as "backed up" -- it may have been locked with non-RF
//         // data. Verify the content actually starts with the RF cali magic
//         // header before deciding to skip.
//         uint8_t otp_head[4] = {0};
//         bk_ret = bk_otp_ahb_read(sg_otp_rf_id[i], otp_head, sizeof(otp_head));
//         if (bk_ret != BK_OK) {
//             bk_printf("read rf cali otp slot %d failed, ret: %d\n", i, bk_ret);
//             continue; // unreadable, try next slot
//         }

//         if (memcmp(otp_head, rf_cali_magic, sizeof(rf_cali_magic)) == 0) {
//             bk_printf("rf cali otp slot %d already locked with valid data, skip backup\n", i);
//             return OPRT_OK; // not error, already backed up
//         }

//         // locked but not valid RF cali data -> keep checking remaining slots
//     }

//     dst = tkl_system_malloc(OTP_FLASH_RFDATA_SIZE);
//     if (NULL == dst) {
//         return OPRT_MALLOC_FAILED;
//     }
//     memset(dst, 0, OTP_FLASH_RFDATA_SIZE);

//     bk_logic_partition_t *pt = bk_flash_partition_get_info(BK_PARTITION_SYS_RF);
//     addr = pt->partition_start_addr;
//     size = OTP_FLASH_RFDATA_SIZE;
//     bk_ret = bk_flash_read_bytes(addr, (uint8_t *)dst, size);
//     if (bk_ret != BK_OK) {
//         bk_printf("read rf cali data from flash failed, ret: %d\n", bk_ret);
//         rt = OPRT_COM_ERROR;
//         goto __EXIT;
//     }

//     // verify the flash RF cali data header: must be 0x54 0x4C 0x56 0x00 (ASCII "TLV\0")
//     if (memcmp(dst, rf_cali_magic, sizeof(rf_cali_magic)) != 0) {
//         bk_printf("invalid rf cali data header, skip backup\n");
//         rt = OPRT_COM_ERROR;
//         goto __EXIT;
//     }

//     // Debug print the RF calibration data
//     bk_printf("RF calibration data read from flash:\n");
//     for (uint32_t i = 0; i < size; i++) {
//         bk_printf("%02X ", dst[i]);
//         if ((i + 1) % 16 == 0) {
//             bk_printf("\n");
//         }
//     }
//     if (size % 16 != 0) {
//         bk_printf("\n");
//     }

//     // check OTP_TUYAOPEN_RFCALI permission (archive slot, 512 bytes)
//     otp_privilege_t permission = bk_otp_ahb_read_permission(OTP_CHIP_OPENSDK_RFCALI_ITEM);
//     if (permission != OTP_READ_WRITE) {
//         // chip otp is locked, skip backup
//         bk_printf("chip otp is locked, skip backup rf cali data\n");
//         rt = OPRT_OK; // not error, just skip
//         goto __EXIT;
//     }

// #if (ENABLE_TUYAOPEN_LICENSE_DEBUG == 0)
//     // write the full flash TLV to the archive slot, no format conversion
//     bk_ret = bk_otp_ahb_update(OTP_CHIP_OPENSDK_RFCALI_ITEM, dst, size);
//     if (bk_ret != BK_OK) {
//         bk_printf("write rf cali data to chip otp failed, ret: %d\n", bk_ret);
//         rt = OPRT_COM_ERROR;
//         goto __EXIT;
//     }

//     // NOTE: do NOT lock OTP permission. Same reason as __chip_otp_write: the
//     // rolck register is shared across 32-word groups, locking OTP_TUYAOPEN_RFCALI
//     // would affect neighboring slots. The official CP RF-calibration path does
//     // not lock either, so we follow the same convention.
//     // bk_ret = bk_otp_ahb_write_permission(OTP_CHIP_OPENSDK_RFCALI_ITEM, OTP_READ_ONLY);
//     // if (bk_ret != BK_OK) {
//     //     bk_printf("lock rf cali data in chip otp failed, ret: %d\n", bk_ret);
//     //     rt = OPRT_COM_ERROR;
//     //     goto __EXIT;
//     // }
// #endif // ENABLE_TUYAOPEN_LICENSE_DEBUG

// __EXIT:
//     if (dst) {
//         tkl_system_free(dst);
//         dst = NULL;
//     }

//     return rt;
// }

int tuyaopen_license_write(const char *data, const uint32_t data_len)
{
    int rt = OPRT_OK;

    uint8_t *enc_data = NULL;
    uint32_t enc_len = 0;

    // Input validation
    if (NULL == data || data_len == 0) {
        return OPRT_INVALID_PARM;
    }

    // for T5
    // chack MAC
    // {"auzkey":"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx","uuid":"uuidxxxxxxxxxxxxxxxx","prod_test":false,"ap_ssid":"SmartLife","mac":"C8478C000018"}
    bk_printf("write raw data len %d\r\n", data_len);
    bk_printf("write raw data: %s\r\n", data);
    cJSON *root = cJSON_ParseWithLength(data, data_len);
    if (root == NULL) {
        bk_printf("Authorization JSON parse failure.\r\n");
        return OPRT_COM_ERROR;
    }
    cJSON *j_mac = cJSON_GetObjectItem(root, "mac");
    if (j_mac == NULL) {
        bk_printf("MAC not found in JSON.\r\n");
        cJSON_Delete(root);
        root = NULL;
        return OPRT_COM_ERROR;
    }
    char *mac_str = j_mac->valuestring;
    if (mac_str == NULL || strlen(mac_str) != 12 || strcmp(mac_str, "C8478C000018") == 0) {
        bk_printf("MAC check failed, invalid MAC: %s\r\n", mac_str ? mac_str : "(null)");
        cJSON_Delete(root);
        root = NULL;
        return OPRT_COM_ERROR;
    }
    // free cJSON root
    cJSON_Delete(root);
    root = NULL;

    // Encryption
    extern int tuyaopen_license_encryption(uint8_t *data, uint32_t datalen, uint8_t **out, uint32_t *outlen);
    rt = tuyaopen_license_encryption((uint8_t *)data, data_len, &enc_data, &enc_len);
    if (OPRT_OK != rt) {
        bk_printf("tuyaopen_license_encryption failed, rt:%d\r\n", rt);
        return rt;
    }

    // Write to OTP
    rt = __chip_otp_write(enc_data, enc_len);
    if (OPRT_OK != rt) {
        bk_printf("Chip OTP write failed, rt:%d\r\n", rt);
        rt = OPRT_COM_ERROR;
        goto __EXIT;
    }

    // backup RF calibration data to OTP
    // Wait beken support flash rf cali data to otp
    // rt = __chip_otp_rf_cali_backup();
    // if (OPRT_OK != rt) {
    //     bk_printf("Chip OTP RF calibration backup failed, rt:%d\r\n", rt);
    //     rt = OPRT_COM_ERROR;
    //     goto __EXIT;
    // }

__EXIT:
    // free enc_data after write to OTP
    if (enc_data) {
        tkl_system_free(enc_data);
        enc_data = NULL;
    }

    return rt;
}

int tuyaopen_license_read(char **data, uint32_t *data_len)
{
    int rt = OPRT_OK;

    uint8_t *enc_data = NULL;
    uint32_t enc_len = 0;

    // Input validation
    if (NULL == data || NULL == data_len) {
        return OPRT_INVALID_PARM;
    }

    // Read from chip OTP
    rt = __chip_otp_license_read(&enc_data, &enc_len);
    if (OPRT_OK == rt) {
        // Decryption
        extern int tuyaopen_license_decryption(uint8_t *data, uint32_t datalen, uint8_t **out, uint32_t *outlen);
        rt = tuyaopen_license_decryption(enc_data, enc_len, (uint8_t **)data, data_len);
        if (OPRT_OK == rt) {
            rt = OPRT_OK;
            goto __EXIT; // success
        }
    }
    if (enc_data) {
        tkl_system_free(enc_data);
        enc_data = NULL;
    }
    enc_len = 0;
    bk_printf("tuyaopen license chip otp read failed, rt:%d\r\n", rt);

    // Read from flash OTP
    rt = __flash_otp_license_read(&enc_data, &enc_len);
    if (OPRT_OK == rt) {
        // Decryption
        extern int tuyaopen_license_decryption(uint8_t *data, uint32_t datalen, uint8_t **out, uint32_t *outlen);
        rt = tuyaopen_license_decryption(enc_data, enc_len, (uint8_t **)data, data_len);
        if (OPRT_OK == rt) {
            rt = OPRT_OK;
            goto __EXIT;
        }
    }
    bk_printf("tuyaopen license flash otp read failed, rt:%d\r\n", rt);

__EXIT:
    // free enc_data after decryption
    if (enc_data) {
        tkl_system_free(enc_data);
        enc_data = NULL;
    }

    return rt;
}
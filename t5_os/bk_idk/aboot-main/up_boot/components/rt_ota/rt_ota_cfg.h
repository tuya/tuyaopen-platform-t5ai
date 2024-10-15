/*
 * File      : rt_ota_cfg.h
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-23     armink       the first version
 */

#ifndef _RT_OTA_CFG_H_
#define _RT_OTA_CFG_H_

#include <board.h>

/* ========================== Debug Configuration =========================== */
#define RT_OTA_DEBUG                   0


/* the flash device name which saved bootloader */
#define RT_OTA_BL_FLASH_DEV_NAME       "beken_onchip"
#define RT_OTA_BL_FLASH_DEV_CRC_NAME   "beken_onchip_crc"

/* bootloader partition name */
#define RT_OTA_BL_PART_NAME            "bootloader"

/* bootloader partition offset address */
#define RT_OTA_BL_PART_OFFSET          0
/* bootloader partition length */
#define RT_OTA_BL_PART_LEN             (64 * 1024)

#endif /* _RT_OTA_CFG_H_ */

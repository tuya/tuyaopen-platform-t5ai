/*
 * File      : fal_cfg.h
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-05-17     armink       the first version
 * 2018-05-30     MurphyZhao   Adapted beken7231 board feature
 */

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#include <rtconfig.h>
#include <board.h>

#define RT_FLASH_PORT_DRIVER_BEKEN
#define RT_FLASH_PORT_DRIVER_BEKEN_CRC

/* ===================== Flash device Configuration ========================= */
extern const struct fal_flash_dev beken_onchip_flash;
extern const struct fal_flash_dev beken_onchip_flash_crc;

/* flash device table */
#define FAL_FLASH_DEV_TABLE                                          \
{                                                                    \
    &beken_onchip_flash,                                             \
    &beken_onchip_flash_crc,                                         \
}

/* the flash device name which saved bootloader */
#define RT_BK_FLASH_DEV_NAME       "beken_onchip"
#define RT_BK_FLASH_DEV_CRC_NAME   "beken_onchip_crc"

/* flash start addr */
#define RT_FLASH_BEKEN_START_ADDR        0x00000000
#define RT_FLASH_BEKEN_SIZE              (4 * 1024 * 1024)                            // unit: bytes, total 4Mbytes

#define RT_FLASH_BEKEN_SECTOR_SIZE       (4096)

/* bootloader partition name */
#define RT_BK_BL_PART_NAME            "bootloader"
#define RT_BK_BL_PART_ADDR            0x00
#define RT_BK_BL_PART_LEN             (64 * 1024)

#define RT_BK_APP_NAME                "app"
#define RT_BK_APP_PART_ADDR           (RT_BK_BL_PART_ADDR + RT_BK_BL_PART_LEN)          // crc, logic address.
#define RT_BK_APP_PART_LEN            (1792*1024)
#define RT_BK_APP_PART_END_ADDR       (RT_BK_APP_PART_ADDR + RT_BK_APP_PART_LEN)

#define RT_OTA_ROMFS_PART_NAME         "romfs"
#define RT_OTA_ROMFS_PART_ADDR         (0x1ED000/68*64)                                 // crc, logic address.
#define RT_OTA_ROMFS_PART_LEN          (544/68*64*1024)
#define RT_OTA_ROMFS_PART_END_ADDR     (RT_OTA_ROMFS_PART_ADDR + RT_OTA_ROMFS_PART_LEN)

#define RT_BK_DL_PART_NAME            "download"
#define RT_BK_DL_PART_ADDR            (0x275000)                                        // no crc, physical address. 
#define RT_BK_DL_PART_LEN             (1496*1024)
#define RT_BK_DL_PART_END_ADDR        (RT_BK_DL_PART_ADDR + RT_BK_DL_PART_LEN)

#define RT_BK_PARAM1_PART_NAME            "param1"
#define RT_BK_PARAM1_PART_ADDR            0x3EB000
#define RT_BK_PARAM1_PART_LEN             (16 * 1024)

#define RT_BK_PARAM2_PART_NAME            "param2"
#define RT_BK_PARAM2_PART_ADDR            0x3EF000
#define RT_BK_PARAM2_PART_LEN             (16 * 1024)

#define RT_BK_PARAM3_PART_NAME            "param3"
#define RT_BK_PARAM3_PART_ADDR            0x3F3000
#define RT_BK_PARAM3_PART_LEN             (4 * 1024)

#define RT_BK_PARAM4_PART_NAME            "param4"
#define RT_BK_PARAM4_PART_ADDR            0x3F7000
#define RT_BK_PARAM4_PART_LEN             (4 * 1024)

/* Preprocess */
/* bootloader_end_addr < app_start_addr; bootloader_end_addr 4K aligned*/
#if (RT_BK_BL_PART_LEN + RT_BK_BL_PART_ADDR) > RT_BK_APP_PART_ADDR
    #error "boot end addr > app start addr!"
#endif

#if (RT_BK_BL_PART_LEN + RT_BK_BL_PART_ADDR)%(4 * 1024) != 0
    #error "boot end addr is not 4K aligned!"
#endif

/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG
/* partition table */
#define FAL_PART_TABLE                                                          \
{                                                                               \
    {FAL_PART_MAGIC_WROD, RT_BK_BL_PART_NAME, RT_BK_FLASH_DEV_CRC_NAME, RT_BK_BL_PART_ADDR, RT_BK_BL_PART_LEN, 0},      \
    {FAL_PART_MAGIC_WROD, RT_BK_APP_NAME, RT_BK_FLASH_DEV_CRC_NAME, RT_BK_APP_PART_ADDR, RT_BK_APP_PART_LEN, 0},    \
    {FAL_PART_MAGIC_WROD, RT_OTA_ROMFS_PART_NAME, RT_BK_FLASH_DEV_CRC_NAME, RT_OTA_ROMFS_PART_ADDR, RT_OTA_ROMFS_PART_LEN, 0},   \
    {FAL_PART_MAGIC_WROD, RT_BK_DL_PART_NAME, RT_BK_FLASH_DEV_NAME, RT_BK_DL_PART_ADDR, RT_BK_DL_PART_LEN, 0},      \
    {FAL_PART_MAGIC_WROD, RT_BK_PARAM1_PART_NAME, RT_BK_FLASH_DEV_NAME, RT_BK_PARAM1_PART_ADDR, RT_BK_PARAM1_PART_LEN, 0},   \
    {FAL_PART_MAGIC_WROD, RT_BK_PARAM2_PART_NAME, RT_BK_FLASH_DEV_NAME, RT_BK_PARAM2_PART_ADDR, RT_BK_PARAM2_PART_LEN, 0},   \
    {FAL_PART_MAGIC_WROD, RT_BK_PARAM3_PART_NAME, RT_BK_FLASH_DEV_NAME, RT_BK_PARAM3_PART_ADDR, RT_BK_PARAM3_PART_LEN, 0},   \
    {FAL_PART_MAGIC_WROD, RT_BK_PARAM4_PART_NAME, RT_BK_FLASH_DEV_NAME, RT_BK_PARAM4_PART_ADDR, RT_BK_PARAM4_PART_LEN, 0},   \
}
#endif /* FAL_PART_HAS_TABLE_CFG */

#endif /* _FAL_CFG_H_ */

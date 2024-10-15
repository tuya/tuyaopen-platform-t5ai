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
#define RT_FLASH_BEKEN_SIZE              (4 * 1024 * 1024)               // unit: bytes, total 16Mbytes

#define RT_FLASH_BEKEN_SECTOR_SIZE       (4096)

/* bootloader partition name */
#define RT_BK_BL_PART_NAME            "bootloader"
#define RT_BK_BL_PART_ADDR            0x00
#define RT_BK_BL_PART_LEN             (64 * 1024)

#define RT_BK_APP_NAME                "app"
#define RT_BK_APP1_NAME               "app1"
#define RT_BK_DL_PART_NAME            "download"
#define RT_OTA_ROMFS_PART_NAME        "romfs"


#endif /* _FAL_CFG_H_ */

/* 
 * Note: 
 * Beken7231 program code is stored in ROM, the address range is 0x0-0x200000(total size 2Mbytes).
 * Code storage features on flash: Add 2 bytes of checksum every 32 bytes using crc16.
 * Address translation: flash storage addr is physical address; CPU execution address is logical address.
 *                      [physical_addr = logical_addr/32*34]
 * 
 * Flash Table: <Logical address>   <Physical address>
 * +------------+  0x0000000          0x00000000
 * |vectors(32b)|
 * |            |
 * |------------+  boot: 64k          68K
 * |Bootloader  |
 * |            |
 * |------------+  partition name: bootloader  
 * | Boot header|                     0xFFA0
 * |   (96B)    | 
 * +------------+  0x0010000          0x11000   #Note: [0x11000] - 4K aligned; 32 aligned; 34 aligned
 * |vectors(32b)|
 * |            |
 * |------------+  partition name: app
 * |text        |
 * |data        |  app size: 0xFA540 + 96  app_crc size: 0x109F94 + 102
 * |            |
 * |------------+  
 * | app header |  0x10A540           0x11AF94  #Note: [0x10A540] - 32 aligned; [0x11AF94] - 34 aligned
 * |  (96+6)B   | 
 * +------------+  0x10A5A0           0x11AFFA  #Note: [0x10A5A0] - 32 aligned; [0x11AFFA] - 34 aligned
 * | 6B Reserved|                     6 bytes is Reserved, it will be erased by app partition
 * +------------+  0x11B000           0x11B000  #Note: [0x11B000] - 4K aligned
 * | DL  header |
 * |   (96B)    |
 * +------------+                     download size: 0xDB000 (876KB)
 * |     OTA    |
 * |   PACKAGE  |  partition name: download  
 * |            |
 * +------------+                     0x1F6000
 * +------------+                     0x1F6000  #Note: [0x1F6000] - 4K aligned
 * |            |  partition name: param1
 * | param1     |                     UserParam: 16k
 * |            |
 * +------------+                     0x1FA000
 * +------------+                     0x1FA000  #Note: [0x1FA000] - 4K aligned
 * |            |  partition name: param2
 * | param2     |                     UserParamBak: 16k
 * |            |
 * +------------+                     0x1FE000
 * +------------+                     0x1FE000  #Note: [0x1FE000] - 4K aligned
 * |            |  partition name: param3
 * | param3     |                     UserParamBak: 4k
 * |            |
 * +------------+                     0x1FF000
 * +------------+                     0x1FF000  #Note: [0x1FF000] - 4K aligned
 * |            |  partition name: param4
 * | param4     |                     SysParam: 4k
 * |            |
 * +------------+  0x200000           0x200000
*/

/*
 * File      : board.h
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2018, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-27     armink      add board.h to this bsp
 * 2018-01-29     Murphy      modify board.h to beken7231 bsp
 * 2018-02-03     Myrphy      Adapted beken7231 board feature
 */

/*
 * Flash Table: <Logical address>   <Physical address>
 * +------------+  0x0000000          0x00000000
 * |vectors(32b)|
 * |            |
 * |------------+  boot: 64k          68K
 * |Bootloader  |
 * |            |
 * |------------+
 * | Boot header|                     0xFFA0
 * |   (96B)    |
 * +------------+  0x0010000          0x11000   #Note: [0x11000] - 4K aligned; 32 aligned; 34 aligned
 * |vectors(32b)|
 * |            |
 * |------------+
 * |text        |
 * |data        |  app: 928k          986k
 * |            |
 * |------------+
 * | app header |  0xF7FA0            0x10779A  #Note: [0x10779A] - 34 aligned
 * |  (96+6)B   |
 * +------------+  0xF8000            0x107800  #Note: [0x107800] - 32 aligned; 34 aligned
 * +------------+  0x108000           0x108000  #Note: [0x108000] - 4K aligned
 * | DL  header |
 * |   (96B)    |
 * +------------+  download: 980k
 * |     OTA    |
 * |   PACKAGE  |
 * |            |
 * +------------+  0x1FD000           0x1FD000
 * +------------+  0X1FD000           0x1FD000  #Note: [0x1FD000] - 4K aligned
 * |User        |
 * |Param       |  UserParam: 4k
 * |            |
 * +------------+  0x1FE000           0x1FE000
 * +------------+  0X1FE000           0x1FE000  #Note: [0x1FE000] - 4K aligned
 * |User        |
 * |Param       |  UserParamBak: 4k
 * |Bak         |
 * +------------+  0x1FF000           0x1FF000
 * +------------+  0x1FF000           0x1FF000
 * |  Sys Param |
 * |   (WiFi)   |  SysParam: 4k
 * |            |
 * +------------+  0x200000           0x200000
*/

#ifndef __BOARD_H__
#define __BOARD_H__

#include <BK_config.h>
#include <BK_System.h>
#include <stdint.h>

// <o> Internal SRAM memory size[Kbytes] <8-112>
// <i>Default: 112
#define BK_SRAM_BEGIN           ((uint32_t)0x00040000)
#define BK_SRAM_SIZE            (256 * 1024)                    // unit: bytes, total 256Kbytes
#define BK_SRAM_END             (BK_SRAM_BEGIN + BK_SRAM_SIZE * 1024)

#define BK_FLASH_BEGIN          ((uint32_t)0x0000000)
#define BK_FLASH_SIZE           (2 * 1024 * 1024)               // unit: bytes, total 2Mbytes
#define BK_FLASH_END            (BK_FLASH_BEGIN + BK_FLASH_SIZE)

#define BK_BOOT_ENTRY           BK_FLASH_BEGIN
#define BK_BOOT_SIZE            (64 * 1024)                     // unit: bytes, total 64Kbytes
#define BK_BOOT_END             (BK_BOOT_ENTRY + BK_BOOT_SIZE)

/* user application start logic address on flash */
#define USER_APP_ENTRY          0x90000
#define USER_APP_SIZE           (928 * 1024)                    // unit: bytes, total 952Kbytes
#define USER_APP_END            (USER_APP_ENTRY + USER_APP_SIZE)// 0xF8000

#define HARDWARE_VERSION        "1.0"

void IWDG_Feed(void);
void bl_hw_board_init(void);
void __attribute__((section(".itcm_write_flash"))) bl_hw_board_deinit(void);
#endif

// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __BK7236_2022_LAYOUT_H__
#define __BK7236_2022_LAYOUT_H__

/* ***************** README ******************
 *
 * The memory layout macros are in the format of:
 *      LAYOUT_[name_abbreviation]_[FLASH/ROM/RAM]_[OFFSET/SIZE/START]
 * For example:
 *      LAYOUT_PRIM_BL_FLASH_OFFSET is the primary bootloader offset in spi flash
 * Here the size is the maximum available space.
 * 
 * This is the Asymmetric mode layout, for using Symmetric mode layout, please reference to
 * bk7236_layout_symmetric.h
 */

#define SZ_2K (0x800)
#define SZ_4K (0x1000)
#define SZ_8K (0x2000)
#define SZ_16K (0x4000)
#define SZ_32K (0x8000)
#define SZ_64K (0x10000)
#define SZ_128K (0x20000)
#define SZ_256K (0x40000)
#define SZ_384K (0x60000)
#define SZ_16M  (0x01000000)


/*
 * Defines the SoC memory map
 */

#define LAYOUT_SPI_FLASH_START      (0x02000000)

#if (CONFIG_FPGA)
#define LAYOUT_ROM_S_BASE           (0x02000000)
#else
#define LAYOUT_ROM_S_BASE           (0x06000000)
#endif

#define LAYOUT_RAM_NS_BASE          (0x38000000)
#define LAYOUT_RAM_S_BASE           (0x28010000)
#define LAYOUT_TOTAL_RAM_SIZE       (SZ_256K)


/*
 * Defines the SPI Flash internal layout
 */

/* control partition part 1 - read by instruction port*/
#if (CONFIG_FPGA)
//FPGA has the BootROM in flash, offset is 0, size is 128K
#define LAYOUT_CTRL_PARTITION_RESERVED_FLASH_OFFSET    (SZ_128K)
#else
#define LAYOUT_CTRL_PARTITION_RESERVED_FLASH_OFFSET    (0x0)
#endif
#define LAYOUT_CTRL_PARTITION_RESERVED_FLASH_SIZE      (SZ_4K) 


/* control partition part 2 - read by data port */
#define LAYOUT_CTRL_PARTITION_FLASH_OFFSET \
	(LAYOUT_CTRL_PARTITION_RESERVED_FLASH_OFFSET + LAYOUT_CTRL_PARTITION_RESERVED_FLASH_SIZE)
#define LAYOUT_CTRL_PARTITION_FLASH_SIZE      (SZ_4K) 

/* the simulated OTP */
#define LAYOUT_SIMU_OTP_FLASH_OFFSET        \
	(LAYOUT_CTRL_PARTITION_FLASH_OFFSET + LAYOUT_CTRL_PARTITION_FLASH_SIZE)
#define LAYOUT_SIMU_OTP_FLASH_SIZE          (SZ_4K)

/* primary manifest */
#define LAYOUT_PRIM_MANIFEST_FLASH_OFFSET \
        (LAYOUT_SIMU_OTP_FLASH_OFFSET + LAYOUT_SIMU_OTP_FLASH_SIZE)
#define LAYOUT_PRIM_MANIFEST_FLASH_SIZE     (SZ_4K)

/* recovery manifest */
#define LAYOUT_RECV_MANIFEST_FLASH_OFFSET \
        (LAYOUT_PRIM_MANIFEST_FLASH_OFFSET + LAYOUT_PRIM_MANIFEST_FLASH_SIZE)
#define LAYOUT_RECV_MANIFEST_FLASH_SIZE     (SZ_4K)

/* cache partition */
#define LAYOUT_CACHE_PARTITION_FLASH_OFFSET (0x00480000)
#define LAYOUT_CACHE_PARTITION_FLASH_SIZE   (SZ_256K)


/*
 * Defines the ROM internal layout
 */
/* BOOTROM */
#define LAYOUT_BOOTROM_ROM_START            (LAYOUT_ROM_S_BASE + 0x001F0000)
//#define LAYOUT_BOOTROM_ROM_START            (LAYOUT_ROM_S_BASE + 0x00000000)

#if (CONFIG_REDUCE_CODE_SIZE)
#define LAYOUT_BOOTROM_ROM_SIZE             (SZ_64K)
#else
#define LAYOUT_BOOTROM_ROM_SIZE             (SZ_128K)
#endif

/*
 * Defines the RAM internal layout
 */
/* BOOTROM */
#define LAYOUT_BOOTROM_RAM_START            (LAYOUT_RAM_S_BASE)
#define LAYOUT_BOOTROM_RAM_SIZE             (LAYOUT_TOTAL_RAM_SIZE)

#define LAYOUT_JUMP_BIN_OFFSET_OFFSET       0xb4
#define LAYOUT_JUMP_BIN_WIDTH_OFFSET        (LAYOUT_JUMP_BIN_OFFSET_OFFSET + 4)

#endif /* __BK7236_2022_LAYOUT_H__ */

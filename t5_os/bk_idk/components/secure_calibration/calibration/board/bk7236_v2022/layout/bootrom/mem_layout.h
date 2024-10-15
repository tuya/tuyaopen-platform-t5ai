/*
 * Copyright (C), 2018-2019, Arm Technology (China) Co., Ltd.
 * All rights reserved
 *
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 * License Agreement between Licensee and Arm Technology (China) Co., Ltd
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and,,
 * any derivative work shall retain this copyright notice.
 */

#ifndef __MEM_LAYOUT_H__
#define __MEM_LAYOUT_H__

#include "bk7236_layout.h"

#define CONFIG_ROM_ADDR             (LAYOUT_BOOTROM_ROM_START)
#define CONFIG_ROM_SIZE             (LAYOUT_BOOTROM_ROM_SIZE)

#define CONFIG_RAM_ADDR             (LAYOUT_BOOTROM_RAM_START)
#define CONFIG_RAM_SIZE             (LAYOUT_BOOTROM_RAM_SIZE)

#define CONFIG_BOOTROM_ADDR         (LAYOUT_BOOTROM_ROM_START)
#define CONFIG_BOOTLOADER_ADDR      (LAYOUT_PRIM_BL_ROM_START)

#define NEXT_IMAGE_ADDR             (LAYOUT_PRIM_BL_ROM_START)

#endif /* __MEM_LAYOUT_H__ */

// Copyright 2022-2023 Beken
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

#pragma once

#include "partitions_gen.h"

#define KB(size)                                      ((size) << 10)
#define MB(size)                                      ((size) << 20)


#define ADDR_PHY_TO_VIRTUAL(addr) ((addr) / 34 * 32)

#ifdef CONFIG_BOOTLOADER_PARTITION_OFFSET
#define CONFIG_BOOTLOADER_PHY_PARTITION_OFFSET      CONFIG_BOOTLOADER_PARTITION_OFFSET
#define CONFIG_BOOTLOADER_PHY_PARTITION_SIZE        CONFIG_BOOTLOADER_PARTITION_SIZE
#define CONFIG_BOOTLOADER_VIRTUAL_PARTITION_OFFSET  ADDR_PHY_TO_VIRTUAL(CONFIG_BOOTLOADER_PARTITION_OFFSET)
#define CONFIG_BOOTLOADER_VIRTUAL_PARTITION_SIZE    ADDR_PHY_TO_VIRTUAL(CONFIG_BOOTLOADER_PARTITION_SIZE)
#endif

#ifdef CONFIG_APPLICATION_PARTITION_OFFSET
#define CONFIG_CP_PHY_PARTITION_OFFSET              CONFIG_APPLICATION_PARTITION_OFFSET
#define CONFIG_CP_PHY_PARTITION_SIZE                CONFIG_APPLICATION_PARTITION_SIZE
#define CONFIG_CP_VIRTUAL_PARTITION_OFFSET          ADDR_PHY_TO_VIRTUAL(CONFIG_APPLICATION_PARTITION_OFFSET)
#define CONFIG_CP_VIRTUAL_PARTITION_SIZE            ADDR_PHY_TO_VIRTUAL(CONFIG_APPLICATION_PARTITION_SIZE)
#endif

#ifdef CONFIG_APPLICATION1_PARTITION_OFFSET
#define CONFIG_AP_PHY_PARTITION_OFFSET              CONFIG_APPLICATION1_PARTITION_OFFSET
#define CONFIG_AP_PHY_PARTITION_SIZE                CONFIG_APPLICATION1_PARTITION_SIZE
#define CONFIG_AP_VIRTUAL_PARTITION_OFFSET          ADDR_PHY_TO_VIRTUAL(CONFIG_APPLICATION1_PARTITION_OFFSET)
#define CONFIG_AP_VIRTUAL_PARTITION_SIZE            ADDR_PHY_TO_VIRTUAL(CONFIG_APPLICATION1_PARTITION_SIZE)
#endif


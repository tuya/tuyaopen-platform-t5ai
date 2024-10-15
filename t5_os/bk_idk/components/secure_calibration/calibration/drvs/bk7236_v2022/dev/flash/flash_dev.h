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

#ifndef __FLASH_DEV_H__
#define __FLASH_DEV_H__

#include <stdint.h>
#include "device.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FLASH_RW_MAX_DATA_SIZE (256)
#define FLASH_SECTOR_SIZE (0x1000)
#define FLASH_ERASE_BLOCK32_SIZE (0x8000)
#define FLASH_ERASE_BLOCK_SIZE (0x10000)
#define FLASH_MAX_SIZE (0x1000000)

struct flash_dev_data {
    size_t max_flash_size;
    union {
        struct spi_flash_config {
            uint32_t spi_freq;
            uint32_t spi_slave_num;
            uint32_t spi_dev_id_value;
        } spi_flash_config;
    };
};

struct flash_driver_api {
    int32_t (*read)(struct device *dev, uint32_t offset, uint8_t *data, size_t len);
    int32_t (*write)(struct device *dev, uint32_t offset, const uint8_t *data, size_t len);
    int32_t (*erase)(struct device *dev, uint32_t offset, size_t len);
    int32_t (*write_protection_set)(struct device *dev, bool enable);
};

int32_t flash_init(struct device *dev);
int32_t flash_read(struct device *dev, uint32_t offset, uint8_t *data, size_t len);
int32_t flash_write(struct device *dev, uint32_t offset, const uint8_t *data, size_t len);
int32_t flash_erase(struct device *dev, uint32_t offset, size_t size);
int32_t flash_write_protection_set(struct device *dev, bool enable);

#ifdef __cplusplus
extern "C" {
#endif

#endif /* __FLASH_DEV_H__ */

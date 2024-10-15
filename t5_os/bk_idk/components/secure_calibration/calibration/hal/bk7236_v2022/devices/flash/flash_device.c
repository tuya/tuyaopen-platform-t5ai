// Copyright 2021-2022 Beken
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

#include "base.h"
#include "platform.h"
#include "flash_dev.h"

#define DRV_NAME "spi_flash_drv"

extern const struct flash_driver_api flash_api_funcs;
extern int32_t flash_init(struct device *dev);

//TODO fix me, change to armino way, remove spi related 
static struct flash_dev_data _flash_dev_data = {
    .max_flash_size                    = 16 * 1024 * 1024,
    .spi_flash_config.spi_freq         = 12000000,
    .spi_flash_config.spi_slave_num    = 0,
    .spi_flash_config.spi_dev_id_value = 0x72362022,
};

DEVICE_DEFINE(spi_flash,
              DRV_NAME,
              &flash_init,
              &_flash_dev_data,
              NULL,
              DEVICE_LEVEL1,
              1,
              &flash_api_funcs);

struct device *spi_flash_device = DEVICE_GET(spi_flash);

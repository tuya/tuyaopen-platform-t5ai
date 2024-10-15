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

#include <stdint.h>
#include <stddef.h>
#include "base.h"
#include "err.h"
#include "device.h"
#include "flash_dev.h"
#include "bk_flash.h"

#define DBG_PRE_PROMPT "flash_dev"

#include "dbg.h"

#include "drv_dep.h"

int32_t flash_init(struct device *dev)
{
    bk_flash_driver_init();
    return 0;
}

static int32_t _flash_read(struct device *dev, uint32_t offset, uint8_t *data,
                           size_t len)
{
    int32_t ret = NO_ERROR;

    DBG("Flash Read offset: %d size: %d\n", offset, len);

    ret = bk_flash_read_bytes(offset, data, len);
    DRV_CHECK_RET("flash_drv_read[read] failed: 0x%x \n", ret);

finish:
    return ret;
}

static int32_t _flash_write(struct device *dev, uint32_t offset,
                            const uint8_t *data, size_t len)
{
    (void)dev;
    return bk_flash_write_bytes(offset, data,len);
}

static int32_t _flash_erase(struct device *dev, uint32_t offset, size_t len)
{
    uint32_t count = len;
    uint32_t current_addr;
    (void)dev;

    current_addr = offset;	
    while(count >= FLASH_SECTOR_SIZE) {
        bk_flash_erase_sector(current_addr);
        count -= FLASH_SECTOR_SIZE;
        current_addr += FLASH_SECTOR_SIZE;
    }

    return 0;
}

static int32_t _flash_write_protection_set(struct device *dev, bool enable)
{
    (void)dev;
	
   if(enable){
        bk_flash_set_protect_type(FLASH_PROTECT_ALL);
    }else{
        bk_flash_set_protect_type(FLASH_PROTECT_NONE);
    }

    return 0;
}

const struct flash_driver_api flash_api_funcs = {
    .read                 = _flash_read,
    .write                = _flash_write,
    .erase                = _flash_erase,
    .write_protection_set = _flash_write_protection_set,
};

int32_t flash_read(struct device *dev, uint32_t offset, uint8_t *data,
                   size_t len)
{
    const struct flash_driver_api *drv_api = dev->driver_api;

    if (NULL == drv_api) {
        return ERR_NOT_FOUND;
    }
    return drv_api->read(dev, offset, data, len);
}

int32_t flash_write(struct device *dev, uint32_t offset, const uint8_t *data,
                    size_t len)
{
    const struct flash_driver_api *drv_api = dev->driver_api;

    if (NULL == drv_api) {
        return ERR_NOT_FOUND;
    }
    return drv_api->write(dev, offset, data, len);
}

int32_t flash_erase(struct device *dev, uint32_t offset, size_t size)
{
    const struct flash_driver_api *drv_api = dev->driver_api;

    if (NULL == drv_api) {
        return ERR_NOT_FOUND;
    }
    return drv_api->erase(dev, offset, size);
}

int32_t flash_write_protection_set(struct device *dev, bool enable)
{
    const struct flash_driver_api *drv_api = dev->driver_api;

    if (NULL == drv_api) {
        return ERR_NOT_FOUND;
    }
    return drv_api->write_protection_set(dev, enable);
}


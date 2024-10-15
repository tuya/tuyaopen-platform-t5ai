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

#ifndef __FLASH_H__
#define __FLASH_H__

#include <stdint.h>
#include "device.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t flash_init(struct device *dev);
int32_t flash_read(struct device *dev, uint32_t offset, uint8_t *data, size_t len);
int32_t flash_write(struct device *dev, uint32_t offset, const uint8_t *data, size_t len);
int32_t flash_erase(struct device *dev, uint32_t offset, size_t size);
int32_t flash_get_sector_size(struct device *dev, size_t *sector_size);
int32_t flash_get_max_size(struct device *dev, size_t *max_flash_size);

#ifdef __cplusplus
extern "C" {
#endif

#endif /* __FLASH_H__ */

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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "spi_types.h"

#if (defined CONFIG_SPI_MST_FLASH)
/**
 * @brief     Init the spi interface as master device for flash with default parameters.
 *            I.E:baud rate, 4-wires...
 *
 * @attention 1. This API should be called before any other spi flash APIs.
 *
 * @param id: which SPI interface(From chip side) is used for flash.
 * 
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_SPI_NOT_INIT: SPI driver not init
 *    - others: other errors.
 */
bk_err_t bk_spi_flash_init(spi_id_t id);

/**
 * @brief     Deinit the SPI flash
 *
 * @param id: spi device id to be accessed
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_SPI_NOT_INIT: SPI driver not init
 *    - others: other errors.
 */
bk_err_t bk_spi_flash_deinit(spi_id_t id);

/**
 * @brief     Read the flash id.
 *
 * @attention 1. This API should be called after bk_spi_flash_init.
 *
 * @param id: which SPI interface(From chip side) is used for flash.
 *
 * @return
 *    - 0: read id failed.
 *    - != 0: flash id.
 */
uint32_t bk_spi_flash_read_id(spi_id_t id);

/**
 * @brief      SPI flash erase sector
 *
 * @param id: SPI device id to be accessed
 * @param base_addr: SPI write address, which is byte alignment
 * @param size: the size of SPI erase operation, which need to be set within the flash capacity.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_spi_flash_erase(spi_id_t id, uint32_t base_addr, uint32_t size);

/**
 * @brief      SPI flash write data
 *
 * @param id: SPI device id to be accessed
 * @param base_addr: SPI write address, which is byte alignment
 * @param data: SPI write data buffer
 * @param size: the size of SPI write data, which need to be set within the flash capacity.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_spi_flash_write(spi_id_t id, uint32_t base_addr, const void *data, uint32_t size);

/**
 * @brief      SPI flash read data
 *
 * @param id: SPI device id to be accessed
 * @param base_addr: SPI read address, which is byte alignment 
 * @param dst_data: SPI read data buffer
 * @param size: the size of SPI data to be read, which need to be set within the flash capacity.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_spi_flash_read(spi_id_t id, uint32_t base_addr, uint8_t *dst_data, uint32_t size);

#endif    //(defined CONFIG_SPI_MST_FLASH)

#ifdef __cplusplus
}
#endif


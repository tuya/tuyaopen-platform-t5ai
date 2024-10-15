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

#ifndef __SPI_H__
#define __SPI_H__

#include <stdint.h>
#include "device.h"
#include "spi/spi_pl022/spi_drv.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t spi_init(struct device *dev);
int32_t spi_transceive(struct device *dev,
                       struct spi_config *config,
                       struct spi_buf_set *tx_bufs,
                       struct spi_buf_set *rx_bufs);
int32_t spi_read(struct device *dev,
                 struct spi_config *config,
                 struct spi_buf_set *rx_bufs);
int32_t spi_write(struct device *dev,
                  struct spi_config *config,
                  struct spi_buf_set *tx_bufs);

#ifdef __cplusplus
}
#endif
#endif /* __SPI_H__ */

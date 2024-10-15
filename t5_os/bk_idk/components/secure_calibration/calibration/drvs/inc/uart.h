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

#ifndef __UART_H__
#define __UART_H__

#include <stdint.h>
#include "device.h"

#ifndef __ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

int32_t uart_init(struct device *dev);
void uart_putc(struct device *dev, uint8_t c);
int32_t uart_getc(struct device *dev, uint8_t *c);
int32_t uart_rx_ready(struct device *dev);

#ifdef __cplusplus
}
#endif
#endif /* __ASSEMBLY__ */

#endif /* __UART_H__ */

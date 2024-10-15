/**
 * Copyright (C), 2018-2018, Arm Technology (China) Co., Ltd.
 * All rights reserved
 * 
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 * License Agreement between Licensee and Arm Technology (China) Co., Ltd
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and,,
 * any derivative work shall retain this copyright notice.
 */
#ifndef __LOG_IO_H__
#define __LOG_IO_H__

#include <stdint.h>

typedef void (*putc_t)(uint8_t c);
extern void log_to_mem(void);
extern void log_to_uart(void);
extern void print_and_clean_mem_log(void);

#endif /* __LOG_IO_H__ */

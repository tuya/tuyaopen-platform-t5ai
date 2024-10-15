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

#ifndef __PRNG_H__
#define __PRNG_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void srand(unsigned seed);
void rand_set_seed(uint32_t seed);
void rand_reset(uint32_t seed);
uint32_t rand32(void);
void rand_bytes(uint8_t *data, uint32_t len);
uint32_t get_system_rand_limit(uint32_t min, uint32_t max);
uint32_t get_system_rand(void);

#ifdef __cplusplus
}
#endif

#endif /* __PRNG_H__ */

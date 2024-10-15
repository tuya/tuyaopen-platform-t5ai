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

#include <stdint.h>
#include "platform.h"

static uint32_t randseed = 12345;

void rand_set_seed(uint32_t seed)
{
    randseed = seed;
}

void __wrap_srand(int32_t seed)
{
    rand_set_seed((uint32_t)seed);
}

void rand_reset(uint32_t seed)
{
    randseed ^= seed;
}

uint32_t rand32(void)
{
    return (uint32_t)(randseed = randseed * 1664525UL + 1013904223UL);
}

int32_t __wrap_rand()
{
    return (int32_t)rand32();
}

void rand_bytes(uint8_t *data, uint32_t len)
{
    uint32_t i;
    uint32_t rand_u32 = 0;

    for (i = 0; i < (len / 4); i++) {
        rand_u32    = platform_get_rand_seed();
        data[i + 0] = ((rand_u32 >> 0) & 0xff);
        data[i + 1] = ((rand_u32 >> 8) & 0xff);
        data[i + 2] = ((rand_u32 >> 16) & 0xff);
        data[i + 3] = ((rand_u32 >> 24) & 0xff);
    }

    for (i = len / 4; i < len; i++) {
        data[i] = ((platform_get_rand_seed()) & 0xff);
    }
    return;
}

uint32_t get_system_rand(void)
{
    return platform_get_rand_seed();
}

uint32_t get_system_rand_limit(uint32_t min, uint32_t max)
{
    uint32_t ret;
    do {
        ret = get_system_rand();
        if (ret >= max) {
            ret = ret % max;
        }

        if (ret <= min) {
            ret += min;
        }

    } while ((ret <= min) || (ret >= max));
    return ret;
}

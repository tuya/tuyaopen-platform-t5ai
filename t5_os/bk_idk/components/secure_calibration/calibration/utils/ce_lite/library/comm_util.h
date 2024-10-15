/*
 * Copyright (c) 2021, Arm Technology (China) Co., Ltd.
 * All rights reserved.
 *
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 * License Agreement between Licensee and Arm Technology (China) Co., Ltd
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and,,
 * any derivative work shall retain this copyright notice.
 */
#ifndef COMM_UTIL_H
#define COMM_UTIL_H

#include <stdint.h>
#include <stddef.h>

/* round "x" up/down to next multiple of "align" (which must be a power of 2) */
#define ROUND_UP(x, a) (((x) + ((a)-1)) & ~((a)-1))
#define ROUND_DOWN(x, a) ((x) & ~((a)-1))

/**
 *  BS_XOR - Apply XOR operation on two byte streams and save the result
 *           to another byte stream. a xor b -> r.
 *  @r:      Buffer to hold the result.
 *  @a:      Buffer to hold the byte stream input#1.
 *  @b:      Buffer to hold the byte stream input#2.
 *  @s:      Length of byte stream to feed into XOR operation.
 */
#define BS_XOR(r, a, b, s)                          \
    do {                                            \
        uint8_t *ptr_r = (uint8_t *)(r);            \
        uint8_t *ptr_a = (uint8_t *)(a);            \
        uint8_t *ptr_b = (uint8_t *)(b);            \
        size_t _i_ = (s);                           \
        while (_i_--) {                             \
            *ptr_r++ = (*ptr_a++) ^ (*ptr_b++);     \
        }                                           \
    } while(0)

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 *  COUNTER_INCREASE - Update IV value scroll up.
 *  @iv:    IV
 *  @bs:    Block size.
 *  @w:     Counter size
 *  @delta: Scroll up value.
 */
#define COUNTER_INCREASE(iv, bs, w, delta)                         \
    do {                                                           \
        size_t _i = 0, _j = 0;                                     \
        for (_i = 0; _i < (delta); _i++) {                         \
            for(_j = (bs); _j > ((bs) - (w)); _j--) {              \
                if (0 != ++((iv)[_j - 1])) {                       \
                    break;                                         \
                }                                                  \
            }                                                      \
        }                                                          \
    } while(0)

#define CTR_SIZE (16U)   /*ctr size is 128bits, thus 16bytes*/

#endif  /* COMM_UTIL_H */

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

#include "hal.h"
#include "pal.h"
#include "prng.h"
#include "hal_src_internal.h"
#include "ce_lite_trng.h"

hal_ret_t hal_gen_random_data(uint8_t *data, size_t data_size)
{
    if (!data) {
        PAL_LOG_ERR("Parameter data is NULL!\n");
        return HAL_ERR_BAD_PARAM;
    }

    (void)ce_trng_read(data, data_size);

    return HAL_OK;
}
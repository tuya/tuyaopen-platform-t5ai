/**
 * Copyright (C), 2018-2021, Arm Technology (China) Co., Ltd.
 * All rights reserved
 *
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 * License Agreement between Licensee and Arm Technology (China) Co., Ltd
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and,,
 * any derivative work shall retain this copyright notice.
 */

#include "hal_platform.h"
#include "pal_time.h"

void pal_msleep( uint32_t msecs )
{
    hal_mdelay(msecs);
}

void pal_udelay( uint32_t usecs )
{
    hal_udelay(usecs);
}


// Copyright (C), 2018-2019, Arm Technology (China) Co., Ltd.
// All rights reserved
//
// The content of this file or document is CONFIDENTIAL and PROPRIETARY
// to Arm Technology (China) Co., Ltd. It is subject to the terms of a
// License Agreement between Licensee and Arm Technology (China) Co., Ltd
// restricting among other things, the use, reproduction, distribution
// and transfer.  Each of the embodiments, including this information and,,
// any derivative work shall retain this copyright notice.

#include "hal_irq.h"

void hal_irq_enable(unsigned int key)
{
    (void)key;
    __asm("nop");
}

unsigned int hal_irq_disable(void)
{
    __asm("nop");
    return 0;
}

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

#include "bk_arch.h"
#include "armstar.h"
#include "fpb.h"

void fpb_enable_breakpoint_unit(void)
{
    FPB->FP_CTRL |= FPB_CTRL_KEY | FPB_CTRL_ENABLE;
}

void fpb_disable_breakpoint_unit(void)
{
    uint32_t val;

    val = FPB->FP_CTRL;
    val &= (~FPB_CTRL_ENABLE);
    val |= FPB_CTRL_KEY;
    FPB->FP_CTRL = val;
}

uint32_t fpb_set_program_breakpoint(uint32_t program_addr)
{
    uint32_t ret = 1;
    uint32_t i, val;

    for(i = 0; i < FP_COMPARATOR_CNT; i ++){
        if(0 == (FPB->FP_COMP[i] & FPB_COMP_ENABLE)){
            program_addr &= (~(1 << 0));
            val = program_addr | FPB_COMP_ENABLE;
            FPB->FP_COMP[i] = val;
            fpb_enable_breakpoint_unit();

            ret = 0;
            break;
        }
    }

    return ret;
}

// eof


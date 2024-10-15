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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "components/log.h"
#include "os/mem.h"
#include "os/str.h"
#include "timer_driver.h"

#include <driver/uart.h>
#include "time_consumption.h"

#if TIME_CONSUMPTION_MEAS

#if CONFIG_ARCH_CM33
#include <soc/soc.h>

#define TIMER0_REG_SET(reg_id, l, h, v) REG_SET((SOC_TIMER0_REG_BASE + ((reg_id) << 2)), (l), (h), (v))
#define TIMER0_PERIOD 0xFFFFFFFF

time_consumption_t g_time_consumption_array[TIME_CONSUMPTION_ARRAY_SIZE] = {0};
time_consumption_idx_stack_t g_time_consumption_idx_stack;
int8_t g_time_consumption_save_idx = 0;

uint32_t timer_hal_get_timer0_cnt(void)
{
    TIMER0_REG_SET(8, 2, 3, 0);
    TIMER0_REG_SET(8, 0, 0, 1);
    while (REG_READ((SOC_TIMER0_REG_BASE + (8 << 2))) & BIT(0));

    return REG_READ(SOC_TIMER0_REG_BASE + (9 << 2));
}

uint32_t get_duration_clk_cycles(uint32_t start, uint32_t end)
{
    return start < end?(end-start):(TIMER0_PERIOD - (start-end) + 1);
}

void time_consumption_init(void)
{
    uint32_t i;

    g_time_consumption_save_idx = 0;

    for(i = 0; i < TIME_CONSUMPTION_ARRAY_SIZE; i++)
    {
        g_time_consumption_array[i].valid = 0;
    }

    os_memset(&g_time_consumption_idx_stack,0,sizeof(g_time_consumption_idx_stack));
}

void time_consumption_idx_push(int8_t idx)
{
    g_time_consumption_idx_stack.stack[g_time_consumption_idx_stack.wr_idx] = idx;
    g_time_consumption_idx_stack.wr_idx = ((g_time_consumption_idx_stack.wr_idx+1)&(TIME_CONSUPTION_CALLING_DEPTH - 1));
}

int8_t time_consumption_idx_pop(void)
{
    g_time_consumption_idx_stack.wr_idx -= 1;  
    if(0 > g_time_consumption_idx_stack.wr_idx)
    {
        TIME_MEAS_LOGE("%s:pop before push!\n",
                          __func__);
    }
    return g_time_consumption_idx_stack.stack[g_time_consumption_idx_stack.wr_idx];
}

void time_consumption_print(void)
{
    uint32_t i;
    uint32_t duration;
    
    TIME_MEAS_LOGI("%s begin\n",__func__);

    for(i = 0; i < TIME_CONSUMPTION_ARRAY_SIZE; i++)
    {
        if(g_time_consumption_array[i].valid)
        {
            duration = get_duration_clk_cycles(g_time_consumption_array[i].start,
                                               g_time_consumption_array[i].end);
            TIME_MEAS_LOGI("idx:%d,N:%s,S:0x%x,E:0x%x,dur:%d, %dus\n",
                                  i,
                                  g_time_consumption_array[i].str,
                                  g_time_consumption_array[i].start,
                                  g_time_consumption_array[i].end,
                                  duration,
                                  duration/26);
            bk_timer_delay_us(10000);
        }
    }
    TIME_MEAS_LOGI("%s end\n",__func__);
}
#endif//CONFIG_ARCH_CM33

#endif//TIME_CONSUMPTION_MEAS


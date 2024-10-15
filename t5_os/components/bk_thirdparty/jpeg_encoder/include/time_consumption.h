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


#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if TIME_CONSUMPTION_MEAS

#if CONFIG_ARCH_CM33

#define TIME_CONSUPTION_CALLING_DEPTH (1<<4)
#define TIME_CONSUMPTION_ARRAY_SIZE (1<<8)

typedef struct
{
  uint32_t start;
  uint32_t end;
  uint8_t  valid;
  char str[50];
}time_consumption_t;
                                      
typedef struct
{
  int8_t wr_idx;
  int8_t stack[TIME_CONSUPTION_CALLING_DEPTH];
}time_consumption_idx_stack_t;

extern time_consumption_t g_time_consumption_array[TIME_CONSUMPTION_ARRAY_SIZE];
extern time_consumption_idx_stack_t g_time_consumption_idx_stack;
extern int8_t g_time_consumption_save_idx;

#define TIME_CONSUMPTOIN_INIT() time_consumption_init();
#define TIME_CONSUMPTOIN_START(name) {g_time_consumption_array[g_time_consumption_save_idx].valid = 1;\
                                      os_strcpy((char *)g_time_consumption_array[g_time_consumption_save_idx].str, name);\
                                      time_consumption_idx_push(g_time_consumption_save_idx);\
                                      g_time_consumption_array[g_time_consumption_save_idx].start = timer_hal_get_timer0_cnt();\
                                      g_time_consumption_save_idx = ((g_time_consumption_save_idx+1)&(TIME_CONSUMPTION_ARRAY_SIZE-1));}
#define TIME_CONSUMPTOIN_END() g_time_consumption_array[time_consumption_idx_pop()].end = timer_hal_get_timer0_cnt();
    
#else/*CONFIG_ARCH_CM33*/
#define TIME_CONSUMPTOIN_INIT() 
#define TIME_CONSUMPTOIN_START(name)
#define TIME_CONSUMPTOIN_END()
#endif

#else
#define TIME_CONSUMPTOIN_INIT() 
#define TIME_CONSUMPTOIN_START(name)
#define TIME_CONSUMPTOIN_END()
#endif

uint32_t timer_hal_get_timer0_cnt(void);
uint32_t get_duration_clk_cycles(uint32_t start, uint32_t end);
void time_consumption_init(void);
void time_consumption_idx_push(int8_t idx);
int8_t time_consumption_idx_pop(void);
void time_consumption_print(void);

#ifdef __cplusplus
}
#endif

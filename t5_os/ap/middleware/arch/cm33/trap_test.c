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

#include "cli.h"
#include <components/system.h>
#include <os/str.h>
#include "bk_sys_ctrl.h"
#include "bk_drv_model.h"
#include "release.h"
#include <driver/efuse.h>
#include <components/system.h>
#include <modules/wifi.h>
#include "sys_driver.h"
#include <driver/wdt.h>
#include <bk_wdt.h>
#include "gpio_driver.h"
#include <driver/gpio.h>
#include "bk_ps.h"
#include "bk_pm_internal_api.h"
#include "reset_reason.h"
#include <modules/pm.h>
#include "sdkconfig.h"
#include <driver/pwr_clk.h>
#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif
#include <components/ate.h>
#include "dwt.h"

static void cli_trap_test_help(void)
{
	CLI_LOGD("trap_test [data][read|write][invalid|align|zero|nonsecure|mpuprotect|null]\r\n");
	CLI_LOGD("trap_test [instruction][zero|invalid_addr|invalid_instruction|nonsecure]\r\n");
	CLI_LOGD("trap_test [memory][func_stack_overwrite|task_stack_overwrite|hw_stack_overwrite|global_overwrite|heap_overwrite|heap_free_malloc_overwrite]\r\n");
}

static uint32_t s_trap_test_data_read = 0;
static uint32_t s_trap_test_data_read_addr = 0;
static uint32_t s_trap_test_data_write = 0;
static uint32_t s_trap_test_data_write_addr = 0;
typedef void (*trap_test_instruction_ptr)(void);
static trap_test_instruction_ptr s_trap_test_instruction_ptr = cli_trap_test_help;
void (*test_instruction_fun)(void);

#define TRAP_TEST_FUNC_STACK_OVERFLOW_SIZE (16 * 1)		//please set it as: 16 * x
void func_stack_test(char *array_1, char *array_2, uint32_t len)
{
	typedef struct {
		char array[TRAP_TEST_FUNC_STACK_OVERFLOW_SIZE];
		char *ptr;
		uint32_t len;
	}array_test;
	array_test array;

	CLI_LOGD("array_addr=0x%x array_1=0x%x array_2=0x%x len=%d\r\n", &array, array_1, array_2, len);

	for(uint32_t i = 0; i < TRAP_TEST_FUNC_STACK_OVERFLOW_SIZE; i+=16)
	{
		CLI_LOGD("array_1:0x%x 0x%x 0x%x 0x%x\r\n", *(uint32_t *)&array_1[i], *(uint32_t *)&array_1[i+4], *(uint32_t *)&array_1[i+8], *(uint32_t *)&array_1[i+12]);
		CLI_LOGD("array_2:0x%x 0x%x 0x%x 0x%x\r\n", *(uint32_t *)&array_2[i], *(uint32_t *)&array_2[i+4], *(uint32_t *)&array_2[i+8], *(uint32_t *)&array_2[i+12]);
	}

	array.ptr = &array.array[0];
	memset(array.ptr, 0x32, len);

	CLI_LOGD("array.array=%s\r\n", array.array);
#if 0
	for(uint32_t i = 0; i < TRAP_TEST_FUNC_STACK_OVERFLOW_SIZE; i+=16)
	{
		CLI_LOGD("array_1:0x%x 0x%x 0x%x 0x%x\r\n", *(uint32_t *)&array_1[i], *(uint32_t *)&array_1[i+4], *(uint32_t *)&array_1[i+8], *(uint32_t *)&array_1[i+12]);
		CLI_LOGD("array_2:0x%x 0x%x 0x%x 0x%x\r\n", *(uint32_t *)&array_2[i], *(uint32_t *)&array_2[i+4], *(uint32_t *)&array_2[i+8], *(uint32_t *)&array_2[i+12]);
	}
#endif
}

void trap_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		goto bk_err_help;
	}

	if (os_strcmp(argv[1], "data") == 0) {
		if(argc < 3)
			goto bk_err_help;
		
		if(os_strcmp(argv[2], "read") == 0) {
			if(argc < 4)
				goto bk_err_help;
			if(os_strcmp(argv[3], "invalid") == 0) {
				s_trap_test_data_read_addr = 0xC0000000;
				CLI_LOGD("data read invalid addr=0x%x\r\n", s_trap_test_data_read_addr);
				s_trap_test_data_read = *(uint32_t *)s_trap_test_data_read_addr;
			}
			else if(os_strcmp(argv[3], "align") == 0) {
				s_trap_test_data_read_addr = 0x2809FFFE;
				CLI_LOGD("data read align addr=0x%x\r\n", s_trap_test_data_read_addr);
				s_trap_test_data_read = *(uint32_t *)s_trap_test_data_read_addr;
			}
			else if(os_strcmp(argv[3], "zero") == 0) {
				s_trap_test_data_read_addr = 0;
				CLI_LOGD("data read zero addr=0x%x\r\n", s_trap_test_data_read_addr);
				s_trap_test_data_read = *(uint32_t *)s_trap_test_data_read_addr;
			}
			else if(os_strcmp(argv[3], "nonsecure") == 0) {
				s_trap_test_data_read_addr = 0;
				CLI_LOGD("data read nonsecure addr=0x%x\r\n", s_trap_test_data_read_addr);
				s_trap_test_data_read = *(uint32_t *)s_trap_test_data_read_addr;
			}
			else if(os_strcmp(argv[3], "mpuprotect") == 0) {
				s_trap_test_data_read_addr = 0;
				CLI_LOGD("data read mpuprotect addr=0x%x\r\n", s_trap_test_data_read_addr);
				s_trap_test_data_read = *(uint32_t *)s_trap_test_data_read_addr;
			}

			CLI_LOGD("data read addr=0x%x,value=0x%x\r\n", s_trap_test_data_read_addr, s_trap_test_data_read);
		}
		else if(os_strcmp(argv[2], "write") == 0) {
			if(argc < 4)
				goto bk_err_help;
			if(os_strcmp(argv[3], "invalid") == 0) {
				s_trap_test_data_write_addr = 0xC0000000;
				CLI_LOGD("data write invalid addr=0x%x\r\n", s_trap_test_data_write_addr);
				*(uint32_t *)s_trap_test_data_write_addr = 0xC0000000;
			}
			else if(os_strcmp(argv[3], "align") == 0) {
				s_trap_test_data_write_addr = 0x2809FFFE;
				CLI_LOGD("data write align addr=0x%x\r\n", s_trap_test_data_write_addr);
				*(uint32_t *)s_trap_test_data_write_addr = 0x2809FFFE;
			}
			else if(os_strcmp(argv[3], "zero") == 0) {
				s_trap_test_data_write_addr = 0;
				CLI_LOGD("data write zero addr=0x%x\r\n", s_trap_test_data_write_addr);
				*(uint32_t *)s_trap_test_data_write_addr = 0;
			}
			else if(os_strcmp(argv[3], "nonsecure") == 0) {
				s_trap_test_data_write_addr = 0;
				CLI_LOGD("data write nonsecure addr=0x%x\r\n", s_trap_test_data_write_addr);
				*(uint32_t *)s_trap_test_data_write_addr = 0;
			}
			else if(os_strcmp(argv[3], "mpuprotect") == 0) {
				s_trap_test_data_write_addr = 0;
				CLI_LOGD("data write mpuprotect addr=0x%x\r\n", s_trap_test_data_write_addr);
				*(uint32_t *)s_trap_test_data_write_addr = 0;
			}

			CLI_LOGD("data write addr=0x%x,value=0x%x\r\n", s_trap_test_data_write_addr, s_trap_test_data_write);
		}
	} else if (os_strcmp(argv[1], "instruction") == 0) {
		if(argc < 3)
			goto bk_err_help;

        if(os_strcmp(argv[3], "zero") == 0) {
            test_instruction_fun = (void (*)())0x00000000;
            CLI_LOGD("instruction zero \r\n");
            test_instruction_fun();

		} else if (os_strcmp(argv[3], "invalid_addr") == 0) {
            test_instruction_fun = (void (*)())0xffffff00;
            CLI_LOGD("instruction invalid_addr \r\n");
            test_instruction_fun();

        } else if (os_strcmp(argv[3], "invalid_instruction") == 0) {
            CLI_LOGD("instruction invalid_instruction \r\n");
            __asm__ volatile(
                "UDF #0     \n"
            );
        } else if (os_strcmp(argv[3], "nonsecure") == 0) {
            test_instruction_fun = (void (*)())0x2035000 ;
            CLI_LOGD("instruction nonsecure \r\n");
            test_instruction_fun();
        }
	} else if (os_strcmp(argv[1], "memory") == 0) {
			if(argc < 3)
				goto bk_err_help;
			//[func_stack|task_stack|hw_stack|global_overwrite|heap_overwrite|heap_free_malloc_overwrite]
			if(os_strcmp(argv[2], "func_stack_overwrite") == 0) {
				char array_1[TRAP_TEST_FUNC_STACK_OVERFLOW_SIZE];
				char array_2[TRAP_TEST_FUNC_STACK_OVERFLOW_SIZE];
				memset(array_1, 0, sizeof(array_1));
				memset(array_2, 0, sizeof(array_2));

				extern int bk_rand(void);
				func_stack_test(array_1, array_2, (uint32_t)(bk_rand()*99)%(TRAP_TEST_FUNC_STACK_OVERFLOW_SIZE*8));

				//the stack variable value is over-writed by func_stack_test
				if((array_1[0] != 0) || (array_2[0] != 0))
					BK_ASSERT(0);
			}
			else if(os_strcmp(argv[2], "task_stack_overwrite") == 0) {
				extern uint32_t vTaskStackSize();
				extern uint32_t * vTaskStackAddr();
				//uint32_t stack_size = vTaskStackSize() * sizeof( StackType_t );

				uint32_t *p_task_stack = vTaskStackAddr();	//stack start addr
#if 1	//( portSTACK_GROWTH < 0 )	//TODO:portSTACK_GROWTH isn't valid in this file.
				strcpy((char *)(p_task_stack), "task_stack_overwrite");
#else
				strcpy((char *)(p_task_stack+vTaskStackSize()-1), "task_stack_overwrite");
#endif
			}
			else if(os_strcmp(argv[2], "hw_stack_overwrite") == 0) {

				rtos_delay_milliseconds(1000);	//wait log out

				{                    
                    CLI_LOGD("psplimit val =0x%x\r\n", __get_PSPLIM());
                    CLI_LOGD("SP val =0x%x\r\n", __get_SP());
                    CLI_LOGD("PSP val =0x%x\r\n", __get_PSP());

                    uint32_t sp = __get_PSPLIM();
                    /* set the stack pointer value less than the psplimit value */
                    sp = sp - 0x4;
                    __set_PSP(sp);
                    __asm__ volatile("bx lr");
				}

				CLI_LOGD("hw_stack test pass\r\n");
			}
			else if(os_strcmp(argv[2], "global_overwrite") == 0) {
				s_trap_test_data_read = 0x5A5A5A5A;
				s_trap_test_data_read_addr = (uint32_t)&s_trap_test_data_read;
				*(uint32_t *)s_trap_test_data_read_addr = 0xCDCDCDCD;	//valid init value

				dwt_set_data_address_write((uint32_t)&s_trap_test_data_read);

				//valid set value
				dwt_disable_debug_monitor_mode();
				*(uint32_t *)s_trap_test_data_read_addr = 0xAAAABBBB;	//valid set value

				//invlaid set value
				dwt_set_data_address_write((uint32_t)&s_trap_test_data_read);
				*(uint32_t *)s_trap_test_data_read_addr = 0x88888888;	//invalid set value, monitor will crash
			}
			else if(os_strcmp(argv[2], "heap_overwrite") == 0) {
				s_trap_test_data_write_addr = (uint32_t)os_malloc(4);
				//*(uint32_t *)(s_trap_test_data_write_addr + 4) = 0x88888888;
				os_memset((uint32_t *)s_trap_test_data_write_addr, 0x88, 16);
				os_free((void *)s_trap_test_data_write_addr);
			}
			else if(os_strcmp(argv[2], "heap_free_malloc_overwrite") == 0) {
				s_trap_test_data_write_addr = (uint32_t)os_malloc(128);
				os_free((void *)s_trap_test_data_write_addr);
				char *tmp_p = os_malloc(128);
				os_memset(tmp_p, 0, 128);
				strcpy(tmp_p, "heap_free_malloc_overwrite");
				os_memset((uint32_t *)s_trap_test_data_write_addr, 0x31, 128);
				if(strcmp(tmp_p, "heap_free_malloc_overwrite") != 0)
					BK_ASSERT(0);
			}
		}else {
		s_trap_test_instruction_ptr();
		return;
	}

bk_err_help:
	cli_trap_test_help();
}

#define TRAP_TEST_CMD_CNT (sizeof(s_trap_test_commands) / sizeof(struct cli_command))
static const struct cli_command s_trap_test_commands[] = {
	{"trap_test", NULL, trap_test},
};

int cli_trap_test_init(void)
{
	return cli_register_commands(s_trap_test_commands, TRAP_TEST_CMD_CNT);
}


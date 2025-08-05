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

#include <components/log.h>
#include <components/system.h>


#ifdef __cplusplus
extern "C" {
#endif

#define BK_ASSERT_TAG "ASSERT"

void bk_reboot(void);

#if (CONFIG_DEBUG_VERSION || CONFIG_DUMP_ENABLE)
void bk_system_dump(const char *, const int);
#define BK_ASSERT_DUMP(func, line)   bk_system_dump(func, line)
#else
#define BK_ASSERT_DUMP(func, line)
#endif

#if CONFIG_STDIO_PRINTF
#define _OS_PRINTF printf
#else
#define _OS_PRINTF os_printf
#endif

#if CONFIG_SHELL_ASYNCLOG

#define BK_DUMP_OUT(format, ... )           shell_assert_out(true, format, ##__VA_ARGS__)
#define BK_DUMP_RAW_OUT(buf, len)           shell_assert_raw(true, buf, len)

#else // #if CONFIG_SHELL_ASYNCLOG

#define BK_DUMP_OUT                         _OS_PRINTF
#define BK_DUMP_RAW_OUT(buf, len)           do { (len); uart_write_string(bk_get_printf_port(), buf); } while (0)

#endif // #if CONFIG_SHELL_ASYNCLOG

__attribute__((__used__)) static volatile  uint32_t g_ap_dump_flag = 0;


#if (CONFIG_SWD_DEBUG_MODE)

#define BK_ASSERT(exp) do{                                   \
	if( !(exp) ){                                            \
		BK_DUMP_OUT("(%d)Assert at: %s:%d\r\n", rtos_get_time(), __FUNCTION__, __LINE__);    \
		rtos_disable_int();                                  \
		while(1); }                                          \
}while(0)

#define BK_ASSERT_EX(exp, format, ... )do{                   \
	if( !(exp) ){                                            \
		BK_DUMP_OUT(format, ##__VA_ARGS__);                  \
		BK_DUMP_OUT("(%d)Assert at: %s:%d\r\n", rtos_get_time(), __FUNCTION__, __LINE__);    \
		rtos_disable_int();                                  \
		while(1); }                                          \
}while(0)

#elif (CONFIG_DEBUG_VERSION || CONFIG_DUMP_ENABLE)

#define BK_ASSERT(exp)                                       \
do {                                                         \
    if ( !(exp) && !g_ap_dump_flag) {                        \
        rtos_disable_int();                                  \
        BK_LOG_FLUSH();                                      \
        BK_ASSERT_DUMP(__FUNCTION__, __LINE__);              \
    }                                                        \
} while (0)

#define BK_ASSERT_EX(exp, format, ... )                      \
do {                                                         \
    if ( !(exp) && !g_ap_dump_flag) {                        \
        rtos_disable_int();                                  \
        BK_LOG_FLUSH();                                      \
        BK_DUMP_OUT(format, ##__VA_ARGS__);                  \
        BK_ASSERT_DUMP(__FUNCTION__, __LINE__);              \
    }                                                        \
} while (0)

#elif (CONFIG_ASSERT_REBOOT)

#define BK_ASSERT(exp)                                       \
do {                                                         \
    if ( !(exp) ) {                                          \
        rtos_disable_int();                                  \
        BK_LOG_FLUSH();                                      \
        BK_DUMP_OUT("(%d)Assert at: %s:%d\r\n", rtos_get_time(), __FUNCTION__, __LINE__);    \
        bk_reboot_ex(RESET_SOURCE_CRASH_ASSERT);                                         \
    }                                                        \
} while (0)

#define BK_ASSERT_EX(exp, format, ... )                      \
do {                                                         \
    if ( !(exp) ) {                                          \
        rtos_disable_int();                                  \
        BK_LOG_FLUSH();                                      \
        BK_DUMP_OUT(format, ##__VA_ARGS__);                  \
        BK_DUMP_OUT("(%d)Assert at: %s:%d\r\n", rtos_get_time(), __FUNCTION__, __LINE__);    \
        bk_reboot_ex(RESET_SOURCE_CRASH_ASSERT);                                          \
    }                                                        \
} while (0)
#else  //(CONFIG_ASSERT_IGNORE)

#define BK_ASSERT(exp) do{                                   \
	if( !(exp) ){                                            \
		}                                                    \
}while(0)

#define BK_ASSERT_EX(exp, format, ... )do{                   \
	if( !(exp) ){                                            \
		}                                                    \
}while(0)


#endif // #if (CONFIG_DEBUG_VERSION || CONFIG_DUMP_ENABLE)

#define BK_ASSERT_HALT			BK_DUMP_OUT


#ifdef __cplusplus

}

#endif

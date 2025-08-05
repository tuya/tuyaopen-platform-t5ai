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

#include "os/os.h"
#include "bk_usb_adaptor.h"
#include <components/log.h>
#include "driver/int.h"
#include "sys_driver.h"
#include "gpio_driver.h"
#include <driver/int_types.h>
#include <driver/hal/hal_int_types.h>
#include <modules/bk_musb.h>
#include <components/system.h>
#include "arch_interrupt.h"
#include <driver/gpio.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>
#include "sys_driver.h"



#define USB_OSI_VERSION              0x00000001

static bk_err_t usb_int_isr_register_wrapper(uint8_t type, void* isr, void*arg)
{
    icu_int_src_t src = INT_SRC_USB;

    if (type == USB_INT_SRC)
    {
        src = INT_SRC_USB;
    }
    else if (type == USB_INT_SRC_PLUG_INOUT)
    {
        src = INT_SRC_USB_PLUG_INOUT;
    }
    else
    {
        //error
    }

    return bk_int_isr_register(src, (int_group_isr_t)isr, arg);
}

static uint32_t disable_int_wrapper(void)
{
    return rtos_enter_critical();
}

static void enable_int_wrapper(uint32_t int_level)
{
    rtos_exit_critical(int_level);
}

static bk_err_t init_queue_wrapper(void** queue, const char* name, uint32_t message_size, uint32_t number_of_messages )
{
    return rtos_init_queue(queue, name, message_size, number_of_messages);
}

static bk_err_t deinit_queue_wrapper( void** queue )
{
    return rtos_deinit_queue(queue);
}

static bk_err_t pop_from_queue_wrapper( void** queue, void* message, uint32_t timeout_ms )
{
    return rtos_pop_from_queue(queue, message, timeout_ms);
}

static bk_err_t push_to_queue_wrapper( void** queue, void* message, uint32_t timeout_ms )
{
    return rtos_push_to_queue(queue, message, timeout_ms);
}

static bk_err_t create_thread_wrapper( void** thread, uint8_t priority, const char* name,
                        void* function, uint32_t stack_size, void* arg )
{
    return rtos_create_thread(thread, priority, name, function, stack_size, arg);
}

static bk_err_t delete_thread_wrapper( void** thread )
{
    return rtos_delete_thread(thread);
}

static bk_err_t delay_milliseconds_wrapper( uint32_t num_ms )
{
    return rtos_delay_milliseconds(num_ms);
}

static uint32_t rtos_get_time_wrapper(void)
{
    return rtos_get_time();
}

static void *malloc_wrapper(size_t size)
{
    return os_malloc(size);
}

static void *realloc_wrapper(void *ptr,size_t size)
{
    return os_realloc(ptr, size);
}

static void free_wrapper(void *ptr)
{
    os_free(ptr);
}

static void memcpy_wrapper(void *out, void *in, uint32_t size)
{
    os_memcpy(out, in, size);
}

static void memset_wrapper(void *b, int c, uint32_t len)
{
    os_memset(b, c, len);
}

static bool is_timer_init_wrapper(void *timer )
{
    return rtos_is_timer_init(timer);
}

static int32_t init_timer_wrapper(void *timer, uint32_t time_ms, void* function, void* arg)
{
	return rtos_init_timer(timer, time_ms, (timer_handler_t)function, arg);
}

static int32_t start_timer_wrapper(void *timer)
{
	return rtos_start_timer(timer);
}

static bool is_timer_running_wrapper(void *timer)
{
	return rtos_is_timer_running(timer);
}

static int32_t stop_timer_wrapper(void *timer )
{
	return rtos_stop_timer(timer);
}

static int32_t deinit_timer_wrapper(void *timer )
{
	return rtos_deinit_timer(timer);
}

static int32_t timer_change_period(void *timer, uint32_t time_ms)
{
	return rtos_change_period(timer, time_ms);
}

static int32_t start_oneshot_timer_wrapper(void *timer)
{
	return rtos_start_oneshot_timer(timer);
}

static int32_t stop_oneshot_timer_wrapper(void *timer)
{
	return rtos_stop_oneshot_timer(timer);
}

static int32_t deinit_oneshot_timer_wrapper(void *timer)
{
	return rtos_deinit_oneshot_timer(timer);
}

static bool is_oneshot_timer_init_wrapper(void *timer)
{
	return rtos_is_oneshot_timer_init(timer);
}

static bool is_oneshot_timer_running_wrapper(void *timer)
{
	return rtos_is_oneshot_timer_running(timer);
}

static int32_t oneshot_reload_timer_wrapper(void *timer)
{
	return rtos_oneshot_reload_timer(timer);
}

static int32_t oneshot_reload_timer_ex_wrapper(void *timer, uint32_t time_ms, void *function, void *larg, void *rarg)
{
	return rtos_oneshot_reload_timer_ex(timer, time_ms, (timer_2handler_t)function, larg, rarg);
}

static int32_t init_oneshot_timer_wrapper(void *timer, uint32_t time_ms, void* function, void *larg, void *rarg)
{
	return rtos_init_oneshot_timer(timer, time_ms, (timer_2handler_t)function, larg, rarg);
}


static int32_t init_mutex(void **mutex)
{
    return rtos_init_mutex((beken_mutex_t *)mutex);
}

static int32_t lock_mutex(void ** mutex)
{
    return rtos_lock_mutex((beken_mutex_t *)mutex);
}

static int32_t unlock_mutex(void **mutex)
{
    return rtos_unlock_mutex((beken_mutex_t *)mutex);
}

static int32_t deinit_mutex(void **mutex)
{
    return rtos_deinit_mutex((beken_mutex_t *)mutex);
}

static int32_t init_semaphore(void **semaphore, int32_t max_count)
{
    return rtos_init_semaphore((beken_semaphore_t *)semaphore, 1);
}

static int32_t set_semaphore(void **semaphore)
{
    return rtos_set_semaphore((beken_semaphore_t *)semaphore);
}

static int32_t get_semaphore(void **semaphore, uint32_t timeout_ms)
{
    return rtos_get_semaphore((beken_semaphore_t *)semaphore, timeout_ms);
}

static int32_t deinit_semaphore(void **semaphore)
{
    return rtos_deinit_semaphore((beken_semaphore_t *)semaphore);
}

static bk_err_t gpio_dev_unmap_wrapper(uint32_t gpio_id)
{
    return gpio_dev_unmap(gpio_id);
}

static bk_err_t gpio_disable_pull_wrapper(uint32_t gpio_id )
{
    return bk_gpio_disable_pull(gpio_id);
}

static bk_err_t gpio_enable_output_wrapper(uint32_t gpio_id)
{
    return bk_gpio_enable_output(gpio_id);
}

static bk_err_t gpio_set_output_high_wrapper(uint32_t gpio_id)
{
    return bk_gpio_set_output_high(gpio_id);
}

static bk_err_t gpio_set_output_low_wrapper(uint32_t gpio_id)
{
    return bk_gpio_set_output_low(gpio_id);
}

static bk_err_t gpio_pull_down_wrapper(uint32_t gpio_id)
{
    return bk_gpio_pull_down(gpio_id);
}

static bk_err_t gpio_pull_up_wrapper(uint32_t gpio_id)
{
    return bk_gpio_pull_up(gpio_id);
}


static const struct usb_osi_funcs_t usb_osi_funcs = {
    ._version = USB_OSI_VERSION,
    .size = sizeof(struct usb_osi_funcs_t),
    .usb_base_addr = SOC_USB_REG_BASE, 
    /* int */
    ._usb_int_isr_register = usb_int_isr_register_wrapper,
    ._disable_int = disable_int_wrapper,
    ._enable_int = enable_int_wrapper,
    /* task */
    ._init_queue = init_queue_wrapper,
    ._deinit_queue = deinit_queue_wrapper,
    ._pop_from_queue = pop_from_queue_wrapper,
    ._push_to_queue = push_to_queue_wrapper,
    ._create_thread = create_thread_wrapper,
    ._delete_thread = delete_thread_wrapper,
    ._delay_milliseconds = delay_milliseconds_wrapper,
    ._rtos_get_time = rtos_get_time_wrapper,
    /* mem */
    ._malloc = malloc_wrapper,
    ._realloc = realloc_wrapper,
    ._free = free_wrapper,
    ._memcpy = memcpy_wrapper,
	._memset = memset_wrapper,
    /* timer */
    ._init_timer = init_timer_wrapper,
    ._deinit_timer = deinit_timer_wrapper,
    ._stop_timer = stop_timer_wrapper,
    ._timer_change_period = timer_change_period,
    ._is_timer_init = is_timer_init_wrapper,
    ._start_timer = start_timer_wrapper,
    ._is_timer_running = is_timer_running_wrapper,
	._init_oneshot_timer = init_oneshot_timer_wrapper,
	._deinit_oneshot_timer = deinit_oneshot_timer_wrapper,
	._start_oneshot_timer = start_oneshot_timer_wrapper,
	._stop_oneshot_timer = stop_oneshot_timer_wrapper,
	._is_oneshot_timer_init = is_oneshot_timer_init_wrapper,
	._is_oneshot_timer_running = is_oneshot_timer_running_wrapper,
	._oneshot_reload_timer = oneshot_reload_timer_wrapper,
	._oneshot_reload_timer_ex = oneshot_reload_timer_ex_wrapper,
    /* mutex */
    ._init_mutex = init_mutex,
    ._lock_mutex = lock_mutex,
    ._unlock_mutex = unlock_mutex,
    ._deinit_mutex = deinit_mutex,
    /* semaphore */
    ._init_semaphore = init_semaphore,
    ._set_semaphore = set_semaphore,
    ._get_semaphore = get_semaphore,
    ._deinit_semaphore = deinit_semaphore,
    /* log */
    ._log = bk_printf_ext,
    /* gpio for debug */
    ._gpio_dev_unmap = gpio_dev_unmap_wrapper,
    ._gpio_disable_pull = gpio_disable_pull_wrapper,
    ._gpio_enable_output = gpio_enable_output_wrapper,
    ._gpio_set_output_high = gpio_set_output_high_wrapper,
    ._gpio_set_output_low = gpio_set_output_low_wrapper,
    ._gpio_pull_down = gpio_pull_down_wrapper,
    ._gpio_pull_up = gpio_pull_up_wrapper,
};

bk_err_t bk_usb_adaptor_init(void)
{
    bk_err_t ret = BK_OK;

    if (usb_osi_funcs_init((void*)&usb_osi_funcs) != 0)
    {
        return BK_FAIL;
    }

    return ret;
}


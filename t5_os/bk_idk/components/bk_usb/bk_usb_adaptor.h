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
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

enum
{
    USB_INT_SRC = 0x00,
    USB_INT_SRC_PLUG_INOUT  = 0x01,
};

/* adapter function */
struct usb_osi_funcs_t {
    uint32_t _version;
    uint32_t size;
    uint32_t usb_base_addr;//USB_BASE_ADDR
    /* int */
    int (*_usb_int_isr_register)(uint8_t type, void* isr, void* arg);
    uint32_t (*_disable_int)(void);
    void (*_enable_int)(uint32_t int_level);
    /* task */
    int (*_init_queue)(void** queue, const char* name, uint32_t message_size, uint32_t number_of_messages );
    int (*_deinit_queue)( void** queue );
    int (*_pop_from_queue)( void** queue, void* message, uint32_t timeout_ms );
    int (*_push_to_queue)( void** queue, void* message, uint32_t timeout_ms );
    int (*_create_thread)( void** thread, uint8_t priority, const char* name,
                            void* function, uint32_t stack_size, void* arg );
    int (*_delete_thread)( void** thread );
    int (*_delay_milliseconds)( uint32_t num_ms );
    uint32_t (*_rtos_get_time)(void);
    /* mem */
    void *(*_malloc)(unsigned int size);
    void *(*_realloc) (void *p, unsigned int size);
    void (*_free)(void *p);
    void (*_memcpy)(void *out, void *in, uint32_t size);
    void (*_memset)(void *b, int c, uint32_t len);
    /* timer */
    int32_t (*_init_timer)( void *timer, uint32_t time_ms, void *function, void* arg);
    int32_t (*_deinit_timer)( void *timer);
    int32_t (*_stop_timer)( void *timer);
    int32_t (*_timer_change_period)( void *timer, uint32_t time_ms);
    bool (*_is_timer_init)(void *timer );
    int32_t (*_start_timer)(void *timer);
    bool (*_is_timer_running)(void *timer);
    int32_t (*_init_oneshot_timer)(void *timer, uint32_t time_ms, void* function, void *larg, void *rarg);
    int32_t (*_deinit_oneshot_timer)(void *timer);
    int32_t (*_start_oneshot_timer)(void *timer);
    int32_t (*_stop_oneshot_timer)(void *timer);
    bool (*_is_oneshot_timer_init)(void *timer);
    bool (*_is_oneshot_timer_running)(void *timer);
    int32_t (*_oneshot_reload_timer)(void *timer);
    int32_t (*_oneshot_reload_timer_ex)(void *timer, uint32_t time_ms, void* function, void *larg, void *rarg);
    /* mutex */
    int32_t (*_init_mutex)(void **mutex);
    int32_t (*_lock_mutex)(void ** mutex);
    int32_t (*_unlock_mutex)(void **mutex);
    int32_t (*_deinit_mutex)(void **mutex);
    /* semaphore */
    int32_t (*_init_semaphore)(void **semaphore, int32_t max_count);
    int32_t (*_set_semaphore)(void **semaphore);
    int32_t (*_get_semaphore)(void **semaphore, uint32_t timeout_ms);
    int32_t (*_deinit_semaphore)(void **semaphore);
    /* log */
    void (*_log)(int level, char *tag, const char *fmt, ...);
    /* gpio for debug */
    int (*_gpio_dev_unmap)(uint32_t gpio_id);
    int (*_gpio_disable_pull)(uint32_t gpio_id );
    int (*_gpio_enable_output)(uint32_t gpio_id);
    int (*_gpio_set_output_high)(uint32_t gpio_id);
    int (*_gpio_set_output_low)(uint32_t gpio_id);
    int (*_gpio_pull_down)(uint32_t gpio_id);
    int (*_gpio_pull_up)(uint32_t gpio_id);

};


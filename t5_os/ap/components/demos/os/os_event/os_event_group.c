/**
 ******************************************************************************
 * @file    os_event_group.c
 * @author
 * @version V1.0.0
 * @date
 * @brief   RTOS event group demo.
 ******************************************************************************
 *
 *  The MIT License
 *  Copyright (c) 2017 Beken Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is furnished
 *  to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 *  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ******************************************************************************
 */

#include <common/bk_include.h>

#if CONFIG_OS_EVENT_DEMO

#include <os/os.h>
#include "os_event_group.h"
#include <driver/timer.h>
#include <components/log.h>

#define TAG "os_event"

#define EVENT_BIT_0  ( 1 << 0 )
#define EVENT_BIT_3  ( 1 << 3 )
#define EVENT_BIT_4  ( 1 << 4 )
#define EVENT_BIT_8  ( 1 << 8 )
#define EVENT_BIT_15 ( 1 << 15 )
#define ALL_SYNC_BITS ( EVENT_BIT_4 | EVENT_BIT_8 | EVENT_BIT_15 )

static beken_thread_t t_handler_1 = NULL;
static beken_thread_t t_handler_2 = NULL;
static beken_thread_t t_handler_3 = NULL;

beken_event_t event_handler;


static timer_id_t timer_id = TIMER_ID0;
static uint32_t timer_ms = 4000;

static void timer0_examples_isr(timer_id_t timer_id)
{
    BK_LOGD(TAG, "timer0(%d) enter timer0_example_isr\r\n", timer_id);
    /* set event in interruption context */
    rtos_set_event_flags(&event_handler, EVENT_BIT_0);
}

static void thread1_function( beken_thread_arg_t arg )
{
    (void)( arg );
    uint32_t uxReturn;

    for(;;) {
        rtos_set_event_flags(&event_handler, EVENT_BIT_3);

        rtos_delay_milliseconds(2000);
        BK_LOGD(TAG, "thread1 set EVENT_BIT_3\r\n");

        /* send EVENT_BIT_4 and  wait a sync event */
        uxReturn = rtos_sync_event_flags(&event_handler, EVENT_BIT_4, ALL_SYNC_BITS, BEKEN_WAIT_FOREVER);
        if ((uxReturn & ALL_SYNC_BITS) == ALL_SYNC_BITS)
        {
            BK_LOGD(TAG, "thread1 wait sync event ok\r\n");
        }
    }
}

static void thread2_function( beken_thread_arg_t arg )
{
    (void)( arg );
    uint32_t wait_event;

    for (;;) {
        wait_event = rtos_wait_for_event_flags (&event_handler, EVENT_BIT_0 | EVENT_BIT_3, true, WAIT_FOR_ALL_EVENTS, BEKEN_WAIT_FOREVER);

        if( ( wait_event & ( EVENT_BIT_0 | EVENT_BIT_3 ) ) == ( EVENT_BIT_0 | EVENT_BIT_3 ) ) 
        {
            BK_LOGD(TAG, "thread2 wait event EVENT_BIT_0 & EVENT_BIT_3 ok!\r\n");
        }

        /* send EVENT_BIT_8 and  wait a sync event */
        wait_event = rtos_sync_event_flags(&event_handler, EVENT_BIT_8, ALL_SYNC_BITS, BEKEN_WAIT_FOREVER);
        if ((wait_event & ALL_SYNC_BITS) == ALL_SYNC_BITS)
        {
            BK_LOGD(TAG, "thread2 wait sync event ok\r\n");
        }

    }
}

static void thread3_function( beken_thread_arg_t arg )
{
    (void)( arg );
    uint32_t wait_event;

    for (;;) {
        wait_event = rtos_wait_for_event_flags (&event_handler, EVENT_BIT_0 | EVENT_BIT_3, false, WAIT_FOR_ANY_EVENT, BEKEN_WAIT_FOREVER);

        if( ( wait_event & ( EVENT_BIT_0 | EVENT_BIT_3 ) ) == ( EVENT_BIT_0 | EVENT_BIT_3 ) ) 
        {
            BK_LOGD(TAG, "thread3 wait event EVENT_BIT_0 & EVENT_BIT_3 ok!\r\n");
        }
        else if( ( wait_event &  EVENT_BIT_0 ) != 0) 
        {
            BK_LOGD(TAG, "thread3 wait event EVENT_BIT_0 ok!\r\n");
        }
        else if (( wait_event &  EVENT_BIT_3 ) != 0)
        {
            BK_LOGD(TAG, "thread3 wait event EVENT_BIT_3 ok!\r\n");
        }

        rtos_delay_milliseconds(1000);

        /* send EVENT_BIT_15 and  wait a sync event */
        wait_event = rtos_sync_event_flags(&event_handler, EVENT_BIT_15, ALL_SYNC_BITS, BEKEN_WAIT_FOREVER);
        if ((wait_event & ALL_SYNC_BITS) == ALL_SYNC_BITS)
        {
            BK_LOGD(TAG, "thread3 wait sync event ok\r\n");
        }
    }
}

void os_event_demo_start( void )
{
    bk_err_t err = kNoErr;

    BK_LOGD(TAG,"\r\n\r\noperating rtos event group demo............\r\n" );

    BK_LOG_ON_ERR(bk_timer_driver_init());
    BK_LOG_ON_ERR(bk_timer_start(timer_id, timer_ms, timer0_examples_isr));

    err = rtos_init_event_flags(&event_handler);
    if(err != kNoErr)
    {
        BK_LOGD(TAG,"ERROR: Unable to init event flag.\r\n" );
    }
#if CONFIG_FREERTOS_SMP
    err = rtos_core1_create_thread( &t_handler_1, 
                              BEKEN_APPLICATION_PRIORITY,
                              "Thread 1",
                              thread1_function,
                              0x1000,
                              0);
#else
    err = rtos_create_thread( &t_handler_1, 
                              BEKEN_APPLICATION_PRIORITY,
                              "Thread 1",
                              thread1_function,
                              0x1000,
                              0);
#endif
    if(err != kNoErr)
    {
        BK_LOGD(TAG,"ERROR: Unable to start the thread 1.\r\n" );
    }

    err = rtos_create_thread( &t_handler_2, 
                              BEKEN_APPLICATION_PRIORITY,
                              "Thread 2",
                              thread2_function,
                              0x1000,
                              0);
    if(err != kNoErr)
    {
        BK_LOGD(TAG,"ERROR: Unable to start the thread 2.\r\n" );
    }

    err = rtos_create_thread( &t_handler_3, 
                              BEKEN_APPLICATION_PRIORITY - 1,
                              "Thread 3",
                              thread3_function,
                              0x1000,
                              0);
    if(err != kNoErr)
    {
        BK_LOGD(TAG,"ERROR: Unable to start the thread 3.\r\n" );
    }
}

#endif
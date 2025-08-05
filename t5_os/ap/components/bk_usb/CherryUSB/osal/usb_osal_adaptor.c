/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "usb_osal.h"
#include <components/cherryusb/usb_errno.h>

//#include "semphr.h"
//#include "timers.h"
//#include "event_groups.h"

usb_osal_thread_t usb_osal_thread_create(const char *name, uint32_t stack_size, uint32_t prio, usb_thread_entry_t entry, void *args)
{
    beken_thread_t htask = NULL;
    rtos_create_thread(&htask, prio, name, (beken_thread_function_t)entry, stack_size, args);
    //xTaskCreate(entry, name, stack_size, args, prio, &htask);
    return (usb_osal_thread_t)htask;
}
int usb_osal_thread_delete(usb_osal_thread_t *thread)
{
    return rtos_delete_thread(thread);
}

usb_osal_sem_t usb_osal_sem_create(uint32_t initial_count)
{
    beken_semaphore_t semaphore = NULL;
    uint32_t ret = kNoErr;
    ret = rtos_init_semaphore(&semaphore, initial_count);
    if(ret != kNoErr) {
        return NULL;
    } else {
        return (usb_osal_sem_t)semaphore;
    }
    //return (usb_osal_sem_t)xSemaphoreCreateCounting(1, initial_count);
}

void usb_osal_sem_delete(usb_osal_sem_t sem)
{
    if(sem != NULL) {
        rtos_deinit_semaphore((beken_semaphore_t *)sem);
    }
    //vSemaphoreDelete((SemaphoreHandle_t)sem);
}

int usb_osal_sem_take(usb_osal_sem_t sem, uint32_t timeout)
{
    if(sem == NULL) return -EINVAL;
    return (rtos_get_semaphore((beken_semaphore_t *)sem, timeout) == BK_OK) ? 0 : -ETIMEDOUT;
    //return (xSemaphoreTake((SemaphoreHandle_t)sem, pdMS_TO_TICKS(timeout)) == pdPASS) ? 0 : -ETIMEDOUT;
}

int usb_osal_sem_give(usb_osal_sem_t sem)
{
    if(sem == NULL) return -EINVAL;
    return rtos_set_semaphore((beken_semaphore_t *)sem);
}

usb_osal_mutex_t usb_osal_mutex_create(void)
{
    beken_mutex_t mutex = NULL;
    rtos_init_mutex(&mutex);
    return (usb_osal_mutex_t)mutex;
    //return (usb_osal_mutex_t)xSemaphoreCreateMutex();
}

void usb_osal_mutex_delete(usb_osal_mutex_t mutex)
{
    rtos_deinit_mutex((beken_mutex_t *)mutex);
    //vSemaphoreDelete((SemaphoreHandle_t)mutex);
}

int usb_osal_mutex_take(usb_osal_mutex_t mutex)
{
    return rtos_lock_mutex((beken_mutex_t *)mutex);
    //return (xSemaphoreTake((SemaphoreHandle_t)mutex, portMAX_DELAY) == pdPASS) ? 0 : -ETIMEDOUT;
}

int usb_osal_mutex_give(usb_osal_mutex_t mutex)
{
    return rtos_unlock_mutex((beken_mutex_t *)mutex);
    //return (xSemaphoreGive((SemaphoreHandle_t)mutex) == pdPASS) ? 0 : -EINVAL;
}

size_t usb_osal_enter_critical_section(void)
{
    //rtos_enter_critical();
    return 1;
}

void usb_osal_leave_critical_section(size_t flag)
{
    //rtos_exit_critical();
}

void usb_osal_msleep(uint32_t delay)
{
    rtos_delay_milliseconds(delay);
}


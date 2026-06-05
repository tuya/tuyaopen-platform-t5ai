/**
 * @file tkl_system.c
 * @brief the default weak implements of tuya os system api, this implement only used when OS=linux
 * @version 0.1
 * @date 2019-08-15
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */

#include "tkl_system.h"
#include "FreeRTOS.h"
#include "task.h"
#include <os/os.h>
#include <components/system.h>
#include "reset_reason.h"
#include <driver/trng.h>
#include "tkl_memory.h"
#include <driver/otp.h>
#include "tkl_ipc.h"
#include "atomic.h"
#include "tkl_semaphore.h"
#include "sdkconfig.h"

extern void bk_printf(const char *fmt, ...);
static TKL_SEM_HANDLE get_cpu_info_sem = NULL;

#if CONFIG_CPU_INDEX == 0
static TaskHandle_t __gi_thread_handle = NULL;
static void __get_info_func(void *arg)
{
    struct ipc_msg_s *msg = (struct ipc_msg_s *)arg;
    struct ipc_msg_param_s *p = (struct ipc_msg_param_s *)msg->req_param;
    if (p != NULL)
        tkl_system_get_cpu_info((TUYA_CPU_INFO_T **)p->p1, (int32_t *)p->p2);

    __gi_thread_handle = NULL;
    vTaskDelete(__gi_thread_handle);
}
#endif

void tkl_sys_ipc_func(struct ipc_msg_s *msg)
{
    switch(msg->subtype) {
        case TKL_IPC_TYPE_SYS_REBOOT:
        {
            tkl_system_reset();
        }
            break;

#if CONFIG_CPU_INDEX == 0
        case TKL_IPC_TYPE_SYS_CPU_INFO:
        {
            bk_printf_raw(BK_LOG_INFO, NULL, "recv cpu info req\r\n");
            xTaskCreate(__get_info_func, "get_info", 1024, msg, 6, (TaskHandle_t * const )&__gi_thread_handle);
            msg->ret_value = 0;
            tuya_ipc_send_no_sync(msg);
        }
            break;
#endif

        case TKL_IPC_TYPE_SYS_CPU_INFO_RSP:
        {
            bk_printf("recv cpu info rsp, post sem\r\n");
            if(get_cpu_info_sem)
                tkl_semaphore_post(get_cpu_info_sem);

            msg->ret_value = 0;
            tuya_ipc_send_no_sync(msg);
        }
            break;

        default:
            break;
    }

    return;
}

/**
* @brief Get system ticket count
*
* @param void
*
* @note This API is used to get system ticket count.
*
* @return system ticket count
*/
SYS_TICK_T tkl_system_get_tick_count(void)
{
    return (SYS_TICK_T)xTaskGetTickCount();
}

/**
* @brief Get system millisecond
*
* @param none
*
* @return system millisecond
*/
SYS_TIME_T tkl_system_get_millisecond(void)
{
    return (SYS_TIME_T)(tkl_system_get_tick_count() * portTICK_RATE_MS);
}

/**
* @brief System sleep
*
* @param[in] msTime: time in MS
*
* @note This API is used for system sleep.
*
* @return void
*/
void tkl_system_sleep(const uint32_t num_ms)
{
    uint32_t ticks = num_ms / portTICK_RATE_MS;

    if (ticks == 0) {
        ticks = 1;
    }

    vTaskDelay(ticks);
}


void tkl_system_sleep_us(uint32_t num_us)
{
    // TODO
    // delay_us(num_us);
}

/**
* @brief System reset
*
* @param void
*
* @note This API is used for system reset.
*
* @return void
*/
void tkl_system_reset(void)
{
#if CONFIG_CPU_INDEX == 0
    bk_reboot();
#else
    bk_printf("ap request reset\r\n");
    struct ipc_msg_s msg = {0};
    msg.type = TKL_IPC_TYPE_SYS;
    msg.subtype = TKL_IPC_TYPE_SYS_REBOOT;

    tuya_ipc_send_no_sync(&msg);
    while(1) {
        tkl_system_sleep(100);
    }
#endif
    return;
}

/**
* @brief Get free heap size
*
* @param void
*
* @note This API is used for getting free heap size.
*
* @return size of free heap
*/
int32_t tkl_system_get_free_heap_size(void)
{
    return (int32_t)xPortGetFreeHeapSize();
}

int32_t tkl_system_get_minimum_heap_size(void)
{
    return (int32_t)xPortGetMinimumEverFreeHeapSize();
}


/**
* @brief Get system reset reason
*
* @param void
*
* @note This API is used for getting system reset reason.
*
* @return reset reason of system
*/
TUYA_RESET_REASON_E tkl_system_get_reset_reason(char** describe)
{
    unsigned char value = 0;

    uint32_t cp_reason = bk_misc_get_cp_reset_reason();
    uint32_t ap_reason = bk_misc_get_ap_reset_reason();

    if ((cp_reason == 0) && (ap_reason == 0)) {
        value = bk_misc_get_reset_reason() & 0xFF;
    } else if ((cp_reason != 0) && (ap_reason == 0)) {
        value = cp_reason;
    } else {
        value = ap_reason;
    }

    TUYA_RESET_REASON_E ty_value;

    switch (value) {
        case RESET_SOURCE_POWERON:
            ty_value = TUYA_RESET_REASON_POWERON;
            break;

        case RESET_SOURCE_REBOOT:
            ty_value = TUYA_RESET_REASON_SOFTWARE;
            break;

        case RESET_SOURCE_WATCHDOG:
        case RESET_SOURCE_NMI_WDT:
            ty_value = TUYA_RESET_REASON_HW_WDOG;
            break;

        case RESET_SOURCE_DEEPPS_GPIO:
        case RESET_SOURCE_DEEPPS_RTC:
        case RESET_SOURCE_DEEPPS_USB:
        case RESET_SOURCE_DEEPPS_TOUCH:
        case RESET_SOURCE_SUPER_DEEP:
            ty_value = TUYA_RESET_REASON_DEEPSLEEP;
            break;

        case RESET_SOURCE_CRASH_ILLEGAL_JUMP:
        case RESET_SOURCE_CRASH_UNDEFINED:
        case RESET_SOURCE_CRASH_PREFETCH_ABORT:
        case RESET_SOURCE_CRASH_DATA_ABORT:
        case RESET_SOURCE_CRASH_UNUSED:
        case RESET_SOURCE_CRASH_ILLEGAL_INSTRUCTION:
        case RESET_SOURCE_CRASH_MISALIGNED:
        case RESET_SOURCE_CRASH_ASSERT:
            ty_value = TUYA_RESET_REASON_CRASH;
            break;

        case RESET_SOURCE_HARD_FAULT:
        case RESET_SOURCE_MPU_FAULT:
        case RESET_SOURCE_BUS_FAULT:
        case RESET_SOURCE_USAGE_FAULT:
        case RESET_SOURCE_SECURE_FAULT:
        case RESET_SOURCE_DEFAULT_EXCEPTION:
            ty_value = TUYA_RESET_REASON_FAULT;
            break;

        default:
            // ty_value = TUYA_RESET_REASON_UNKNOWN;
            ty_value = TUYA_RESET_REASON_POWERON;
            break;
    }

    bk_printf("bk_value:%x / %x / %x, ty_value:%x\r\n", value, cp_reason, ap_reason, ty_value);
    return ty_value;

}

/**
* @brief Get a random number in the specified range
*
* @param[in] range: range
*
* @note This API is used for getting a random number in the specified range
*
* @return a random number in the specified range
*/
int32_t tkl_system_get_random(const uint32_t range)
{
    unsigned int trange = range;

    if (range == 0) {
        trange = 0xFF;
    }

    static char exec_flag = FALSE;

    if (!exec_flag) {
        exec_flag = TRUE;
    }

    return (bk_rand() % trange);
}

#define EFUSE_DEVICE_ID_BYTE_NUM 5
#define OTP_DEVICE_ID 29

OPERATE_RET tkl_system_get_cpu_info(TUYA_CPU_INFO_T **cpu_ary, int *cpu_cnt)
{
    // TODO
    struct ipc_msg_s msg = {0};

    memset(&msg, 0, sizeof(struct ipc_msg_s));

    msg.type = TKL_IPC_TYPE_SYS;

#if CONFIG_CPU_INDEX == 0
    TUYA_CPU_INFO_T *cpu = *cpu_ary;

    bk_otp_apb_read(OTP_DEVICE_ID, cpu->chipid, EFUSE_DEVICE_ID_BYTE_NUM);
    cpu->chipidlen = EFUSE_DEVICE_ID_BYTE_NUM;
    if (cpu_cnt) {
        *cpu_cnt = 1;
    }

    bk_printf_raw(BK_LOG_INFO, NULL, "send cpu info rsp, %p, 0x%02x%02x%02x%02x%02x\r\n",
            *cpu, cpu->chipid[0], cpu->chipid[1],
            cpu->chipid[2], cpu->chipid[3], cpu->chipid[4]);

    msg.subtype = TKL_IPC_TYPE_SYS_CPU_INFO_RSP;
    tuya_ipc_send_sync(&msg);
#else

    struct ipc_msg_param_s param = {0};
    msg.subtype = TKL_IPC_TYPE_SYS_CPU_INFO;

    TUYA_CPU_INFO_T *cpu = tkl_system_malloc(sizeof(TUYA_CPU_INFO_T));
    if (NULL == cpu) {
        bk_printf("get info malloc failed\r\n");
        return OPRT_MALLOC_FAILED;
    }

    // wait cp response
    OPERATE_RET ret = tkl_semaphore_create_init(&get_cpu_info_sem, 0, 1);
    if (ret !=  OPRT_OK) {
        bk_printf("create semaphore failed\r\n");
        tkl_system_free(cpu);
        cpu = NULL;
        return ret;
    }

    memset(cpu, 0, sizeof(TUYA_CPU_INFO_T));
    *cpu_ary = cpu;

    param.p1 = (void *)((uint32_t)cpu_ary);
    param.p2 = (void *)((uint32_t)cpu_cnt);

    msg.req_param = &param;
    msg.req_len = sizeof(param);

    bk_printf("send cpu info req\r\n");
    tuya_ipc_send_sync(&msg);

    bk_printf("wait cpu info\r\n");
    ret = tkl_semaphore_wait(get_cpu_info_sem, 5000);
    tkl_semaphore_release(get_cpu_info_sem);
    get_cpu_info_sem = NULL;
    if (ret !=  OPRT_OK) {
        bk_printf("wait cpu timeout\r\n");
        return ret;
    }


#endif

    return OPRT_OK;
}

/**
 * @brief system enter critical
 *
 * @return  irq status
 */
uint32_t tkl_system_enter_critical(void)
{
#if (CONFIG_FREERTOS_SMP)
    return rtos_enter_critical();
#else
    return rtos_disable_int();
#endif
}

/**
 * @brief system exit critical
 *
 * @param[in]   irq_mask: irq mask
 * @return  none
 */
void tkl_system_exit_critical(uint32_t irq_mask)
{
#if (CONFIG_FREERTOS_SMP)
    rtos_exit_critical(irq_mask);
#else
    rtos_enable_int(irq_mask);
#endif
}

#if (CONFIG_FREERTOS_SMP)
/**
 * @brief system exit critical
 *
 * @return  core ID
 */
uint32_t tkl_system_get_coreid(void)
{
    return rtos_get_core_id();
}
#endif

void tkl_system_task_info_dump(void)
{
    rtos_dump_task_list();
    rtos_dump_task_runtime_stats();
}


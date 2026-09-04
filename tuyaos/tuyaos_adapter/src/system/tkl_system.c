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
	bk_reboot();
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
    if (cpu_ary == NULL) {
        return OPRT_INVALID_PARM;
    }

    *cpu_ary = NULL;
    if (cpu_cnt) {
        *cpu_cnt = 0;
    }

    TUYA_CPU_INFO_T *cpu = (TUYA_CPU_INFO_T *)tkl_system_malloc(sizeof(TUYA_CPU_INFO_T));
    if (cpu == NULL) {
        return OPRT_MALLOC_FAILED;
    }

    memset(cpu, 0, sizeof(TUYA_CPU_INFO_T));

    bk_otp_apb_read(OTP_DEVICE_ID, cpu->chipid, EFUSE_DEVICE_ID_BYTE_NUM);
    cpu->chipidlen = EFUSE_DEVICE_ID_BYTE_NUM;

    if (cpu_cnt) {
        *cpu_cnt = 1;
    }

    *cpu_ary = cpu;

    return OPRT_OK;
}

/**
 * @brief get hardware unique id
 *
 * @param[in,out] id   buffer provided by caller; filled with the unique id on success
 * @param[in,out] len  in: buffer capacity; out: actual unique id length
 * @return OPRT_OK on success; OPRT_INVALID_PARM / OPRT_COM_ERROR / OPRT_NOT_SUPPORTED on error
 *
 * @note source: BK7258 OTP DEVICE_ID (factory-programmed, per-device unique). This core
 *       (CPU1) cannot access OTP directly, so the chipid is obtained via
 *       tkl_system_get_cpu_info(), which IPCs to the OTP-owning core (CPU0).
 *       Satisfies the platform contract: per-device unique / immutable across reset,
 *       power-off, OTA and factory-reset / read-only / not all-0x00 or all-0xFF.
 */
OPERATE_RET tkl_system_get_hw_unique_id(uint8_t *id, uint8_t *len)
{
    if ((id == NULL) || (len == NULL)) {
        return OPRT_INVALID_PARM;
    }

    /* Reuse the chipid getter: on CPU1 it IPCs to CPU0 to read OTP DEVICE_ID.
     * Do not call bk_otp_apb_read() here -- that symbol is not linked on CPU1. */
    TUYA_CPU_INFO_T *cpu = NULL;
    int cnt = 0;
    OPERATE_RET ret = tkl_system_get_cpu_info(&cpu, &cnt);
    if ((ret != OPRT_OK) || (cpu == NULL)) {
        return OPRT_COM_ERROR;
    }

    uint8_t clen = cpu->chipidlen;
    if ((clen < 1) || (clen > 32)) {
        tkl_system_free(cpu);
        return OPRT_COM_ERROR;
    }
    os_memcpy(id, cpu->chipid, clen);
    tkl_system_free(cpu);

    /* Contract forbids all-0x00 / all-0xFF; blank/unprogrammed OTP defaults to 0x00 */
    BOOL_T all_zero = TRUE, all_ff = TRUE;
    for (uint8_t i = 0; i < clen; i++) {
        if (id[i] != 0x00) { all_zero = FALSE; }
        if (id[i] != 0xFF) { all_ff  = FALSE; }
    }
    if (all_zero || all_ff) {
        *len = 0;
        return OPRT_NOT_SUPPORTED;
    }

    *len = clen;
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


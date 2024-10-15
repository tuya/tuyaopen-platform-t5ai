/*
 * tkl_wakeup.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "tkl_wakeup.h"
#include "sdkconfig.h"
#include <modules/pm.h>
#include <driver/gpio.h>
#if CONFIG_AON_RTC
#include <driver/aon_rtc_types.h>
#include <driver/hal/hal_aon_rtc_types.h>
#include <driver/aon_rtc.h>
#endif

#define WAKEUP_SOURCE_CNT   6
static TUYA_WAKEUP_SOURCE_BASE_CFG_T  *wakeup_source = NULL;
static uint32_t wakeup_source_init_status = 0;

static int __save_wakeup_node(const TUYA_WAKEUP_SOURCE_BASE_CFG_T  *param)
{
    int i = 0;

    if (wakeup_source == NULL) {
        bk_printf("wakeup source not init, %d\r\n", __LINE__);
        return -1;
    }

    for (i = 0; i < WAKEUP_SOURCE_CNT; i++) {
        if ((param->source == TUYA_WAKEUP_SOURCE_TIMER) &&
                (wakeup_source[i].source == TUYA_WAKEUP_SOURCE_TIMER)) {
            // rtc
            memcpy(&wakeup_source[i], param, sizeof(TUYA_WAKEUP_SOURCE_BASE_CFG_T));
            return i;
        }
        if ((param->source == TUYA_WAKEUP_SOURCE_GPIO) &&
                (wakeup_source[i].source == TUYA_WAKEUP_SOURCE_GPIO)) {
            // gpio
            if (param->wakeup_para.gpio_param.gpio_num ==
                    wakeup_source[i].wakeup_para.gpio_param.gpio_num) {
                memcpy(&wakeup_source[i], param, sizeof(TUYA_WAKEUP_SOURCE_BASE_CFG_T));
                return i;
            }
        }
    }

    for (i = 0; i < WAKEUP_SOURCE_CNT; i++) {
        if (wakeup_source[i].source == 0xff) {
            memcpy(&wakeup_source[i], param, sizeof(TUYA_WAKEUP_SOURCE_BASE_CFG_T));
            return i;
        }
    }

    bk_printf("no free wakeup space\r\n");
    return -1;
}

/**
 * @brief wake up source set
 *
 * @param[in] param: wake up source set,
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wakeup_source_set(const TUYA_WAKEUP_SOURCE_BASE_CFG_T  *param)
{
    if (param == NULL)
        return OPRT_INVALID_PARM;

    if (wakeup_source == NULL) {
        wakeup_source = tkl_system_malloc(WAKEUP_SOURCE_CNT * sizeof(TUYA_WAKEUP_SOURCE_BASE_CFG_T));
        if (wakeup_source == NULL) {
            bk_printf("wakeup source malloc failed\r\n");
            return OPRT_INVALID_PARM;
        }
        memset(wakeup_source, 0xff, WAKEUP_SOURCE_CNT * sizeof(TUYA_WAKEUP_SOURCE_BASE_CFG_T));
    }

    if ((param->source == TUYA_WAKEUP_SOURCE_GPIO) ||
        (param->source == TUYA_WAKEUP_SOURCE_TIMER)) {
        int ret = __save_wakeup_node(param);
        if (ret < 0)
            return OPRT_COM_ERROR;
    } else {
        bk_printf("unsupport source: %d\r\n", param->source);
        return OPRT_INVALID_PARM;
    }
    //wakeup_source_init_status |= (1 << param->source);
    bk_printf("wakeup source: %d %x\r\n", param->source, wakeup_source_init_status);

    return OPRT_OK;
}

/**
 * @brief wake up source get
 *
 * @param[in] param:  wake up source get
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wakeup_source_get(TUYA_WAKEUP_SOURCE_BASE_CFG_T *param, uint32_t *status)
{
    if (param == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (wakeup_source == NULL) {
        bk_printf("wakeup source not set\r\n");
        return OPRT_NOT_FOUND;
    }

    memcpy(param, wakeup_source, WAKEUP_SOURCE_CNT * sizeof(TUYA_WAKEUP_SOURCE_BASE_CFG_T));
    *status = wakeup_source_init_status;

    return OPRT_OK;
}

/**
 * @brief wake up source clear
 *
 * @param[in] param:  wake up source clear,
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wakeup_source_clear(const TUYA_WAKEUP_SOURCE_BASE_CFG_T *param)
{
    return OPRT_OK;
}

#if CONFIG_GPIO_WAKEUP_SUPPORT
static void __gpio_wakeup_source_set(gpio_id_t id, TUYA_GPIO_LEVEL_E level)
{
    gpio_int_type_t int_type = GPIO_INT_TYPE_RISING_EDGE;
    // TODO irq type
    switch (level) {
        case TUYA_GPIO_LEVEL_LOW:
            int_type = GPIO_INT_TYPE_LOW_LEVEL;
            break;
        case TUYA_GPIO_LEVEL_HIGH:
            int_type = GPIO_INT_TYPE_HIGH_LEVEL;
            break;
        default:
            //rising
            break;
    }

    bk_gpio_register_wakeup_source(id, int_type);
    bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_GPIO, NULL);
}
#endif //CONFIG_GPIO_WAKEUP_SUPPORT

#if CONFIG_AON_RTC
static void __rtc_wakeup_source_set(uint32_t ms)
{
    uint32_t rtc_ms_tick = bk_rtc_get_clock_freq()/1000;
    aon_pmu_drv_lpo_src_set(PM_LPO_SRC_ROSC);
    alarm_info_t deepsleep_alarm = {
        "low_vol", ms * rtc_ms_tick, 1, NULL, NULL
    };
    if(ms < 500) {
        os_printf("param %d invalid ! must > %dms.\r\n", ms, 500);
        return OPRT_OS_ADAPTER_CPU_LPMODE_SET_FAILED;
    }
    //force unregister previous if doesn't finish.
    bk_alarm_unregister(AON_RTC_ID_1, deepsleep_alarm.name);
    bk_alarm_register(AON_RTC_ID_1, &deepsleep_alarm);
    bk_printf("rtc wakeup: %d, %d\r\n", rtc_ms_tick, ms);
    bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);
}
#endif // CONFIG_AON_RTC

// set wakeup source to low level logic
void tkl_set_ll_wakeup_source(void)
{
    int i = 0;

    if (wakeup_source == NULL) {
        bk_printf("wakeup source not init, %d\r\n", __LINE__);
        return;
    }

    for (i = 0; i < WAKEUP_SOURCE_CNT; i++) {
        if (wakeup_source[i].source == TUYA_WAKEUP_SOURCE_GPIO) {
#if CONFIG_GPIO_WAKEUP_SUPPORT
            bk_printf("set gpio wakeup source %d\r\n",
                    wakeup_source[i].wakeup_para.gpio_param.gpio_num);
            __gpio_wakeup_source_set(wakeup_source[i].wakeup_para.gpio_param.gpio_num,
                    wakeup_source[i].wakeup_para.gpio_param.level);
#endif // CONFIG_GPIO_WAKEUP_SUPPORT
        } else if (wakeup_source[i].source == TUYA_WAKEUP_SOURCE_TIMER) {
#if CONFIG_AON_RTC
            bk_printf("set rtc wakeup source %d\r\n",
                    wakeup_source[i].wakeup_para.timer_param.ms);
            __rtc_wakeup_source_set(wakeup_source[i].wakeup_para.timer_param.ms);
#endif // CONFIG_AON_RTC
        }
    }
}


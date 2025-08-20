#include "tkl_sleep.h"
#include <modules/pm.h>
#include "tkl_wakeup.h"
#include "tkl_wifi.h"
#include <driver/gpio.h>
#include "aon_pmu_driver.h"

#if CONFIG_AON_RTC
#include <driver/aon_rtc_types.h>
#include <driver/hal/hal_aon_rtc_types.h>
#include <driver/aon_rtc.h>
#endif
#include "tkl_gpio.h"
#include <driver/uart.h>
#include "uart_statis.h"
#include "bk_uart.h"
#include "tuya_gpio_map.h"

#include "tkl_flash.h"
#include "tkl_wakeup.h"


static uint32_t is_prepare_deepsleep = 0;

extern void tkl_set_ll_wakeup_source(void);
extern void bk_printf(const char *fmt, ...);
#if CONFIG_AON_RTC
/*
 ******************************************************************************
 * Note:
 *      1s RTC定时器，在未连接wifi或者wifi断开时候启动
 *      调用投票机制后，芯片只能依赖唤醒源进行唤醒，系统调度不能幻想cpu，
 *      因此启动1s RTC定时器作为唤醒源处理该情况，时长1s可调
 *
 ******************************************************************************
 */
#define RTC_TIME 1000
alarm_info_t low_valtage_alarm;
void _bk_rtc_wakeup_register(unsigned int rtc_time)
{
    if (is_prepare_deepsleep)
        return;

    bk_printf("TODO %s\r\n", __func__);
    memcpy(low_valtage_alarm.name, "rtc_wakeup", sizeof("rtc_wakeup"));
    low_valtage_alarm.period_tick = rtc_time*AON_RTC_MS_TICK_CNT;
    low_valtage_alarm.period_cnt = 0xFFFFFFFF;
    low_valtage_alarm.callback = NULL;
    low_valtage_alarm.param_p = NULL;

    //force unregister previous if doesn't finish.
	pm_rtc_wakeup_config_t rtc_wakeup = {0};
	rtc_wakeup.rtc_period = rtc_time*AON_RTC_MS_TICK_CNT;//10s
	bk_pm_ap_rtc_wakeup_source_config(PM_MODE_LOW_VOLTAGE,WAKEUP_SOURCE_INT_RTC,&rtc_wakeup);
    bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);
}

void _bk_rtc_wakeup_unregister(void)
{
    bk_printf("TODO %s\r\n", __func__);
    // bk_alarm_unregister(AON_RTC_ID_1, low_valtage_alarm.name);
}
#endif // CONFIG_AON_RTC

/******************************************************************************/
static void __pm_debug_8(void)
{
    pm_debug_ctrl(8);
#if CONFIG_SYS_CPU0
    pm_debug_pwr_clk_state();
    pm_debug_lv_state();
#endif
}

static void tkl_sleep_param_dump(const char *tag, TKL_DS_PARAM_T *ds)
{
    if (tag != NULL) {
        bk_printf("=== %s\r\n", tag);
    }

    bk_printf("magic: 0x%x\r\n", ds->magic);
    bk_printf("entry flag: 0x%x\r\n", ds->entry_flag);
    for (int i = 0; i < DS_MAX_CFG_ITEM; i++) {
        if (ds->cfg[i].source == TUYA_WAKEUP_SOURCE_GPIO)
            bk_printf("io: %d, level: %d\r\n", ds->cfg[i].wakeup_para.gpio_param.gpio_num, ds->cfg[i].wakeup_para.gpio_param.level);
        else if (ds->cfg[i].source == TUYA_WAKEUP_SOURCE_RTC)
            bk_printf("rtc enable, time: %d\r\n", ds->cfg[i].wakeup_para.rtc_param.ms);
    }
    bk_printf("sum: 0x%x\r\n", ds->sum);
}

static int tkl_sleep_param_save(void)
{
    int i;
    TKL_DS_PARAM_T ds;

    memset(&ds, 0, sizeof(TKL_DS_PARAM_T));

    tkl_flash_read(DEEPSLEEP_PARAMETER_ADDRESS, &ds, sizeof(TKL_DS_PARAM_T));

    if ((ds.magic == DS_INVALID_VALUE) &&
        (ds.entry_flag == DS_INVALID_VALUE)) {
        // get and write parameter, then reboot
        uint32_t status = 0;

        bk_printf("get ds cfg\r\n");

        tkl_wakeup_source_get(ds.cfg, DS_MAX_CFG_ITEM, &status);

        ds.magic = DEEPSLEEP_MAGIC;
        ds.entry_flag = DS_ENTRY_FLAG;
        ds.sum = 0;

        uint8_t *tmp = (uint8_t *)&ds;
        for (i = 0; i < sizeof(TKL_DS_PARAM_T) - 4; i++) {
            ds.sum += tmp[i];
        }

        tkl_flash_erase(DEEPSLEEP_PARAMETER_ADDRESS, 4096);
        tkl_flash_write(DEEPSLEEP_PARAMETER_ADDRESS, &ds, sizeof(TKL_DS_PARAM_T));
        bk_delay_us(2000000);

        tkl_sleep_param_dump("save config && reboot", &ds);
        tkl_system_reset();

    } else {
        bk_printf("wrong parameter: 0x%x, 0x%x\r\n", ds.magic, ds.entry_flag);
        return -1;
    }
}

int tkl_sleep_param_check_and_set(void)
{
    int i;
    TKL_DS_PARAM_T ds;

    memset(&ds, 0, sizeof(TKL_DS_PARAM_T));

    tkl_flash_read(DEEPSLEEP_PARAMETER_ADDRESS, &ds, sizeof(TKL_DS_PARAM_T));

    if ((ds.magic == DEEPSLEEP_MAGIC) &&
            (ds.entry_flag == DS_ENTRY_FLAG)) {
        // config wakeup source, then entry deepsleep
        for (i = 0; i < DS_MAX_CFG_ITEM; i++) {
            if ((ds.cfg[i].source == TUYA_WAKEUP_SOURCE_GPIO) || (ds.cfg[i].source == TUYA_WAKEUP_SOURCE_RTC)) {
                tkl_wakeup_source_set(&ds.cfg[i]);
            }
        }

        // clean
        tkl_flash_erase(DEEPSLEEP_PARAMETER_ADDRESS, 4096);

        tkl_sleep_param_dump("entry ds ...", &ds);

        __pm_debug_8();
        __asm volatile ( "dsb" ::: "memory" );
        __asm volatile ( "isb" );
        bk_pm_ap_sleep_mode_set(PM_MODE_DEEP_SLEEP);
    } else {
        bk_printf("parameter: 0x%x, 0x%x\r\n", ds.magic, ds.entry_flag);
        return -1;
    }
}

/*******************************************************************/

/**
* @brief Set the low power mode of CPU
*
* @param[in] enable: enable switch
* @param[in] mode:   cpu sleep mode
*
* @note This API is used for setting the low power mode of CPU.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_cpu_sleep_mode_set(BOOL_T enable, TUYA_CPU_SLEEP_MODE_E mode)
{
    bk_printf("-- cpu sleep set enable:%d, mode:%d\r\n", enable, mode);

    if(mode == TUYA_CPU_SLEEP) {
        if(tkl_get_lp_flag()) {
            if(enable) {
                bk_printf("app vote sleep\r\n");
                __pm_debug_8();
	            bk_pm_ap_sleep_mode_set(PM_MODE_LOW_VOLTAGE);
			    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x1,0);
            }else {
			    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x0,0);
                // bk7236 连上路由后，cpu 一直保持在睡眠状态，唤醒周期由wifi唤醒决定
                // bk_printf("bk_pm_module_vote_sleep_ctrl disable !!!\r\n");
            }
        } else {
            //默认cpu就是睡眠模式（调度和中断能自己唤醒），不需要设置
        }
    } else if (mode == TUYA_CPU_DEEP_SLEEP) {
        if(enable) {
            // PM_MODE_DEEP_SLEEP
            bk_printf("prepare to deepsleep\r\n");
            tkl_sleep_param_save();
        }
    } else {
        return OPRT_OS_ADAPTER_CPU_LPMODE_SET_FAILED;
    }
    return OPRT_OK;
}

#define AP_CONNECT_POWER_RATIO      (220)
#define NET_CONNECT_POWER_RATIO     (80)

OPERATE_RET tkl_get_cpu_sleep_param(uint32_t* ap_conn_power_ratio, uint32_t* net_conn_power_ratio)
{
    *ap_conn_power_ratio = AP_CONNECT_POWER_RATIO;
    *net_conn_power_ratio = NET_CONNECT_POWER_RATIO;
    return OPRT_OK;
}


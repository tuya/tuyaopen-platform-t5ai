#include "tkl_sleep.h"
#include <modules/pm.h>
#include "tkl_wakeup.h"
#include "tkl_wifi.h"
#include <driver/gpio.h>
#if CONFIG_AON_RTC
#include <driver/aon_rtc_types.h>
#include <driver/hal/hal_aon_rtc_types.h>
#include <driver/aon_rtc.h>
#endif
#include "tkl_gpio.h"
#include <driver/uart.h>
#include "uart_statis.h"
#include "bk_uart.h"
// #include "tuya_gpio_map.h"

uint8_t ready_to_deepsleep = 0;
extern OPERATE_RET tkl_wakeup_source_get(TUYA_WAKEUP_SOURCE_BASE_CFG_T *param, uint32_t *status);
extern void tuya_multimedia_power_off(void);
extern void tkl_set_ll_wakeup_source(void);
//static int uart1_exit_power_save(uint64_t sleep_time, void *args);

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
#if defined(ENABLE_WIFI_ULTRA_LOWPOWER) && (ENABLE_WIFI_ULTRA_LOWPOWER == 1)
#define RTC_TIME 1000
alarm_info_t low_valtage_alarm;
void _bk_rtc_wakeup_register(unsigned int rtc_time)
{
    if (ready_to_deepsleep) {
        bk_printf("ready to deepsleep, not init 1s rtc\r\n");
        tkl_system_sleep(200);
        return;
    }
    bk_printf("%s\r\n", __func__);
    memcpy(low_valtage_alarm.name, "rtc_wakeup", sizeof("rtc_wakeup"));
    low_valtage_alarm.period_tick = rtc_time*AON_RTC_MS_TICK_CNT;
    low_valtage_alarm.period_cnt = 0xFFFFFFFF;
    low_valtage_alarm.callback = NULL;
    low_valtage_alarm.param_p = NULL;

    //force unregister previous if doesn't finish.
    bk_alarm_unregister(AON_RTC_ID_1, low_valtage_alarm.name);
    bk_alarm_register(AON_RTC_ID_1, &low_valtage_alarm);
    bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);
}

void _bk_rtc_wakeup_unregister(void)
{
    bk_printf("%s\r\n", __func__);
    bk_alarm_unregister(AON_RTC_ID_1, low_valtage_alarm.name);
}
#endif // ENABLE_WIFI_ULTRA_LOWPOWER
#endif // CONFIG_AON_RTC

/******************************************************************************/

#ifdef USER_UART_PORT
static uint8_t __test_uart_init_flag = 0;

/******************************************************************************
 * Note: user uart 低功耗处理
 ******************************************************************************/
static void uart_rx_wakeup(int gpio_id)
{
    //bk_gpio_register_isr(gpio_id, NULL);
    uart_exit_power_save(0, NULL);
}

struct uart_pm_map_s {
    uint32_t port;
    uint32_t pm_idx;
};
static struct uart_pm_map_s __uart_pm_map[] = {
    {.port = UART_ID_0, .pm_idx = PM_DEV_ID_UART1},
    {.port = UART_ID_1, .pm_idx = PM_DEV_ID_UART2},
    {.port = UART_ID_2, .pm_idx = PM_DEV_ID_UART3},
};

static inline int __get_uart_idx(uint32_t port)
{
    for (int i = 0; i < sizeof(__uart_pm_map)/sizeof(struct uart_pm_map_s); i++) {
        if (__uart_pm_map[i].port == port) {
            return i;
        }
    }
    return -1;
}

static int uart_enter_power_save(uint64_t sleep_time, void *args)
{
    int id = __get_uart_idx(USER_UART_PORT);
    if (id < 0) {
        return -1;
    }

    extern bool bk_uart_is_tx_over(uart_id_t id);
    while(bk_uart_is_tx_over(__uart_pm_map[id].port) == 0);

    // TX/RX_SUSPEND
    bk_uart_set_enable_tx(__uart_pm_map[id].port, 0);
    bk_uart_set_enable_rx(__uart_pm_map[id].port, 0);

    bk_uart_pm_backup(__uart_pm_map[id].port);
#if 1
    uint32_t  gpio_id = bk_uart_get_rx_gpio(__uart_pm_map[id].port);
    //bk_gpio_register_isr(gpio_id, (gpio_isr_t)uart_rx_wakeup);

    TUYA_GPIO_IRQ_T cfg;
    cfg.mode = TUYA_GPIO_IRQ_FALL;
    cfg.cb = uart_rx_wakeup;
    cfg.arg = NULL;
    tkl_gpio_irq_init(gpio_id, &cfg);
#endif
}

int uart_exit_power_save(uint64_t sleep_time, void *args)
{
    int id = __get_uart_idx(USER_UART_PORT);
    if (id < 0) {
        return -1;
    }

    bk_uart_set_enable_tx(__uart_pm_map[id].port, 1);
    bk_uart_set_enable_rx(__uart_pm_map[id].port, 1);
}

static void uart_pm_handle(void)
{
    if (__test_uart_init_flag == 0) {
        int id = __get_uart_idx(USER_UART_PORT);
        if (id < 0) {
            return -1;
        }

        pm_cb_conf_t enter_config;
        enter_config.cb = (pm_cb)uart_enter_power_save;
        enter_config.args = NULL;

        pm_cb_conf_t exit_config;
        exit_config.cb = (pm_cb)uart_exit_power_save;
        exit_config.args = NULL;

        bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, __uart_pm_map[id].pm_idx, &enter_config, &exit_config);
        bk_pm_module_lv_sleep_state_clear(__uart_pm_map[id].pm_idx);

        __test_uart_init_flag = 1;
    }
}
#endif // USER_UART_PORT
/******************************************************************************/
static void __pm_debug_8(void)
{
    pm_debug_ctrl(8);
#if CONFIG_SYS_CPU0
    pm_debug_pwr_clk_state();
    pm_debug_lv_state();
#endif
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
extern void sys_hal_enter_low_analog(void);
extern void sys_hal_exit_low_analog(void);
OPERATE_RET tkl_cpu_sleep_mode_set(BOOL_T enable, TUYA_CPU_SLEEP_MODE_E mode)
{
    bk_printf("-- cpu sleep:%d, %d\r\n", enable, mode);
    if(mode == TUYA_CPU_SLEEP) {
#if defined(ENABLE_WIFI_ULTRA_LOWPOWER) && (ENABLE_WIFI_ULTRA_LOWPOWER == 1)
        if(enable) {
            uart_pm_handle();
            #if CONFIG_SYS_CPU0 && (CONFIG_CPU_CNT > 1)
            tuya_multimedia_power_off();
            #endif

            tkl_set_ll_wakeup_source();

            __pm_debug_8();

            //sys_hal_enter_low_analog();

            bk_pm_module_vote_sleep_ctrl(12, 1, 0);
            os_printf("bk_pm_module_vote_sleep_ctrl enable !!!\r\n");
            bk_pm_sleep_mode_set(PM_MODE_LOW_VOLTAGE);
        }else {
            bk_pm_module_vote_sleep_ctrl(12, 0, 0);
            //sys_hal_exit_low_analog();
            // bk7236 连上路由后，cpu 一直保持在睡眠状态，唤醒周期由wifi唤醒决定
            os_printf("bk_pm_module_vote_sleep_ctrl disable !!!\r\n");
        }
#else
        //默认cpu就是睡眠模式（调度和中断能自己唤醒），不需要设置
#endif
    } else if (mode == TUYA_CPU_DEEP_SLEEP) {
        if(enable) {
            // PM_MODE_DEEP_SLEEP
            bk_printf("prepare to deepsleep\r\n");

            ready_to_deepsleep = 1;
            //_bk_rtc_wakeup_unregister();
            // 1. disable tuya ble
            //tuya_ble_set_serv_switch(false);

            // 2. set wakeup source
            tkl_set_ll_wakeup_source();

            // 3. stop cpu 1
            //#if CONFIG_SYS_CPU0 && (CONFIG_CPU_CNT > 1)
            //stop_cpu1_core();
            //#endif

            __pm_debug_8();

            //bk_pm_clean_bakp();
            // 4. set deepsleep mode
            bk_pm_sleep_mode_set(PM_MODE_DEEP_SLEEP);
            __pm_debug_8();
        }
    } else {
        return OPRT_OS_ADAPTER_CPU_LPMODE_SET_FAILED;
    }

    return OPRT_OK;
}

#if defined(ENABLE_WIFI_ULTRA_LOWPOWER) && (ENABLE_WIFI_ULTRA_LOWPOWER == 1)
#define AP_CONNECT_POWER_RATIO      (220)
#define NET_CONNECT_POWER_RATIO     (80)

OPERATE_RET tkl_get_cpu_sleep_param(uint32_t* ap_conn_power_ratio, uint32_t* net_conn_power_ratio)
{
    *ap_conn_power_ratio = AP_CONNECT_POWER_RATIO;
    *net_conn_power_ratio = NET_CONNECT_POWER_RATIO;
    return OPRT_OK;
}
#endif

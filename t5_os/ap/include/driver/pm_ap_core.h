#ifndef __PM_AP_CORE_H__
#define __PM_AP_CORE_H__
#include <driver/aon_rtc.h>
#include <modules/pm.h>
#include <driver/pwr_clk.h>

typedef enum
{
	PM_AP_CORE_STATE_ENTER_DEEPSLEEP = 0,
    PM_AP_CORE_STATE_CTRL_CP2,
    PM_AP_CORE_AP_RECOVERY,
    PM_AP_CORE_SLEEP_WAKEUP_NOTIFY,
    PM_AP_CORE_PSRAM_STATE_NOTIFY,
	PM_AP_CORE_STATE_MAX
}pm_ap_core_state_e;

typedef struct
{
    uint32_t event;
    uint32_t param1;
	uint32_t param2;
    uint32_t param3;
} pm_ap_core_msg_t;

typedef enum
{
    PM_AP_CLOSE_AP_MODULE_MEDIA = 0,
    PM_AP_CLOSE_AP_MODULE_WIFI,
    PM_AP_CLOSE_AP_MODULE_BT,
    PM_AP_CLOSE_AP_MODULE_APP,
    PM_AP_CLOSE_AP_MODULE_MAX,
}pm_ap_close_ap_module_e;

typedef bk_err_t (*close_ap_callback_fn)(void* param1,uint32_t param2);

typedef struct
{
    pm_ap_close_ap_module_e module;
    close_ap_callback_fn close_ap_cb_fn;
    void* param1;//it can use the variable address, it need global variable,otherwise the param1 invalid.
    uint32_t param2;
} pm_ap_close_ap_callback_info_t;

typedef enum
{
    PM_AP_PSRAM_POWER_ON = 0,
    PM_AP_PSRAM_POWER_OFF,
    PM_AP_PSRAM_POWER_MAX,
}pm_ap_psram_power_state_e;

typedef enum
{
    PM_AP_USING_PSRAM_POWER_STATE_DEV_MEDIA = 0,
    PM_AP_USING_PSRAM_POWER_STATE_DEV_WIFI,
    PM_AP_USING_PSRAM_POWER_STATE_DEV_BT,
    PM_AP_USING_PSRAM_POWER_STATE_DEV_MEMORY,
    PM_AP_USING_PSRAM_POWER_STATE_DEV_APP,
    PM_AP_USING_PSRAM_POWER_STATE_DEV_MAX,
}pm_ap_using_psram_power_state_dev_e;

typedef bk_err_t (*psram_power_state_callback_fn)(uint32_t param1,uint32_t param2);

typedef struct
{
    pm_power_psram_module_name_e dev_id;
    psram_power_state_callback_fn psram_on_cb_fn;
    psram_power_state_callback_fn psram_off_cb_fn;
    uint32_t param1;
    uint32_t param2;
} pm_ap_psram_power_state_callback_info_t;

typedef enum
{
    PM_AP_USING_SYS_WAKEUP_DEV_MEDIA = 0,
    PM_AP_USING_SYS_WAKEUP_DEV_WIFI,
    PM_AP_USING_SYS_WAKEUP_DEV_BT,
    PM_AP_USING_SYS_WAKEUP_DEV_APP,
    PM_AP_USING_SYS_WAKEUP_DEV_DEMO,
    PM_AP_USING_SYS_WAKEUP_DEV_MAX,
}pm_ap_using_wakeup_dev_e;
typedef bk_err_t (*system_wakeup_cb_fn)(pm_wakeup_source_e wake_source,uint32_t wake_src_param);

typedef struct
{
    pm_ap_using_wakeup_dev_e dev_id;
    pm_sleep_mode_e  sleep_mode;
    pm_wakeup_source_e  wakeup_source;
    system_wakeup_cb_fn sys_wakeup_fn;
} pm_ap_system_wakeup_cb_info_t;
/**
 * @brief handle psram power on/off callback
 *
 * @attention
 * - This API is used to handle psram power on/off callback.
 *
 * @param
 * -psram_power_state:0x0:PM_AP_PSRAM_POWER_ON,0x1:PM_AP_PSRAM_POWER_OFF
 * -dev_id:device name
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_ap_psram_power_state_handle_callback(pm_power_psram_module_name_e dev_id,pm_ap_psram_power_state_e psram_power_state);
/**
 * @brief unregister psram power on/off callback
 *
 * @attention
 * - This API is used to unregister psram power on/off callback.
 *
 * @param
 * -p_psram_power_state_callback_info: psram power callback information
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_ap_psram_power_state_unregister_callback(pm_ap_psram_power_state_callback_info_t * p_psram_power_state_callback_info);
/**
 * @brief register psram power on/off callback
 *
 * @attention
 * - This API is used to register psram power on/off callback.
 *
 * @param
 * -p_psram_power_state_callback_info: psram power callback information
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_ap_psram_power_state_register_callback(pm_ap_psram_power_state_callback_info_t * p_psram_power_state_callback_info);
/**
 * @brief handle wakeup callback
 *
 * @attention
 * - This API is used to handle wakeup callback.
 *
 * @param
 * -void
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_ap_system_wakeup_handle_callback();
/**
 * @brief unregister system wakeup callback
 *
 * @attention
 * - This API is used to unregister system wakeup callback.
 *
 * @param
 * -p_sys_wakeup_callback_info: system wakeup callback information
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_ap_system_wakeup_unregister_callback(pm_ap_system_wakeup_cb_info_t * p_sys_wakeup_callback_info);
/**
 * @brief register system wakeup callback
 *
 * @attention
 * - This API is used to register system wakeup callback info.
 *
 * @param
 * -p_sys_wakeup_callback_info: system wakeup callback information
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_ap_system_wakeup_register_callback(pm_ap_system_wakeup_cb_info_t * p_sys_wakeup_callback_info);
/**
 * @brief handle close ap callback
 *
 * @attention
 * - This API is used to handle close cp2 callback.
 *
 * @param
 * -void
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
 bk_err_t bk_pm_ap_close_ap_handle_callback();
/**
 * @brief unregister close cp2 callback
 *
 * @attention
 * - This API is used to unregister close cp2 callback.
 *
 * @param
 * -p_close_ap_callback_info: close ap callback information
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_ap_close_ap_unregister_callback(pm_ap_close_ap_callback_info_t * p_close_ap_callback_info);
/**
 * @brief register close ap callback
 *
 * @attention
 * - This API is used to register close ap callback info.
 *
 * @param
 * -p_close_ap_callback_info: close ap callback information
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_ap_close_ap_register_callback(pm_ap_close_ap_callback_info_t * p_close_ap_callback_info);
/**
 * @brief set the rtc tick at the begin of startup
 *
 * @attention
 * - This API is used to set the rtc tick at the begin of startup.
 *
 * @param
 * -time_tick:rtc tick
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_ap_misc_startup_rtc_tick_set(uint64_t time_tick);
/**
 * @brief get the time interval from the startup(unit:us)
 *
 * @attention
 * - This API is used to get the time interval from the startup(unit:us).
 *
 * @param
 * -time_interval[output]:time interval(unit:us)
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_ap_misc_get_time_interval_from_startup(uint32_t* time_interval);
/**
 * @brief enter deepsleep ,wakeup source using rtc
 *
 * @attention
 * - This API is used to enter deepsleep ,wakeup source using rtc(unit:ms).
 *
 * @param
 * -time_interval[input]:time interval(unit:ms)
 * -callback[input]:register callback
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_ap_misc_rtc_enter_deepsleep(uint32_t time_interval , aon_rtc_isr_t callback);
/**
 * @brief low pwr core init
 *
 * aov pm init
 *
 * @attention
 * - This API is to init low pwr core
 *
 * @param
 * -void
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_ap_core_init();
/**
 * @brief send pwr_core msg
 *
 * send pwr_core msg
 *
 * @attention
 * - This API is to send low pwr_core msg
 *
 * @param
 * -msg
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_pm_ap_core_send_msg(pm_ap_core_msg_t *msg);
#endif

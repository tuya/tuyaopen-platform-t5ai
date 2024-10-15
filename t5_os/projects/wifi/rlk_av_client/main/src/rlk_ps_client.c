#include "cli.h"
//#include "wlan_ui_pub.h"
#include "bk_manual_ps.h"
#include "bk_mac_ps.h"
#include "bk_mcu_ps.h"
#include "bk_ps.h"
#include "modules/pm.h"
#include "sys_driver.h"
#include "bk_pm_internal_api.h"
#include <driver/gpio.h>
#include <driver/hal/hal_aon_rtc_types.h>
#include <driver/aon_rtc_types.h>
#include <driver/aon_rtc.h>
#include <driver/timer.h>
#include "rlk_control_client.h"
#include "driver/gpio.h"


static UINT32 s_rlk_client_sleep_mode = 0;
static UINT32 s_pm_vote1       = 0;
static UINT32 s_pm_vote2       = 0;
static UINT32 s_pm_vote3       = 0;

static void rlk_client_rtc_callback(aon_rtc_id_t id, uint8_t *name_p, void *param)
{
    if (s_rlk_client_sleep_mode == PM_MODE_LOW_VOLTAGE)
    {
        bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
        bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x0,0x0);
        rlk_client_wakeup_from_lowvoltage();
    }
    else
    {
        bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
        bk_pm_module_vote_sleep_ctrl(s_pm_vote1,0x0,0x0);
        bk_pm_module_vote_sleep_ctrl(s_pm_vote2,0x0,0x0);
        bk_pm_module_vote_sleep_ctrl(s_pm_vote3,0x0,0x0);
    }
}

void rlk_client_rtc_sleep_start(UINT8 pm_sleep_mode, UINT32 rtc_time_ms, UINT8 pm_mode_name1,UINT8 pm_mode_name2,UINT8 pm_mode_name3,UINT8 pm_param)
{
   // rtos_stop_timer(&lowpower_keeplive_client_timer);
    s_rlk_client_sleep_mode = pm_sleep_mode;
    if ((pm_sleep_mode > PM_MODE_DEFAULT))
    {
        os_printf("set low power  parameter value  invalid\r\n");
        return;
    }

    if (pm_sleep_mode == PM_MODE_LOW_VOLTAGE)
    {
        if((pm_mode_name1 > PM_SLEEP_MODULE_NAME_MAX) ||(pm_mode_name2 > PM_SLEEP_MODULE_NAME_MAX) ||(pm_mode_name3 > PM_SLEEP_MODULE_NAME_MAX))
        {
            os_printf("set pm vote low vol parameter value invalid\r\n");
            return;
        }
    }

    alarm_info_t low_valtage_alarm = {
        "low_vol",
        rtc_time_ms * bk_rtc_get_ms_tick_count(),
        1,
        rlk_client_rtc_callback,
        NULL};

    // force unregister previous if doesn't finish.
    bk_alarm_unregister(AON_RTC_ID_1, low_valtage_alarm.name);
    bk_alarm_register(AON_RTC_ID_1, &low_valtage_alarm);

    bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);

    if(pm_sleep_mode == PM_MODE_LOW_VOLTAGE)
    {
        //#if PM_MANUAL_LOW_VOL_VOTE_ENABLE
        if(pm_mode_name1 == PM_SLEEP_MODULE_NAME_APP)
        {
            bk_pm_module_vote_sleep_ctrl(pm_mode_name1,0x1,pm_param);
        }
        else
        {
            bk_pm_module_vote_sleep_ctrl(pm_mode_name1,0x1,0x0);
        }

        if(pm_mode_name2 == PM_SLEEP_MODULE_NAME_APP)
        {
            bk_pm_module_vote_sleep_ctrl(pm_mode_name2,0x1,pm_param);
        }
        else
        {
            bk_pm_module_vote_sleep_ctrl(pm_mode_name2,0x1,0x0);
        }

        if(pm_mode_name3 == PM_SLEEP_MODULE_NAME_APP)
        {
            bk_pm_module_vote_sleep_ctrl(pm_mode_name3,0x1,pm_param);
        }
        else
        {
            bk_pm_module_vote_sleep_ctrl(pm_mode_name3,0x1,0x0);
        }

        if((pm_mode_name1 == PM_SLEEP_MODULE_NAME_APP))
        {
            bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x1,pm_param);
        }
    }
    else
    {
        ;//do something
    }
    
    bk_pm_sleep_mode_set(pm_sleep_mode);
}

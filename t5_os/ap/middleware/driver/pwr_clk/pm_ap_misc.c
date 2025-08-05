// Copyright 2025-2026 Beken
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

#include <common/bk_include.h>
#include <driver/pwr_clk.h>
#include <modules/pm.h>
#include "sys_driver.h"
#include "sys_types.h"
#include <driver/aon_rtc.h>
#include <os/os.h>
#include <components/system.h>
#include <common/bk_kernel_err.h>
#include "aon_pmu_hal.h"
#include <components/log.h>
#include "common/bk_err.h"
#include "driver/pm_ap_core.h"
/*=====================DEFINE  SECTION  START=====================*/

#define PM_AP_TAG "pm_ap"
#define LOGI(...) BK_LOGI(PM_AP_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(PM_AP_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(PM_AP_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(PM_AP_TAG, ##__VA_ARGS__)


/*=====================DEFINE  SECTION  END=====================*/
typedef enum
{
	PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE = 0,
	PM_SYSTEM_WAKEUP_MODE_DEEP_SLEEP ,
	PM_SYSTEM_WAKEUP_MODE_SUPER_DEEP_SLEEP ,
	PM_SYSTEM_WAKEUP_MODE_MAX
}pm_system_wakeup_mode_e;
/*=====================VARIABLE  SECTION  START=================*/


static pm_ap_close_ap_callback_info_t s_close_ap_cb_arry[PM_AP_CLOSE_AP_MODULE_MAX];

static pm_ap_system_wakeup_cb_info_t s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_MAX][PM_AP_USING_SYS_WAKEUP_DEV_MAX];

static pm_ap_psram_power_state_callback_info_t s_psram_power_state_cb_arry[PM_POWER_PSRAM_MODULE_NAME_MAX];

static uint32_t s_pm_register_psram_callback_state = 0;
static uint32_t s_pm_handle_psram_callback_state   = 0;

/*=====================VARIABLE  SECTION  END=================*/

/*================FUNCTION DECLARATION  SECTION  START========*/


/*================FUNCTION DECLARATION  SECTION  END========*/
bk_err_t bk_pm_ap_misc_rtc_enter_deepsleep(uint32_t time_interval , aon_rtc_isr_t callback)
{
	return BK_OK;
}

bk_err_t bk_pm_ap_misc_startup_rtc_tick_set(uint64_t time_tick)
{

	return BK_OK;
}

bk_err_t bk_pm_ap_close_ap_register_callback(pm_ap_close_ap_callback_info_t * p_close_ap_callback_info)
{
    if(p_close_ap_callback_info == NULL)
    {
        return BK_FAIL;
    }
    if(p_close_ap_callback_info->module >= PM_AP_CLOSE_AP_MODULE_MAX)
    {
        return BK_FAIL;
    }
    LOGD("Reg close ap_cb:0x%x,%d,0x%x,%d\r\n",p_close_ap_callback_info->close_ap_cb_fn,p_close_ap_callback_info->module,p_close_ap_callback_info->param1,p_close_ap_callback_info->param2);
	s_close_ap_cb_arry[p_close_ap_callback_info->module].close_ap_cb_fn = p_close_ap_callback_info->close_ap_cb_fn;
    s_close_ap_cb_arry[p_close_ap_callback_info->module].module= p_close_ap_callback_info->module;
    s_close_ap_cb_arry[p_close_ap_callback_info->module].param1 = p_close_ap_callback_info->param1;
    s_close_ap_cb_arry[p_close_ap_callback_info->module].param2 = p_close_ap_callback_info->param2;
    return BK_OK;
}

bk_err_t bk_pm_ap_close_ap_unregister_callback(pm_ap_close_ap_callback_info_t * p_close_ap_callback_info)
{
	if(p_close_ap_callback_info == NULL)
    {
        return BK_FAIL;
    }
    if(p_close_ap_callback_info->module >= PM_AP_CLOSE_AP_MODULE_MAX)
    {
        return BK_FAIL;
    }
    for(int i = 0; i < sizeof(s_close_ap_cb_arry)/sizeof(pm_ap_close_ap_callback_info_t);i++)
    {
        if(s_close_ap_cb_arry[i].module == p_close_ap_callback_info->module)
        {
            s_close_ap_cb_arry[i].close_ap_cb_fn = NULL;
            s_close_ap_cb_arry[i].module= PM_AP_CLOSE_AP_MODULE_MAX;
            s_close_ap_cb_arry[i].param1 = NULL;
            s_close_ap_cb_arry[i].param2 = 0;
        }
    }
    return BK_OK;
}

bk_err_t bk_pm_ap_close_ap_handle_callback()
{
    for(int i = 0; i < sizeof(s_close_ap_cb_arry)/sizeof(pm_ap_close_ap_callback_info_t);i++)
    {
        if(s_close_ap_cb_arry[i].close_ap_cb_fn != NULL)
        {
            s_close_ap_cb_arry[i].close_ap_cb_fn(s_close_ap_cb_arry[i].param1,s_close_ap_cb_arry[i].param2);
            //LOGD("Handle close ap cb:%d,0x%x\r\n",i,s_close_ap_cb_arry[i].close_ap_cb_fn);
        }
    }
    return BK_OK;
}

bk_err_t bk_pm_ap_system_wakeup_register_callback(pm_ap_system_wakeup_cb_info_t * p_sys_wakeup_callback_info)
{
    if(p_sys_wakeup_callback_info == NULL)
    {
        return BK_FAIL;
    }
    if(p_sys_wakeup_callback_info->dev_id >= PM_AP_USING_SYS_WAKEUP_DEV_MAX)
    {
        return BK_FAIL;
    }
    LOGD("Reg system_wakeup_cb:0x%x,%d,0x%x,%d\r\n",p_sys_wakeup_callback_info->sys_wakeup_fn,p_sys_wakeup_callback_info->dev_id,p_sys_wakeup_callback_info->sleep_mode,p_sys_wakeup_callback_info->wakeup_source);

    s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][p_sys_wakeup_callback_info->dev_id].sys_wakeup_fn = p_sys_wakeup_callback_info->sys_wakeup_fn;
    s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][p_sys_wakeup_callback_info->dev_id].dev_id= p_sys_wakeup_callback_info->dev_id;
    s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][p_sys_wakeup_callback_info->dev_id].sleep_mode = p_sys_wakeup_callback_info->sleep_mode;
    s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][p_sys_wakeup_callback_info->dev_id].wakeup_source = p_sys_wakeup_callback_info->wakeup_source;
    return BK_OK;
}

bk_err_t bk_pm_ap_system_wakeup_unregister_callback(pm_ap_system_wakeup_cb_info_t * p_sys_wakeup_callback_info)
{
	if(p_sys_wakeup_callback_info == NULL)
    {
        return BK_FAIL;
    }
    if(p_sys_wakeup_callback_info->dev_id >= PM_AP_USING_SYS_WAKEUP_DEV_MAX)
    {
        return BK_FAIL;
    }
    for(int i = 0; i < PM_AP_USING_SYS_WAKEUP_DEV_MAX;i++)
    {
        if(s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][i].dev_id == p_sys_wakeup_callback_info->dev_id)
        {
            s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][i].sys_wakeup_fn = NULL;
            s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][i].dev_id = PM_AP_USING_SYS_WAKEUP_DEV_MAX;
            s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][i].sleep_mode = PM_MODE_DEFAULT;
            s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][i].wakeup_source = PM_WAKEUP_SOURCE_INT_NONE;
        }
    }
    return BK_OK;
}

bk_err_t bk_pm_ap_system_wakeup_handle_callback(pm_ap_core_msg_t *msg)
{
    for(int i = 0; i < PM_AP_USING_SYS_WAKEUP_DEV_MAX;i++)
    {
        if(s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][i].sys_wakeup_fn != NULL)
        {
            s_system_wakeup_cb_arry[PM_SYSTEM_WAKEUP_MODE_LOW_VOLTAGE][i].sys_wakeup_fn(0,0);
        }
    }
    return BK_OK;
}

bk_err_t bk_pm_ap_psram_power_state_register_callback(pm_ap_psram_power_state_callback_info_t * p_psram_power_state_callback_info)
{
    if(p_psram_power_state_callback_info == NULL)
    {
        return BK_FAIL;
    }
    if(p_psram_power_state_callback_info->dev_id >= PM_POWER_PSRAM_MODULE_NAME_MAX)
    {
        return BK_FAIL;
    }
    //LOGD("Reg close ap_cb:0x%x,%d,0x%x,%d\r\n",p_psram_power_state_callback_info->psram_on_cb_fn,p_psram_power_state_callback_info->dev_id,p_psram_power_state_callback_info->param1,p_psram_power_state_callback_info->param2);
	s_psram_power_state_cb_arry[p_psram_power_state_callback_info->dev_id].psram_on_cb_fn = p_psram_power_state_callback_info->psram_on_cb_fn;
    s_psram_power_state_cb_arry[p_psram_power_state_callback_info->dev_id].psram_off_cb_fn = p_psram_power_state_callback_info->psram_off_cb_fn;
    s_psram_power_state_cb_arry[p_psram_power_state_callback_info->dev_id].dev_id= p_psram_power_state_callback_info->dev_id;
    s_psram_power_state_cb_arry[p_psram_power_state_callback_info->dev_id].param1 = p_psram_power_state_callback_info->param1;
    s_psram_power_state_cb_arry[p_psram_power_state_callback_info->dev_id].param2 = p_psram_power_state_callback_info->param2;
    s_pm_register_psram_callback_state |= 0x1 << p_psram_power_state_callback_info->dev_id;
    s_pm_handle_psram_callback_state   |= 0x1 << p_psram_power_state_callback_info->dev_id;
    return BK_OK;
}

bk_err_t bk_pm_ap_psram_power_state_unregister_callback(pm_ap_psram_power_state_callback_info_t * p_psram_power_state_callback_info)
{
	if(p_psram_power_state_callback_info == NULL)
    {
        return BK_FAIL;
    }
    if(p_psram_power_state_callback_info->dev_id >= PM_POWER_PSRAM_MODULE_NAME_MAX)
    {
        return BK_FAIL;
    }
    for(int i = 0; i < sizeof(s_psram_power_state_cb_arry)/sizeof(pm_ap_psram_power_state_callback_info_t);i++)
    {
        if(s_psram_power_state_cb_arry[i].dev_id == p_psram_power_state_callback_info->dev_id)
        {
            s_psram_power_state_cb_arry[i].psram_on_cb_fn = NULL;
            s_psram_power_state_cb_arry[i].psram_off_cb_fn = NULL;
            s_psram_power_state_cb_arry[i].dev_id= PM_AP_USING_PSRAM_POWER_STATE_DEV_MAX;
            s_psram_power_state_cb_arry[i].param1 = 0;
            s_psram_power_state_cb_arry[i].param2 = 0;
            s_pm_register_psram_callback_state &= ~(0x1 << p_psram_power_state_callback_info->dev_id);
        }
    }
    return BK_OK;
}

bk_err_t bk_pm_ap_psram_power_state_handle_callback(pm_power_psram_module_name_e dev_id,pm_ap_psram_power_state_e psram_power_state)
{
    if(psram_power_state == PM_AP_PSRAM_POWER_ON)
    {
        for(int i = 0; i < sizeof(s_psram_power_state_cb_arry)/sizeof(pm_ap_psram_power_state_callback_info_t);i++)
        {
            if(s_pm_handle_psram_callback_state & (0x1 << i))
            {
                if(s_psram_power_state_cb_arry[i].psram_on_cb_fn != NULL)
                {
                    s_psram_power_state_cb_arry[i].psram_on_cb_fn(0,0);
                    /*Clear the handled callback state:It have handled callback, it cannot process again*/
                    s_pm_handle_psram_callback_state &= ~(0x1 << i);
                }
            }
        }
    }
    else if(psram_power_state == PM_AP_PSRAM_POWER_OFF)
    {
        for(int i = 0; i < sizeof(s_psram_power_state_cb_arry)/sizeof(pm_ap_psram_power_state_callback_info_t);i++)
        {
            if(s_psram_power_state_cb_arry[i].psram_off_cb_fn != NULL)
            {
                s_psram_power_state_cb_arry[i].psram_off_cb_fn(0,0);
                s_pm_handle_psram_callback_state   |= 0x1 << i;
            }
        }
    }
    else
    {
        ;
    }

    return BK_OK;
}
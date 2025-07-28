// Copyright 2020-2021 Beken
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
#if CONFIG_MAILBOX
#include <driver/mailbox_channel.h>
#endif
#include <modules/pm.h>
#include "sys_driver.h"
#if CONFIG_PSRAM
#include <driver/psram.h>
#endif
#include <os/mem.h>
#include "sys_types.h"
#include <driver/aon_rtc.h>
#include <os/os.h>
#include <driver/psram.h>
#include <components/system.h>
#include "bk_pm_internal_api.h"
#include <common/bk_kernel_err.h>
#include "aon_pmu_hal.h"
#if CONFIG_WDT_EN
#include "wdt_driver.h"
#endif
#include "driver/pm_ap_core.h"

/*=====================DEFINE  SECTION  START=====================*/
#define TAG "AP"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)
#define PM_SEND_CMD_CP1_RESPONSE_TIEM        (100)  //100ms
#define PM_BOOT_CP1_WAITING_TIEM             (500) // 0.5s
#define PM_CP1_RECOVERY_DEFAULT_VALUE        (0xFFFFFFFFFFFFFFFF)
#define PM_OPEN_CP1_TIMEOUT                  (20000) //20s
#define PM_SEMA_WAIT_FOREVER                 (0xFFFFFFFF)    /*Wait Forever*/

#define PM_BOOT_CP1_TRY_COUNT                (3)


/*=====================DEFINE  SECTION  END=====================*/

/*=====================VARIABLE  SECTION  START=================*/

static volatile  pm_mailbox_communication_state_e s_pm_cp1_pwr_finish            = 0;
static volatile  pm_mailbox_communication_state_e s_pm_cp1_clk_finish            = 0;
static volatile  pm_mailbox_communication_state_e s_pm_cp1_sleep_finish          = 0;
static volatile  pm_mailbox_communication_state_e s_pm_cp1_cpu_freq_finish       = 0;
static volatile  pm_mailbox_communication_state_e s_pm_cp1_init                  = 0;
static volatile  pm_mailbox_communication_state_e s_pm_external_ldo_ctrl         = 0;
static volatile  pm_mailbox_communication_state_e s_pm_psram_power_ctrl          = 0;
static volatile  pm_mailbox_communication_state_e s_pm_cp2_rtc_deepsleep_finish  = 0;
static volatile  pm_mailbox_communication_state_e s_pm_cp2_startup_time_finish   = 0;
static volatile  pm_mailbox_communication_state_e s_pm_cp2_ctrl_state_finish     = 0;
static volatile  pm_mailbox_communication_state_e s_pm_cp2_deepsleep_finish      = 0;
static volatile  pm_mailbox_communication_state_e s_pm_cp2_wakeup_src_cfg_finish = 0;

static volatile  uint32_t                         s_chip_startup_time            = 0;

static volatile  uint32_t                         s_pm_cp1_boot_try_count        = 0;

/*=====================VARIABLE  SECTION  END=================*/

/*================FUNCTION DECLARATION  SECTION  START========*/

#if CONFIG_MAILBOX
static void pm_cp1_mailbox_init();
bk_err_t pm_cp1_mailbox_response(uint32_t cmd, int ret);
bk_err_t bk_pm_cp1_ctrl_state_set(pm_mailbox_communication_state_e state);
pm_mailbox_communication_state_e bk_pm_cp1_ctrl_state_get();
bk_err_t pm_cp1_mailbox_send_data(uint32_t cmd, uint32_t param1,uint32_t param2,uint32_t param3);
#endif


/*================FUNCTION DECLARATION  SECTION  END========*/

pm_lpo_src_e bk_clk_32k_customer_config_get(void)
{
#if CONFIG_LPO_MP_A_FORCE_USE_EXT32K
	uint32_t chip_id = aon_pmu_hal_get_chipid();
	if ((chip_id & PM_CHIP_ID_MASK) == (PM_CHIP_ID_MP_A & PM_CHIP_ID_MASK))
	{
		return PM_LPO_SRC_X32K;
	}
	else
	{
		#if CONFIG_EXTERN_32K
			return PM_LPO_SRC_X32K;
		#elif CONFIG_LPO_SRC_26M32K
			return PM_LPO_SRC_DIVD;
		#else
			return PM_LPO_SRC_ROSC;
		#endif
	}
#else
	#if CONFIG_EXTERN_32K
		return PM_LPO_SRC_X32K;
	#elif CONFIG_LPO_SRC_26M32K
		return PM_LPO_SRC_DIVD;
	#else
		return PM_LPO_SRC_ROSC;
	#endif
#endif
	return PM_LPO_SRC_ROSC;
}
bk_err_t bk_pm_mailbox_init()
{
	/*cp1 mailbox init*/
#if CONFIG_MAILBOX
	pm_cp1_mailbox_init();
#endif

	return BK_OK;
}

#if CONFIG_MAILBOX
bk_err_t bk_pm_cp1_boot_ok_response_set()
{
	if(bk_pm_cp1_ctrl_state_get() == 0x0)
	{
		bk_pm_cp1_ctrl_state_set(PM_MAILBOX_COMMUNICATION_FINISH);
        pm_cp1_mailbox_send_data(PM_CPU1_BOOT_READY_CMD,0x1,0,0);
	}
	return BK_OK;
}
pm_mailbox_communication_state_e bk_pm_ap_wakeup_source_config_state_get()
{
    return s_pm_cp2_wakeup_src_cfg_finish;
}
bk_err_t bk_pm_ap_wakeup_source_config_state_set(pm_mailbox_communication_state_e state)
{
    s_pm_cp2_wakeup_src_cfg_finish = state;
    return BK_OK;
}
pm_mailbox_communication_state_e bk_pm_ap_enter_deepsleep_state_get()
{
    return s_pm_cp2_deepsleep_finish;
}
bk_err_t bk_pm_ap_enter_deepsleep_state_set(pm_mailbox_communication_state_e state)
{
    s_pm_cp2_deepsleep_finish = state;
    return BK_OK;
}

pm_mailbox_communication_state_e bk_pm_ap_ctrl_state_get()
{
    return s_pm_cp2_ctrl_state_finish;
}
bk_err_t bk_pm_ap_ctrl_state_set(pm_mailbox_communication_state_e state)
{
    s_pm_cp2_ctrl_state_finish = state;
    return BK_OK;
}

pm_mailbox_communication_state_e bk_pm_ap_startup_time_get()
{
    return s_pm_cp2_startup_time_finish;
}
bk_err_t bk_pm_ap_startup_time_set(pm_mailbox_communication_state_e state)
{
    s_pm_cp2_startup_time_finish = state;
    return BK_OK;
}

pm_mailbox_communication_state_e bk_pm_ap_rtc_deepsleep_state_get()
{
    return s_pm_cp2_rtc_deepsleep_finish;
}
bk_err_t bk_pm_ap_rtc_deepsleep_state_set(pm_mailbox_communication_state_e state)
{
    s_pm_cp2_rtc_deepsleep_finish = state;
    return BK_OK;
}
pm_mailbox_communication_state_e bk_pm_cp1_pwr_ctrl_state_get()
{
	return s_pm_cp1_pwr_finish;
}
bk_err_t bk_pm_cp1_pwr_ctrl_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_cp1_pwr_finish = state;
	return BK_OK;
}

pm_mailbox_communication_state_e bk_pm_cp1_clk_ctrl_state_get()
{
	return s_pm_cp1_clk_finish;
}
 bk_err_t bk_pm_cp1_clk_ctrl_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_cp1_clk_finish = state;
	return BK_OK;
}

pm_mailbox_communication_state_e bk_pm_cp1_sleep_ctrl_state_get()
{
	return s_pm_cp1_sleep_finish;
}
bk_err_t bk_pm_cp1_sleep_ctrl_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_cp1_sleep_finish = state;
	return BK_OK;
}

pm_mailbox_communication_state_e bk_pm_cp1_cpu_freq_ctrl_state_get()
{
	return s_pm_cp1_cpu_freq_finish;
}
bk_err_t bk_pm_cp1_cpu_freq_ctrl_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_cp1_cpu_freq_finish = state;
	return BK_OK;
}
pm_mailbox_communication_state_e bk_pm_cp1_external_ldo_ctrl_state_get()
{
	return s_pm_external_ldo_ctrl;
}
bk_err_t bk_pm_cp1_external_ldo_ctrl_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_external_ldo_ctrl = state;
	return BK_OK;
}
pm_mailbox_communication_state_e bk_pm_cp1_psram_power_state_get()
{
	return s_pm_psram_power_ctrl;
}
bk_err_t bk_pm_cp1_psram_power_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_psram_power_ctrl = state;
	return BK_OK;
}
pm_mailbox_communication_state_e bk_pm_cp1_ctrl_state_get()
{
	return s_pm_cp1_init;
}
bk_err_t bk_pm_cp1_ctrl_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_cp1_init = state;
	return BK_OK;
}
bk_err_t bk_pm_cp1_recovery_response(uint32_t cmd, pm_cp1_prepare_close_module_name_e module_name,pm_cp1_module_recovery_state_e state)
{
	pm_cp1_mailbox_send_data(cmd,module_name,state,0);
	return BK_OK;
}
bk_err_t pm_cp1_mailbox_send_data(uint32_t cmd, uint32_t param1,uint32_t param2,uint32_t param3)
{
	bk_err_t ret         = BK_OK;
	mb_chnl_cmd_t mb_cmd = {0};
    uint8_t  retry_count = 0;

	mb_cmd.hdr.cmd = cmd;
	mb_cmd.param1 = param1;
	mb_cmd.param2 = param2;
	mb_cmd.param3 = param3;

	ret = mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
    while(ret != BK_OK)
	{
	    retry_count++;
		ret = mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
		rtos_delay_milliseconds(2);
        if(retry_count > 5)
        {
            LOGE("Mailbox send data fail[ret:%d]\r\n",ret);
            return ret;
        }
	}
    return BK_OK;
}
static void pm_cp1_mailbox_tx_cmpl_isr(int *pm_mb, mb_chnl_ack_t *cmd_buf)
{
}
static void pm_cp1_mailbox_rx_isr(int *pm_mb, mb_chnl_cmd_t *cmd_buf)
{
	bk_err_t ret = BK_OK;
	uint32_t used_count;
    pm_ap_core_msg_t msg = {0};
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	switch(cmd_buf->hdr.cmd) {
		case PM_POWER_CTRL_CMD:
			if(cmd_buf->param1 == BK_OK)
			{
				s_pm_cp1_pwr_finish = PM_MAILBOX_COMMUNICATION_FINISH;
			}
			else
			{
				ret = BK_FAIL;
			}
			break;
		case PM_CLK_CTRL_CMD:
			if(cmd_buf->param1 == BK_OK)
			{
				s_pm_cp1_clk_finish = PM_MAILBOX_COMMUNICATION_FINISH;
			}
			else
			{
				ret = BK_FAIL;
			}
			break;
		 case PM_SLEEP_CTRL_CMD:
			if(cmd_buf->param1 == BK_OK)
			{
				bk_pm_cp1_sleep_ctrl_state_set(PM_MAILBOX_COMMUNICATION_FINISH);
			}
			else
			{
				ret = BK_FAIL;
			}
			break;
		 case PM_CPU_FREQ_CTRL_CMD:
			if(cmd_buf->param1 == BK_OK)
			{
				s_pm_cp1_cpu_freq_finish = PM_MAILBOX_COMMUNICATION_FINISH;
			}
			else
			{
				ret = BK_FAIL;
			}
			break;
		case PM_CTRL_EXTERNAL_LDO_CMD:
			if(cmd_buf->param1 == BK_OK)
			{
				s_pm_external_ldo_ctrl = PM_MAILBOX_COMMUNICATION_FINISH;
			}
			else
			{
				ret = BK_FAIL;
			}
			break;
		case PM_CTRL_PSRAM_POWER_CMD:
			if(cmd_buf->param1 == PM_POWER_MODULE_STATE_ON)
			{
				msg.event= PM_AP_CORE_PSRAM_STATE_NOTIFY;
				msg.param1 = 0x0;//cmd_buf->param1;
				msg.param2 = cmd_buf->param2;
				bk_pm_ap_core_send_msg(&msg);

			}
			else if(cmd_buf->param1 ==PM_POWER_MODULE_STATE_OFF)
			{

			}
			else
			{
				ret = BK_FAIL;
			}
			s_pm_psram_power_ctrl = PM_MAILBOX_COMMUNICATION_FINISH;
			break;
		case PM_CP1_PSRAM_MALLOC_STATE_CMD:
			if(cmd_buf->param1 == 0x1)
			{
				msg.event= PM_AP_CORE_PSRAM_STATE_NOTIFY;
				msg.param1 = 0x1;//recovery media using psram
				msg.param2 = cmd_buf->param2;
				bk_pm_ap_core_send_msg(&msg);
				pm_cp1_mailbox_send_data(PM_CP1_PSRAM_MALLOC_STATE_CMD,0x2,0,0);
			}
			else if(cmd_buf->param1 == 0x0)
			{
				used_count = bk_psram_heap_get_used_count();
				//bk_pm_ap_psram_power_state_handle_callback(0x1);//recovery media using psram
				pm_cp1_mailbox_send_data(PM_CP1_PSRAM_MALLOC_STATE_CMD,0x1,used_count,0);
			}
			//BK_LOGD(NULL, "cp1 bk_psram_heap_get_used_count[%d]\r\n", bk_psram_heap_get_used_count());
			break;
		case PM_CP1_DUMP_PSRAM_MALLOC_INFO_CMD:
			bk_psram_heap_get_used_state();
			break;
		case PM_CP1_RECOVERY_CMD:
            msg.event= PM_AP_CORE_AP_RECOVERY;
			msg.param1 = cmd_buf->param1;
			msg.param2 = cmd_buf->param2;
			bk_pm_ap_core_send_msg(&msg);
			bk_pm_cp1_ctrl_state_set(PM_MAILBOX_COMMUNICATION_INIT);
			break;
        case PM_RTC_DEEPSLEEP_CMD:
			if(cmd_buf->param1 == BK_OK)
			{
				bk_pm_ap_rtc_deepsleep_state_set(PM_MAILBOX_COMMUNICATION_FINISH);
			}
			else
			{
				ret = BK_FAIL;
			}
			break;
		case PM_STARTUP_TIME_CMD:
			s_chip_startup_time = cmd_buf->param1;
			s_pm_cp2_startup_time_finish = PM_MAILBOX_COMMUNICATION_FINISH;
			break;
		case PM_CTRL_AP_STATE_CMD:
			bk_pm_ap_ctrl_state_set(PM_MAILBOX_COMMUNICATION_FINISH);
			break;
		case PM_ENTER_DEEP_SLEEP_CMD:
			bk_pm_ap_enter_deepsleep_state_set(PM_MAILBOX_COMMUNICATION_FINISH);
			break;
		case PM_WAKEUP_CONFIG_CMD:
			bk_pm_ap_wakeup_source_config_state_set(PM_MAILBOX_COMMUNICATION_FINISH);
			break;
		case PM_SLEEP_WAKEUP_NOTIFY_CMD:
			msg.event= PM_AP_CORE_SLEEP_WAKEUP_NOTIFY;
			msg.param1 = cmd_buf->param1;
			msg.param2 = cmd_buf->param2;
			msg.param3 = cmd_buf->param3;
			bk_pm_ap_core_send_msg(&msg);
			break;
		default:
			break;
	}
	GLOBAL_INT_RESTORE();

	if(ret != BK_OK)
	{
		LOGV("cp1 response: cp0 handle msg error\r\n");
	}
	//if(pm_debug_mode()&0x2)
	{
      if(cmd_buf->hdr.cmd != PM_CP1_PSRAM_MALLOC_STATE_CMD)
      {
		LOGV("enter cp1_mailbox_rx_isr %d %d %d \r\n",cmd_buf->hdr.cmd,cmd_buf->param1,cmd_buf->param2);
      }
	}
}
static void pm_cp1_mailbox_tx_isr(int *pm_mb)
{
}

static void pm_cp1_mailbox_init()
{
	mb_chnl_open(MB_CHNL_PWC, NULL);
	if (pm_cp1_mailbox_rx_isr != NULL)
		mb_chnl_ctrl(MB_CHNL_PWC, MB_CHNL_SET_RX_ISR, pm_cp1_mailbox_rx_isr);
	if (pm_cp1_mailbox_tx_isr != NULL)
		mb_chnl_ctrl(MB_CHNL_PWC, MB_CHNL_SET_TX_ISR, pm_cp1_mailbox_tx_isr);
	if (pm_cp1_mailbox_tx_cmpl_isr != NULL)
		mb_chnl_ctrl(MB_CHNL_PWC, MB_CHNL_SET_TX_CMPL_ISR, pm_cp1_mailbox_tx_cmpl_isr);
}
#endif //CONFIG_MAILBOX

bk_err_t bk_pm_module_vote_psram_ctrl(pm_power_psram_module_name_e module,pm_power_module_state_e power_state)
{

#if CONFIG_MAILBOX
	uint64_t previous_tick  = 0;
	uint64_t current_tick   = 0;
	bk_err_t ret            =  BK_OK;
	bk_pm_cp1_psram_power_state_set(PM_MAILBOX_COMMUNICATION_INIT);

    ret = pm_cp1_mailbox_send_data(PM_CTRL_PSRAM_POWER_CMD, module,power_state,0);
    if(ret != BK_OK)
    {
        return BK_FAIL;
    }

	previous_tick = pm_cp1_aon_rtc_counter_get();
	current_tick = previous_tick;
	while((current_tick - previous_tick) < (PM_SEND_CMD_CP1_RESPONSE_TIEM*PM_AON_RTC_DEFAULT_TICK_COUNT))
	{
	    if (bk_pm_cp1_psram_power_state_get()) // wait the cp0 response
	    {
			break;
	    }
	    current_tick = pm_cp1_aon_rtc_counter_get();
	}

	if(!bk_pm_cp1_psram_power_state_get())
	{
	    LOGE("cp1 get psram state time out\r\n");
	}

	if(power_state == PM_POWER_MODULE_STATE_ON)
	{
		bk_pm_ap_psram_power_state_handle_callback(module,power_state);
	}
	LOGD("Ap vote psram_P E\r\n");
#endif
	return BK_OK;

}

bk_err_t bk_pm_module_vote_ctrl_external_ldo(gpio_ctrl_ldo_module_e module,gpio_id_t gpio_id,gpio_output_state_e value)
{
#if CONFIG_MAILBOX
	uint64_t previous_tick  = 0;
	uint64_t current_tick   = 0;
    int ret = 0;
	bk_pm_cp1_external_ldo_ctrl_state_set(PM_MAILBOX_COMMUNICATION_INIT);

    ret = pm_cp1_mailbox_send_data(PM_CTRL_EXTERNAL_LDO_CMD, module,gpio_id,value);
    if(ret != BK_OK)
    {
        return BK_FAIL;
    }
	previous_tick = pm_cp1_aon_rtc_counter_get();
	current_tick = previous_tick;
	while((current_tick - previous_tick) < (PM_SEND_CMD_CP1_RESPONSE_TIEM*PM_AON_RTC_DEFAULT_TICK_COUNT))
	{
	    if (bk_pm_cp1_external_ldo_ctrl_state_get()) // wait the cp0 response
	    {
			break;
	    }
	    current_tick = pm_cp1_aon_rtc_counter_get();
	}

	if(!bk_pm_cp1_external_ldo_ctrl_state_get())
	{
	    LOGE("cp1 ctr extLdo timeout\r\n");
	}

	LOGD("cp1 vote ctr_extLdo\r\n");
#endif
	return BK_OK;
}

bk_err_t bk_pm_ap_rtc_enter_deepsleep(pm_ap_rtc_enter_deepsleep_module_name_e module,uint32_t sleep_time)
{

#if CONFIG_MAILBOX
	uint64_t previous_tick  = 0;
	uint64_t current_tick   = 0;
    int ret                 = 0;
	bk_pm_ap_rtc_deepsleep_state_set(PM_MAILBOX_COMMUNICATION_INIT);

    ret = pm_cp1_mailbox_send_data(PM_RTC_DEEPSLEEP_CMD, module,sleep_time,0);
    if(ret != BK_OK)
    {
        return BK_FAIL;
    }

	previous_tick = pm_cp1_aon_rtc_counter_get();
	current_tick = previous_tick;
	while((current_tick - previous_tick) < (PM_SEND_CMD_CP1_RESPONSE_TIEM*PM_AON_RTC_DEFAULT_TICK_COUNT))
	{
	    if (bk_pm_ap_rtc_deepsleep_state_get()) // wait the cp0 response
	    {
			break;
	    }
	    current_tick = pm_cp1_aon_rtc_counter_get();
	}
	if(!bk_pm_ap_rtc_deepsleep_state_get())
	{
	    LOGE("ap:rtc deepsleep time out\r\n");
	}
#endif

    return BK_OK;
}
bk_err_t bk_pm_ap_misc_get_time_interval_from_startup(uint32_t* time_interval)
{

#if CONFIG_MAILBOX
    uint64_t previous_tick  = 0;
    uint64_t current_tick   = 0;
    int ret = 0;

    bk_pm_ap_startup_time_set(PM_MAILBOX_COMMUNICATION_INIT);
    ret = pm_cp1_mailbox_send_data(PM_STARTUP_TIME_CMD, 0,0,0);
    if(ret != BK_OK)
    {
        return BK_FAIL;
    }

    previous_tick = pm_cp1_aon_rtc_counter_get();
    current_tick = previous_tick;
    while((current_tick - previous_tick) < (PM_SEND_CMD_CP1_RESPONSE_TIEM*PM_AON_RTC_DEFAULT_TICK_COUNT))
    {
        if (bk_pm_ap_startup_time_get()) // wait the cp0 response
        {
            break;
        }
        current_tick = pm_cp1_aon_rtc_counter_get();
    }

    if(!bk_pm_ap_startup_time_get())
    {
        LOGE("ap get startup time time out\r\n");
    }
    *time_interval = s_chip_startup_time;
    LOGD("ap get startup time\r\n");
#endif//CONFIG_MAILBOX
	return BK_OK;
}
bk_err_t bk_pm_module_vote_boot_cp1_ctrl(pm_boot_cp1_module_name_e module,pm_power_module_state_e power_state)
{
#if CONFIG_MAILBOX

    uint64_t previous_tick  = 0;
    uint64_t current_tick   = 0;
    bk_err_t ret            = 0;
    bk_pm_ap_ctrl_state_set(PM_MAILBOX_COMMUNICATION_INIT);

    ret = pm_cp1_mailbox_send_data(PM_CTRL_AP_STATE_CMD, module,power_state,0);
    if(ret != BK_OK)
    {
        return BK_FAIL;
    }

    previous_tick = pm_cp1_aon_rtc_counter_get();
    current_tick = previous_tick;
    while((current_tick - previous_tick) < (PM_SEND_CMD_CP1_RESPONSE_TIEM*PM_AON_RTC_DEFAULT_TICK_COUNT))
    {
        if (bk_pm_ap_ctrl_state_get()) // wait the cp0 response
        {
            break;
        }
        current_tick = pm_cp1_aon_rtc_counter_get();
    }

    if(!bk_pm_ap_ctrl_state_get())
    {
        LOGE("ap vote ctrl ap time out\r\n");
    }
#endif//CONFIG_MAILBOX

    return BK_OK;
}
bk_err_t bk_pm_ap_sleep_mode_set(pm_sleep_mode_e sleep_mode)
{
    #if CONFIG_MAILBOX
    uint64_t previous_tick  = 0;
    uint64_t current_tick   = 0;
    int ret                 = 0;

    bk_pm_ap_enter_deepsleep_state_set(PM_MAILBOX_COMMUNICATION_INIT);

    ret = pm_cp1_mailbox_send_data(PM_ENTER_DEEP_SLEEP_CMD, sleep_mode,0,0);
    if(ret != BK_OK)
    {
        return BK_FAIL;
    }

    previous_tick = pm_cp1_aon_rtc_counter_get();
    current_tick = previous_tick;
    while((current_tick - previous_tick) < (PM_SEND_CMD_CP1_RESPONSE_TIEM*PM_AON_RTC_DEFAULT_TICK_COUNT))
    {
        if (bk_pm_ap_enter_deepsleep_state_get()) // wait the cp0 response
        {
            break;
        }
        current_tick = pm_cp1_aon_rtc_counter_get();
    }

    if(!bk_pm_ap_enter_deepsleep_state_get())
    {
        LOGE("set sleep mode time out\r\n");
    }
    #endif//CONFIG_MAILBOX

	return BK_OK;
}
static bk_err_t pm_wakeup_source_config(pm_sleep_mode_e sleep_mode,pm_wakeup_source_e wakeup_source,uint32_t data)
{
    #if CONFIG_MAILBOX
    uint64_t previous_tick  = 0;
    uint64_t current_tick   = 0;
    int ret                 = 0;
    bk_pm_ap_wakeup_source_config_state_set(PM_MAILBOX_COMMUNICATION_INIT);
	LOGD("wakeup data:%d\r\n",data);
    ret = pm_cp1_mailbox_send_data(PM_WAKEUP_CONFIG_CMD, sleep_mode,wakeup_source,data);
    if(ret != BK_OK)
    {
        return BK_FAIL;
    }

    previous_tick = pm_cp1_aon_rtc_counter_get();
    current_tick = previous_tick;
    while((current_tick - previous_tick) < (PM_SEND_CMD_CP1_RESPONSE_TIEM*PM_AON_RTC_DEFAULT_TICK_COUNT))
    {
        if (bk_pm_ap_wakeup_source_config_state_get()) // wait the cp0 response
        {
            break;
        }
        current_tick = pm_cp1_aon_rtc_counter_get();
    }

    if(!bk_pm_ap_wakeup_source_config_state_get())
    {
        LOGE("Wakeup src cfg time out\r\n");
    }

    #endif//CONFIG_MAILBOX
	return BK_OK;
}
bk_err_t bk_pm_ap_rtc_wakeup_source_config(pm_sleep_mode_e sleep_mode,pm_wakeup_source_e wakeup_source,pm_rtc_wakeup_config_t *prtc_wakeup_cfg)
{
	int ret =  BK_OK;
	if(prtc_wakeup_cfg == NULL)
	{
		return BK_FAIL;
	}
	ret = pm_wakeup_source_config(sleep_mode,wakeup_source,prtc_wakeup_cfg->rtc_period);
	return ret;
}
bk_err_t bk_pm_ap_gpio_wakeup_source_config(pm_sleep_mode_e sleep_mode,pm_wakeup_source_e wakeup_source,pm_gpio_wakeup_config_t* pgpio_wakeup_cfg)
{
	int ret =  BK_OK;
	if(pgpio_wakeup_cfg == NULL)
	{
		return BK_FAIL;
	}
	uint32_t data = pgpio_wakeup_cfg->gpio_id & 0xFFFF;
	data |= pgpio_wakeup_cfg->int_type << 16;
	ret = pm_wakeup_source_config(sleep_mode,wakeup_source,data);
	return ret;
}
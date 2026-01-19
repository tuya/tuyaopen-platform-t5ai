#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "driver/low_pwr_core.h"
#include <os/mem.h>
#include "low_pwr_misc.h"
#include <driver/pwr_clk.h>

/*=====================DEFINE  SECTION  START=====================*/

#define TAG "pwr_core"

#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define LOW_PWR_CORE_STACK_SIZE              (1536)
#define LOW_PWR_CORE_QUEUE_NUMBER_OF_MESSAGE (10)

/*=====================DEFINE  SECTION  END=====================*/


/*=====================STRUCT AND ENUM  SECTION  START==========*/
typedef struct
{
    beken_thread_t thd;
    beken_queue_t queue;
}low_pwr_core_info_t;
/*=====================STRUCT AND ENUM  SECTION  END=============*/


/*=====================VARIABLE  SECTION  START==================*/
static low_pwr_core_info_t *s_pm_info = NULL;

/*=====================VARIABLE  SECTION  END===================*/

/*================FUNCTION DECLARATION  SECTION  START==========*/


/*================FUNCTION DECLARATION  SECTION  END===========*/
static void low_pwr_core_rtc_callback(aon_rtc_id_t id, uint8_t *name_p, void *param)
{
	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_LV_WAKEUP,0x0,0x0);
	low_pwr_core_msg_t msg = {0};
	msg.event= LOW_PWR_CORE_RTC_WAKEUPED;
	bk_low_pwr_core_send_msg(&msg);
	LOGD("rtc_cb[%d]\r\n",bk_pm_exit_low_vol_wakeup_source_get());
	bk_pm_cp0_response_cp1(PM_SLEEP_WAKEUP_NOTIFY_CMD,PM_MODE_LOW_VOLTAGE,PM_WAKEUP_SOURCE_INT_RTC,0);
}
static void low_pwr_core_gpio_callback(gpio_id_t gpio_id)
{
	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_LV_WAKEUP,0x0,0x0);
	LOGD("gpio_cb[%d][%d]\r\n",bk_pm_exit_low_vol_wakeup_source_get(),gpio_id);
	low_pwr_core_msg_t msg = {0};
	msg.event= LOW_PWR_CORE_GPIO_WAKEUPED;
	bk_low_pwr_core_send_msg(&msg);
	bk_pm_cp0_response_cp1(PM_SLEEP_WAKEUP_NOTIFY_CMD,PM_MODE_LOW_VOLTAGE,PM_WAKEUP_SOURCE_INT_GPIO,gpio_id);
}
static bk_err_t low_pwr_core_rtc_wakeup_config(low_pwr_core_msg_t* msg)
{
	bk_err_t ret = BK_OK;
	pm_rtc_wakeup_config_t *rtc_cfg = (pm_rtc_wakeup_config_t*)msg->param3;

#if CONFIG_AON_RTC || CONFIG_ANA_RTC
	alarm_info_t lv_alarm = {
						"lv_rtc",
						(rtc_cfg->rtc_period)*AON_RTC_MS_TICK_CNT,
						rtc_cfg->rtc_cnt,
						low_pwr_core_rtc_callback,
						NULL
						};

	bk_alarm_unregister(AON_RTC_ID_1, lv_alarm.name);
	bk_alarm_register(AON_RTC_ID_1, &lv_alarm);
#endif //CONFIG_AON_RTC
	bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);
	return ret;
}
static bk_err_t low_pwr_core_gpio_wakeup_config(low_pwr_core_msg_t* msg)
{
	bk_err_t ret = BK_OK;
	int gpio_id;
	gpio_int_type_t int_type;
	if(msg == NULL)
	{
		return BK_FAIL;
	}
	gpio_id = msg->param3&0xFFFF;
	int_type = (msg->param3 >> 16)&0xFFFF;
	LOGD("gpio cfg[%d][%d]\r\n",gpio_id,int_type);
	#if CONFIG_GPIO_WAKEUP_SUPPORT || CONFIG_ANA_GPIO
	bk_gpio_register_isr(gpio_id , low_pwr_core_gpio_callback);
	bk_gpio_register_wakeup_source(gpio_id ,int_type);
	bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_GPIO, NULL);
	#endif //CONFIG_GPIO_WAKEUP_SUPPORT
	return ret;
}
static bk_err_t low_pwr_core_wakeup_src_cfg_handle(low_pwr_core_msg_t* msg)
{
	bk_err_t ret = BK_OK;
	if(msg == NULL)
	{
		return BK_FAIL;
	}
	pm_sleep_mode_e sleep_mode = msg->param1;
	pm_wakeup_source_e wakeup_source = msg->param2;

	switch(sleep_mode)
	{
		case PM_MODE_LOW_VOLTAGE:
			if(wakeup_source == WAKEUP_SOURCE_INT_RTC)
			{
				low_pwr_core_rtc_wakeup_config(msg);
			}
			else if(wakeup_source == WAKEUP_SOURCE_INT_GPIO)
			{
				low_pwr_core_gpio_wakeup_config(msg);
			}
		break;
		case PM_MODE_DEEP_SLEEP:
			if(wakeup_source == WAKEUP_SOURCE_INT_RTC)
			{
				low_pwr_core_rtc_wakeup_config(msg);
			}
			else if(wakeup_source == WAKEUP_SOURCE_INT_GPIO)
			{
				low_pwr_core_gpio_wakeup_config(msg);
			}
		break;
        default:
			break;
	}

	return ret;
}
bk_err_t bk_low_pwr_core_send_msg(low_pwr_core_msg_t *msg)
{
    bk_err_t ret = BK_OK;
	if(msg == NULL || s_pm_info == NULL)
	{
		LOGE("Pm core send msg error\r\n");
		return BK_FAIL;
	}

    if (s_pm_info->queue)
    {
        ret = rtos_push_to_queue(&s_pm_info->queue, msg, BEKEN_NO_WAIT);

        if (BK_OK != ret)
        {
            LOGE("%s failed\n", __func__);
            return BK_FAIL;
        }

        return ret;
    }

    return ret;
}
static bk_err_t low_pwr_core_message_handle(void)
{
    bk_err_t ret = BK_OK;
    low_pwr_core_msg_t msg;

    while (1)
    {
        ret = rtos_pop_from_queue(&s_pm_info->queue, &msg, BEKEN_WAIT_FOREVER);
		LOGV("LP event:%d\n", msg.event);
        if (kNoErr == ret)
        {
            switch (msg.event)
            {
                case LOW_PWR_CORE_STATE_ENTER_DEEPSLEEP:
                {
					bk_pm_cp0_response_cp1(PM_ENTER_DEEP_SLEEP_CMD,BK_OK,0,0);
					bk_pm_sleep_mode_set(msg.param1);
                }
                break;
				case LOW_PWR_CORE_CTRL_CP2_STATE:
				{
					bk_pm_cp0_response_cp1(PM_CTRL_AP_STATE_CMD, BK_OK,0,0);
					bk_pm_module_vote_boot_cp1_ctrl(msg.param1,msg.param2);
				}
				break;
				case LOW_PWR_CORE_CP2_RECOVERY:
				{
					bk_pm_cp1_recovery_module_state_ctrl(msg.param1,msg.param2);
				}
				break;
				case LOW_PWR_CORE_RTC_DEEPSLEEP:
				{
					bk_pm_cp0_response_cp1(PM_RTC_DEEPSLEEP_CMD, BK_OK,0,0);
					bk_low_pwr_misc_rtc_enter_deepsleep(msg.param2,NULL);
                }
				break;
				case LOW_PWR_CORE_GET_CP_DATA:
				{
					uint32_t cp_pm_data = 0;
					pm_ap_get_cp_data_type_e data_type = msg.param1;
					switch(data_type)
					{
						case PM_CP_DATE_TYPE_TIME_INTERVAL_FROM_STARTUP:
							bk_low_pwr_misc_get_time_interval_from_startup(&cp_pm_data);
						break;
						case PM_CP_DATE_TYPE_DEEP_SLEEP_WAKEUP_SOURCE:
							cp_pm_data = bk_pm_deep_sleep_wakeup_source_get();
						break;
						case PM_CP_DATE_TYPE_EXIT_LOW_VOL_WAKEUP_SOURCE:
							cp_pm_data =  bk_pm_exit_low_vol_wakeup_source_get();
						break;
						default:
						break;
					}
					bk_pm_cp0_response_cp1(PM_GET_PM_DATA_CMD,data_type,cp_pm_data,0);
				}
				break;
				case LOW_PWR_CORE_PSRAM_POWER:
				{
					ret = bk_pm_module_vote_psram_ctrl(msg.param1,msg.param2);
					bk_pm_cp0_response_cp1(PM_CTRL_PSRAM_POWER_CMD, msg.param2,0,0);
				}
				break;
				case LOW_PWR_CORE_POWER_CTRL:
				{
					ret = bk_pm_module_vote_power_ctrl(msg.param1,msg.param2);
					bk_pm_cp0_response_cp1(PM_POWER_CTRL_CMD,ret,0,0);
				}
				break;
				case LOW_PWR_CORE_CLK_CTRL:
				{
					ret = bk_pm_clock_ctrl(msg.param1,msg.param2);
					bk_pm_cp0_response_cp1(PM_CLK_CTRL_CMD, ret,0,0);
				}
				break;
				case LOW_PWR_CORE_SLEEP_CTRL:
				{
					//bk_pm_cp0_response_cp1(PM_SLEEP_CTRL_CMD,BK_OK,0,0);
					ret = bk_pm_module_vote_sleep_ctrl(msg.param1,msg.param2,msg.param3);
				}
				break;
				case LOW_PWR_CORE_FREQ_CTRL:
				{
					ret = bk_pm_module_vote_cpu_freq(msg.param1,msg.param2);
					bk_pm_cp0_response_cp1(PM_CPU_FREQ_CTRL_CMD, ret,0,0);
				}
				break;
				case LOW_PWR_CORE_EXTERNAL_LDO:
				{
					ret = bk_pm_module_vote_ctrl_external_ldo(msg.param1,msg.param2,msg.param3);
					bk_pm_cp0_response_cp1(PM_CTRL_EXTERNAL_LDO_CMD, ret,0,0);
				}
				break;
				case LOW_PWR_CORE_WAKEUP_SRC_CFG:
				{
					bk_pm_cp0_response_cp1(PM_WAKEUP_CONFIG_CMD, ret,0,0);
					low_pwr_core_wakeup_src_cfg_handle(&msg);
				}
				break;
				case LOW_PWR_CORE_RTC_WAKEUPED:
				{
					if(!bk_pm_cp1_work_state_get())
					{
						bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_APP,PM_POWER_MODULE_STATE_ON);
					}
				}
				break;
				case LOW_PWR_CORE_GPIO_WAKEUPED:
				{
					if(!bk_pm_cp1_work_state_get())
					{
						bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_APP,PM_POWER_MODULE_STATE_ON);
					}
				}
				break;
                default:
                    break;
            }
        }
    }

	return  ret;
}

bk_err_t bk_low_pwr_core_init(void)
{
    bk_err_t ret = BK_OK;

    if (s_pm_info == NULL)
    {
        s_pm_info = os_malloc(sizeof(low_pwr_core_info_t));

        if (s_pm_info == NULL)
        {
            LOGE("%s, malloc pm_info failed\n", __func__);
            goto error;
        }

        os_memset(s_pm_info, 0, sizeof(low_pwr_core_info_t));
    }

    if (s_pm_info->queue != NULL)
    {
        ret = BK_FAIL;
        LOGE("%s, pm_info->queue allready init, exit!\n", __func__);
        goto error;
    }

    if (s_pm_info->thd != NULL)
    {
        ret = BK_FAIL;
        LOGE("%s, pm_info->thd allready init, exit!\n", __func__);
        goto error;
    }

    ret = rtos_init_queue(&s_pm_info->queue,
                          "pm_info->queue",
                          sizeof(low_pwr_core_msg_t),
                          LOW_PWR_CORE_QUEUE_NUMBER_OF_MESSAGE);

    if (ret != BK_OK)
    {
        LOGE("%s,ceate queue failed\n");
        goto error;
    }

    ret = rtos_create_thread(&s_pm_info->thd,
                             BEKEN_DEFAULT_WORKER_PRIORITY - 2,/*pm contrl cmd thread priority need higher*/
                             "pm_info->thd",
                             (beken_thread_function_t)low_pwr_core_message_handle,
                             LOW_PWR_CORE_STACK_SIZE,
                             NULL);
    if (ret != BK_OK)
    {
        LOGE("create thread fail\n");
        goto error;
    }

    LOGE("%s success\n", __func__);

    return ret;

error:

    LOGE("%s fail\n", __func__);
	if(s_pm_info->queue != NULL)
	{
		rtos_deinit_queue(&s_pm_info->queue);
	}
	if(s_pm_info->thd != NULL)
	{
		rtos_delete_thread(&s_pm_info->thd);
	}
	os_free(s_pm_info);
	return BK_FAIL;
}


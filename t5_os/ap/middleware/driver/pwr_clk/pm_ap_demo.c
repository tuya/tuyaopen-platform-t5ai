#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <driver/mailbox.h>
#include "cli.h"
#include <driver/pwr_clk.h>
#include <modules/pm.h>
#include "driver/pm_ap_core.h"

/*=====================DEFINE  SECTION  START=====================*/
#define TAG "pm_demo"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define PM_DEMO_STACK_SIZE              (1024)
#define PM_DEMO_QUEUE_NUMBER_OF_MESSAGE (10)

/*=====================DEFINE  SECTION  END=====================*/
/*=====================STRUCT AND ENUM  SECTION  START==========*/
typedef enum
{
   PM_DEMO_ENTER_LOW_VOLTAGE = 0,
   PM_DEMO_ENTER_DEEP_SLEEP,
}pm_demo_sleep_mode_e;
/*=====================STRUCT AND ENUM  SECTION  END=============*/
/*=====================VARIABLE  SECTION  START==================*/
static  beken_thread_t s_thd;
static  beken_queue_t  s_queue;
/*=====================VARIABLE  SECTION  END==================*/

/*================FUNCTION DECLARATION  SECTION  START==========*/
extern void rtos_set_user_app_entry(beken_thread_function_t entry);
extern bk_err_t bk_pm_module_vote_boot_cp1_ctrl(pm_boot_cp1_module_name_e module,pm_power_module_state_e power_state);
/*================FUNCTION DECLARATION  SECTION  END===========*/
static bk_err_t pm_demo_sleep_wakeup_callback(void* param1,uint32_t param2)
{
    LOGD("%s\r\n",__func__);
    return BK_OK;
}
static bk_err_t pm_demo_cpu1_shutdown_callback(void* param1,uint32_t param2)
{
    bk_pm_cp1_recovery_response(PM_CP1_RECOVERY_CMD, PM_CP1_PREPARE_CLOSE_MODULE_NAME_APP, PM_CP1_MODULE_RECOVERY_STATE_FINISH);
    return BK_OK;
}
static bk_err_t pm_demo_psram_power_on_callback(uint32_t param1,uint32_t param2)
{
    LOGD("%s\r\n",__func__);
    return BK_OK;
}
static bk_err_t pm_demo_psram_power_off_callback(uint32_t param1,uint32_t param2)
{
    LOGD("%s\r\n",__func__);
    return BK_OK;
}
static bk_err_t pm_demo_init()
{
    /* resource recovery in A cpu*/
    int param = 5;
    pm_ap_close_ap_callback_info_t cb_info = {PM_AP_CLOSE_AP_MODULE_APP,pm_demo_cpu1_shutdown_callback,&param,param};
    bk_pm_ap_close_ap_register_callback(&cb_info);
    bk_pm_cp1_recovery_response(PM_CP1_RECOVERY_CMD, PM_CP1_PREPARE_CLOSE_MODULE_NAME_APP,PM_CP1_MODULE_RECOVERY_STATE_INIT);

    //pm_ap_system_wakeup_cb_info_t cb_info_sleep_wakeup = {PM_AP_CLOSE_AP_MODULE_APP,pm_demo_sleep_wakeup_callback,&param,param};
    //bk_pm_ap_system_wakeup_register_callback(&cb_info_sleep_wakeup);

    /*aov,wifi vote cp1 power*/
    rtos_delay_milliseconds(2);
    bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_AOV,PM_POWER_MODULE_STATE_ON);
    rtos_delay_milliseconds(2);
    bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_WIFI,PM_POWER_MODULE_STATE_ON);

    pm_ap_psram_power_state_callback_info_t  power_state_cb = {0};
    power_state_cb.dev_id = PM_AP_USING_PSRAM_POWER_STATE_DEV_MEDIA;
    power_state_cb.psram_off_cb_fn = pm_demo_psram_power_off_callback;
    power_state_cb.psram_on_cb_fn = pm_demo_psram_power_on_callback;
    power_state_cb.param1 = 0;
    power_state_cb.param2 = 0;
    bk_pm_ap_psram_power_state_register_callback(&power_state_cb);

    return BK_OK;
}
bk_err_t bk_pm_demo_send_msg(pm_ap_core_msg_t *msg)
{
    bk_err_t ret = BK_OK;
	if(msg == NULL)
	{
	    LOGE("Pm core send msg error\r\n");
		return BK_FAIL;
	}

    if (s_queue)
    {
        ret = rtos_push_to_queue(&s_queue, msg, BEKEN_NO_WAIT);

        if (BK_OK != ret)
        {
            LOGE("%s failed\n", __func__);
            return BK_FAIL;
        }
        return ret;
    }

    return ret;
}
//pm_rtc_wakeup_config_t rtc_wakeup = {0};
//pm_gpio_wakeup_config_t gpio_wakeup= {GPIO_20,GPIO_INT_TYPE_HIGH_LEVEL};
static bk_err_t pm_demo_message_handle(void)
{
    bk_err_t ret = BK_OK;
    pm_ap_core_msg_t msg;

	pm_demo_init();

    while (1)
    {
        ret = rtos_pop_from_queue(&s_queue, &msg, BEKEN_WAIT_FOREVER);
        LOGD("%s event:%d\n", __func__,msg.event);
        if (kNoErr == ret)
        {
            switch (msg.event)
            {
                case PM_DEMO_ENTER_LOW_VOLTAGE:
                {
					/*config rtc wakeup source*/
					pm_rtc_wakeup_config_t rtc_wakeup = {0};
					rtc_wakeup.rtc_period = 10*1000;//10s
					bk_pm_ap_rtc_wakeup_source_config(PM_MODE_LOW_VOLTAGE,WAKEUP_SOURCE_INT_RTC,&rtc_wakeup);

					/*config gpio wakeup source*/
					pm_gpio_wakeup_config_t gpio_wakeup= {GPIO_20,GPIO_INT_TYPE_HIGH_LEVEL};
					bk_pm_ap_gpio_wakeup_source_config(PM_MODE_LOW_VOLTAGE,WAKEUP_SOURCE_INT_GPIO,&gpio_wakeup);

					/*AOV vote close cp1*/
					bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_AOV,PM_POWER_MODULE_STATE_OFF);
					//rtos_delay_milliseconds(2);

					/*WIFI vote close cp1*/
					bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_WIFI,PM_POWER_MODULE_STATE_OFF);
					//rtos_delay_milliseconds(2);

					/*multimedia vote close cp1*/
					bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_APP,PM_POWER_MODULE_STATE_OFF);

					/*Close cpu2*/
					extern void stop_cpu2_core(void);
					stop_cpu2_core();

					/*APP vote enter low voltage*/
					bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x1,0x0);
                }
                break;
                case PM_DEMO_ENTER_DEEP_SLEEP:
                {
					/*config rtc wakeup source*/
					pm_rtc_wakeup_config_t rtc_wakeup = {0};
					rtc_wakeup.rtc_period = 10*1000;//10s
					bk_pm_ap_rtc_wakeup_source_config(PM_MODE_LOW_VOLTAGE,WAKEUP_SOURCE_INT_RTC,&rtc_wakeup);

					/*config gpio wakeup source*/
					pm_gpio_wakeup_config_t gpio_wakeup= {GPIO_20,GPIO_INT_TYPE_HIGH_LEVEL};
					bk_pm_ap_gpio_wakeup_source_config(PM_MODE_LOW_VOLTAGE,WAKEUP_SOURCE_INT_GPIO,&gpio_wakeup);

					/*AOV vote close cp1*/
					bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_AOV,PM_POWER_MODULE_STATE_OFF);
					//rtos_delay_milliseconds(2);

					/*WIFI vote close cp1*/
					bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_WIFI,PM_POWER_MODULE_STATE_OFF);
					//rtos_delay_milliseconds(2);

					/*multimedia vote close cp1*/
					bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_MULTIMEDIA,PM_POWER_MODULE_STATE_OFF);

					/*Enter deep sleep*/
					bk_pm_ap_sleep_mode_set(PM_MODE_DEEP_SLEEP);
                }
                break;
                default:
                    break;
            }
        }
    }

	return  ret;
}

bk_err_t pm_demo_thread_main(void)
{
    bk_err_t ret = BK_OK;
	ret = rtos_init_queue(&s_queue,
                          "demo_queue",
                          sizeof(pm_ap_core_msg_t),
                          PM_DEMO_QUEUE_NUMBER_OF_MESSAGE);

    if (ret != BK_OK)
    {
        LOGE("create pm demo que fail\n");
    }
	ret = rtos_create_thread(&s_thd,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "pm_demo_thd",
                             (beken_thread_function_t)pm_demo_message_handle,
                             PM_DEMO_STACK_SIZE,
                             NULL);
    if (ret != BK_OK)
    {
        LOGE("create pm demo thrd fail\n");
    }
	return 0;
}
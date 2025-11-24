#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "driver/pm_ap_core.h"
#include <os/mem.h>
#include "FreeRTOS.h"

/*=====================DEFINE  SECTION  START=====================*/

#define TAG "pwr_core"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define PM_AP_CORE_STACK_SIZE              (1536)
#define PM_AP_CORE_QUEUE_NUMBER_OF_MESSAGE (10)

/*=====================DEFINE  SECTION  END=====================*/


/*=====================STRUCT AND ENUM  SECTION  START==========*/
typedef struct
{
    beken_thread_t thd;
    beken_queue_t queue;
}pm_ap_core_info_t;
/*=====================STRUCT AND ENUM  SECTION  END=============*/


/*=====================VARIABLE  SECTION  START==================*/
static pm_ap_core_info_t *s_pm_info = NULL;

/*=====================VARIABLE  SECTION  END===================*/

/*================FUNCTION DECLARATION  SECTION  START==========*/


/*================FUNCTION DECLARATION  SECTION  END===========*/

bk_err_t bk_pm_ap_core_send_msg(pm_ap_core_msg_t *msg)
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

static bk_err_t pm_ap_core_message_handle(void)
{
    bk_err_t ret = BK_OK;
    pm_ap_core_msg_t msg;

    while (1)
    {
        ret = rtos_pop_from_queue(&s_pm_info->queue, &msg, BEKEN_WAIT_FOREVER);
        //LOGD("%s event:%d,param:%d,%d,%d\n", __func__,msg.event,msg.param1,msg.param2,msg.param3);
        if (kNoErr == ret)
        {
            switch (msg.event)
            {
                case PM_AP_CORE_STATE_ENTER_DEEPSLEEP:
                {
                }
                break;
                case PM_AP_CORE_AP_RECOVERY:
                {
                    bk_pm_ap_close_ap_handle_callback();
                }
                break;
                case PM_AP_CORE_SLEEP_WAKEUP_NOTIFY:
                {
                    // bk_err_t ret = portYIELD_CORE(1);
                    // while(ret != BK_OK)
                    // {
                    //     ret = portYIELD_CORE(1);
                    //     LOGE("Wakeup cpu2 failed[%d]\r\n",ret);
                    // }
                    FIXED_ADDR_WAKEUP_AP0_COUNT += 1;
                    bk_pm_ap_system_wakeup_handle_callback(&msg);
                }
                break;
                case PM_AP_CORE_PSRAM_STATE_NOTIFY:
                {
                    if(msg.param1 == PM_AP_PSRAM_POWER_OFF)
                    {
                        bk_pm_ap_psram_power_state_handle_callback(PM_POWER_PSRAM_MODULE_NAME_MAX,msg.param1);
                    }
                }
                break;
                case PM_AP_CORE_SLEEP_DEMO_HANDLE:
                {
                    if(msg.param1 == PM_MODE_LOW_VOLTAGE)
                    {
                        bk_pm_ap_sleep_mode_set(PM_MODE_DEFAULT);
                        bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x0,0x0);
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

bk_err_t bk_pm_ap_core_init(void)
{
    bk_err_t ret = BK_OK;

    if (s_pm_info == NULL)
    {
        s_pm_info = os_malloc(sizeof(pm_ap_core_info_t));

        if (s_pm_info == NULL)
        {
            LOGE("%s, malloc pm_info failed\n", __func__);
            goto error;
        }

        os_memset(s_pm_info, 0, sizeof(pm_ap_core_info_t));
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
                          sizeof(pm_ap_core_msg_t),
                          PM_AP_CORE_QUEUE_NUMBER_OF_MESSAGE);

    if (ret != BK_OK)
    {
        LOGE("%s,ceate queue failed\n");
        goto error;
    }

    ret = rtos_core0_create_thread(&s_pm_info->thd,
                             BEKEN_DEFAULT_WORKER_PRIORITY - 2,/*pm contrl cmd thread priority need higher*/
                             "pm_info->thd",
                             (beken_thread_function_t)pm_ap_core_message_handle,
                             PM_AP_CORE_STACK_SIZE,
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


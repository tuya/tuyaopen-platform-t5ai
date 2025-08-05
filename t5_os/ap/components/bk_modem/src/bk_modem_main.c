/**
 ****************************************************************************************
 *
 * @file bk_modem_main.c
 *
 * @brief Main thread + state machine control, etc., as the main controller.
 *
 ****************************************************************************************
 */
#include "bk_modem_main.h"
#include "bk_modem_dte.h"
#include "bk_modem_at_cmd.h"
#include "bk_modem_usbh_if.h"
#include "../../include/os/os.h"
#include "common/bk_err.h"
#include "../../include/driver/pwr_clk.h"
 
beken_thread_t bk_modem_thread = NULL;
beken_queue_t bk_modem_queue = NULL;

static enum bk_modem_state_e s_bk_modem_state = WAIT_MODEM_CONN;
struct bk_modem_env_s bk_modem_env;
static uint8_t bk_modem_status = 0;

static void bk_modem_thread_main(void *args)
{
    int ret;
    BUS_MSG_T msg;
    
    while (1) 
    {
        ret = rtos_pop_from_queue(&bk_modem_queue, &msg, BEKEN_WAIT_FOREVER);

        if (ret)
            continue;

        switch (msg.type)
        {
            case MSG_MODEM_CONN_IND:
            {
                bk_modem_dte_handle_conn_ind(&msg);
                break;
            }

            case MSG_MODEM_CHECK:
            {
                bk_modem_dte_handle_modem_check();
                break;
            }

            case MSG_PPP_START:
            {
                bk_modem_dte_handle_ppp_start();
                break;
            }

            case MSG_PPP_CONNECT_IND:
            {
                bk_modem_dte_handle_ppp_connect_ind();
                break;
            }
            
            case MSG_PPP_STATUS_IND:
            {
                bk_modem_dte_handle_ppp_stutus_ind(&msg);
                break;
            }

            case MSG_PPP_STOP:
            {
                bk_modem_dte_handle_ppp_stop(&msg);
                break;
            }

            case MSG_MODEM_DISC_IND:
            {
                bk_modem_dte_handle_disc_ind();
                break;
            }            

            case MSG_MODEM_USBH_POWER_ON:
            {
                rtos_delay_milliseconds(3000);
                bk_modem_power_on_modem();              
                bk_modem_usbh_poweron_ind();
                break;
            }  
            
            default:
            {
                BK_MODEM_LOGI("%s: error!", __func__);
                break;
            }
        }
    }

    rtos_delete_thread(&bk_modem_thread);
}

void bk_modem_del_resource(void)
{
    if (bk_modem_queue)
    {
        rtos_deinit_queue(&bk_modem_queue);
        bk_modem_queue = NULL;
    }
    
    if (bk_modem_thread)
    {
        rtos_delete_thread(&bk_modem_thread);
        bk_modem_thread = NULL;
    }

    bk_modem_at_dinit();
    bk_modem_usbh_close();
    bk_modem_power_off_modem();    
    bk_modem_status = 0;
    
    #if CONFIG_SYS_CPU1
    bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_BK_MODEM, PM_POWER_MODULE_STATE_OFF);
    #endif    
}

void bk_modem_deinit(void)
{
    BK_MODEM_LOGI("%s \r\n", __func__);

    if (bk_modem_status == 1)
    {
        bk_modem_set_state(PPP_STOP);

        BUS_MSG_T msg;
        msg.type = MSG_PPP_STOP;
        msg.arg = ACTIVE_STOP;
        msg.len = 0;
        msg.sema = NULL;
        msg.param = NULL;
        bk_modem_dte_handle_ppp_stop(&msg);
    }
    else
    {
        BK_MODEM_LOGE("bk modem already close, no need close again\r\n");
    }
}

bk_err_t bk_modem_init(void)
{
    int ret;

    BK_MODEM_LOGI("%s: status %d \r\n", __func__, bk_modem_status);

    if (bk_modem_status == 1)
    {
        BK_MODEM_LOGE("bk modem already start, no need start again\r\n");
        return BK_FAIL; 
    }

    // Create bk modem thread and initialize bk mode queue.
    if ((bk_modem_thread != NULL) || (bk_modem_queue != NULL))
    {
        goto thread_fail;
    }

    // Initialize bk modem queue
    ret = rtos_init_queue(&bk_modem_queue, "bk_modem_queue", sizeof(BUS_MSG_T), 16);
    if (ret != kNoErr)
    {
        goto thread_fail;
    }

    if (bk_modem_at_init() == BK_FAIL)
    {
        goto thread_fail;
    }

    // Create bk modem thread
    ret = rtos_create_thread(&bk_modem_thread, BEKEN_DEFAULT_WORKER_PRIORITY, "bk_modem",
                       bk_modem_thread_main, 2048, (beken_thread_arg_t)0);

    if (ret != kNoErr)
    {
        goto thread_fail;
    }

    bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_BK_MODEM, PM_POWER_MODULE_STATE_ON);

    bk_modem_send_msg(MSG_MODEM_USBH_POWER_ON, 0, 0, 0);

    bk_modem_status = 1;
    return BK_OK;
    
thread_fail:
    bk_modem_deinit();
    return BK_FAIL;    
}

void bk_modem_set_state(enum bk_modem_state_e bk_modem_state)
{
    s_bk_modem_state = bk_modem_state;
}

enum bk_modem_state_e bk_modem_get_state(void)
{
    return s_bk_modem_state;
}

int bk_modem_send_msg(int type, uint32_t arg, uint32_t len, void *param)
{
    BUS_MSG_T msg;
    int ret;

    if (bk_modem_queue == NULL)
    {
        BK_MODEM_LOGI("%s: queue is null!", __func__);
        return -1;
    }

    msg.type = type;
    msg.arg = (uint32_t)arg;
    msg.len = len;
    msg.sema = NULL;
    msg.param = (void *)param;

    ret = rtos_push_to_queue(&bk_modem_queue, &msg, BEKEN_NO_WAIT);
    if (ret)
        BK_ASSERT(0);

    return ret;
}

void bk_modem_power_on_modem(void)
{
    /// Maybe usb slave such as modem will be powered on by usb host 
    /// please use this interface to power on modem, if modem can be independent powered control
}

void bk_modem_power_off_modem(void)
{
    /// Maybe usb slave such as modem will be powered off by usb host 
    /// please use this interface to power off modem, if modem can be independent powered control  
}


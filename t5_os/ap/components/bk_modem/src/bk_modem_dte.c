/**
 ****************************************************************************************
 *
 * @file bk_modem_dte.c
 *
 * @brief MCU transmission and reception process.
 *
 ****************************************************************************************
 */
#include "bk_modem_main.h"
#include "bk_modem_dte.h"
#include "bk_modem_dce.h"
#include "bk_modem_netif.h"
#include "../../include/os/os.h"
#include "bk_modem_usbh_if.h"
#include "bk_modem_at_cmd.h"

void bk_modem_dte_send_data(uint32_t data_length, uint8_t *data, enum bk_modem_ppp_mode_e ppp_mode)
{
    if ((data_length == 0) || (data == NULL))
    {
        BK_MODEM_LOGE("%s:invalid data length\r\n",__func__);
        return;
    }

    if (bk_modem_env.bk_modem_ppp_mode == ppp_mode)
    {
        bk_modem_usbh_bulkout_ind((char *)data, data_length);
    }
    else
        BK_MODEM_LOGE("%s: different ppp mode. %d %d\r\n",__func__, bk_modem_env.bk_modem_ppp_mode, ppp_mode);
}

void bk_modem_dte_recv_data(uint32_t data_length, uint8_t *data)
{
    if ((data_length == 0) || (data == NULL))
    {
        BK_MODEM_LOGE("%s:invalid data input %d\r\n",__func__, data_length);
        return;
    }
    
    if (bk_modem_env.bk_modem_ppp_mode == PPP_CMD_MODE)
    {
        bk_modem_at_rcv_resp((char *)data, data_length);
    }
    else if (bk_modem_env.bk_modem_ppp_mode == PPP_DATA_MODE)
    {
        bk_modem_netif_lwip_ppp_input(data, data_length);
    }
    else
    {
        BK_MODEM_LOGE("%s:invalid ppp mode %d\r\n",__func__, bk_modem_env.bk_modem_ppp_mode);
    }
}

void bk_modem_dte_handle_conn_ind(BUS_MSG_T *msg)
{
    BK_MODEM_LOGI("%s: state %d, cnt %d\r\n", __func__, bk_modem_get_state(), msg->arg);

    bk_modem_env.port_num = (uint8_t)msg->arg;
    bk_modem_set_state(MODEM_CHECK);
    bk_modem_send_msg(MSG_MODEM_CHECK, 0,0,0);
    bk_modem_env.bk_modem_ppp_mode = PPP_INIT_MODE;
}

void bk_modem_dte_handle_modem_check(void)
{
    uint8_t temp_flag = 0xff;
    static uint8_t sim_check_cnt = 0;
    uint32_t retry_time = 0;
    do
    {
        // disc state will be set to wait modem conn
        if (bk_modem_get_state() != MODEM_CHECK)
        {
            BK_MODEM_LOGI("%s: not handle in state %d \r\n", __func__, bk_modem_get_state());
            return;
        }

        bk_modem_env.bk_modem_ppp_mode = PPP_CMD_MODE;
        if (!bk_modem_dce_send_at())
        {
            temp_flag = 1;
            break;
        }

        if (!bk_modem_dce_check_sim())
        {
            temp_flag = 2;
            sim_check_cnt++;
            break;
        }
        
        if (!bk_modem_dce_check_signal())
        {
            temp_flag = 3;
            break;
        }  
        
        if (!bk_modem_dce_check_register())
        {
            temp_flag = 4;
            break;
        }
        sim_check_cnt = 0;
        bk_modem_set_state(PPP_START);
        bk_modem_send_msg(MSG_PPP_START, 0,0,0);
        BK_MODEM_LOGI("%s: modem check pass\r\n", __func__);
        return;
        
    }while(0);

    retry_time = 3000;

    if ((temp_flag == 2) && (sim_check_cnt > 10))
    {
        if (!bk_modem_dce_enter_flight_mode())
        {
            temp_flag = 5;
            goto retry;
        }
        else
        {
            rtos_delay_milliseconds(1000);
        }

        if (!bk_modem_dce_exit_flight_mode())
        {
            temp_flag = 6;
            goto retry;
        }

        if (sim_check_cnt > 20)
        {
            retry_time = 10000; ///After trying 10 times flight mode, will change 10s dealy for sim-check.
        }
    }

retry:    
    BK_MODEM_LOGI("%s: modem check fail %d\r\n", __func__, temp_flag);
    if (temp_flag == 1)
    {
        bk_modem_usbh_close();
        bk_modem_power_off_modem();    
        rtos_delay_milliseconds(retry_time);
        bk_modem_set_state(WAIT_MODEM_CONN);
        bk_modem_power_on_modem();          
        bk_modem_usbh_poweron_ind();
    }
    else
    {
        rtos_delay_milliseconds(retry_time);
        bk_modem_set_state(MODEM_CHECK);
        bk_modem_send_msg(MSG_MODEM_CHECK, 0,0,0);
    }
}

void bk_modem_dte_handle_ppp_start(void)
{
    uint8_t temp_flag = 0xff;
    
    if (bk_modem_get_state() != PPP_START)
    {
        temp_flag = 0;
        goto retry;
    }

    if (bk_modem_env.bk_modem_ppp_mode == PPP_CMD_MODE)
    {
        if (!bk_modem_dce_set_apn())
        {
            temp_flag = 1;
            goto retry;
        }
        
        if (!bk_modem_dce_check_attach())
        {
            temp_flag = 2;
            goto enter_flight_mode;
        }  
        
        if (!bk_modem_dce_start_ppp())
        {
            temp_flag = 3;
            goto retry;
        }
        #if 0
        if (rtos_init_oneshot_timer(&bk_modem_timer, 5000, (timer_2handler_t)bk_modem_timer_cb_func, 0, 0))
        {
            temp_flag = 4;
            goto retry;
        }

        rtos_start_oneshot_timer(&bk_modem_timer);
        #endif        
        bk_modem_send_msg(MSG_PPP_CONNECT_IND, 0,0,0);
        bk_modem_env.bk_modem_ppp_mode = PPP_DATA_MODE;

        BK_MODEM_LOGI("%s: ppp dial ok&recv conncet\r\n", __func__);
        
        return;
    }
    else
    {
        temp_flag = 5;
        goto retry;
    }

enter_flight_mode:
    if (!bk_modem_dce_enter_flight_mode())
     {
         temp_flag = 6;
         goto retry;
     }
     else
     {
         rtos_delay_milliseconds(1000);
     }

     if (!bk_modem_dce_exit_flight_mode())
     {
         temp_flag = 7;
         goto retry;
     }

retry:
    BK_MODEM_LOGI("%s: ppp dail fail%d\r\n", __func__, temp_flag);    
    if ((temp_flag == 2) || (temp_flag == 3) || (temp_flag > 5))
    {
        bk_modem_usbh_close();
        bk_modem_power_off_modem();    
        rtos_delay_milliseconds(3000);
        bk_modem_set_state(WAIT_MODEM_CONN);
        bk_modem_power_on_modem();          
        bk_modem_usbh_poweron_ind();
    }
    else    
    {
        rtos_delay_milliseconds(3000);
        bk_modem_set_state(MODEM_CHECK);
        bk_modem_send_msg(MSG_MODEM_CHECK, 0,0,0);
    }
}

void bk_modem_dte_handle_ppp_connect_ind(void)
{
    uint32_t temp_flag = 0xff;

    if (bk_modem_get_state() != PPP_START)
    {
        temp_flag = 0;
        goto fail;
    }

    //rtos_stop_oneshot_timer(&bk_modem_timer);
    if (!bk_modem_env.is_ppp_started)
    {
        if (bk_modem_netif_new_ppp() != BK_OK)
        {
            temp_flag = 1;
            goto fail; 
        }
        bk_modem_netif_ppp_set_default_netif();
        bk_modem_env.is_ppp_started = true;
    }
    
    if (bk_modem_netif_start_ppp() != BK_OK)
    {
        temp_flag = 2;
        goto fail; 
    }
    BK_MODEM_LOGI("%s: ppp start \r\n", __func__);

    return;
    
fail:
    //rtos_delay_milliseconds(100);
    BK_MODEM_LOGI("%s: ppp start fail %d\r\n", __func__, temp_flag);

    bk_modem_set_state(PPP_STOP);
    bk_modem_send_msg(MSG_PPP_STOP, ABNORMAL_STOP,0,0);
}

void bk_modem_dte_handle_ppp_stutus_ind(BUS_MSG_T *msg)
{
    /// used for handling ppp status ind according to function on_ppp_status_changed
    
}

void bk_modem_dte_handle_ppp_stop(BUS_MSG_T *msg)
{
    uint32_t stop_reason = msg->arg;
    uint32_t temp_flag = 0xff;
    enum bk_modem_state_e old_state = bk_modem_get_state();

    if ((bk_modem_get_state() != PPP_STOP) || !bk_modem_env.is_ppp_started)
    {
        temp_flag = 0;
        goto fail;
    }

    bk_modem_env.is_ppp_started = false;
    
    if ((stop_reason == ACTIVE_STOP) || (stop_reason == ABNORMAL_STOP))
    {
        if (bk_modem_env.bk_modem_ppp_mode == PPP_DATA_MODE)
        {
            bk_modem_env.bk_modem_ppp_mode = PPP_CMD_MODE;
            if (!bk_modem_dce_enter_cmd_mode())
            {
                temp_flag = 1;
                //goto fail;
            }
            if (!bk_modem_dce_stop_ppp())
            {
                temp_flag = 2;
                //goto fail;
            }
            
            if (bk_modem_netif_stop_ppp() != BK_OK)
            {
                temp_flag = 3;
                //goto fail;
            }  

            bk_modem_netif_destroy_ppp();
            
            bk_modem_env.bk_modem_ppp_mode = PPP_INIT_MODE;
            if (stop_reason == ACTIVE_STOP)
            {
                bk_modem_del_resource();
                bk_modem_set_state(WAIT_MODEM_CONN);
            }
            else
            {
                bk_modem_set_state(MODEM_CHECK);
                bk_modem_send_msg(MSG_MODEM_CHECK, 0,0,0);
            }
        }
    }
    else if ((stop_reason == NO_CARRIER_STOP) || (stop_reason == DSIC_STOP))
    {
        if (bk_modem_netif_stop_ppp() != BK_OK)
        {
            temp_flag = 5;
            //goto fail;
        }  

        bk_modem_netif_destroy_ppp();

        if (stop_reason == NO_CARRIER_STOP)
        {
            bk_modem_set_state(MODEM_CHECK);
            bk_modem_send_msg(MSG_MODEM_CHECK, 0,0,0);
        }
        else if (stop_reason == DSIC_STOP)
        {
            bk_modem_set_state(WAIT_MODEM_CONN);       
        }
    }

    BK_MODEM_LOGI("%s: ppp stop reason %d, old_state %d state %d\r\n", __func__, stop_reason, old_state, bk_modem_get_state());

    return;

fail:
    BK_MODEM_LOGI("%s: ppp stop fail %d,%d\r\n", __func__, temp_flag,bk_modem_get_state());
    
    if ((temp_flag == 0) && !bk_modem_env.is_ppp_started && (stop_reason == ACTIVE_STOP))
    {
        bk_modem_del_resource();
    }
    
    bk_modem_env.bk_modem_ppp_mode = PPP_INIT_MODE;
    bk_modem_set_state(MODEM_CHECK);
}

void bk_modem_dte_handle_disc_ind(void)
{
    BK_MODEM_LOGI("%s: disc %d\r\n", __func__, bk_modem_get_state());
    if (bk_modem_get_state() == PPP_START)
    {
        bk_modem_set_state(PPP_STOP);
        bk_modem_send_msg(MSG_PPP_STOP, DSIC_STOP,0,0); 
    }
    else
    {
        bk_modem_set_state(WAIT_MODEM_CONN);
    }
}

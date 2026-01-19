/**
 ****************************************************************************************
 *
 * @file bk_modem_dte.c
 *
 * @brief Data Terminal Equipment (DTE) Module
 *        This module handles data transmission and reception processes between the MCU
 *        and the modem, including USB and UART communication channels, PPP protocol handling,
 *        and EC (Ethernet Controller) mode operations.
 *
 ****************************************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "bk_modem_main.h"
#include "bk_modem_dte.h"
#include "bk_modem_dce.h"
#include "bk_modem_netif.h"
#include "os/os.h"
#include "bk_modem_usbh_if.h"
#include "bk_modem_at_cmd.h"
#include "bk_modem_uart.h"
#include "modemif.h"
#include "net.h"
//#include <os/os.h>
#include <os/mem.h>
/**
 * @brief Semaphore for EC handshake synchronization
 *        This semaphore is used to synchronize the handshake process in EC (Ethernet Controller) mode.
 */
beken_semaphore_t bk_modem_ec_hs_sema = NULL;

/**
 * @brief EC handshake status flag
 *        Tracks the current state of the EC handshake process:
 *        0 = Not initialized, 1 = Handshake requested, 2 = Handshake completed successfully
 */
uint8_t bk_modem_ec_hs = 0;

extern uint8_t bk_modem_mf_test;
extern xSemaphoreHandle bk_modem_mf_test_sem;

static void bk_modem_dte_output(struct netif *netif, struct pbuf *p)
{
    BK_ASSERT(p->next == NULL);
    //pbuf_ref(p); for flow control
    bk_modem_dte_send_data_uart(p->len, p->payload, NIC_DATA_MODE);
}

/**
 * @brief Send EC handshake request data
 *        This function initiates the EC handshake process by sending a request packet
 *        and setting the handshake state to requested.
 */
static void bk_modem_dte_ec_send_hs_data(void)
{
    bk_modem_ec_hs = 1;

    uint8_t hs_req[12] = {0x53,0x50,0x49,0x43,0x02,0x04,0x0C,0x00,0x00,0x00,0x00,0x00};
    
    bk_modem_dte_send_data_uart(12, &hs_req[0], NIC_DATA_MODE);
}

/**
 * @brief Validate EC handshake response
 * @param data_length Length of the received data
 * @param data Pointer to the received data buffer
 *        This function verifies if the received data matches the expected EC handshake response.
 *        If validation fails, it resets the handshake state and transitions the modem to check state.
 */
static void bk_modem_dte_ec_check_hs_data(uint32_t data_length, uint8_t *data)
{
    uint8_t hs_resp[6] = {0x53,0x50,0x49,0x43,0x01,0x80};

    if ((data_length == 32) && (bk_modem_ec_hs == 1))
    {
        BK_MODEM_LOGI("%s: data length is 32 discard\r\n",__func__);
        return;
    }
    rtos_set_semaphore(&bk_modem_ec_hs_sema);
    for (uint8_t i = 0; i < 6; i++)
    {
        BK_MODEM_LOGI("%s: data is %d, hs_resp is %d\r\n",__func__, data[i], hs_resp[i]);
        if(hs_resp[i] != data[i])
        {
            bk_modem_ec_hs = 0;
            bk_modem_set_state(MODEM_CHECK);
            bk_modem_send_msg(MSG_MODEM_CHECK, 0,0,0);
            BK_MODEM_LOGW("%s:invalid hs Data pos is %d\r\n",__func__, i);
            return;
        }
    }

    bk_modem_ec_hs = 2;

    net_modem_init();
    bk_modemif_register_callback(bk_modem_dte_output);

    modem_ip_start();
}

/**
 * @brief Send data through the configured communication interface
 * @param data_length Length of data to send
 * @param data Pointer to the data buffer
 * @param ppp_mode PPP mode (command or data)
 *        This function sends data through the configured interface (USB) based on the
 *        current PPP mode setting. It performs input validation and logs errors for invalid parameters.
 */
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

/**
 * @brief Send data through UART interface
 * @param data_length Length of data to send
 * @param data Pointer to the data buffer
 * @param uart_trx_mode UART transaction mode (AT command or NIC data)
 *        This function sends data through the UART interface using the specified transaction mode.
 *        It performs input validation and logs errors for invalid parameters.
 */
void bk_modem_dte_send_data_uart(uint32_t data_length, uint8_t *data, enum bk_modem_uart_trx_mode_e uart_trx_mode)
{
    if ((data_length == 0) || (data == NULL))
    {
        BK_MODEM_LOGE("%s:invalid data length\r\n",__func__);
        return;
    }

    bk_modem_uart_data_send(data_length, data, uart_trx_mode);
}

/**
 * @brief Receive and process data from modem
 * @param data_length Length of received data
 * @param data Pointer to the received data buffer
 *        This function handles data received from the modem and routes it to the appropriate
 *        handler based on the current PPP mode (AT command processing or PPP network stack).
 */
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

/**
 * @brief Receive and process data from UART interface
 * @param data_length Length of received data
 * @param data Pointer to the received data buffer
 * @param data_type Type of data received (AT command or NIC data)
 *        This function handles data received through UART and routes it to the appropriate
 *        handler based on the data type, including special handling for EC handshake responses.
 */
void bk_modem_dte_recv_data_uart(uint32_t data_length, uint8_t *data, uint8_t data_type)
{
    if ((data_length == 0) || (data == NULL))
    {
        BK_MODEM_LOGE("%s:invalid data input %d\r\n",__func__, data_length);
        return;
    }
    
    if (data_type == AT_CMD_MODE)
    {
        bk_modem_at_rcv_resp((char *)data, data_length);
    }
    else if (data_type == NIC_DATA_MODE)
    {
        if (bk_modem_ec_hs == 1)
        {
            bk_modem_dte_ec_check_hs_data(data_length, data);
        }
        else if (bk_modem_ec_hs == 2)
        {
            struct netif *netif = net_get_modem_handle();
            if (netif)
            {
                struct pbuf *p = pbuf_alloc(PBUF_RAW, data_length, PBUF_POOL);
                        
                if (p)
                {
                    os_memcpy(p->payload, data, data_length);
                    p->len = data_length;
                    p->tot_len = data_length;
                    modemif_input(netif, p);
                }
                else
                {
                    BK_MODEM_LOGE("%s:pbuf alloc failed\r\n",__func__);
                }                
            }
            else
            {
                BK_MODEM_LOGE("%s:netif is null\r\n",__func__);
            }
        }
    }
    else
    {
        BK_MODEM_LOGE("%s:mode %d\r\n",__func__, data_type);
    }
}

/**
 * @brief Handle modem connection indication
 * @param msg Pointer to the bus message
 *        This function processes the modem connection indication message, transitions
 *        the modem to check state, and initializes the PPP mode to initial state.
 */
void bk_modem_dte_handle_conn_ind(BUS_MSG_T *msg)
{
    BK_MODEM_LOGI("%s: state %d, cnt %d, comm_if %d\r\n", __func__, bk_modem_get_state(), msg->arg, bk_modem_env.comm_if);

    if (bk_modem_env.comm_if == USB_IF)
    {
        bk_modem_set_state(MODEM_CHECK);
        bk_modem_send_msg(MSG_MODEM_CHECK, 0,0,0);
        bk_modem_env.bk_modem_ppp_mode = PPP_INIT_MODE;
    }
}

/**
 * @brief Perform modem health and connectivity checks
 *        This function executes a series of checks to verify modem readiness, including:
 *        - AT command responsiveness
 *        - SIM card detection
 *        - Signal strength validation
 *        - Network registration status
 *        - Packet domain attachment
 *        It handles retries and recovery procedures for different failure scenarios.
 */
void bk_modem_dte_handle_modem_check(void)
{
    uint8_t temp_flag = 0xff;
    static uint8_t sim_check_cnt = 0;
    uint32_t retry_time = 1000;
    do
    {
        // Check if we're in the correct state to perform modem checks
        if (bk_modem_get_state() != MODEM_CHECK)
        {
            BK_MODEM_LOGI("%s: not handle in state %d \r\n", __func__, bk_modem_get_state());
            return;
        }

        if (bk_modem_env.comm_proto == PPP_MODE)
        {
            bk_modem_env.bk_modem_ppp_mode = PPP_CMD_MODE;
        }
        
        // Sequence of modem health checks
        if (!bk_modem_dce_send_at())
        {
            temp_flag = 1;
            break;
        }

		if (!bk_modem_mf_test) {
#if 0            
            if (!bk_modem_dce_get_cgsn()) {
                temp_flag = 9;
                //break;
            }

            if (!bk_modem_dce_get_cfsn()) {
                temp_flag = 10;
                //break;
            }

            //if (!bk_modem_dce_get_cfun()) {
            //    temp_flag = 11;
            //    break;
            //}

            if (!bk_mode_dce_get_cgmr()) {
                temp_flag = 12;
                //break;
            }
#endif
            if (!bk_modem_dce_check_sim())
            {
                temp_flag = 2;
                sim_check_cnt++;
                break;
            }
        
			if (!bk_modem_dce_get_ccid())
            {
                temp_flag = 13;
                break;
            }

            //if (!bk_modem_dce_get_cbc())
            //{
            //    temp_flag = 4;
            //    //break;
            //}
			
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
        
            if (!bk_modem_dce_check_attach())
            {
                temp_flag = 7;
                break;
            }          
        
            // All checks passed, transition to appropriate mode
            sim_check_cnt = 0;
            if (bk_modem_env.comm_proto == PPP_MODE)
            {
                bk_modem_set_state(PPP_START);
                bk_modem_send_msg(MSG_PPP_START, 0,0,0);
            }
            else if (bk_modem_env.comm_proto == UART_NIC_MODE)
            {
                bk_modem_set_state(UART_NIC_START);
                bk_modem_send_msg(MSG_MODEM_UART_NIC_START, 0,0,0);            
            }
            BK_MODEM_LOGI("%s: modem check pass\r\n", __func__);
		} /* endif (!bk_modem_mf_test) { */
		
		if (bk_modem_mf_test && bk_modem_mf_test_sem) {
            xSemaphoreGive(bk_modem_mf_test_sem);
        }
        return;
        
    }while(0);

    retry_time = 3000;

    // Special handling for SIM card detection failures
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
    } else if (temp_flag == 5) {
        if (retry_time < 3000) {
            retry_time += 500;
        }
    }

retry:    
    BK_MODEM_LOGI("%s: modem check fail %d\r\n", __func__, temp_flag);
    
    // Recovery actions based on failure type and interface
    if ((temp_flag == 1) && (bk_modem_env.comm_if == USB_IF))
    {
        bk_modem_usbh_close();
        bk_modem_dec_pdp_ctx_deinit();
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

/**
 * @brief Initiate PPP connection process
 *        This function handles the PPP connection establishment sequence, including:
 *        - Configuration validation
 *        - Starting the PPP connection
 *        - Transitioning to data mode
 *        It includes error handling and recovery procedures for connection failures.
 */
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
        #if 0
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
        #endif
        
        // Attempt to start PPP connection
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
        
        // PPP connection established, transition to data mode
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

retry:
    BK_MODEM_LOGI("%s: ppp dail fail%d\r\n", __func__, temp_flag);    
    
    // Recovery actions based on failure type
    if ((temp_flag == 2) || (temp_flag == 3) || (temp_flag > 5))
    {
        bk_modem_usbh_close();
        bk_modem_dec_pdp_ctx_deinit();
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

/**
 * @brief Handle PPP connection indication
 *        This function processes the PPP connection indication and initializes the network interface,
 *        setting it as the default network interface for the system.
 */
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

/**
 * @brief Handle PPP status indication
 * @param msg Pointer to the bus message
 *        This function is a placeholder for handling PPP status change notifications.
 */
void bk_modem_dte_handle_ppp_stutus_ind(BUS_MSG_T *msg)
{
    /// used for handling ppp status ind according to function on_ppp_status_changed
    
}

/**
 * @brief Handle PPP disconnection process
 * @param msg Pointer to the bus message containing stop reason
 *        This function processes PPP disconnection requests, cleans up resources,
 *        and transitions to the appropriate state based on the disconnection reason.
 */
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
    
    // Handle different disconnection scenarios
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

/**
 * @brief Handle modem disconnection indication
 *        This function processes modem disconnection notifications and transitions
 *        to the appropriate state based on the current operating mode.
 */
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

/**
 * @brief Handle UART NIC mode initialization
 *        This function initializes the UART NIC (Network Interface Controller) mode,
 *        including NAT configuration, RNDIS setup, and EC handshake process.
 *        It includes error handling and recovery procedures for initialization failures.
 */
void bk_modem_dte_handle_uart_nic_start(void)
{
    uint8_t temp_flag = 0xff;
    bk_err_t os_ret;
    if (bk_modem_get_state() != UART_NIC_START)
    {
        temp_flag = 0;
        goto fail;
    } 
    
    // Configure EC mode settings
    if ((bk_modem_dce_ec_check_nat()) && (!bk_modem_env.is_ec_nat_set))
    {
        if (!bk_modem_dce_ec_close_rndis())
        {
            temp_flag = 1;
            goto fail;
        }

        if (!bk_modem_dce_ec_open_datapath())
        {
            temp_flag = 2;
            goto fail;    
        }

        if (!bk_modem_dce_ec_set_nat())
        {
            temp_flag =3;
            goto fail;
        }
        bk_modem_env.is_ec_nat_set = true;
        bk_modem_dce_ec_rst();
        rtos_delay_milliseconds(3000);

        bk_modem_set_state(MODEM_CHECK);
        bk_modem_send_msg(MSG_MODEM_CHECK, 0,0,0);

        return;
    }

    // Initialize and perform EC handshake
    os_ret = rtos_init_semaphore(&bk_modem_ec_hs_sema, 1);
    if (os_ret != kNoErr)
    {
        temp_flag =4;
        goto fail;     
    }

    bk_modem_dte_ec_send_hs_data();
    os_ret = rtos_get_semaphore(&bk_modem_ec_hs_sema, 1000);
    if (os_ret != kNoErr)
    {
        temp_flag = 5;
        bk_modem_ec_hs = 0;
        goto fail;        
    }
    rtos_deinit_semaphore(&bk_modem_ec_hs_sema);
    bk_modem_ec_hs_sema = NULL;

    return;
    
fail:
    BK_MODEM_LOGI("%s: uart nic start fail %d\r\n", __func__, temp_flag);

    if (temp_flag == 5)
    {
        rtos_deinit_semaphore(&bk_modem_ec_hs_sema);
        bk_modem_ec_hs_sema = NULL;
    }
    bk_modem_env.is_ec_nat_set = false;
    bk_modem_set_state(MODEM_CHECK);
    bk_modem_send_msg(MSG_MODEM_CHECK, 0,0,0);
}

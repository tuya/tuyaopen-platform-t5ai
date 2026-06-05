/**
 ****************************************************************************************
 *
 * @file bk_modem_main.c
 *
 * @brief Main thread + state machine control, etc., as the main controller.
 *        This file implements the core functionality for managing the modem interface,
 *        including thread creation, message queue handling, state management, and
 *        communication protocol initialization.
 *
 ****************************************************************************************/
#include <string.h>
#include "bk_modem_main.h"
#include "bk_modem_dte.h"
#include "bk_modem_dce.h"
#include "bk_modem_at_cmd.h"
#include "bk_modem_usbh_if.h"
#include "bk_modem_uart.h"
#include "../../include/os/os.h"
#include "common/bk_err.h"
#include "../../include/driver/pwr_clk.h"
#include "net.h"

// Global thread handle for modem main thread
beken_thread_t bk_modem_thread = NULL;
// Global message queue handle for modem communication
beken_queue_t bk_modem_queue = NULL;

// Current state of the modem state machine
static enum bk_modem_state_e s_bk_modem_state = WAIT_MODEM_CONN;
// Modem environment structure containing communication parameters
struct bk_modem_env_s bk_modem_env;
// Modem status flag (0 = not initialized, 1 = initialized)
static uint8_t bk_modem_status = 0;
uint8_t bk_modem_mf_test = 0;
struct bk_modem_dce_pdp_ctx_s dce_pdp_ctx;

void bk_modem_dec_pdp_ctx_init(char *apn)
{
    memset(&dce_pdp_ctx, 0, sizeof(dce_pdp_ctx));
    dce_pdp_ctx.rssi = BK_MODEM_INVALID_RSSI_VAL;
    if (NULL != apn) {
        strncpy(dce_pdp_ctx.apn, apn, BK_MODEM_DCE_APN_LEN);
    }
}

void bk_modem_dec_pdp_ctx_deinit(void)
{
    dce_pdp_ctx.rssi = BK_MODEM_INVALID_RSSI_VAL;
    dce_pdp_ctx.volt = 0;
    memset(dce_pdp_ctx.cid, 0, BK_MODEM_DCE_CID_LEN);
    memset(dce_pdp_ctx.imei, 0, BK_MODEM_DCE_IMEI_LEN);
    memset(dce_pdp_ctx.sn, 0, BK_MODEM_DCE_SN_LEN);
    memset(dce_pdp_ctx.sw_ver, 0, BK_MODEM_DCE_SVER_LEN);
}

/**
 * @brief Main thread function for modem management
 * @param args Unused thread argument
 * @details This function processes messages from the modem queue and dispatches
 *          them to the appropriate handler based on message type.
 */
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
                // Handle modem connection indication
                bk_modem_dte_handle_conn_ind(&msg);
                break;
            }

            case MSG_MODEM_CHECK:
            {
                // Handle modem check request
                bk_modem_dte_handle_modem_check();
                break;
            }

            case MSG_PPP_START:
            {
                // Handle PPP connection start request
                bk_modem_dte_handle_ppp_start();
                break;
            }

            case MSG_PPP_CONNECT_IND:
            {
                // Handle PPP connection indication
                bk_modem_dte_handle_ppp_connect_ind();
                break;
            }
            
            case MSG_PPP_STATUS_IND:
            {
                // Handle PPP status indication
                bk_modem_dte_handle_ppp_stutus_ind(&msg);
                break;
            }

            case MSG_PPP_STOP:
            {
                // Handle PPP connection stop request
                bk_modem_dte_handle_ppp_stop(&msg);
                break;
            }

            case MSG_MODEM_DISC_IND:
            {
                // Handle modem disconnection indication
                bk_modem_dte_handle_disc_ind();
                break;
            }            

            case MSG_MODEM_USBH_POWER_ON:
            {
                // Power on modem via USB host interface
                rtos_delay_milliseconds(3000);
                bk_modem_power_on_modem();              
                bk_modem_usbh_poweron_ind();
                break;
            }  

            case MSG_MODEM_UART_INIT:
            {
                // Initialize UART interface to modem
                uint32_t baud = (bk_modem_env.comm_proto == PPP_MODE) ?
                                BK_MODEM_UART_BAUD : BK_MODEM_UART_3M_BAUD;
                rtos_delay_milliseconds(3000);
                bk_modem_power_on_modem();

                if (BK_OK == bk_modem_uart_init(baud))
                {
                    bk_modem_set_state(MODEM_CHECK);
                    bk_modem_send_msg(MSG_MODEM_CHECK, 0, 0, 0);
                }
                else
                {
                    // Retry UART initialization if failed
                    bk_modem_send_msg(MSG_MODEM_UART_INIT, 0, 0, 0);
                }
                break;
            } 

            case MSG_MODEM_UART_NIC_START:
            {
                // Start UART NIC mode
                bk_modem_dte_handle_uart_nic_start();
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

/**
 * @brief Release all modem resources
 * @details Frees queue, thread, AT command handler, and communication interface resources
 */
void bk_modem_del_resource(void)
{
    // Delete message queue
    if (bk_modem_queue)
    {
        rtos_deinit_queue(&bk_modem_queue);
        bk_modem_queue = NULL;
    }
    
    // Delete main thread
    if (bk_modem_thread)
    {
        rtos_delete_thread(&bk_modem_thread);
        bk_modem_thread = NULL;
    }

    // Deinitialize AT command handler
    bk_modem_at_dinit();
    
    // Deinitialize communication interface based on configuration
    if (bk_modem_env.comm_if == UART_IF)
    {
        bk_modem_uart_deinit();
    }
    else if (bk_modem_env.comm_if == USB_IF)
    {
        bk_modem_usbh_close();
        bk_modem_power_off_modem();    
    }
    bk_modem_dec_pdp_ctx_deinit();
    // Reset status and environment variables
    bk_modem_status = 0;
    bk_modem_env.comm_proto = INVALID_MODE;
    bk_modem_env.comm_if = INVALID_IF;
    bk_modem_env.is_ec_nat_set = false;
    
    bk_pm_module_vote_power_ctrl(PM_SLEEP_MODULE_NAME_BK_MODEM, PM_POWER_MODULE_STATE_OFF);
}

/**
 * @brief Deinitialize the modem module
 * @return BK_OK on success, otherwise error code
 * @details Stops any active PPP connection and resets modem state
 */
bk_err_t bk_modem_deinit(void)
{
    BK_MODEM_LOGI("%s: status %d \r\n", __func__, bk_modem_status);

    if (bk_modem_status == 1)
    {
        if (bk_modem_env.comm_proto == PPP_MODE)
        {
            // Set state to PPP_STOP and send stop message
            bk_modem_set_state(PPP_STOP);

            BUS_MSG_T msg;
            msg.type = MSG_PPP_STOP;
            msg.arg = ACTIVE_STOP;
            msg.len = 0;
            msg.sema = NULL;
            msg.param = NULL;
            bk_modem_dte_handle_ppp_stop(&msg);
        }
        else if (bk_modem_env.comm_proto == UART_NIC_MODE)
        {
            modem_ip_down();
            net_modem_remove_netif();
            bk_modem_del_resource();
        }
    }
    else
    {
        BK_MODEM_LOGE("bk modem already close, no need close again\r\n");
    }

    return BK_OK;
}

/**
 * @brief Initialize the modem module
 * @param comm_proto Communication protocol (PPP_MODE or UART_NIC_MODE)
 * @param comm_if Communication interface (USB_IF or UART_IF)
 * @return BK_OK on success, BK_FAIL on failure
 * @details Creates thread and queue, initializes AT commands, and starts the
 *          appropriate communication interface based on parameters
 */
bk_err_t bk_modem_init(uint8_t comm_proto, uint8_t comm_if)
{
    int ret;

    BK_MODEM_LOGI("%s: status %d \r\n", __func__, bk_modem_status);

    // Check if already initialized
    if (bk_modem_status == 1)
    {
        BK_MODEM_LOGE("bk modem already start, no need start again\r\n");
        return BK_FAIL; 
    }

    // Check if resources are already allocated
    if ((bk_modem_thread != NULL) || (bk_modem_queue != NULL))
    {
        goto init_fail;
    }

    // Initialize message queue
    ret = rtos_init_queue(&bk_modem_queue, "bk_modem_queue", sizeof(BUS_MSG_T), 16);
    if (ret != kNoErr)
    {
        goto init_fail;
    }

    // Initialize AT command handler
    if (bk_modem_at_init() == BK_FAIL)
    {
        goto init_fail;
    }

    // Create main thread
    ret = rtos_create_thread(&bk_modem_thread, BEKEN_DEFAULT_WORKER_PRIORITY, "bk_modem",
                       bk_modem_thread_main, 2048, (beken_thread_arg_t)0);

    if (ret != kNoErr)
    {
        goto init_fail;
    }

    // Set communication parameters and start appropriate interface
    bk_modem_env.comm_proto = comm_proto;
    bk_modem_env.comm_if = comm_if;
    if (comm_proto == PPP_MODE)
    {
        if (comm_if == USB_IF)
        {
            bk_modem_send_msg(MSG_MODEM_USBH_POWER_ON, 0, 0, 0);
        }
        else if (comm_if == UART_IF)
        {
            bk_modem_send_msg(MSG_MODEM_UART_INIT, 0, 0, 0);
        }
        else
            goto init_fail;
    }
    else if (comm_proto == UART_NIC_MODE)
    {
        if (comm_if == UART_IF)
            // Start UART interface in NIC mode
            bk_modem_send_msg(MSG_MODEM_UART_INIT, 0, 0, 0);
        else
            goto init_fail;
    }
    else
        // Unsupported protocol
        goto init_fail;

    // Set initialization status and return success
    bk_modem_status = 1;
    return BK_OK;
    
init_fail:
    // Clean up resources if initialization failed
    bk_modem_deinit();
    return BK_FAIL;    
}

/**
 * @brief Set the modem state machine state
 * @param bk_modem_state New state to set
 */
void bk_modem_set_state(enum bk_modem_state_e bk_modem_state)
{
    s_bk_modem_state = bk_modem_state;
}

/**
 * @brief Get the current modem state machine state
 * @return Current modem state
 */
enum bk_modem_state_e bk_modem_get_state(void)
{
    return s_bk_modem_state;
}

/**
 * @brief Send a message to the modem message queue
 * @param type Message type
 * @param arg Message argument
 * @param len Message length
 * @param param Optional message parameter
 * @return 0 on success, -1 on failure
 */
int bk_modem_send_msg(int type, uint32_t arg, uint32_t len, void *param)
{
    BUS_MSG_T msg;
    int ret;

    // Check if queue is initialized
    if (bk_modem_queue == NULL)
    {
        BK_MODEM_LOGI("%s: queue is null!", __func__);
        return -1;
    }

    // Prepare message
    msg.type = type;
    msg.arg = (uint32_t)arg;
    msg.len = len;
    msg.sema = NULL;
    msg.param = (void *)param;

    // Push message to queue
    ret = rtos_push_to_queue(&bk_modem_queue, &msg, BEKEN_NO_WAIT);
    if (ret)
        BK_ASSERT(0);

    return ret;
}

/**
 * @brief Power on the modem hardware
 * @details This is a placeholder function that should be implemented based on the
 *          specific hardware platform to control modem power
 */
void bk_modem_power_on_modem(void)
{
    /// Maybe usb slave such as modem will be powered on by usb host 
    /// please use this interface to power on modem, if modem can be independent powered control
}

/**
 * @brief Power off the modem hardware
 * @details This is a placeholder function that should be implemented based on the
 *          specific hardware platform to control modem power
 */
void bk_modem_power_off_modem(void)
{
    /// Maybe usb slave such as modem will be powered off by usb host 
    /// please use this interface to power off modem, if modem can be independent powered control  
}


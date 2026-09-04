/**
 ****************************************************************************************
 *
 * @file bk_modem_dce.c
 *
 * @brief 4G Modem Related Processes.
 *
 ****************************************************************************************
 */
#include <common/bk_include.h>
#include "bk_modem_dce.h"
#include "bk_modem_at_cmd.h"
#include "bk_modem_uart.h"
#include "bk_modem_main.h"
#include <driver/uart.h>

extern struct bk_modem_dce_pdp_ctx_s dce_pdp_ctx;

static inline void bk_modem_dec_reset(void)
{
    rtos_delay_milliseconds(1000);
    bk_modem_dce_enter_cmd_mode();
    rtos_delay_milliseconds(1000);
}

bool bk_modem_dce_send_at(void)
{
    /* Step 1: Try AT directly at 921600 (modem may already be in cmd mode) */
    if (bk_modem_env.comm_if == USB_IF)
    {
        if (BK_OK == bk_modem_at_ready())
        {
            return true;
        }
    }
    else if (bk_modem_env.comm_if == UART_IF)
    {
        BK_MODEM_LOGI("modem working baudrate %d\r\n", BK_MODEM_UART_BAUD);
        if (BK_OK == bk_modem_at_ready())
        {
            return true;
        }
        /* Step 2: No response at 921600, maybe in data mode - send +++ with guard time */
        bk_modem_dec_reset();

        if (BK_OK == bk_modem_at_ready())
        {
            return true;
        }

        /* Step 3: Switch to 115200, try again */
        BK_MODEM_LOGI("modem try baudrate %d\r\n", BK_MODEM_UART_115200_BAUD);
        bk_uart_set_baud_rate(BK_MODEM_UART_ID, BK_MODEM_UART_115200_BAUD);

        if (BK_OK == bk_modem_at_ready())
        {
            /* Got response, modem already stay in cmd mode, do baudrate change next */

            /* Modem responds at 115200, try to change it to 921600 */
            BK_MODEM_LOGI("modem change baudrate from %d to %d\r\n", BK_MODEM_UART_115200_BAUD, BK_MODEM_UART_BAUD);
            if (BK_OK == bk_modem_at_change_baudrate(BK_MODEM_UART_BAUD))
            {
                rtos_delay_milliseconds(200);
                bk_uart_set_baud_rate(BK_MODEM_UART_ID, BK_MODEM_UART_BAUD);
                rtos_delay_milliseconds(200);

                if (BK_OK == bk_modem_at_ready())
                {
                    BK_MODEM_LOGI("modem working baudrate %d\r\n", BK_MODEM_UART_BAUD);
                    return true;
                }
                /* Baud switch failed, fall back to 115200 */
                bk_uart_set_baud_rate(BK_MODEM_UART_ID, BK_MODEM_UART_115200_BAUD);
                rtos_delay_milliseconds(200);
            }
            /* Stay at 115200 */
            BK_MODEM_LOGI("modem working baudrate %d\r\n", BK_MODEM_UART_115200_BAUD);
            return (BK_OK == bk_modem_at_ready());
        }

        /* Step 4: No response, maybe in data mode - send +++ with guard time */
        bk_modem_dec_reset();

        if (BK_OK == bk_modem_at_ready())
        {
            /* Got response, cmd mode, do baudrate change next */

            /* Modem responds at 115200, try to change it to 921600 */
            BK_MODEM_LOGI("modem change baudrate from %d to %d\r\n", BK_MODEM_UART_115200_BAUD, BK_MODEM_UART_BAUD);
            if (BK_OK == bk_modem_at_change_baudrate(BK_MODEM_UART_BAUD))
            {
                rtos_delay_milliseconds(200);
                bk_uart_set_baud_rate(BK_MODEM_UART_ID, BK_MODEM_UART_BAUD);
                rtos_delay_milliseconds(200);

                if (BK_OK == bk_modem_at_ready())
                {
                    BK_MODEM_LOGI("modem working baudrate %d\r\n", BK_MODEM_UART_BAUD);
                    return true;
                }
                /* Baud switch failed, fall back to 115200 */
                bk_uart_set_baud_rate(BK_MODEM_UART_ID, BK_MODEM_UART_115200_BAUD);
                rtos_delay_milliseconds(200);
            }
            /* Stay at 115200 */
            BK_MODEM_LOGI("modem working baudrate %d\r\n", BK_MODEM_UART_115200_BAUD);
            return (BK_OK == bk_modem_at_ready());
        }
    }

    BK_MODEM_LOGI("modem no response\r\n");
    return false;
}

bool bk_modem_dce_check_sim(void)
{
    return (BK_OK == bk_modem_at_cpin());
}

bool bk_modem_dce_check_signal(void)
{
    return (BK_OK == bk_modem_at_csq());
}

bool bk_modem_dce_get_ccid(void)
{
    return (BK_OK == bk_modem_at_ccid());
}

bool bk_modem_dce_get_cbc(void)
{
    return (BK_OK == bk_modem_at_cbc());
}

bool bk_modem_dce_check_register(void)
{
    return (BK_OK == bk_modem_at_get_operator_name());
}

bool bk_modem_dce_set_apn(void)
{
    return (BK_OK == bk_modem_at_cgdcont(1,"ipv4v6", dce_pdp_ctx.apn));
}

bool bk_modem_dce_check_attach(void)
{
    return (BK_OK == bk_modem_at_get_ps_reg());
}

bool bk_modem_dce_start_ppp(void)
{
    return (BK_OK == bk_modem_at_ppp_connect());
}

bool bk_modem_dce_enter_cmd_mode(void)
{
    return (BK_OK == bk_modem_at_enter_cmd_mode());
}

bool bk_modem_dce_stop_ppp(void)
{
    return (BK_OK == bk_modem_at_disconnect());
}

bool bk_modem_dce_enter_flight_mode(void)
{
    return (BK_OK == bk_modem_at_cfun(0));
}

bool bk_modem_dce_exit_flight_mode(void)
{
    return (BK_OK == bk_modem_at_cfun(1));
}

/// ec own at cmd
bool bk_modem_dce_ec_check_nat(void)
{
    return (BK_OK == bk_modem_ec_at_check_nat());
}

bool bk_modem_dce_ec_close_rndis(void)
{
    return (BK_OK == bk_modem_ec_at_close_rndis());
}

bool bk_modem_dce_ec_open_datapath(void)
{
    return (BK_OK == bk_modem_ec_at_open_datapath());
}

bool bk_modem_dce_ec_set_nat(void)
{
    return (BK_OK == bk_modem_ec_at_set_nat());
}

bool bk_modem_dce_ec_rst(void)
{
    return (BK_OK == bk_modem_ec_at_rst());
}

bool bk_modem_dce_get_ati(void)
{
    return (BK_OK == bk_modem_at_get_ati());
}

bool bk_modem_dce_get_cgsn(void)
{
    return (BK_OK == bk_modem_at_get_cgsn());
}

bool bk_modem_dce_get_cfsn(void)
{
    return (BK_OK == bk_modem_at_get_cfsn());
}

bool bk_modem_dce_get_cfun(void)
{
    return (BK_OK == bk_modem_at_get_cfun());
}

bool bk_mode_dce_get_cgmr(void)
{
    return (BK_OK == bk_modem_at_get_cgmr());
}

#if BK_MODEM_CAT1_LP_DTR_SLEEP
bool bk_modem_dce_lp_config(void)
{
    return (BK_OK == bk_modem_at_lp_config());
}
#endif

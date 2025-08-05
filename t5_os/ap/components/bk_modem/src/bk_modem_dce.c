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

bool bk_modem_dce_send_at(void)
{
    return (BK_OK == bk_modem_at_ready());
}

bool bk_modem_dce_check_sim(void)
{
    return (BK_OK == bk_modem_at_cpin());
}

bool bk_modem_dce_check_signal(void)
{
    return (BK_OK == bk_modem_at_csq());
}

bool bk_modem_dce_check_register(void)
{
    return (BK_OK == bk_modem_at_get_operator_name());
}

bool bk_modem_dce_set_apn(void)
{
    return (BK_OK == bk_modem_at_cgdcont(1,"ipv4v6",""));
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

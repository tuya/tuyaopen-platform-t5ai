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

extern struct bk_modem_dce_pdp_ctx_s dce_pdp_ctx;

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


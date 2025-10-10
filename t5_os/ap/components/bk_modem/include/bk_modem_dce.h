
#ifndef _BK_MODEM_DCE_H_
#define _BK_MODEM_DCE_H_

#endif

#include <common/bk_include.h>

#define BK_MODEM_DCE_APN_LEN 64
#define BK_MODEM_DCE_CID_LEN 20

struct bk_modem_dce_pdp_ctx_s
{
    char            rssi;
    uint32_t        volt;
    char            apn[BK_MODEM_DCE_APN_LEN+1];
    char            cid[BK_MODEM_DCE_CID_LEN+1];
};

extern bool bk_modem_dce_send_at(void);
extern bool bk_modem_dce_check_sim(void);
extern bool bk_modem_dce_get_ccid(void);
extern bool bk_modem_dce_check_signal(void);
extern bool bk_modem_dce_check_register(void);
extern bool bk_modem_dce_set_apn(void);
extern bool bk_modem_dce_check_attach(void);
extern bool bk_modem_dce_start_ppp(void);
extern bool bk_modem_dce_enter_cmd_mode(void);
extern bool bk_modem_dce_stop_ppp(void);
extern bool bk_modem_dce_enter_flight_mode(void);
extern bool bk_modem_dce_exit_flight_mode(void);
extern bk_err_t bk_modem_at_get_ps_reg(void);
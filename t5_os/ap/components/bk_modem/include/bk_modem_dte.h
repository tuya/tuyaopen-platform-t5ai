
#ifndef _BK_MODEM_DTE_H_
#define _BK_MODEM_DTE_H_

#include "bk_modem_main.h"

extern void bk_modem_dte_handle_conn_ind(BUS_MSG_T *msg);
extern void bk_modem_dte_handle_modem_check(void);
extern void bk_modem_dte_handle_ppp_start(void);
extern void bk_modem_dte_handle_ppp_connect_ind(void);
void bk_modem_dte_handle_ppp_stutus_ind(BUS_MSG_T *msg);
extern void bk_modem_dte_handle_ppp_stop(BUS_MSG_T *msg);
extern void bk_modem_dte_handle_disc_ind(void);
extern void bk_modem_dte_send_data(uint32_t data_length, uint8_t *data, enum bk_modem_ppp_mode_e ppp_mode);
extern void bk_modem_dte_recv_data(uint32_t data_length, uint8_t *data);
#endif
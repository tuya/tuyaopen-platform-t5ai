
#ifndef _BK_MODEM_DTE_H_
#define _BK_MODEM_DTE_H_

#include "bk_modem_main.h"
extern uint8_t bk_modem_ec_hs;
extern void bk_modem_dte_handle_conn_ind(BUS_MSG_T *msg);
extern void bk_modem_dte_handle_modem_check(void);
extern void bk_modem_dte_handle_ppp_start(void);
extern void bk_modem_dte_handle_ppp_connect_ind(void);
void bk_modem_dte_handle_ppp_stutus_ind(BUS_MSG_T *msg);
extern void bk_modem_dte_handle_ppp_stop(BUS_MSG_T *msg);
extern void bk_modem_dte_handle_disc_ind(void);
extern void bk_modem_dte_send_data(uint32_t data_length, uint8_t *data, enum bk_modem_ppp_mode_e ppp_mode);
extern void bk_modem_dte_recv_data(uint32_t data_length, uint8_t *data);
extern void bk_modem_dte_send_data_uart(uint32_t data_length, uint8_t *data, enum bk_modem_uart_trx_mode_e uart_trx_mode);
extern void bk_modem_dte_handle_uart_nic_start(void);
extern bool bk_modem_dce_ec_check_nat(void);
extern bool bk_modem_dce_ec_close_rndis(void);
extern bool bk_modem_dce_ec_open_datapath(void);
extern bool bk_modem_dce_ec_set_nat(void);
extern bool bk_modem_dce_ec_rst(void);
extern void bk_modem_dte_recv_data_uart(uint32_t data_length, uint8_t *data, uint8_t data_type);
#endif
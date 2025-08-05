
#ifndef _BK_MODEM_NETIF_H_
#define _BK_MODEM_NETIF_H_

#include "common/bk_err.h"
#include "common/bk_typedef.h"

bk_err_t bk_modem_netif_new_ppp(void);
void bk_modem_netif_ppp_set_default_netif(void);
bk_err_t bk_modem_netif_ppp_set_auth(uint8_t authtype, const char *user, const char *passwd);
bk_err_t bk_modem_netif_start_ppp(void);
bk_err_t bk_modem_netif_stop_ppp(void);
void bk_modem_netif_destroy_ppp(void);
bk_err_t bk_modem_netif_lwip_ppp_input(void *buffer, size_t len);

#endif

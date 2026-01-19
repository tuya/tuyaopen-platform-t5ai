#ifndef __MODEM_IF_H__
#define __MODEM_IF_H__


#include "lwip/err.h"
#include "lwip/netif.h"

#define MODEM_HWADDR_LEN 6

err_t modem_netif_init(struct netif *netif);
void modemif_input(struct netif *netif, struct pbuf *p);

typedef void (* modemif_output_cb)(struct netif *netif, struct pbuf *p);
void bk_modemif_register_callback(modemif_output_cb callback);

#endif


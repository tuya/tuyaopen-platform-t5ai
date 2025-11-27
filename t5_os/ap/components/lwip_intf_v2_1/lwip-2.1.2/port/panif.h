#ifndef __PAN_IF_H__
#define __PAN_IF_H__


#include "lwip/err.h"
#include "lwip/netif.h"

#define PAN_HWADDR_LEN 6

err_t pan_netif_init(struct netif *netif);
void panif_input(struct netif *netif, struct pbuf *p);

typedef void (* panif_output_cb)(struct netif *netif, struct pbuf *p);
void bk_panif_register_callback(panif_output_cb callback);

#endif


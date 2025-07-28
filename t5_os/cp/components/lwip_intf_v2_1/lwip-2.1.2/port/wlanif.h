#ifndef __WLAN_IF_H__
#define __WLAN_IF_H__


#include "lwip/err.h"
#include "lwip/netif.h"


void wlanif_recv(struct netif *netif, int total_len);
err_t wlanif_init(struct netif *netif);


#endif

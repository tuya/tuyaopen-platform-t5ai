#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "cif_main.h"
#include "lwip/prot/ethernet.h"

#define CTRL_IF_DATA    CIF_LOGV

#define LOCAL_PORT_RANGE_START      0x1000
#define LOCAL_PORT_RANGE_END        0x1010
#define BK_MIN_PORT                 800000
#define BK_ICMP_PORT                 BK_MIN_PORT+1
#define BK_IGMP_PORT                 BK_MIN_PORT+2
#define BK_ICMP_REQ_PORT             BK_MIN_PORT+3

extern bk_err_t cif_handle_txdata(void *head);
bool cif_rx_local_packet_check(struct pbuf **p_ptr,struct eth_hdr * ethhdr,void* vif);
void cif_filter_add_customer_filter(uint32_t ip, uint16_t port);

#ifdef __cplusplus
}
#endif
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "cif_main.h"
#include "lwip/pbuf.h"
#include "lwip/prot/etharp.h"
#include "lwip/prot/ethernet.h"
#define CTRL_IF_DATA    CIF_LOGV

#define LOCAL_PORT_RANGE_START      0x1000
#define LOCAL_PORT_RANGE_END        0x1010
#define BK_MIN_PORT                 800000
#define BK_ICMP_PORT                 BK_MIN_PORT+1
#define BK_IGMP_PORT                 BK_MIN_PORT+2
#define BK_ICMP_REQ_PORT             BK_MIN_PORT+3

extern bk_err_t cif_handle_txdata(void *head);
bool cif_rx_local_packet_check(struct pbuf **p_ptr,struct eth_hdr * ethhdr,void* vif, uint8_t dst_idx);
void cif_filter_add_customer_filter(uint32_t ip, uint16_t port);

static inline bool cif_is_arp_request(const struct pbuf *p)
{
    if (p == NULL || p->len < (SIZEOF_ETH_HDR + SIZEOF_ETHARP_HDR)) {
        return 0;
    }

    const struct eth_hdr *ethhdr = (const struct eth_hdr *)p->payload;

    if (ethhdr->type == htons(ETHTYPE_ARP)) {
        const struct etharp_hdr *arphdr = (const struct etharp_hdr *)((const u8_t *)ethhdr + SIZEOF_ETH_HDR);

        if (arphdr->opcode == htons(ARP_REQUEST)) {
            return 1;
        }
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
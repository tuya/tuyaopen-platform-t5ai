#include "tkl_lwip.h"
#include "tkl_queue.h"
#include "bk_wifi.h"
#include "bk_private/bk_wifi.h"
#ifdef CONFIG_BRIDGE
#include "bridgeif.h"
#include "rwnx_config.h"
#endif
#include "bk_drv_model.h"

#include "common/bk_include.h"
#include <stdio.h>
#include <string.h>
#include "bk_drv_model.h"
#include <os/mem.h>
#ifdef CONFIG_BRIDGE
#include "bridgeif.h"
#include "rwnx_config.h"
#endif
#include <os/os.h>
#include "tkl_ipc.h"
#include "netif.h"
#include "netif/ethernet.h"
#include "lwip/ethernetif.h"

#include "tkl_thread.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pbuf.h"
#include "tkl_wifi.h"


extern void *net_get_sta_handle(void);
extern void *net_get_uap_handle(void);
extern void *net_get_eth_handle(void);

struct netif *tkl_lwip_get_netif_by_index(int net_if_idx)
{
    if(net_if_idx == 0)
        return net_get_sta_handle();
    else if(net_if_idx == 1)
        return net_get_uap_handle();
    else if(net_if_idx == 2)
        return net_get_eth_handle();

    return NULL;
}

static void low_level_init(struct netif *netif)
{
    NW_MAC_S macptr;
    WF_IF_E id = WF_STATION;
    if(strncmp(netif->name, "r0", 2) == 0)	{
        id = WF_STATION;
    }else if(strncmp(netif->name, "r1", 2) == 0) {
        id = WF_AP;
    }else if(strncmp(netif->name, "r2", 2) == 0) {
        id = WF_STATION;
        // ethernetif_init(netif);
        // return;
    }else {
        bk_printf("netif->name error %c  %c!\r\n", netif->name[0],netif->name[1]);
        return;
    }

    tkl_wifi_get_mac(id, &macptr);

    if(strncmp(netif->name, "r2", 2) == 0) {
        macptr.mac[5] += 3;
    }

    /* set MAC hardware address length */
    bk_printf("mac %2x:%2x:%2x:%2x:%2x:%2x\r\n",
            macptr.mac[0], macptr.mac[1], macptr.mac[2],
            macptr.mac[3], macptr.mac[4], macptr.mac[5]);

    netif->hwaddr_len = MAC_ADDR_LEN;
    os_memcpy(netif->hwaddr, macptr.mac, MAC_ADDR_LEN);
}

/**
 * @brief ethernet interface hardware init
 *
 * @param[in]      netif     the netif to which to send the packet
 * @return  err_t  SEE "err_enum_t" in "lwip/err.h" to see the lwip err(ERR_OK: SUCCESS other:fail)
 */
OPERATE_RET tkl_ethernetif_init(TKL_NETIF_HANDLE netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

    /* initialize the hardware */
    low_level_init(netif);

    // TODO ??????????
    // extern void sys_init(void);
    // sys_init();

    return OPRT_OK;
}

extern bool cif_rx_local_packet_check(struct pbuf **p_ptr,struct eth_hdr * ethhdr);
void ethernetif_input(int iface, struct pbuf *p)
{
    struct eth_hdr *ethhdr;
    struct netif *netif;
    void *vif;

    if (p->len <= 14) {
        pbuf_free(p);
        return;
    }

    vif = wifi_netif_vifid_to_vif(iface);
    netif = (struct netif *)wifi_netif_get_vif_private_data(vif);
    if(!netif) {
        //bk_printf("ethernetif_input no netif found %d\r\n", iface);
        pbuf_free(p);
        p = NULL;
        return;
    }

    /* points to packet payload, which starts with an Ethernet header */
    ethhdr = p->payload;

#ifdef CONFIG_WIFI_VNET_CONTROLLER
    if(false == cif_rx_local_packet_check(&p,ethhdr)) {
        return;
    }
#endif

#if 0 // SMP
    /* need to forward */
#if 0
    if (wifi_netif_vif_to_netif_type(vif) == NETIF_IF_AP) {
        // If dest sta is known, or packet is multicast, forward this packet
        if ((ethhdr->dest.addr[0] & 1) || dst_idx != 0xff) {
            // check if is arp request to us, doesn't need to forward
            struct pbuf *q;

            // unicast frame, check da staidx
            // for softap+ap, if sta under softap sends packets to router,
            // dst_idx will be valid, and packets will be forwarded in our
            // softap bss.
            if (!(ethhdr->dest.addr[0] & 1) && dst_idx < NX_REMOTE_STA_MAX) {
                void *sta = sta_mgmt_get_entry(dst_idx);
                // if STA doesn't belong to this vif
                if (mac_sta_mgmt_get_inst_nbr(sta) != mac_vif_mgmt_get_index(vif)) {
                    goto process;
                }
            }

            if (ethhdr->type == PP_HTONS(ETHTYPE_ARP)) {
                struct etharp_hdr *hdr = (struct etharp_hdr *)(ethhdr + 1);
                if (hdr->opcode != PP_HTONS(ARP_REQUEST))
                    goto forward;
#if CONFIG_BRIDGE
                // FIXME: Handle ARP Probe
                struct netif *brif;
                bridgeif_port_t *port;
                if (bridgeif_netif_client_id != 0xff) {
                    port = (bridgeif_port_t *)netif_get_client_data(netif, bridgeif_netif_client_id);
                    if (!port || !port->bridge || !port->bridge->netif)
                        goto forward;
                    brif = port->bridge->netif;
                    if (!memcmp(&hdr->dipaddr, &brif->ip_addr, 4)) {
                        // os_printf("DIP TO BR\n");
                        goto process;
                    }
                } else {
                    if (!memcmp(&hdr->dipaddr, &netif->ip_addr, 4)) {
                        // os_printf("DIP TO SOFTAP\n");
                        goto process;
                    }
                }
#else
                if (!memcmp(&hdr->dipaddr, &netif->ip_addr, 4)) {
                    // os_printf("DIP TO SOFTAP\n");
                    goto process;
                }
#endif
            }
forward:
            q = pbuf_clone(PBUF_RAW_TX, PBUF_RAM, p);
            if (q != NULL) {
                low_level_output(netif, q);
                pbuf_free(q);
            } else {
                bk_printf("alloc pbuf failed, don't forward\r\n");
            }
        }
    }
#endif

process:
    switch (htons(ethhdr->type))
    {
#if 0
        /* IP or ARP packet? */
        case ETHTYPE_IP:
        case ETHTYPE_ARP:
#ifdef CONFIG_IPV6
        case ETHTYPE_IPV6:
            wlan_set_multicast_flag();
#endif
#if PPPOE_SUPPORT
            /* PPPoE packet? */
        case ETHTYPE_PPPOEDISC:
        case ETHTYPE_PPPOE:
#endif /* PPPOE_SUPPORT */
            /* full packet send to tcpip_thread to process */
            if (netif->input(p, netif) != ERR_OK)    // ethernet_input
            {
                LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\r\n"));
                pbuf_free(p);
                p = NULL;
            }
            break;
#endif
        case 0x888E:    // ETHTYPE_EAPOL
            ke_l2_packet_tx(p->payload, p->len, iface);
            pbuf_free(p);
            p = NULL;
            break;

        default:
            tkl_ethernetif_recv(netif, p);
            pbuf_free(p);
            p = NULL;
            break;
    }
#endif // SMP

    return;
}


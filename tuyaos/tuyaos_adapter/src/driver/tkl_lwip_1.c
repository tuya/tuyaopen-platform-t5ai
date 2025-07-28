
#if 0   // TODO
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/tcpip.h"
#include "lwip/icmp.h"
#include "lwip/inet.h"
#include "netif/etharp.h"
#include "lwip/err.h"
#include "lwip/ethernetif.h"
#endif

#include "tkl_lwip.h"
#include "tkl_ipc.h"
#include "tkl_semaphore.h"
#include "tkl_system.h"
#include "sdkconfig.h"

/**
 * @brief get netif by index
 *
 * @param[in]      net_if_idx     netif index
 * @return  err_t  SEE "err_enum_t" in "lwip/err.h" to see the lwip err(ERR_OK: SUCCESS other:fail)
 */
struct netif *tkl_lwip_get_netif_by_index(int netif_idx)
{
#if 0
    if (netif_idx == NETIF_STA_IDX) {
        return net_get_sta_handle();
    } else if (netif_idx == NETIF_AP_IDX) {
        return net_get_uap_handle();
    }
#ifdef LWIP_DUAL_NET_SUPPORT && CONFIG_ETH
    else if (netif_idx == NETIF_ETH_IDX) {
        reutrn net_get_eth_handle();
    }
#endif // LWIP_DUAL_NET_SUPPORT && CONFIG_ETH
    else {
        bk_printf("not support netif index: %d\r\n", netif_idx);
        return NULL;
    }
#endif
}

/**
 * @brief ethernet interface hardware init
 *
 * @param[in]      netif     the netif to which to send the packet
 * @return  err_t  SEE "err_enum_t" in "lwip/err.h" to see the lwip err(ERR_OK: SUCCESS other:fail)
 */
OPERATE_RET tkl_ethernetif_init(TKL_NETIF_HANDLE netif)
{
#if 0
    OPERATE_RET ret;
	struct netif *pnetif = netif;
	pnetif->output = etharp_output;
    pnetif->linkoutput = tkl_ethernetif_output;

    uint8_t macptr[6] = {0};
    if (netif == net_get_sta_handle())
        bk_wifi_sta_get_mac(macptr);
    else if (netif == net_get_uap_handle())
        bk_wifi_ap_get_mac(macptr);
    else
        return;

    pnetif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

    bk_printf("netif init, mac %2x:%2x:%2x:%2x:%2x:%2x\r\n", macptr[0], macptr[1], macptr[2],
                 macptr[3], macptr[4], macptr[5]);
    memcpy(pnetif->hwaddr, macptr, ETHARP_HWADDR_LEN);
#endif
    return 0;
}

/**
 * @brief ethernet interface sendout the pbuf packet
 *
 * @param[in]      netif     the netif to which to send the packet
 * @param[in]      p         the packet to be send, in pbuf mode
 * @return  err_t  SEE "err_enum_t" in "lwip/err.h" to see the lwip err(ERR_OK: SUCCESS other:fail)
 */
// SEND
OPERATE_RET tkl_ethernetif_output(TKL_NETIF_HANDLE netif, TKL_PBUF_HANDLE p)
{
#if 0
    int ret;
    err_t err = ERR_OK;
    uint8_t vif_idx = 0;//TODO: add vif index.wifi_netif_vif_to_vifid(netif->state);
                        // Sanity check
    if (vif_idx == 0xff)
        return ERR_ARG;

    struct pbuf *tmp = (struct pbuf *)p;
    bk_printf("%s p:%x next:%x payload%x sizeof:%d\r\n",__func__, tmp, tmp->next, tmp->payload, sizeof(struct pbuf));
    ret = wdrv_txdata_sender(p,vif_idx);

    if(0 != ret)
    {
        err = ERR_TIMEOUT;
    }
    return err;
#endif
    return 0;
}


// RECV
#if 0
void ethernetif_input(int iface, struct pbuf *p)
{
    struct eth_hdr *ethhdr;
    struct netif *netif = NULL;

    if (p->len <= SIZEOF_ETH_HDR) {
        pbuf_free(p);
        return;
    }

#if 0
    netif = net_get_sta_handle();
#if CONFIG_WIFI_SOFTAP
    netif = net_get_uap_handle();
#endif
#endif

#if 1
     if (iface == 0)
         netif = net_get_sta_handle();
     else if (iface == 1)
         netif = net_get_uap_handle();
     else
         return;
#endif
    if(!netif) {
        LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input no netif found\r\n"));
        pbuf_free(p);
        p = NULL;
        return;
    }

    /* points to packet payload, which starts with an Ethernet header */
    ethhdr = p->payload;

    if( (memcmp(netif->hwaddr,ethhdr->src.addr,NETIF_MAX_HWADDR_LEN)==0) && (htons(ethhdr->type) !=ETHTYPE_ARP) )
    {
        LWIP_DEBUGF(ETHARP_DEBUG ,("ethernet_input frame is my send,drop it\r\n"));
        pbuf_free(p);
        return;
    }

    switch (htons(ethhdr->type))
    {
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

    case ETHTYPE_EAPOL:
		pbuf_free(p);
		p = NULL;
        break;

    default:
        pbuf_free(p);
        // cppcheck-suppress uselessAssignmentPtrArg
        p = NULL;
        break;
    }

}
#endif



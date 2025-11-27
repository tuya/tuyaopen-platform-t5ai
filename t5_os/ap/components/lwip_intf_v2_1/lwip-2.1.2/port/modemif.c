#include "lwip/netif.h"
#include "lwip/ip.h"
#include "lwip/tcpip.h"
#include "lwip/pbuf.h"
#include "bk_wifi.h"
#include "net.h"
#include <os/mem.h>
#include <os/os.h>
#include "netif/etharp.h"
#include "modemif.h"

/* Define modem interface */
#define IFNAME0 'm'
#define IFNAME1 'd'

static modemif_output_cb s_modemif_output_cb = NULL;

void bk_modemif_register_callback(modemif_output_cb callback)
{
	s_modemif_output_cb = callback ;
}

static void modem_low_level_init(struct netif *netif)
{
    u8 macptr[6] = {0};

    if (netif == net_get_modem_handle())
        bk_modem_get_mac(macptr);
    else
        return;

    /* set MAC hardware address length */
    LWIP_LOGV("mac %2x:%2x:%2x:%2x:%2x:%2x\r\n", macptr[0], macptr[1], macptr[2],
                 macptr[3], macptr[4], macptr[5]);

    netif->hwaddr_len = MODEM_HWADDR_LEN;
    os_memcpy(netif->hwaddr, macptr, MODEM_HWADDR_LEN);
    /* maximum transfer unit */
    netif->mtu = 1500;
    /* device capabilities */
    /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

}

static err_t modem_low_level_output(struct netif *netif, struct pbuf *p)
{
    if (s_modemif_output_cb) {
        s_modemif_output_cb(netif, p);
    }
    return ERR_OK;
}

void 
modemif_input(struct netif *netif, struct pbuf *p)
{
    struct eth_hdr *ethhdr;
    if (p->len <= SIZEOF_ETH_HDR) {
        goto free_pbuf;

    }

    netif = net_get_modem_handle();
    if(!netif) {
        //LWIP_LOGD("ethernetif_input no netif found %d\r\n", iface);
        pbuf_free(p);
        goto free_pbuf;
    }

    /* points to packet payload, which starts with an Ethernet header */
    ethhdr = (struct eth_hdr *)p->payload;

    if( (os_memcmp(netif->hwaddr,ethhdr->src.addr,NETIF_MAX_HWADDR_LEN)==0) && (htons(ethhdr->type) !=ETHTYPE_ARP) )
    {
        LWIP_DEBUGF(NETIF_DEBUG ,("ethernet_input frame is my send,drop it\r\n"));
        goto free_pbuf;
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
        if (netif->input(p, netif) != ERR_OK)	 // ethernet_input
        {
            LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\r\n"));
            pbuf_free(p);
            p = NULL;
        }
        break;

    case ETHTYPE_EAPOL:
        LWIP_DEBUGF(NETIF_DEBUG, ("panif_input: EAPOL frame dropped\r\n"));
        break;

    default:
        LWIP_DEBUGF(NETIF_DEBUG, ("panif_input: unsupported frame type: 0x%04x\r\n", htons(ethhdr->type)));
        break;
    }
free_pbuf:
     pbuf_free(p);

}

err_t
modem_netif_init(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;

    netif->output = etharp_output;
    netif->linkoutput = modem_low_level_output;

    /* initialize the hardware */
    modem_low_level_init(netif);
    return ERR_OK;

}


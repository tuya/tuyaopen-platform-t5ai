#ifndef _NET_H_
#define _NET_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "lwip_netif_address.h"

extern void uap_ip_down(void);
extern void uap_ip_start(void);
extern void sta_ip_down(void);
extern void sta_ip_start(void);
extern uint32_t uap_ip_is_start(void);
extern uint32_t sta_ip_is_start(void);
extern void *net_get_sta_handle(void);
extern void *net_get_uap_handle(void);
extern int net_wlan_remove_netif(uint8_t *mac);
extern int net_get_if_macaddr(void *macaddr, void *intrfc_handle);
extern int net_get_if_addr(struct wlan_ip_config *addr, void *intrfc_handle);
extern void ip_address_set(int iface, int dhcp, char *ip, char *mask, char*gw, char*dns);
extern void sta_ip_mode_set(int dhcp);
#if CONFIG_WIFI6_CODE_STACK
extern bool etharp_tmr_flag;
extern void net_begin_send_arp_reply(bool is_send_arp, bool is_allow_send_req);
#endif
extern void net_restart_dhcp(void);
#ifdef CONFIG_ETH
extern int net_eth_add_netif(uint8_t *mac);
extern int net_eth_remove_netif(void);
extern void *net_get_eth_handle(void);
extern void eth_ip_start(void);
extern void eth_ip_down(void);
#endif
#if CONFIG_BRIDGE
extern void bridge_set_ip_start_flag(bool enable);
extern void bridge_ip_start(void);
extern void bridge_ip_stop(void);
extern uint32_t bridge_ip_is_start(void);
extern void *net_get_br_handle(void);
#endif
#if CONFIG_PAN
void net_pan_init(void);
extern void *net_get_pan_handle(void);
extern void pan_ip_start(void);
extern void pan_set_ip_start_flag(bool enable);
extern uint32_t pan_ip_is_start(void);
bk_err_t bk_pan_get_mac(uint8_t *mac);
// Modified by TUYA Start
extern void pan_netif_notify_got_ip(void);
extern void pan_ip_down(void);
int net_pan_remove_netif(void);
#endif
#if CONFIG_BK_MODEM
extern void net_modem_init(void);
extern void *net_get_modem_handle(void);
extern void modem_ip_start(void);
extern void modem_set_ip_start_flag(bool enable);
extern uint32_t modem_ip_is_start(void);
bk_err_t bk_modem_get_mac(uint8_t *mac);
extern void modem_ip_down(void);
extern int net_modem_remove_netif(void);
extern void modem_netif_notify_got_ip(void);
#endif
// Modified by TUYA End
#ifdef CONFIG_WIFI_VNET_CONTROLLER
int host_wlan_add_netif(uint8_t *mac);
int host_wlan_remove_netif(void);
int host_wlan_remove_sap_netif(void);
#endif
#if CONFIG_LWIP_PPP_SUPPORT
void *net_get_ppp_netif_handle(void);
void *net_get_ppp_pcb_handle(void);
void net_set_ppp_pcb_handle(void *ppp);  
uint32_t ppp_ip_is_start(void);
// Modified by TUYA Start
void ppp_ip_start(void);
void ppp_ip_down(void);
// Modified by TUYA End
#endif
// Modified by TUYA Start
#if LWIP_NETIF_LOOPBACK
void bk_netif_trigger_loopnetif_msg(void);
#endif
void bk_netif_add_dns_server(uint8_t idx, const char* szIpv4);
// Modified by TUYA End
#ifdef __cplusplus
}
#endif

#endif // _NET_H_
// eof


#include "tuya_cloud_types.h"
#include "tuya_error_code.h"
#include "ethernetif.h"
#include "lwip/netifapi.h"
#include "net.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "tkl_wired.h"
#include "tkl_thread.h"
#include "tkl_mutex.h"
#include "tkl_system.h"
#include "spi_eth_drv.h"

#define IPADDR2STR(__addr)    (uint8_t)((__addr) >> 24), (uint8_t)((__addr) >> 16), (uint8_t)((__addr) >> 8), (__addr) & 0xFF

TKL_WIRED_STATUS_CHANGE_CB netif_link_chg_cb = NULL;
extern TKL_WIRED_BASE_CFG_T eth_cfg;
extern int net_eth_start();

/**
 * @brief  init create wired link
 *
 * @param[in]   cfg: the configure for wired link
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wired_init(TKL_WIRED_BASE_CFG_T *cfg)
{
#ifdef CONFIG_ETH
	net_eth_start();
#endif
    return OPRT_OK;
}

/**
 * @brief  get the link status of wired link
 *
 * @param[out]  is_up: the wired link status is up or not
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wired_get_status(TKL_WIRED_STAT_E *status)
{
    struct netif *netif;
    uint32_t ip;
    uint32_t mask;
    uint32_t gw;
#ifdef CONFIG_ETH
    netif = (struct netif *)net_get_eth_handle();
    if (NULL == netif) {
         return OPRT_COM_ERROR;
    }

    ip = netif->ip_addr.addr;
    mask = netif->netmask.addr;
    gw = netif->gw.addr;

    if (netif_is_up(netif) && ip && mask && gw) {
         *status = TKL_WIRED_LINK_UP;
    } else {
         *status = TKL_WIRED_LINK_DOWN;
    }
#endif
    return OPRT_OK;
}

/**
 * @brief  set the status change callback
 *
 * @param[in]   cb: the callback when link status changed
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wired_set_status_cb(TKL_WIRED_STATUS_CHANGE_CB cb)
{
    netif_link_chg_cb = cb;
    return OPRT_OK;
}

/**
 * @brief  get the ip address of the wired link
 * 
 * @param[in]   ip: the ip address
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wired_get_ip(NW_IP_S *ip)
{
    unsigned int ip_addr;
#ifdef CONFIG_ETH
    struct netif * netif = net_get_eth_handle();
    if (NULL == netif) {
         return OPRT_COM_ERROR;
    }
    ip_addr = netif->ip_addr.addr;
    sprintf(ip->ip, "%d.%d.%d.%d", IPADDR2STR(ip_addr));

    ip_addr = netif->gw.addr;
    sprintf(ip->gw, "%d.%d.%d.%d", IPADDR2STR(ip_addr));

    ip_addr = netif->netmask.addr;
    sprintf(ip->mask, "%d.%d.%d.%d", IPADDR2STR(ip_addr));
#endif
    return OPRT_OK;
}

/**
 * @brief  set the ip address of the wired link
 * 
 * @param[in]   ip: the ip address
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wired_set_ip(NW_IP_S *ip)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief  get the mac address of the wired link
 * 
 * @param[in]   mac: the mac address
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wired_get_mac(NW_MAC_S *mac)
{
#ifdef CONFIG_ETH
    struct netif * netif = net_get_eth_handle();
    if (NULL == netif) {
         return OPRT_COM_ERROR;
    }
    memcpy(mac->mac, netif->hwaddr, 6);
#endif
    return OPRT_OK;
}

/**
 * @brief  set the mac address of the wired link
 * 
 * @param[in]   mac: the mac address
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wired_set_mac(const NW_MAC_S *mac)
{
    return OPRT_NOT_SUPPORTED;
}

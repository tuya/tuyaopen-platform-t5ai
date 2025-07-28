#include "tuya_cloud_types.h"
#include "tuya_error_code.h"
#include "ethernetif.h"
#include "lwip/netifapi.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "tkl_wired.h"
#include "tkl_thread.h"
#include "tkl_mutex.h"
#include "tkl_system.h"
#include "spi_eth_drv.h"

#if CONFIG_SPI_ETH
TKL_WIRED_STATUS_CHANGE_CB spi_netif_link_chg_cb = NULL;
extern TKL_WIRED_BASE_CFG_T spi_eth_cfg;
#endif /* CONFIG_SPI_ETH */

/**
 * @brief  init create wired link
 *
 * @param[in]   cfg: the configure for wired link
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wired_init(TKL_WIRED_BASE_CFG_T *cfg)
{
#if CONFIG_SPI_ETH
    spi_eth_cfg.int_gpio = cfg->int_gpio;
    if (net_spi_eth_init() < 0) {
        SPI_LAN_ERR("%s: call net_spi_eth_init failed", __func__);
        return OPRT_COM_ERROR;
    }
    
    SPI_LAN_DBG("%s: init wired int_gpio %d",
        __func__, spi_eth_cfg.int_gpio);
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
#if CONFIG_SPI_ETH    
    struct netif *netif;
    uint32_t ip;
    uint32_t mask;
    uint32_t gw;

    netif = (struct netif *)net_get_spi_eth_handle();
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

    SPI_LAN_DBG("%s: wired link status %d", __func__, *status);
#endif /* CONFIG_SPI_ETH */
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
#if CONFIG_SPI_ETH     
    spi_netif_link_chg_cb = cb;
#endif /* CONFIG_SPI_ETH */
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
#if CONFIG_SPI_ETH     
    unsigned int ip_addr;
    struct netif *netif;

    netif = (struct netif *)net_get_spi_eth_handle();
    if (NULL == netif) {
         return OPRT_COM_ERROR;
    }

    ip_addr = netif->ip_addr.addr;
    sprintf(ip->ip, "%d.%d.%d.%d", IPADDR2STR(ip_addr));

    ip_addr = netif->gw.addr;
    sprintf(ip->gw, "%d.%d.%d.%d", IPADDR2STR(ip_addr));

    ip_addr = netif->netmask.addr;
    sprintf(ip->mask, "%d.%d.%d.%d", IPADDR2STR(ip_addr));

    //SPI_LAN_DBG("%s: get wired ip %s mask %s gw %s",
    //    __func__, ip->ip, ip->mask, ip->gw);
#endif /* CONFIG_SPI_ETH */    
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
#if CONFIG_SPI_ETH    
    struct netif *netif;

    netif = (struct netif *)net_get_spi_eth_handle();
    if (NULL == netif) {
         return OPRT_COM_ERROR;
    }

    memcpy(mac->mac, netif->hwaddr, 6);
    //SPI_LAN_DBG("%s: get wired mac address %02x:%02x:%02x:%02x:%02x:%02x",
    //    __func__, mac->mac[0], mac->mac[1], mac->mac[2], mac->mac[3], mac->mac[4], mac->mac[5]);
#endif /* CONFIG_SPI_ETH */        
    return OPRT_OK;
}

/**
 * @brief  set the mac address of the wired link
 * 
 * @param[in]   mac: the mac address
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wired_set_mac(CONST NW_MAC_S *mac)
{
    return OPRT_NOT_SUPPORTED;
}

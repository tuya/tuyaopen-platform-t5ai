/**
* @file tkl_cellular.h
* @brief Common process - adapter the cellular api
* @version 0.1
* @date 2025-04-21
*
* @copyright Copyright 2020-2025 Tuya Inc. All Rights Reserved.
*
*/
#include <string.h>
#include "tuya_cloud_types.h"
#include "tuya_error_code.h"
#include "sdkconfig.h"
#include "ethernetif.h"
#include "lwip/netifapi.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "tkl_gpio.h"
#include "tkl_cellular.h"
#include "tkl_thread.h"
#include "tkl_mutex.h"
#include "tkl_system.h"
#include "tkl_lwip.h"
#include "spi_eth_drv.h"
#if CONFIG_LWIP_PPP_SUPPORT
#include "net.h"
#endif
#if CONFIG_BK_MODEM
#include "bk_modem_dce.h"
#endif

#ifndef IPADDR2STR
#define IPADDR2STR(ip) (unsigned char)(ip & 0xFF), (unsigned char)((ip >> 8) & 0xFF), \
                        (unsigned char)((ip >> 16) & 0xFF), (unsigned char)((ip >> 24) & 0xFF)
#endif /* IPADDR2STR */

#if CONFIG_BK_MODEM
// extern bk_err_t bk_modem_init(struct bk_modem_dce_pdp_ctx_s *ctx);
extern bk_err_t bk_modem_init(void);
#if CONFIG_LWIP_PPP_SUPPORT
TKL_CELLULAR_STATUS_CHANGE_CB ppp_netif_link_chg_cb = NULL;
#endif /* CONFIG_LWIP_PPP_SUPPORT */
#endif /* CONFIG_BK_MODEM */

/**
 * @brief  init create cellular link
 *
 * @param[in]   cfg: the configure for cellular link
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_cellular_init(TKL_CELLULAR_BASE_CFG_T *cfg)
{
#if CONFIG_BK_MODEM
    // struct bk_modem_dce_pdp_ctx_s ctx;
    // memset(&ctx, 0, sizeof(ctx));
    // strncpy(ctx.apn, cfg->apn, TKL_CELLULAR_APN_LEN);
    // bk_printf("%s: Start USB Cellular Network\r\n", __func__);
    // bk_modem_init(&ctx);
    bk_modem_init();
    return OPRT_OK;
#else
    return OPRT_NOT_SUPPORTED;
#endif
}

/**
 * @brief  get the link status of cellular link
 *
 * @param[out]  is_up: the cellular link status is up or not
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_cellular_get_status(TKL_CELLULAR_STAT_E *status)
{
#if CONFIG_BK_MODEM
#if CONFIG_LWIP_PPP_SUPPORT
    struct netif *netif;
    uint32_t ip;
    uint32_t mask;
    uint32_t gw;

    // netif = (struct netif *)tkl_lwip_get_netif_by_index(3);
    netif = (struct netif *)net_get_ppp_netif_handle();
    if (NULL == netif) {
        *status = TKL_CELLULAR_LINK_DOWN;
        return OPRT_COM_ERROR;
    }

    ip = netif->ip_addr.addr;
    mask = netif->netmask.addr;
    gw = netif->gw.addr;

    if (netif_is_up(netif) && ip && mask && gw) {
        *status = TKL_CELLULAR_LINK_UP;
    } else {
        *status = TKL_CELLULAR_LINK_DOWN;
    }

    //bk_printf("%s: wired link status %d\r\n", __func__, *status);
    return OPRT_OK;
#endif /* CONFIG_LWIP_PPP_SUPPORT */
#else  
    return OPRT_NOT_SUPPORTED;
#endif /* CONFIG_BK_MODEM */
}

/**
 * @brief  set the status change callback
 *
 * @param[in]   cb: the callback when link status changed
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_cellular_set_status_cb(TKL_CELLULAR_STATUS_CHANGE_CB cb)
{
#if CONFIG_BK_MODEM
#if CONFIG_LWIP_PPP_SUPPORT
    ppp_netif_link_chg_cb = cb;
    // bk_printf("%s: Set status cb %p\r\n", __func__, cb);
    return OPRT_OK;
#endif
#else
    return OPRT_NOT_SUPPORTED;
#endif /* CONFIG_BK_MODEM */    
}

/**
 * @brief  get the ip address of the cellular link
 * 
 * @param[in]   ip: the ip address
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_cellular_get_ip(NW_IP_S *ip)
{
#if CONFIG_BK_MODEM    
#if CONFIG_LWIP_PPP_SUPPORT
    unsigned int ip_addr;
    struct netif *netif;

    // netif = (struct netif *)tkl_lwip_get_netif_by_index(3);
    netif = (struct netif *)net_get_ppp_netif_handle();
    if (NULL == netif) {
        return OPRT_COM_ERROR;
    }

    ip_addr = netif->ip_addr.addr;
    sprintf(ip->ip, "%d.%d.%d.%d", IPADDR2STR(ip_addr));

    ip_addr = netif->gw.addr;
    sprintf(ip->gw, "%d.%d.%d.%d", IPADDR2STR(ip_addr));

    ip_addr = netif->netmask.addr;
    sprintf(ip->mask, "%d.%d.%d.%d", IPADDR2STR(ip_addr));

    //bk_printf("%s: get wired ip %s mask %s gw %s\r\n",
    //    __func__, ip->ip, ip->mask, ip->gw);
    return OPRT_OK;
#endif
#else
    bk_printf("%s: Cellular link not supported\r\n", __func__);
    return OPRT_NOT_SUPPORTED;
#endif    
}

/**
 * @brief  get the ip address of the cellular link
 * 
 * @param[in]   ip: the ip address
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_cellular_get_ipv6(NW_IP_TYPE type, NW_IP_S *ip)
{
    return OPRT_NOT_SUPPORTED;
}


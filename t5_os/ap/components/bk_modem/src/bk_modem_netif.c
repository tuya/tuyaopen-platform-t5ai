/**
 ****************************************************************************************
 *
 * @file bk_modem_netif.c
 *
 * @brief Interface with PPP, lwip, netif, network-related ports, and external PPP interface encapsulation.
 *
 ****************************************************************************************
 */
#include "lwip/dns.h"
#include "netif/ppp/pppapi.h"
#include "netif/ppp/pppos.h"
#include <../../lwip_intf_v2_1/lwip-2.1.2/port/net.h>
#include <string.h>
#include "lwip/ip6_addr.h"
#include "components/log.h"
#include <components/event.h>
#include <components/netif.h>
#include <components/netif_types.h>
#include "common/bk_err.h"
#include "bk_modem_dte.h"

#if CONFIG_LWIP_PPP_SUPPORT
#if PPP_SUPPORT && PPP_AUTH_SUPPORT
typedef struct {
    struct tcpip_api_call_data call;
    ppp_pcb *ppp;
    u8_t authtype;
    const char *user;
    const char *passwd;
} set_auth_msg_t;

int bk_modem_ppp_netif_event_cb(void *arg, event_module_t event_module,
					   int event_id, void *event_data);

static err_t pppapi_do_ppp_set_auth(struct tcpip_api_call_data *m)
{
    set_auth_msg_t *msg = (set_auth_msg_t *)m;
    ppp_set_auth(msg->ppp, msg->authtype, msg->user, msg->passwd);
    return ERR_OK;
}

static void pppapi_set_auth(ppp_pcb *pcb, u8_t authtype, const char *user, const char *passwd)
{
    set_auth_msg_t msg = { .ppp = pcb, .authtype = authtype, .user = user, .passwd = passwd};
    tcpip_api_call(pppapi_do_ppp_set_auth, &msg.call);
}
#endif // PPP_SUPPORT && PPP_AUTH_SUPPORT

/**
 * @brief lwip callback from PPP client used here to produce PPP error related events,
 * as well as some IP events
 */
static void on_ppp_status_changed(ppp_pcb *pcb, int err_code, void *ctx)
{
    struct netif *netif = pcb->netif;
    netif_event_got_ip4_t event_data = {0};

    switch (err_code) {
        case PPPERR_NONE:
            event_data.netif_if = NETIF_IF_PPP;
            BK_MODEM_LOGI("Connected\r\n");
            BK_MODEM_LOGI("IP address: %s\r\n", ip4addr_ntoa(netif_ip4_addr(netif)));
            BK_MODEM_LOGI("Gateway: %s\r\n", ip4addr_ntoa(netif_ip4_gw(netif)));
            BK_MODEM_LOGI("Netmask: %s\r\n", ip4addr_ntoa(netif_ip4_netmask(netif)));
#if IP_NAPT
extern const ip_addr_t *sta_dns;
            sta_dns = dns_getserver(0);
#endif
            bk_event_post(EVENT_MOD_NETIF, EVENT_NETIF_GOT_IP4,
                                   &event_data, sizeof(event_data), BEKEN_NEVER_TIMEOUT);
            break;
        case PPPERR_PARAM:
            BK_MODEM_LOGI("Invalid parameter\r\n");
            break;
        case PPPERR_OPEN:
            BK_MODEM_LOGI("Unable to open PPP session\r\n");
            break;
        case PPPERR_DEVICE:
            BK_MODEM_LOGI("Invalid I/O device for PPP\r\n");
            break;
        case PPPERR_ALLOC:
            BK_MODEM_LOGI("Unable to allocate resources\r\n");
            break;
        case PPPERR_USER: /* User interrupt */
            BK_MODEM_LOGI("User interrupt\r\n");
            break;
        case PPPERR_CONNECT: /* Connection lost */
            BK_MODEM_LOGI("Connection lost\r\n");
            //TODO post connection lost event
            bk_modem_set_state(PPP_STOP);
            bk_modem_send_msg(MSG_PPP_STOP, ABNORMAL_STOP,0,0);
            return;

        case PPPERR_AUTHFAIL:
            BK_MODEM_LOGE("Failed authentication challenge\r\n");
            ///bk_modem_send_msg(MSG_PPP_STOP, 0,0,0);
            ///maybe need to stop PPP and restart
            break;
        case PPPERR_PROTOCOL:
            BK_MODEM_LOGE("Failed to meet protocol\r\n");
            break;
        case PPPERR_PEERDEAD:
            BK_MODEM_LOGE("Connection timeout\r\n");
            break;
        case PPPERR_IDLETIMEOUT:
            BK_MODEM_LOGE("Idle Timeout\r\n");
            break;
        case PPPERR_CONNECTTIME:
            BK_MODEM_LOGE("Max connect time reached\r\n");
            break;
        case PPPERR_LOOPBACK:
            BK_MODEM_LOGE("Loopback detected\r\n");
            break;
        default:
            BK_MODEM_LOGE("Unknown error code %d\r\n", err_code);
            break;
    }
}

#if PPP_NOTIFY_PHASE
/**
 * @brief Notify phase callback which is called on each PPP internal state change
 *
 * @param pcb PPP control block
 * @param phase Phase ID
 * @param ctx Context of callback
 */
static void on_ppp_notify_phase(ppp_pcb *pcb, u8_t phase, void *ctx)
{
    switch (phase) {
        case PPP_PHASE_DEAD:
            BK_MODEM_LOGV("Phase Dead\r\n");
            break;
        case PPP_PHASE_INITIALIZE:
            BK_MODEM_LOGV("Phase Start\r\n");
            break;
        case PPP_PHASE_ESTABLISH:
            BK_MODEM_LOGV("Phase Establish\r\n");
            break;
        case PPP_PHASE_AUTHENTICATE:
            BK_MODEM_LOGV("Phase Authenticate\r\n");
            break;
        case PPP_PHASE_NETWORK:
            BK_MODEM_LOGV("Phase Network\r\n");
            break;
        case PPP_PHASE_RUNNING:
            BK_MODEM_LOGV("Phase Running\r\n");
            break;
        case PPP_PHASE_TERMINATE:
            BK_MODEM_LOGV("Phase Terminate\r\n");
            break;
        case PPP_PHASE_DISCONNECT:
            BK_MODEM_LOGV("Phase Disconnect\r\n");
            break;
        default:
            BK_MODEM_LOGW("Phase Unknown: %d\r\n", phase);
            break;
    }
}
#endif // PPP_NOTIFY_PHASE

/**
 * @brief PPP low level output callback used to transmit data using standard esp-netif interafce
 *
 * @param pcb PPP control block
 * @param data Buffer to write to serial port
 * @param len Length of the data buffer
 * @param ctx Context of callback
 *
 * @return uint32_t Length of data successfully sent
 */
static uint32_t pppos_low_level_output(ppp_pcb *pcb, uint8_t *data, uint32_t len, void *netif)
{
    bk_modem_dte_send_data(len, data, PPP_DATA_MODE);
    //TODO need to return bk_modem_dte_send_data 
    return len;
}

bk_err_t bk_modem_netif_ppp_set_auth(uint8_t authtype, const char *user, const char *passwd)
{
    ppp_pcb *ppp = net_get_ppp_pcb_handle();

    if (!ppp) {
        BK_MODEM_LOGE("%s, %d\r\n", __func__, __LINE__);
        return BK_FAIL;
    }
#if PPP_AUTH_SUPPORT
    pppapi_set_auth(ppp, authtype, user, passwd);
    return BK_OK;
#else
    BK_MODEM_LOGE("%s failed: No authorisation enabled in menuconfig\r\n", __func__);
    return BK_FAIL;
#endif
}

void bk_modem_netif_ppp_set_default_netif(void)
{
    ppp_pcb *ppp = net_get_ppp_pcb_handle();

    if (!ppp) {
        BK_MODEM_LOGE("%s, %d\r\n", __func__, __LINE__);
        return;
    }
    ppp_set_default(ppp);
}

int bk_modem_ppp_netif_event_cb(void *arg, event_module_t event_module,
					   int event_id, void *event_data)
{
    netif_event_got_ip4_t *got_ip;

    switch (event_id) {
    case EVENT_NETIF_GOT_IP4:
		got_ip = (netif_event_got_ip4_t *)event_data;
		if (got_ip->netif_if == NETIF_IF_PPP)
			BK_MODEM_LOGI("BK PPP got ip\r\n");
		break;
	default:
		break;
	}
	return BK_OK;
}

bk_err_t bk_modem_netif_new_ppp(void)
{
    struct netif *netif = net_get_ppp_netif_handle();
    ppp_pcb *ppp = net_get_ppp_pcb_handle();

    ppp = pppapi_pppos_create(netif, pppos_low_level_output, on_ppp_status_changed, NULL);
    BK_MODEM_LOGI("%s: PPP connection created: %p\r\n", __func__, ppp);
    if (!ppp) {
        BK_MODEM_LOGE("%s: lwIP PPP connection cannot be created\r\n", __func__);
        return BK_FAIL;
    }
    net_set_ppp_pcb_handle(ppp);
#if PPP_NOTIFY_PHASE
    ppp_set_notify_phase_callback(ppp, on_ppp_notify_phase);
#endif
    ppp_set_usepeerdns(ppp, 1);

    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, bk_modem_ppp_netif_event_cb, NULL));
    return BK_OK;
}

bk_err_t bk_modem_netif_start_ppp(void)
{
    ppp_pcb *ppp = net_get_ppp_pcb_handle();

    if (!ppp) {
        BK_MODEM_LOGE("%s, %d\r\n", __func__, __LINE__);
        return BK_FAIL;
    }
    BK_MODEM_LOGV("%s: Starting PPP connection: %p\r\n", __func__, ppp);
    bk_err_t err = pppapi_connect(ppp, 0);
    if (err != BK_OK) {
        BK_MODEM_LOGE("%s: PPP connection cannot be started\r\n", __func__);
        //TODO post connect fail
        return BK_FAIL;
    }
    return BK_OK;
}

bk_err_t bk_modem_netif_lwip_ppp_input(void *buffer, size_t len)
{
    ppp_pcb *ppp = net_get_ppp_pcb_handle();

    if (!ppp) {
        BK_MODEM_LOGE("%s, %d\r\n", __func__, __LINE__);
        return BK_FAIL;
    }
    err_t ret = pppos_input_tcpip(ppp, buffer, len);
    if (ret != ERR_OK) {
        BK_MODEM_LOGE("pppos_input_tcpip failed with %d\r\n", ret);
        return BK_FAIL;
    }
    return BK_OK;
}

bk_err_t bk_modem_netif_stop_ppp(void)
{
    ppp_pcb *ppp = net_get_ppp_pcb_handle();

    if (!ppp) {
        BK_MODEM_LOGE("%s, %d\r\n", __func__, __LINE__);
        return BK_FAIL;
    }
    BK_MODEM_LOGV("%s: Stopped PPP connection: %p\r\n", __func__, ppp);
    err_t ret = pppapi_close(ppp, 1);
    if (ret != ERR_OK) {
        BK_MODEM_LOGE("pppapi_close failed with %d\r\n", ret);
        return BK_FAIL;
    }
    return BK_OK;
}

void bk_modem_netif_destroy_ppp(void)
{
    ppp_pcb *ppp = net_get_ppp_pcb_handle();

    if (!ppp) {
        BK_MODEM_LOGE("%s, %d\r\n", __func__, __LINE__);
        return;
    }
    pppapi_free(ppp);
}

#endif

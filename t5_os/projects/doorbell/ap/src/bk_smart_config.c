#include "wdrv_cntrl.h"
#include <components/log.h>
#include "doorbell_boarding.h"
#include <components/netif.h>
#include <components/event.h>


#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)
#define STREAM_TO_UINT16(u16, p) {u16 = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); (p) += 2;}


#define TAG "bk_sconf"

static int bk_sconf_netif_event_cb(void *arg, event_module_t event_module, int event_id, void *event_data)
{
    netif_event_got_ip4_t *got_ip;
    __maybe_unused wifi_sta_config_t sta_config = {0};

    switch (event_id)
    {
        case EVENT_NETIF_GOT_IP4:
            got_ip = (netif_event_got_ip4_t *)event_data;
            LOGD("%s got ip\n", got_ip->netif_if == NETIF_IF_STA ? "STA" : "unknown netif");
            break;
        default:
            LOGD("rx event <%d %d>\n", event_module, event_id);
            break;
    }

    return BK_OK;
}

static int bk_sconf_wifi_event_cb(void *arg, event_module_t event_module, int event_id, void *event_data)
{
    __maybe_unused wifi_event_sta_disconnected_t *sta_disconnected;
    wifi_event_sta_connected_t *sta_connected;

    switch (event_id)
    {
        case EVENT_WIFI_STA_CONNECTED:
            sta_connected = (wifi_event_sta_connected_t *)event_data;
            LOGD("STA connected to %s\n", sta_connected->ssid);
            break;

        case EVENT_WIFI_STA_DISCONNECTED:
            sta_disconnected = (wifi_event_sta_disconnected_t *)event_data;
            LOGD("STA disconnected, reason(%d)\n", sta_disconnected->disconnect_reason);
            break;

        default:
            LOGD("rx event <%d %d>\n", event_module, event_id);
            break;
    }

    return BK_OK;
}

void event_handler_init(void)
{
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, bk_sconf_wifi_event_cb, NULL));
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, bk_sconf_netif_event_cb, NULL));
}

int bk_smart_config_init(void)
{
    event_handler_init();

    bk_customer_event_register_callback(bk_rx_handle_customer_event);

    return 0;
}

void bk_rx_handle_customer_event(void *data, uint16_t len)
{
    CIFD_CUST_DATA *cifd_cust_data = (CIFD_CUST_DATA *)data;
    uint16_t opcode, length;
    uint8_t *customer_data = cifd_cust_data->data;
    LOGD("%s, %d, %d\n", __func__, __LINE__, cifd_cust_data->header.cid);
    switch(cifd_cust_data->header.cid) {
        case CIFD_EVENT_BLE_DATA_TO_USER:
            STREAM_TO_UINT16(opcode, customer_data);
            STREAM_TO_UINT16(length, customer_data);
            doorbell_boarding_operation_handle(opcode, length, customer_data);
            break;
        default:
            LOGD("%s, %d\n", __func__, __LINE__);
            break;
     }
}


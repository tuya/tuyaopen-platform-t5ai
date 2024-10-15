#include <stdlib.h>
#include <string.h>

#include <common/bk_include.h>
#include <common/bk_err.h>
#include <os/os.h>
#include <os/mem.h>

#include <modules/wifi.h>
#include <modules/wifi_types.h>
#include <modules/raw_link.h>
#include "rlk_control_client.h"
#include "rlk_multimedia_client.h"
#include "rlk_ps_client.h"
#include "modules/pm.h"

#include <driver/media_types.h>
#include <driver/lcd_types.h>

rlk_local_env_t rlk_client_local_env;
static beken_queue_t rlk_client_queue;
static beken_thread_t rlk_cntrl_client_handle;
beken_timer_t rlk_cntrl_client_timer = {0};
beken_timer_t rlk_client_wake_dur_timer = {0};
beken_semaphore_t s_rlk_cntrl_client_sem;

#if 0
video_device_t video_dev = {
    .device.type = UVC_CAMERA,
    .device.mode = JPEG_MODE,
    .device.fmt = PIXEL_FMT_JPEG,
    .device.info.resolution.width = 800,
    .device.info.resolution.height = 480,
    .device.info.fps = FPS25,
    .lcd_dev.device_ppi = PPI_480X272,
    .lcd_dev.device_name = "st7282",
};
#endif

void rlk_client_wakeup_from_lowvoltage()
{
    struct rlk_msg_t msg;
    msg.msg_id = RLK_MSG_WAKEUP_SLEEP;
    msg.arg = 0;
    msg.len = 0;

    if (BK_OK != rtos_push_to_queue(&rlk_client_queue, &msg, BEKEN_NO_WAIT))
    {
        LOGI("RLK client timer push MSG failed\n");
    }
}

bk_err_t rlk_cntrl_client_send_callback(const uint8_t *peer_mac_addr, bk_rlk_send_status_t status);

static void rlk_cntrl_client_timer_handler(void)
{
    uint32_t time = 0;
    struct rlk_msg_t msg;

    if (rlk_client_local_env.state != RLK_STATE_CONNECTED)
    {
        return;
    }

    time = rtos_get_time();
    if ((time > rlk_client_local_env.last_rx_tick) && 
        ((time - rlk_client_local_env.last_rx_tick) > RLK_KEEPALIVE_TIME_INTERVAL))
    {
        LOGI("RLK client keepalive timeout, update connect to PROBEING\n");
        rlk_client_local_env.state = RLK_STATE_PROBEING;
        //rlk_mm_client_video_aud_deinit();
        bk_rlk_register_send_cb(rlk_cntrl_client_send_callback);

        msg.msg_id = RLK_MSG_TX_MGMT_CB;
        msg.arg = 0;
        msg.len = 0;

        if (BK_OK != rtos_push_to_queue(&rlk_client_queue, &msg, BEKEN_NO_WAIT))
        {
            LOGI("RLK client timer push MSG failed\n");
        }
        return;
    }

    rtos_start_timer(&rlk_cntrl_client_timer);
}

///ask rlk client go to sleep only from cli
void rlk_client_ps_cmd_handler(void)
{
    rtos_stop_timer(&rlk_cntrl_client_timer);
    bk_rlk_sleep();
    rlk_mm_client_prepare_sleep();
    rlk_client_local_env.state = RLK_STATE_SLEEP;
    rlk_client_rtc_sleep_start(PM_MODE_LOW_VOLTAGE, RLK_LP_RTC_PS_INTERVAL, PM_SLEEP_MODULE_NAME_APP, 0, 0, 0);
}

//// rlk client wake duration time end,pearper to sleep
void rlk_client_wake_duration_end_handler(void)
{
    if (rlk_client_local_env.state == RLK_STATE_SLEEP)
    {
        rtos_stop_timer(&rlk_cntrl_client_timer);
        rtos_stop_timer(&rlk_client_wake_dur_timer);
        bk_rlk_sleep();
        rlk_client_local_env.state = RLK_STATE_SLEEP;
        rlk_client_rtc_sleep_start(PM_MODE_LOW_VOLTAGE, RLK_LP_RTC_PS_INTERVAL, PM_SLEEP_MODULE_NAME_APP, 0, 0, 0);
    }
    else
    {
        rtos_stop_timer(&rlk_client_wake_dur_timer);
    }
}

bk_err_t rlk_cntrl_client_add_peer(const uint8_t *mac_addr)
{
    bk_rlk_peer_info_t peer;
    peer.channel = rlk_client_local_env.curr_channel;
    peer.ifidx = rlk_client_local_env.ifidx;
    peer.state = rlk_client_local_env.state;
    peer.encrypt = rlk_client_local_env.encrypt;
    memcpy(peer.mac_addr, mac_addr, RLK_WIFI_MAC_ADDR_LEN);

    return bk_rlk_add_peer(&peer);
}

bk_err_t rlk_cntrl_client_recv_process(bk_rlk_recv_info_t *rx_info)
{
    bk_err_t ret = BK_OK;
    bk_rlk_recv_info_t *rx_info_local = NULL;
    struct rlk_msg_t msg;
    uint8_t *data = rx_info->data;
    rlk_mm_pre_header_t *header = (rlk_mm_pre_header_t *)data;

    if (!rlk_client_local_env.is_inited)
    {
        return BK_FAIL;
    }

    if ((header->data_type != RLK_MM_HEADER_TYPE_MGMT) &&
        (header->data_type != RLK_MM_HEADER_TYPE_DATA))
    {
        LOGI("unknow data type %x\n", header->data_type);
        return BK_FAIL;
    }

    rx_info_local = os_malloc(sizeof(bk_rlk_recv_info_t) + rx_info->len);

    if (rx_info_local == NULL)
    {
        return BK_FAIL;
    }

    os_memcpy(rx_info_local, rx_info, sizeof(bk_rlk_recv_info_t));
    rx_info_local->data = (uint8_t *)rx_info_local + sizeof(bk_rlk_recv_info_t);
    os_memcpy(rx_info_local->data, rx_info->data, rx_info->len);

    if (header->data_type == RLK_MM_HEADER_TYPE_MGMT)
    {
        msg.msg_id = RLK_MSG_RX_MGMT;
        msg.arg = (uint32_t)rx_info_local;
        msg.len = 0;
    }
    else if (header->data_type == RLK_MM_HEADER_TYPE_DATA)
    {
        msg.msg_id = RLK_MSG_RX_DATA;
        msg.arg = (uint32_t)rx_info_local;
        msg.len = 0;
    }

    ret = rtos_push_to_queue(&rlk_client_queue, &msg, BEKEN_NO_WAIT);

    if (ret != BK_OK)
    {
        os_free(rx_info_local);
        return BK_FAIL;
    }

    return ret;
}
bk_err_t rlk_cntrl_client_send_callback(const uint8_t *peer_mac_addr, bk_rlk_send_status_t status)
{
    bk_err_t ret = BK_OK;
    struct rlk_msg_t msg;

    if (!rlk_client_local_env.is_inited)
    {
        return BK_FAIL;
    }

    msg.msg_id = RLK_MSG_TX_MGMT_CB;
    msg.arg = status;
    msg.len = 0;

    ret = rtos_push_to_queue(&rlk_client_queue, &msg, BEKEN_NO_WAIT);

    return ret;
}

bk_err_t rlk_cntrl_client_tx_mgmt_data(const uint8_t *mac_addr, uint8_t msg_type)
{
    bk_err_t ret;
    rlk_mm_pre_header_t header;

    header.data_type = RLK_MM_HEADER_TYPE_MGMT;
    header.data_subtype = msg_type;

    ret = bk_rlk_send(mac_addr, &header, sizeof(header));

    if (ret < 0)
    {
        LOGI("rlk_cntrl_client_tx_mgmt_data failed ret:%d\n", ret);
    }

    return ret;
}

void rlk_cntrl_client_handle_data_rx(struct rlk_msg_t msg)
{
    bk_err_t ret = BK_OK;
    bk_rlk_recv_info_t *rx_info = (bk_rlk_recv_info_t *)msg.arg;
    uint8_t *data = rx_info->data;

    ret = rlk_mm_client_recv(data, rx_info->len);

    if (ret == BK_OK)
        rlk_client_local_env.last_rx_tick = rtos_get_time();
    os_free(rx_info);
}

void rlk_cntrl_client_handle_mgmt_rx(struct rlk_msg_t msg)
{
    bk_rlk_recv_info_t *rx_info = (bk_rlk_recv_info_t *)msg.arg;
    uint8_t *data = rx_info->data;
    rlk_mm_pre_header_t *header = (rlk_mm_pre_header_t *)data;

    if (header->data_type != RLK_MM_HEADER_TYPE_MGMT)
    {
        os_free(rx_info);
        return;
    }

    if ((rlk_client_local_env.state == RLK_STATE_SLEEP) && (header->data_subtype == RLK_MM_HEADER_MGMT_SUBTYPE_WAKEUP_REQ))
    {
        // TX WAKEUP response frame to server and move state to WAIT_SLEEP_END, the state will be moved to 
        // CONNECTED if WAKEUP response frame is send successfully later
        LOGI("RLK client RX MGMT subtype:%x state:%d\n", header->data_subtype,rlk_client_local_env.state);
        rtos_stop_timer(&rlk_client_wake_dur_timer);

        rlk_cntrl_client_add_peer(rx_info->src_addr);
        memcpy(rlk_client_local_env.peer_mac_addr, rx_info->src_addr, RLK_WIFI_MAC_ADDR_LEN);

        LOGI("RLK client update state to RLK_STATE_WAIT_SLEEP_END\n");
        rlk_client_local_env.state = RLK_STATE_WAIT_SLEEP_END;

        //Register RLK send handle function for internor
        BK_LOG_ON_ERR(bk_rlk_register_send_cb(rlk_cntrl_client_send_callback));

        rlk_cntrl_client_tx_mgmt_data(rx_info->src_addr, RLK_MM_HEADER_MGMT_SUBTYPE_WAKEUP_RSP);
    }
    else if ((rlk_client_local_env.state == RLK_STATE_PROBEING) && (header->data_subtype == RLK_MM_HEADER_MGMT_SUBTYPE_PROBE_RSP))
    {
        LOGI("RLK client RX MGMT subtype:%x state:%d\n", header->data_subtype,rlk_client_local_env.state);
        rlk_cntrl_client_add_peer(rx_info->src_addr);
        memcpy(rlk_client_local_env.peer_mac_addr, rx_info->src_addr, RLK_WIFI_MAC_ADDR_LEN);

        LOGI("RLK client update state to CONNECTED\n");
        rlk_client_local_env.state = RLK_STATE_CONNECTED;
        bk_rlk_unregister_send_cb();

#if 0//(CONFIG_SOC_BK7256XX)
        // init audio and video
        //rlk_mm_client_init(&video_dev);
#else
        rlk_mm_client_init();
#endif

        rtos_start_timer(&rlk_cntrl_client_timer);
    }

    os_free(rx_info);
}

void rlk_cntrl_client_handle_tx_cb(struct rlk_msg_t msg)
{
    if (rlk_client_local_env.state == RLK_STATE_PROBEING)
    {
        rlk_cntrl_client_tx_mgmt_data(rlk_client_local_env.bc_mac_addr, RLK_MM_HEADER_MGMT_SUBTYPE_PROBE_REQ);
    }
    else if ((rlk_client_local_env.state == RLK_STATE_WAIT_SLEEP_END) && (msg.arg != BK_RLK_SEND_SUCCESS))
    {
        // send wakeup rsp frame until it has been send successfully
        rlk_cntrl_client_tx_mgmt_data(rlk_client_local_env.peer_mac_addr, RLK_MM_HEADER_MGMT_SUBTYPE_WAKEUP_RSP);
    }
    else if ((rlk_client_local_env.state == RLK_STATE_WAIT_SLEEP_END) && (msg.arg == BK_RLK_SEND_SUCCESS))
    {
        // wakeup rsp frame has been send successfully to peer, move to CONNECT state and start init audio and video
        rlk_client_local_env.state = RLK_STATE_CONNECTED;
        bk_rlk_unregister_send_cb();

#if  0//(CONFIG_SOC_BK7256XX)
        // init audio and video
        //rlk_mm_client_init(&video_dev);
#else
        rlk_mm_client_init();
#endif
        rtos_start_timer(&rlk_cntrl_client_timer);
    }
}

void rlk_cntrl_client_main(void *arg)
{
    bk_err_t ret;
    struct rlk_msg_t msg;

    ret = rtos_get_semaphore(&s_rlk_cntrl_client_sem, BEKEN_WAIT_FOREVER);

    if (BK_OK != ret) {
        return;
    }

    ret = rlk_cntrl_client_init();

    if (BK_OK != ret) {
        return;
    }

    rlk_cntrl_client_add_peer(rlk_client_local_env.bc_mac_addr);
    rlk_cntrl_client_tx_mgmt_data(rlk_client_local_env.bc_mac_addr, RLK_MM_HEADER_MGMT_SUBTYPE_PROBE_REQ);
    rlk_client_local_env.state = RLK_STATE_PROBEING;

    while (1)
    {
        ret = rtos_pop_from_queue(&rlk_client_queue, &msg, BEKEN_WAIT_FOREVER);

        if (BK_OK != ret)
        {
             continue;
        }

        switch (msg.msg_id) {
            case RLK_MSG_TX_MGMT_CB:
                rlk_cntrl_client_handle_tx_cb(msg);
                break;
            case RLK_MSG_RX_MGMT:
                rlk_cntrl_client_handle_mgmt_rx(msg);
                break;
            case RLK_MSG_RX_DATA:
                rlk_cntrl_client_handle_data_rx(msg);
                break;
            case RLK_MSG_WAKEUP_SLEEP:
                rlk_cntrl_client_handle_wakeup();
                break;
            default:
                break;
        }
    }
}

bk_err_t rlk_cntrl_client_env_init(void)
{
    rlk_client_local_env.curr_channel = DEFAULT_RLK_CHANNEL;
    bk_wifi_sta_get_mac((uint8_t *)(rlk_client_local_env.local_mac_addr));
    rlk_client_local_env.ifidx = WIFI_IF_STA;
    rlk_client_local_env.state = RLK_STATE_IDLE;
    rlk_client_local_env.encrypt = false;
    //memcpy(rlk_client_local_env.peer_mac_addr, DEFAULT_MAC_ADDR_BROADCAST, RLK_WIFI_MAC_ADDR_LEN);
    memcpy(rlk_client_local_env.bc_mac_addr, DEFAULT_MAC_ADDR_BROADCAST, RLK_WIFI_MAC_ADDR_LEN);

    return BK_OK;
}

bk_err_t rlk_cntrl_client_init(void)
{
    LOGI("rlk_cntrl_client_init start\r\n");
    BK_LOG_ON_ERR(bk_rlk_init());
    //Register RLK receive handle function for internor
    BK_LOG_ON_ERR(bk_rlk_register_recv_cb(rlk_cntrl_client_recv_process));
    //Register RLK send handle function for internor
    BK_LOG_ON_ERR(bk_rlk_register_send_cb(rlk_cntrl_client_send_callback));
    // set channel
    BK_LOG_ON_ERR(bk_rlk_set_channel(rlk_client_local_env.curr_channel));
    LOGI("rlk_cntrl_client_init end\r\n");

    return BK_OK;
}

///function for monitor recover
void rlk_cntrl_client_handle_wakeup(void)
{
    bk_rlk_wakeup();
    rtos_start_timer(&rlk_client_wake_dur_timer);
}

bk_err_t rlk_client_init(void)
{
    int ret;

    LOGI("%s %d\n", __func__,__LINE__);

    rlk_cntrl_client_env_init();
    rtos_init_timer(&rlk_cntrl_client_timer, RLK_KEEPALIVE_TIME_INTERVAL, (timer_handler_t)rlk_cntrl_client_timer_handler, NULL);
    rtos_init_timer(&rlk_client_wake_dur_timer, RLK_LP_WAKEUP_DURATION, (timer_handler_t)rlk_client_wake_duration_end_handler, NULL);

    ret = rtos_init_queue(&rlk_client_queue, "rlk_client_queue",
                          sizeof(struct rlk_msg_t), RLK_MSG_QUEUE_LEN);

    if (BK_OK != ret) {
        return BK_FAIL;
    }

    ret = rtos_init_semaphore(&s_rlk_cntrl_client_sem, 1);

    if (BK_OK != ret)
    {
        LOGE("%s semaphore init failed\n", __func__);
        return BK_FAIL;
    }

    ret = rtos_create_thread(&rlk_cntrl_client_handle,
                             BEKEN_APPLICATION_PRIORITY,
                             "rlk_client_thread",
                             (beken_thread_function_t)rlk_cntrl_client_main,
                             RLK_THREAD_SIZE,
                             (beken_thread_arg_t)0);

    if (BK_OK != ret)
    {
        if (rlk_client_queue)
        {
             rtos_deinit_queue(&rlk_client_queue);
             rlk_client_queue = 0;
        }
        return BK_FAIL;
    }

    rlk_client_local_env.is_inited = true;

    return ret;
}

void rlk_client_deinit(void)
{
    bk_rlk_deinit();
    rtos_delete_thread(NULL);
}


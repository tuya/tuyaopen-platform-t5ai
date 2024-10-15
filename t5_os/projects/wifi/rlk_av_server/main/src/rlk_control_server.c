#include <stdlib.h>
#include <string.h>

#include <common/bk_include.h>
#include <common/bk_err.h>
#include <os/os.h>
#include <os/mem.h>

#include <modules/wifi.h>
#include <modules/wifi_types.h>
#include <modules/raw_link.h>
#include "rlk_control_server.h"
#include "rlk_multimedia_server.h"

rlk_local_env_t rlk_server_local_env;
static beken_queue_t rlk_server_queue;
static beken_thread_t rlk_cntrl_server_handle;
beken_timer_t rlk_cntrl_server_timer = {0};
beken_semaphore_t s_rlk_cntrl_server_sem;

bk_err_t rlk_cntrl_server_send_callback(const uint8_t *peer_mac_addr, bk_rlk_send_status_t status);
static void rlk_cntrl_server_timer_handler(void)
{
    uint32_t time = 0;

    if (rlk_server_local_env.state != RLK_STATE_CONNECTED)
    {
        return;
    }

    time = rtos_get_time();
    if ((time > rlk_server_local_env.last_rx_tick) && 
        ((time - rlk_server_local_env.last_rx_tick) > RLK_KEEPALIVE_TIME_INTERVAL))
    {
        LOGI("RLK server keepalive timeout, update connect to PROBEING\n");
        rlk_server_local_env.state = RLK_STATE_PROBEING;
        //rlk_mm_server_deinit();
        bk_rlk_register_send_cb(rlk_cntrl_server_send_callback);
        return;
    }

    rtos_start_timer(&rlk_cntrl_server_timer);
}
bk_err_t rlk_cntrl_server_add_peer(const uint8_t *mac_addr)
{
    bk_rlk_peer_info_t peer;
    peer.channel = rlk_server_local_env.curr_channel;
    peer.ifidx = rlk_server_local_env.ifidx;
    peer.state = rlk_server_local_env.state;
    peer.encrypt = rlk_server_local_env.encrypt;
    memcpy(peer.mac_addr, mac_addr, RLK_WIFI_MAC_ADDR_LEN);

    return bk_rlk_add_peer(&peer);
}
bk_err_t rlk_cntrl_server_recv_process(bk_rlk_recv_info_t *rx_info)
{
    bk_err_t ret = BK_OK;
    bk_rlk_recv_info_t *rx_info_local = NULL;
    struct rlk_msg_t msg;
    uint8_t *data = rx_info->data;
    rlk_mm_pre_header_t *header = (rlk_mm_pre_header_t *)data;

    if (!rlk_server_local_env.is_inited)
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

    ret = rtos_push_to_queue(&rlk_server_queue, &msg, BEKEN_NO_WAIT);

    if (ret != BK_OK)
    {
        os_free(rx_info_local);
        return BK_FAIL;
    }

    return ret;
}
bk_err_t rlk_cntrl_server_send_callback(const uint8_t *peer_mac_addr, bk_rlk_send_status_t status)
{
    bk_err_t ret = BK_OK;
    struct rlk_msg_t msg;

    if (!rlk_server_local_env.is_inited)
    {
        return BK_FAIL;
    }

    msg.msg_id = RLK_MSG_TX_MGMT_CB;
    msg.arg = status;
    msg.len = 0;

    ret = rtos_push_to_queue(&rlk_server_queue, &msg, BEKEN_NO_WAIT);

    return ret;
}

bk_err_t rlk_cntrl_server_tx_mgmt_data(const uint8_t *mac_addr, uint8_t msg_type)
{
    bk_err_t ret;
    rlk_mm_pre_header_t header;

    header.data_type = RLK_MM_HEADER_TYPE_MGMT;
    header.data_subtype = msg_type;

    ret = bk_rlk_send(mac_addr, &header, sizeof(header));

    if (ret < 0)
    {
        LOGI("rlk_cntrl_server_tx_mgmt_data failed ret:%d\n", ret);
    }

    return ret;
}

void rlk_cntrl_server_wakeup_peer(void)
{
    LOGI("rlk_cntrl_server_wakeup_peer state:%d\n", rlk_server_local_env.state);
    //step 1 if need register tx cb
    if (rlk_server_local_env.state == RLK_STATE_CONNECTED)
    {
        ///tx cb re register
        BK_LOG_ON_ERR(bk_rlk_register_send_cb(rlk_cntrl_server_send_callback));
    }
    else if (rlk_server_local_env.state == RLK_STATE_PROBEING)
    {
        //tx cb already reg
    }

    //step 2 statue change to WAIT_PEER_WAKEUP
    rlk_server_local_env.state = RLK_STATE_WAIT_PEER_WAKEUP;
    rlk_cntrl_server_add_peer(rlk_server_local_env.bc_mac_addr);
    rlk_cntrl_server_tx_mgmt_data(rlk_server_local_env.bc_mac_addr, RLK_MM_HEADER_MGMT_SUBTYPE_WAKEUP_REQ);

}

void rlk_cntrl_server_handle_data_rx(struct rlk_msg_t msg)
{
    bk_err_t ret = BK_OK;
    bk_rlk_recv_info_t *rx_info = (bk_rlk_recv_info_t *)msg.arg;
    uint8_t *data = rx_info->data;

    ret = rlk_mm_server_recv(data, rx_info->len);

    if (ret == BK_OK)
        rlk_server_local_env.last_rx_tick = rtos_get_time();

    os_free(rx_info);
}

void rlk_cntrl_server_handle_mgmt_rx(struct rlk_msg_t msg)
{
    bk_rlk_recv_info_t *rx_info = (bk_rlk_recv_info_t *)msg.arg;
    uint8_t *data = rx_info->data;
    rlk_mm_pre_header_t *header = (rlk_mm_pre_header_t *)data;

    if (header->data_type != RLK_MM_HEADER_TYPE_MGMT)
    {
        os_free(rx_info);
        return;
    }

    if ((rlk_server_local_env.state == RLK_STATE_PROBEING) && (header->data_subtype == RLK_MM_HEADER_MGMT_SUBTYPE_PROBE_REQ))
    {
        LOGI("RLK server RX MGMT subtype:%x, state:%x\n", header->data_subtype, rlk_server_local_env.state);
        rlk_cntrl_server_add_peer(rx_info->src_addr);
        memcpy(rlk_server_local_env.peer_mac_addr, rx_info->src_addr, RLK_WIFI_MAC_ADDR_LEN);

        rlk_cntrl_server_tx_mgmt_data(rx_info->src_addr, RLK_MM_HEADER_MGMT_SUBTYPE_PROBE_RSP);
        rlk_server_local_env.state = RLK_STATE_WAIT_PROBE_END;
    }
    else if ((rlk_server_local_env.state == RLK_STATE_WAIT_PEER_WAKEUP) && (header->data_subtype == RLK_MM_HEADER_MGMT_SUBTYPE_WAKEUP_RSP))
    {
        LOGI("RLK server RX MGMT subtype:%x, state:%x\n", header->data_subtype, rlk_server_local_env.state);
        rlk_cntrl_server_add_peer(rx_info->src_addr);
        memcpy(rlk_server_local_env.peer_mac_addr, rx_info->src_addr, RLK_WIFI_MAC_ADDR_LEN);

        LOGI("RLK server WAKEUP PEER SUCCESS\n");
        rlk_server_local_env.state = RLK_STATE_CONNECTED;
        bk_rlk_unregister_send_cb();
        // init audio and video
        rlk_mm_server_init();
        rtos_start_timer(&rlk_cntrl_server_timer);
    }

    os_free(rx_info);
}

void rlk_cntrl_server_handle_tx_cb(struct rlk_msg_t msg)
{
    if (rlk_server_local_env.state == RLK_STATE_PROBEING)
    {
        rlk_cntrl_server_tx_mgmt_data(rlk_server_local_env.bc_mac_addr, RLK_MM_HEADER_MGMT_SUBTYPE_PROBE_REQ);
    }
    else if (rlk_server_local_env.state == RLK_STATE_WAIT_PROBE_END)
    {
        LOGI("RLK server update state to CONNECTED\n");
        rlk_server_local_env.state = RLK_STATE_CONNECTED;
        bk_rlk_unregister_send_cb();
        // init audio and video
        rlk_mm_server_init();

        rtos_start_timer(&rlk_cntrl_server_timer);
    }
    else if (rlk_server_local_env.state == RLK_STATE_WAIT_PEER_WAKEUP)
    {
        //still send wakeup frame
        rlk_cntrl_server_tx_mgmt_data(rlk_server_local_env.bc_mac_addr, RLK_MM_HEADER_MGMT_SUBTYPE_WAKEUP_REQ);
    }
}

void rlk_cntrl_server_main(void *arg)
{
    bk_err_t ret;
    struct rlk_msg_t msg;

    ret = rtos_get_semaphore(&s_rlk_cntrl_server_sem, BEKEN_WAIT_FOREVER);

    if (BK_OK != ret) {
        return;
    }

    ret = rlk_cntrl_server_init();

    if (BK_OK != ret) {
        return;
    }

    //rlk_cntrl_server_tx_mgmt_data(rlk_server_local_env.bc_mac_addr, RLK_MM_HEADER_MGMT_SUBTYPE_PROBE_REQ);
    rlk_server_local_env.state = RLK_STATE_PROBEING;

    while (1)
    {
        ret = rtos_pop_from_queue(&rlk_server_queue, &msg, BEKEN_WAIT_FOREVER);

        if (BK_OK != ret)
        {
             continue;
        }

        switch (msg.msg_id) {
            case RLK_MSG_TX_MGMT_CB:
                rlk_cntrl_server_handle_tx_cb(msg);
                break;
            case RLK_MSG_RX_MGMT:
                rlk_cntrl_server_handle_mgmt_rx(msg);
                break;
            case RLK_MSG_RX_DATA:
                rlk_cntrl_server_handle_data_rx(msg);
                break;
            default:
                break;
        }
    }
}

bk_err_t rlk_cntrl_server_env_init(void)
{
    rlk_server_local_env.curr_channel = DEFAULT_RLK_CHANNEL;
    bk_wifi_sta_get_mac((uint8_t *)(rlk_server_local_env.local_mac_addr));
    rlk_server_local_env.ifidx = WIFI_IF_STA;
    rlk_server_local_env.state = RLK_STATE_IDLE;
    rlk_server_local_env.encrypt = false;
    //memcpy(rlk_server_local_env.peer_mac_addr, DEFAULT_MAC_ADDR_BROADCAST, RLK_WIFI_MAC_ADDR_LEN);
    memcpy(rlk_server_local_env.bc_mac_addr, DEFAULT_MAC_ADDR_BROADCAST, RLK_WIFI_MAC_ADDR_LEN);

    return BK_OK;
}

bk_err_t rlk_cntrl_server_init(void)
{
    LOGI("rlk_cntrl_server_init start\r\n");
    BK_LOG_ON_ERR(bk_rlk_init());
    //Register RLK receive handle function for internor
    BK_LOG_ON_ERR(bk_rlk_register_recv_cb(rlk_cntrl_server_recv_process));
    //Register RLK send handle function for internor
    BK_LOG_ON_ERR(bk_rlk_register_send_cb(rlk_cntrl_server_send_callback));
    // set channel
    BK_LOG_ON_ERR(bk_rlk_set_channel(rlk_server_local_env.curr_channel));
    LOGI("rlk_cntrl_server_init end\r\n");

    return BK_OK;
}

bk_err_t rlk_server_init(void)
{
    int ret;

    ret = rlk_cntrl_server_env_init();
    rtos_init_timer(&rlk_cntrl_server_timer, RLK_KEEPALIVE_TIME_INTERVAL, (timer_handler_t)rlk_cntrl_server_timer_handler, NULL);

    ret = rtos_init_queue(&rlk_server_queue, "rlk_server_queue",
                          sizeof(struct rlk_msg_t), RLK_MSG_QUEUE_LEN);

    if (BK_OK != ret) {
        return BK_FAIL;
    }

    ret = rtos_init_semaphore(&s_rlk_cntrl_server_sem, 1);

    if (BK_OK != ret)
    {
        LOGE("%s semaphore init failed\n", __func__);
        return BK_FAIL;
    }

    ret = rtos_create_thread(&rlk_cntrl_server_handle,
                             BEKEN_APPLICATION_PRIORITY,
                             "rlk_server_thread",
                             (beken_thread_function_t)rlk_cntrl_server_main,
                             RLK_THREAD_SIZE,
                             (beken_thread_arg_t)0);

    if (BK_OK != ret)
    {
        if (rlk_server_queue)
        {
             rtos_deinit_queue(&rlk_server_queue);
             rlk_server_queue = 0;
        }
        return BK_FAIL;
    }

    rlk_server_local_env.is_inited = true;

    return ret;
}



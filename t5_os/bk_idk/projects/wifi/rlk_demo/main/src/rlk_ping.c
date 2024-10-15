#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <common/bk_include.h>
#include <common/bk_err.h>
#include <os/os.h>
#include <os/mem.h>

#include <modules/wifi.h>
#include <modules/wifi_types.h>
#include <modules/raw_link.h>
#include "rlk_demo_cli.h"
#include "rlk_common.h"

/** rlk ping receive timeout - in milliseconds */
#define RLK_PING_RCV_TIMEO 2000

/** rlk ping delay - in milliseconds */
#define RLK_PING_DELAY     1000


/** rlk ping additional data size to include in the packet */
#define RLK_PING_DATA_SIZE 32
#define RLK_PING_RSP_SIZE 32

RLK_PING_PARAM rlk_ping_env = {0};
bk_err_t rlk_ping_add_peer(const uint8_t *mac_addr)
{
    bk_rlk_peer_info_t peer;
    peer.channel = rlk_demo_env.chnl;
    peer.ifidx = 0;
    peer.state = 0;
    peer.encrypt = 0;
    memcpy(peer.mac_addr, mac_addr, RLK_WIFI_MAC_ADDR_LEN);

    return bk_rlk_add_peer(&peer);
}

bk_err_t rlk_ping_response(bk_rlk_recv_info_t *rx_info)
{
    RLK_APP_HDR* app_ptr,*app_recv_ptr;
    bk_err_t ret;

    app_ptr = (RLK_APP_HDR*)os_malloc(RLK_PING_RSP_SIZE);
    
    if (app_ptr == NULL)
    {
        RLK_LOGI("rlk_ping_response malloc failed\n");
        return BK_FAIL;
    }
    memset((void*)app_ptr,0,RLK_PING_RSP_SIZE);

    app_recv_ptr = (RLK_APP_HDR*)rx_info->data;

    app_ptr->type = RLK_APP_TYPE_DATA;
    app_ptr->subtype = RLK_APP_ECHO_RPLY;
    app_ptr->len = RLK_PING_RSP_SIZE;
    app_ptr->seq = app_recv_ptr->seq;

    os_memcpy(rlk_ping_env.mac_addr_rsp,rx_info->src_addr,RLK_WIFI_MAC_ADDR_LEN);
    rlk_ping_add_peer(rlk_ping_env.mac_addr_rsp);
    ret = bk_rlk_send(rlk_ping_env.mac_addr_rsp,app_ptr,RLK_PING_RSP_SIZE);

    if (ret < 0)
    {
        RLK_LOGI("rlk_ping_send failed ret:%d\n", ret);

        os_free(app_ptr);
    }
    return ret;

}

void rlk_ping_usage(void)
{
    os_printf("Usage: rlk_ping target [options]\n");
    os_printf("\n");
    os_printf("target:  must be mac address(full 6 byte or last 3 byte)\n");
    os_printf("option:\n");
    os_printf("       -c        ping count, default is 4 times\n");
    os_printf("       -i        interval, default is 1 second\n");
    os_printf("       -s        packetsize, default is 32byte\n");
    os_printf("       -t        timeout, default is 1 second\n");
    os_printf("       --stop    stop ping program\n");
    os_printf("       -h        help info\n");

    return ;
}
void rlk_ping_default_set(void)
{
    if(rlk_ping_env.count == 0) rlk_ping_env.count = 4;
    if(rlk_ping_env.size == 0) rlk_ping_env.size = 32;
    if(rlk_ping_env.timeout == 0) rlk_ping_env.timeout = 1;
    if(rlk_ping_env.interval == 0) rlk_ping_env.interval = 1;
}

void rlk_ping_stop(void)
{
    if (rlk_ping_env.state == RLK_PING_STATE_STARTED) {
        rlk_ping_env.state = RLK_PING_STATE_STOPPING;
        RLK_LOGI("rlk_ping: rlk_ping is stopping...\n");
    }
}
void ping_queue_init(void)
{
    rtos_init_queue(&rlk_ping_env.conn.recvmbox, "ping_recv", sizeof(uint32), PING_RECV_MSG_NUM);
}

void ping_queue_deinit()
{
    rlk_ping_env.state = RLK_PING_STATE_STOPPED;
    rtos_deinit_queue(&rlk_ping_env.conn.recvmbox);
}

bk_err_t rlk_ping_send(void)
{
    RLK_APP_HDR* app_ptr;
    bk_err_t ret;

    app_ptr = (RLK_APP_HDR*)os_malloc(rlk_ping_env.size);

    if(app_ptr == NULL) return BK_FAIL;

    memset((void*)app_ptr,0,rlk_ping_env.size);
    app_ptr->type = RLK_APP_TYPE_DATA;
    app_ptr->subtype = RLK_APP_ECHO_REQ;
    app_ptr->len = rlk_ping_env.size;
    app_ptr->seq = rlk_ping_env.seq;
    rlk_ping_add_peer(rlk_ping_env.mac_addr);

    ret = bk_rlk_send(rlk_ping_env.mac_addr,app_ptr,rlk_ping_env.size);

    if (ret != BK_OK)
    {
        RLK_LOGI("rlk_ping_send failed ret:%d\n", ret);

        os_free(app_ptr);
    }
    return ret;
}

bk_err_t rlk_ping_recv(void)
{
    RLK_APP_HDR *app_ptr = NULL;
    uint16_t len = 0;
    uint32_t buf = 0;
    
    //timeout set 1000 ticks
    while(kNoErr == rtos_pop_from_queue(&rlk_ping_env.conn.recvmbox, &buf, rlk_ping_env.timeout*1000))
    {
        app_ptr = (RLK_APP_HDR *)buf;
        if((app_ptr->type == RLK_APP_TYPE_DATA)&&(app_ptr->subtype == RLK_APP_ECHO_RPLY)&&(app_ptr->seq == rlk_ping_env.seq))
        {
            len = app_ptr->len;
        }
        else
        {
            len = 0;
        }
        
        if (buf)
            os_free((void*)buf);

        if(len > 0)    break;
    }

    return len;

}



uint8_t rlk_ping()
{
    int recv_len;
    uint16_t send_times;
    uint32_t recv_start_tick;

    int recv_cnt = 0;
    int sent_cnt = 0;
    send_times = 0;

    uint16_t ping_counts = 0;

    if(rlk_ping_env.count == 0)
    {
        ping_counts = RLK_PING_DEFAULT_CNT;
    }
    else
    {
        ping_counts = rlk_ping_env.count;
    }


    while (rlk_ping_env.state == RLK_PING_STATE_STARTED)
    {
        rlk_ping_env.seq = send_times;
        if (rlk_ping_send() >= BK_OK)
        {
            sent_cnt++;
            recv_start_tick = rlk_sys_now();
            if ((recv_len = rlk_ping_recv()) > 0)
            {
                recv_cnt++;
                RLK_LOGI("%d bytes from %s rlk_ping_seq=%d time=%d ticks\n", recv_len, rlk_ping_env.mac_str, send_times,
                        rlk_sys_now() - recv_start_tick);
            }
            else
            {
                RLK_LOGI("From %s rlk_ping_seq=%d timeout\n", rlk_ping_env.mac_str, send_times);
            }
        }
        else
        {
                RLK_LOGI("Send %s - error\n", rlk_ping_env.mac_str);
        }

        send_times++;
        if (send_times >= ping_counts)
        {
            RLK_LOGI("ping end, sent cnt: %d, recv cnt: %d, drop cnt: %d(%.1f%%)\n", 
                sent_cnt,
                recv_cnt,
                (ping_counts-recv_cnt),
                (float)(((ping_counts-recv_cnt)*100)/ping_counts));
            /* send ping times reached, stop */
            break;
        }

        if (rlk_ping_env.state != RLK_PING_STATE_STARTED)
            break;

        rtos_delay_milliseconds(rlk_ping_env.interval*1000);
    }
    return 0;
}

void rlk_ping_thread(void *thread_param)
{
    ping_queue_init();
    rlk_ping();
    ping_queue_deinit();
    rtos_delete_thread(NULL);
}

void rlk_ping_start()
{
    if (rlk_ping_env.state == RLK_PING_STATE_STOPPED) 
    {
        if (rlk_ping_env.mac_addr) {
            rlk_ping_env.state = RLK_PING_STATE_STARTED;
#ifdef CONFIG_FREERTOS_SMP
            rtos_create_thread_with_affinity(NULL, -1, RLK_PING_PRIORITY, "RLK ping",
                               rlk_ping_thread, RLK_PING_THREAD_SIZ,
                               (beken_thread_arg_t) 0);
#else
            rtos_create_thread(NULL, RLK_PING_PRIORITY, "RLK ping",
                               rlk_ping_thread, RLK_PING_THREAD_SIZ,
                               (beken_thread_arg_t) 0); 
#endif
        }
        else
            RLK_LOGI("Please input: rlk_ping <mac address>\n");

    }
    else if (rlk_ping_env.state == RLK_PING_STATE_STOPPING)
        RLK_LOGI("rlk_ping: rlk_ping is stopping, try again later!\n");
    else
        RLK_LOGI("rlk_ping: rlk_ping is running, stop first!\n");
}




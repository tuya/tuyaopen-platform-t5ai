#ifndef _BK_RLK_CONTROL_SERVER_H_
#define _BK_RLK_CONTROL_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <modules/raw_link.h>


#define TAG "RMS" //Raw-link Multimedia Server

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define DEFAULT_MAC_ADDR_BROADCAST "\xFF\xFF\xFF\xFF\xFF\xFF"
#define DEFAULT_RLK_CHANNEL     8

//#define DEFAULT_MAC_ADDR_BROADCAST "\xC8\x47\x8C\xB1\xB1\x12"

#define RLK_MSG_QUEUE_LEN       64
#define RLK_THREAD_SIZE         4*1024


#define RLK_MM_HEADER_TYPE_DATA                     1
#define RLK_MM_HEADER_TYPE_MGMT                     2

#define RLK_MM_HEADER_DATA_SUBTYPE_VIDEO            1
#define RLK_MM_HEADER_DATA_SUBTYPE_AUDIO            2

#define RLK_MM_HEADER_MGMT_SUBTYPE_PROBE_REQ        1
#define RLK_MM_HEADER_MGMT_SUBTYPE_PROBE_RSP        2
#define RLK_MM_HEADER_MGMT_SUBTYPE_DISCONNECT       3
#define RLK_MM_HEADER_MGMT_SUBTYPE_WAKEUP_REQ       4
#define RLK_MM_HEADER_MGMT_SUBTYPE_WAKEUP_RSP       5

#define RLK_MM_PRE_HEADER_LEN   sizeof(rlk_mm_pre_header_t)

/// connect state
#define RLK_STATE_IDLE              1
#define RLK_STATE_PROBEING          2
#define RLK_STATE_WAIT_PROBE_END    3
#define RLK_STATE_CONNECTED         4
#define RLK_STATE_DISCONNECT        5
///only for server
#define RLK_STATE_WAIT_PEER_WAKEUP  6

#define RLK_KEEPALIVE_TIME_INTERVAL                 (1000 * 2)

struct rlk_msg_t {
    uint32_t msg_id;
    uint32_t arg;
    uint32_t len;
};
typedef struct rlk_local_env_tag {
    uint8_t curr_channel;
    bk_rlk_wifi_if_t ifidx;
    uint8_t state;
    bool encrypt;
    uint8_t local_mac_addr[RLK_WIFI_MAC_ADDR_LEN];
    uint8_t peer_mac_addr[RLK_WIFI_MAC_ADDR_LEN];
    uint8_t bc_mac_addr[RLK_WIFI_MAC_ADDR_LEN];
    uint8_t connect_state;
    bool is_inited;
    uint32_t last_rx_tick;
}rlk_local_env_t;

typedef struct rlk_mm_pre_header_tag {
    uint8_t data_type;
    uint8_t data_subtype;
}rlk_mm_pre_header_t;

typedef enum
{
    RLK_MSG_TX_MGMT_CB = 0x1,
    RLK_MSG_RX_MGMT,
    RLK_MSG_RX_DATA,
} rlk_cntrl_msg_e;


extern rlk_local_env_t rlk_server_local_env;

extern beken_semaphore_t s_rlk_cntrl_server_sem;

/**********************************************************/

void rlk_cntrl_server_wakeup_peer(void);
bk_err_t rlk_cntrl_server_init(void);
bk_err_t rlk_server_init(void);

#ifdef __cplusplus
}
#endif
#endif //_BK_RLK_CONTROL_SERVER_H_
// eof


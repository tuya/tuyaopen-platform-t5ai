#ifndef _BK_RLK_PING_H_
#define _BK_RLK_PING_H_

#include "rlk_common.h"

#define RLK_PING_DEFAULT_CNT         4
#define RLK_PING_PRIORITY            4
#define RLK_PING_THREAD_SIZ          1536
#define PING_RECV_MSG_NUM            32
/* rlk ping mode */
enum {
    RLK_PING_STATE_STOPPED = 0,
    RLK_PING_STATE_STOPPING,
    RLK_PING_STATE_STARTED,
};
typedef struct
{
  /** mbox where received packets are stored until they are fetched
      by the netconn application thread (can grow quite big) */
  beken_queue_t recvmbox;
}RLK_PING_CONN;

typedef struct
{
    int state;
    char mac_str[RLK_MAC_STRING_LEN];
    //used for rlk ping sender
    uint8_t mac_addr[RLK_WIFI_MAC_ADDR_LEN];
    //used for rlk ping responder
    uint8_t mac_addr_rsp[RLK_WIFI_MAC_ADDR_LEN];
    uint16_t count;
    uint16_t size;
    uint16_t interval;
    uint16_t timeout;
    RLK_PING_CONN conn;
    uint16_t recv_size;
    uint16_t seq;
} RLK_PING_PARAM;


extern void cli_rlk_ping_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern bk_err_t rlk_ping_response(bk_rlk_recv_info_t *rx_info);
extern RLK_PING_PARAM rlk_ping_env;
extern void rlk_ping_start();
extern void rlk_ping_stop();
extern void rlk_ping_default_set(void);
extern void rlk_ping_usage();
#endif //_BK_RLK_PING_H_
// eof


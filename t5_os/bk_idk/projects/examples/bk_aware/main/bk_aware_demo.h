/* BK_AWARE Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef BK_AWARE_EXAMPLE_H
#define BK_AWARE_EXAMPLE_H

#define CONFIG_BK_AWARE_CHANNEL 1
#define CONFIG_BK_AWARE_SEND_COUNT 100
#define CONFIG_BK_AWARE_SEND_DELAY 1000
#define CONFIG_BK_AWARE_SEND_LEN 10

#define BK_NOW_ETH_ALEN            6
#define BK_AWARE_QUEUE_SIZE        6

#define IS_BROADCAST_ADDR(addr) (memcmp(addr, s_example_broadcast_mac, BK_NOW_ETH_ALEN) == 0)

typedef enum {
    EXAMPLE_BK_AWARE_SEND_CB,
    EXAMPLE_BK_AWARE_RECV_CB,
    EXAMPLE_BK_AWARE_EXIT,
} example_bk_aware_event_id_t;

typedef struct {
    uint8_t mac_addr[BK_NOW_ETH_ALEN];
    bk_rlk_send_status_t status;
} example_bk_aware_event_send_cb_t;

typedef struct {
    uint8_t mac_addr[BK_NOW_ETH_ALEN];
    uint8_t *data;
    int data_len;
} example_bk_aware_event_recv_cb_t;

typedef union {
    example_bk_aware_event_send_cb_t send_cb;
    example_bk_aware_event_recv_cb_t recv_cb;
} example_bk_aware_event_info_t;

/* When BK_AWARE sending or receiving callback function is called, post event to BK_AWARE task. */
typedef struct {
    example_bk_aware_event_id_t id;
    example_bk_aware_event_info_t info;
} example_bk_aware_event_t;

enum {
    EXAMPLE_BK_AWARE_DATA_BROADCAST,
    EXAMPLE_BK_AWARE_DATA_UNICAST,
    EXAMPLE_BK_AWARE_DATA_MAX,
};

/* User defined field of BK_AWARE data in this example. */
typedef struct {
    uint8_t type;                         //Broadcast or unicast BK_AWARE data.
    uint8_t state;                        //Indicate that if has received broadcast BK_AWARE data or not.
    uint16_t seq_num;                     //Sequence number of BK_AWARE data.
    uint16_t crc;                         //CRC16 value of BK_AWARE data.
    uint32_t magic;                       //Magic number which is used to determine which device to send unicast BK_AWARE data.
    uint8_t payload[0];                   //Real payload of BK_AWARE data.
} __attribute__((packed)) example_bk_aware_data_t;

/* Parameters of sending BK_AWARE data. */
typedef struct {
    bool unicast;                         //Send unicast BK_AWARE data.
    bool broadcast;                       //Send broadcast BK_AWARE data.
    uint8_t state;                        //Indicate that if has received broadcast BK_AWARE data or not.
    uint32_t magic;                       //Magic number which is used to determine which device to send unicast BK_AWARE data.
    uint16_t count;                       //Total count of unicast BK_AWARE data to be sent.
    uint16_t delay;                       //Delay between sending two BK_AWARE data, unit: ms.
    int len;                              //Length of BK_AWARE data to be sent, unit: byte.
    uint8_t *buffer;                      //Buffer pointing to BK_AWARE data.
    uint8_t dest_mac[BK_NOW_ETH_ALEN];   //MAC address of destination device.
} example_bk_aware_send_param_t;

void bk_aware_demo_main(void);
void bk_aware_demo_stop();

#endif

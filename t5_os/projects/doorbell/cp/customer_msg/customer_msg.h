#ifndef __CIFD_MSG_WITH_HOST_H__
#define __CIFD_MSG_WITH_HOST_H__
#include "cif_main.h"
#define CIFD_CUST_DEBUG_CODE_MAGIC                  (0xAABBCCDD)

#define CIFD_CUST_PATTERN  (0xa5a6)
#define MAX_CIFD_CUST_SIZE   256


typedef enum{
    CIFD_CMD_BLE_DATA_TO_APK             = 0x0001,


    CIFD_EVENT_BLE_DATA_TO_USER          = 0x1001,

}CIFD_CMD_EVENT;

typedef struct{
    uint16_t magic;
    uint16_t cid;
    uint16_t ctl;
    uint16_t seq;
    uint16_t checksum;
    uint16_t len;
}CIFD_PROTO_HDR;

typedef struct{
    CIFD_PROTO_HDR header;
    uint8_t data[MAX_CIFD_CUST_SIZE];
}CIFD_CUST_DATA;

typedef struct cifd_cust_msg_hdr
{
    uint16_t cmd_id;
    uint16_t len;
    uint8_t payload[0];
}cifd_cust_msg_hdr_t;

extern int cifd_send_cust_event(uint16_t event_id, uint8_t *data, uint16_t len);
extern void cifd_cust_msg_init(void);
#endif

/**
* @file tkl_ipc.h
* @brief Common process - adapter the inter-processor communication api
* @version 0.1
* @date 2021-08-18
*
* @copyright Copyright 2021-2030 Tuya Inc. All Rights Reserved.
*
*/
#ifndef __TKL_IPC_H__
#define __TKL_IPC_H__

#include "tuya_cloud_types.h"
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    TKL_IPC_TYPE_HCI,
    TKL_IPC_TYPE_SYS,
    TKL_IPC_TYPE_WIRED,
    TKL_IPC_TYPE_TEST,
};

// (CPU0 and CPU1) ipc cmd  subtype for wifi
enum {
    TKL_IPC_TYPE_WF_SCAN = 0x00,
    TKL_IPC_TYPE_WF_SCAN_RES,
    TKL_IPC_TYPE_WF_SCAN_FREE_MEM,
    TKL_IPC_TYPE_WF_AP_START,
    TKL_IPC_TYPE_WF_AP_STOP,
    TKL_IPC_TYPE_WF_CHANNEL_SET,
    TKL_IPC_TYPE_WF_CHANNEL_GET,
    TKL_IPC_TYPE_WF_SNIFFER_SET,
    TKL_IPC_TYPE_WF_SNIFFER_DATA,
    TKL_IPC_TYPE_WF_IP_GET,
    TKL_IPC_TYPE_WF_IPv6_GET,
    TKL_IPC_TYPE_WF_IP_SET,
    TKL_IPC_TYPE_WF_MAC_SET,
    TKL_IPC_TYPE_WF_MAC_GET,
    TKL_IPC_TYPE_WF_WORKMODE_SET,
    TKL_IPC_TYPE_WF_WORKMODE_GET,
    TKL_IPC_TYPE_WF_FASTCONN_INFO_GET,
    TKL_IPC_TYPE_WF_BSSID_GET,
    TKL_IPC_TYPE_WF_CCODE_SET,
    TKL_IPC_TYPE_WF_CCODE_GET,
    TKL_IPC_TYPE_WF_RF_CALIBRATED_SET,
    TKL_IPC_TYPE_WF_LP_MODE_SET,
    TKL_IPC_TYPE_WF_FAST_CONNECT,
    TKL_IPC_TYPE_WF_CONNECT,
    TKL_IPC_TYPE_WF_CONNECT_RES,//WIFI
    TKL_IPC_TYPE_WF_DISCONNECT,
    TKL_IPC_TYPE_WF_RSSI_GET,
    TKL_IPC_TYPE_WF_STATION_STATUS_GET,
    TKL_IPC_TYPE_WF_MGNT_SEND,
    TKL_IPC_TYPE_WF_RECV_MGNT_SET,
    TKL_IPC_TYPE_WF_MGNT_RECV,
    TKL_IPC_TYPE_WF_IOCTL,
};

// (CPU0 and CPU1) ipc cmd  subtype for ble
enum {
    TKL_IPC_TYPE_HCI_INIT = 0x00,
    TKL_IPC_TYPE_HCI_DEINIT,
    TKL_IPC_TYPE_HCI_RESET,
    TKL_IPC_TYPE_HCI_CMD_SEND,
    TKL_IPC_TYPE_HCI_ACL_SEND,
    TKL_IPC_TYPE_HCI_EVENT_TO_HOST,
    TKL_IPC_TYPE_HCI_ACL_TO_HOST,
    TKL_IPC_TYPE_HCI_REG_CALLBACK,
};

// (CPU0 and CPU1) ipc cmd  subtype for lwip
enum {
    TKL_IPC_TYPE_LWIP_INIT = 0x00,
    TKL_IPC_TYPE_LWIP_GET_NETIF_BY_IDX,
    TKL_IPC_TYPE_LWIP_SEND,
    TKL_IPC_TYPE_LWIP_SEND_RES,
    TKL_IPC_TYPE_LWIP_RECV,
    TKL_IPC_TYPE_LWIP_PBUF_ALLOC,
    TKL_IPC_TYPE_LWIP_PBUF_CAT,
    TKL_IPC_TYPE_LWIP_PBUF_COALESCE,
    TKL_IPC_TYPE_LWIP_PBUF_FREE,
    TKL_IPC_TYPE_LWIP_PBUF_HEADER,
    TKL_IPC_TYPE_LWIP_PBUF_REF,
};

// (CPU0 and CPU1) ipc cmd  subtype for wired
enum {
    TKL_IPC_TYPE_WIRED_INIT = 0x00,
    TKL_IPC_TYPE_WIRED_SET_MAC,
    TKL_IPC_TYPE_WIRED_GET_MAC,
    TKL_IPC_TYPE_WIRED_STATUS_CHANGED,
    TKL_IPC_TYPE_WIRED_SET_GPIO,
};

enum {
    TKL_IPC_TYPE_SYS_REBOOT,
    TKL_IPC_TYPE_SYS_CPU_INFO,
    TKL_IPC_TYPE_SYS_CPU_INFO_RSP,
};

enum {
    TKL_IPC_TYPE_TEST_CMD,
    TKL_IPC_TYPE_TEST_SYSTEM_INFO,
    TKL_IPC_TYPE_TEST_MEDIA,
    TKL_IPC_TYPE_TEST_MP3,
};

struct ipc_msg_s {
    uint8_t type;
    uint8_t subtype;
    uint16_t ret_value;//0:ok; other:errcode
    uint8_t *req_param;
    uint32_t req_len;
    uint8_t *res_param;
    uint32_t res_len;
};

struct ipc_msg_param_s {
    void  *p1;
    void  *p2;
    void  *p3;
    void  *p4;
};

typedef void* TKL_IPC_HANDLE;

typedef OPERATE_RET (*TKL_IPC_FUNC_CB)(uint8_t *buf, uint32_t buf_len);

typedef struct {
    TKL_IPC_FUNC_CB  cb;
} TKL_IPC_CONF_T;

/**
 * @brief Function for initializing the inter-processor communication
 *
 * @param[in] config:  @see TKL_IPC_CONF_T
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_ipc_init(TKL_IPC_CONF_T *config);

/**
 * @brief   Function for send message between processors
 * @param[in] buf     message buffer
 * @param[in] buf_len message length
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_ipc_send(const uint8_t *buf, uint32_t buf_len);

OPERATE_RET tuya_ipc_send_sync(struct ipc_msg_s *msg);
OPERATE_RET tuya_ipc_send_no_sync(struct ipc_msg_s *msg);

#ifdef __cplusplus
}
#endif

#endif

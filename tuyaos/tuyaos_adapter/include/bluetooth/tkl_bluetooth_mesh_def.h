/**
 * @file tkl_bluetooth_mesh_def.h
 * @brief This is tuya tal_adc file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2031 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TKL_BLUETOOTH_MESH_DEF_H__
#define __TKL_BLUETOOTH_MESH_DEF_H__

#include "tuya_cloud_types.h"
#include "tuya_error_code.h"
#include "tkl_bluetooth.h"

#ifndef _PACKED_
#define _PACKED_  __attribute__ ((packed))
#endif

#ifndef _WEAK_
#define _WEAK_ __attribute__((weak))
#endif

 typedef struct {
    UCHAR_T     *data;      /**< Mesh data. */
    USHORT_T    data_len;   /**< Mesh data lenth. */
}TKL_MESH_PROXY_DATA_T;

/**@brief mesh access msg parameters. */
typedef struct {
    uint32_t    opcode;        /**< Mesh opcode. */
    UCHAR_T   *data;         /**< Mesh data. */
    USHORT_T  data_len;      /**< Mesh data lenth. */
} TKL_MESH_ACCESS_MSG_T;

 typedef struct {
    UCHAR_T     adv_data[31];   /**< Mesh Connect Adv Data. */
    USHORT_T    adv_len;        /**< Mesh Connect Adv Data Len. */

    UCHAR_T     rsp_data[31];   /**< Rsp Data. */
    USHORT_T    rsp_len;        /**< Rsp Data Len. */

    UCHAR_T     count;          /**< The number of transmissions is the Transmit Count + 1 */
    UCHAR_T     interval_steps; /**< Transmission interval = (Network Retransmit Interval Steps + 1) * 10 */
}TKL_MESH_CONNECT_ADV_PARAM_T;

/**@brief network parameters. */
typedef struct {
    USHORT_T  src_addr;           /**< Source unicast address. */
    USHORT_T  dst_addr;           /**< Destination  unicast address. */
    UCHAR_T   model_index;        /**< Msg model index. */
    uint32_t    seq;                /**< Sequence num of this msg. */
    UCHAR_T   ttl;                /**< Time To Live. */
    USHORT_T  app_key_index;      /**< The appkey index of this msg ues. */
    USHORT_T  net_key_index;      /**< The networkkey index of this msg ues. */
    char    rssi;               /**< used when rx in adv bearer. */
} TKL_MESH_NET_PARAM_T;

/**@brief network provision data. */
typedef struct{
    UCHAR_T   net_key[16];        /**< Network key. */
    USHORT_T  key_index;          /**< Network key index. */
    UCHAR_T   flags;              /**< Network key flag. */
    UCHAR_T   iv_index[4];        /**< Network IV index. */
    USHORT_T  unicast_address;    /**< Node unicast address. */
} _PACKED_ TKL_NET_PROV_DATA_T;

/**@brief app key data. */
typedef struct{
    UCHAR_T   net_app_idx[3];     /**< Network key index. */
    UCHAR_T   app_key[16];        /**< App key. */
} _PACKED_ TKL_APP_KEY_DATA_T;

typedef OPERATE_RET (*TKL_MESH_MSG_RECV_CB)(TKL_MESH_ACCESS_MSG_T *msg_raw, TKL_MESH_NET_PARAM_T *net_param);

/**@brief model handle. */
typedef struct {
    uint32_t                 model_id;        /**< Model identifier. */
    TKL_MESH_MSG_RECV_CB   model_receive;   /**< Model receive callback. */
    USHORT_T               model_handle;    /**< Model handel or model index. */
} TKL_MESH_MODEL_HADLE_T;

/**@brief mesh node feature. */
typedef enum {
    MESH_FEATURE_RELAY = 0x00,  /**< Relay. */
    MESH_FEATURE_PROXY,         /**< Proxy. */
    MESH_FEATURE_FRIEND,        /**< Friend. */
    MESH_FEATURE_LPN,           /**< LPN. */
} MESH_FEATURE_T;

typedef enum {
    TKL_MESH_EVT_STACK_INIT = 0x01,                 /**< Mesh Stack Initial Callback Event */

    TKL_MESH_EVT_STACK_DEINIT,                      /**< Mesh Stack Deinit Callback Event */

    TKL_MESH_EVT_STACK_RESET,                       /**< Mesh Stack Reset Callback Event */

    TKL_MESH_EVT_UNPROV_BEACON_REPORT,              /**< Receive Mesh Unprovisioned Beacon Data */

    TKL_MESH_EVT_PRIVATE_BEACON_REPORT,             /**< Receive Tuya-Private Beacon Data */

    TKL_MESH_EVT_INVITE_EVT,                        /**< Invite Mesh Device Event */

    TKL_MESH_EVT_CONFIG_DATA_RX,                    /**< Receive Mesh Configuration Message */

    TKL_MESH_EVT_MODEL_DATA_RX,                     /**< Receive Mesh Model Message */
} TKL_MESH_EVT_TYPE_E;

typedef struct {
    UCHAR_T     peer_addr[6];                       /**< Mesh Device Mac Address */
    UCHAR_T     peer_uuid[16];                      /**< Mesh Device UUID */

    USHORT_T    node_addr;                          /**< Mesh Node Address */
    UCHAR_T     node_devkey[16];                    /**< Mesh Device key */
}TKL_MESH_DEVICE_INFO_T;

typedef struct {
    USHORT_T    local_addr;                         /**< Mesh Provisioner Local Address */

    UCHAR_T     netkey[16];                         /**< Mesh Provisioner Local Netkey */
    UCHAR_T     appkey[16];                         /**< Mesh Provisioner Local Appkey */
}TKL_MESH_LOCAL_INFO_T;

typedef struct {
    UCHAR_T     mac[6];                             /**< Mesh Provisioner Scan Adv Mac Address */
    UCHAR_T     uuid[16];                           /**< Mesh Provisioner Scan Mesh Device UUID */
    USHORT_T    oob;                                /**< Mesh Provisioner Scan Mesh Device OOB */
    uint32_t      uri_hash;                           /**< Mesh Provisioner Scan Mesh Device URI Hash */
    char      rssi;                               /**< Mesh Provisioner Scan Mesh Device Rssi */
}TKL_MESH_UNPROV_BEACON_T;

typedef struct {
    UCHAR_T     mac[6];                             /**< Beacon Central Scan Adv Mac Address while in mesh mode*/
    UCHAR_T     length;                             /**< Beacon Central Scan advertising data length*/
    UCHAR_T     *p_data;                            /**< Beacon Central Scan advertising data */
    char      rssi;                               /**< Beacon Central Scan advertising Rssi */
}TKL_PRIVATE_BEACON_T;

typedef struct {
    uint32_t      opcode;                             /**< Mesh opcode. Indicate the mesh data with opcode */
    UCHAR_T     count;                              /**< [Mesh& Gateway Special] The number of transmissions is the Transmit Count + 1 */
    UCHAR_T     interval_steps;                     /**< [Mesh& Gateway Special] Transmission interval = (Network Retransmit Interval Steps + 1) * 10 */

    USHORT_T    data_len;                           /**< Mesh Data Length */
    UCHAR_T     *p_data;                            /**< Pointer For Mesh Data */
}TKL_MESH_DATA_T;

typedef struct {
    uint32_t      opcode;                             /**< Mesh opcode. Point the mesh opcode while receiving data. */
    USHORT_T    node_addr;                          /**< Sending Node Address */
    USHORT_T    dest_addr;                          /**< Receive Destination Address */
    UCHAR_T     recv_ttl;                           /**< Receive TTL */

    USHORT_T    data_len;                           /**< Recevie Mesh Data Length */
    UCHAR_T     *p_data;                            /**< Recevie Pointer of Mesh Data */
}TKL_MESH_DATA_RECEIVE_T;

typedef struct {
    USHORT_T    node_addr;                          /**< Assign mesh Node Address */
    UCHAR_T     devkey[16];                         /**< Get Mesh Node Dev-key After Provision*/
}TKL_MESH_PROV_T;

struct TKL_MESH_DFU_TARGET_PARAM_T{
    USHORT_T    target_addr;                        /**< Mesh DFU Target-Node Addr */
    UCHAR_T     img_idx;                            /**< Mesh DFU Target image index, default: 0*/
    UCHAR_T     result;                             /**< Mesh DFU Target Finish Result*/
} __attribute__ ((packed));

struct TKL_MESH_DFU_BLOB_CONFIG_T{
    USHORT_T    group_addr;                         /**< Mesh DFU Target Group Addr or Single Target Address*/

    USHORT_T    area_id;                            /**< Mesh DFU Image ID */
    UCHAR_T     blob_id[8];                         /**< Mesh BLOB ID */
    uint32_t      blob_size;                          /**< Mesh BLOB Size, Image Size*/
}  __attribute__ ((packed));

typedef struct {
    USHORT_T    number;                             /**< Mesh DFU Target Number */
    struct TKL_MESH_DFU_TARGET_PARAM_T *target;     /**< Mesh DFU Target List for Result */
}TKL_MESH_DFU_TARGET_LIST_T;

typedef enum {
    TKL_MESH_DFU_OP_TARGET_ADD = 0x01,          /**< TKL Mesh Provision Add Target for BLOB Client*/

    TKL_MESH_DFU_OP_PROGRESS_CHECK,             /**< TKL Mesh Provision Check Progress*/

    TKL_MESH_DFU_OP_PROGRESS_GET,               /**< TKL Mesh Provision DFU Progress Report*/

    TKL_MESH_DFU_OP_RESET,                      /**< TKL Mesh Provision DFU Operations: Reset Current Status and data */

    TKL_MESH_DFU_OP_MAX,                        /**< TKL Mesh Provision DFU Event Max*/
} TKL_MESH_DFU_OPERATION_E;

typedef struct {
    /* Called when the reader is opened for reading.*/
    OPERATE_RET (*dfu_open)(USHORT_T area_id, void *user_data);

    /* Used by the BLOB Transfer Client to fetch outgoing data.*/
    uint32_t (*dfu_read)(USHORT_T area_id, uint32_t offset, UCHAR_T *pbuff, uint32_t size);

    /* Called when the reader is closed. */
    OPERATE_RET (*dfu_close)(USHORT_T area_id);

    /* Called when the client report @TKL_MESH_DFU_STATUS_E status. */
    OPERATE_RET (*dfu_start)(USHORT_T group_addr, int result);

    /* Called when the client report result. */
    OPERATE_RET (*dfu_result)(USHORT_T group_addr, TKL_MESH_DFU_TARGET_LIST_T target_list);
}TKL_MESH_DFU_IO_T;

typedef struct {
    TKL_MESH_EVT_TYPE_E         type;               /**< Mesh Event Type */
    int                       state;              /**< Mesh Event States */

    union {
        TKL_MESH_UNPROV_BEACON_T unprov_report;     /**< Receive Mesh Unprovisioned Beacon Data */
        TKL_PRIVATE_BEACON_T     beacon_report;     /**< Receive Tuya Private Beacon Data while running beacon cental in mesh mode*/

        TKL_MESH_PROV_T         prov;               /**< Provision Callback State */
        TKL_MESH_DATA_RECEIVE_T config_report;      /**< Report Mesh Model Configuration Data */
        TKL_MESH_DATA_RECEIVE_T model_report;       /**< Report Mesh Model Message Data */
    }mesh_event;
} TKL_MESH_EVT_PARAMS_T;

/**< Define Event Callback for mesh*/
typedef void(*TKL_MESH_EVT_FUNC_CB)(TKL_MESH_EVT_PARAMS_T *p_event);

typedef enum{
    TKL_MESH_PROVISION_SUCCESS = 0,
    TKL_MESH_PROVISION_START,
    TKL_MESH_PROVISION_TIMEOUT,
    TKL_MESH_RESET,
    TKL_MESH_RESET_IN_RAM,
    TKL_MESH_REVERT_IN_MESH,
    TKL_MESH_GROUP_SUB_ADD,
    TKL_MESH_GROUP_SUB_DEL,

    TKL_OTA_START,
    TKL_OTA_SUCCESS,
    TKL_OTA_FAIL,
}TKL_MESH_STATE_T;

typedef void(*TKL_MESH_NET_STATE_CB_T)(TKL_MESH_STATE_T state);

#endif


#pragma once

#include "bk_dm_bluetooth_types.h"

#ifdef __cplusplus
extern "C" {
#endif


/// subclass of hid device
#define BK_HID_CLASS_UNKNOWN      (0x00<<2)           /*!< unknown HID device subclass */
#define BK_HID_CLASS_JOS          (0x01<<2)           /*!< joystick */
#define BK_HID_CLASS_GPD          (0x02<<2)           /*!< game pad */
#define BK_HID_CLASS_RMC          (0x03<<2)           /*!< remote control */
#define BK_HID_CLASS_SED          (0x04<<2)           /*!< sensing device */
#define BK_HID_CLASS_DGT          (0x05<<2)           /*!< digitizer tablet */
#define BK_HID_CLASS_CDR          (0x06<<2)           /*!< card reader */
#define BK_HID_CLASS_KBD          (0x10<<2)           /*!< keyboard */
#define BK_HID_CLASS_MIC          (0x20<<2)           /*!< pointing device */
#define BK_HID_CLASS_COM          (0x30<<2)           /*!< combo keyboard/pointing */

/**
 * @brief HIDD handshake result code
 */
typedef enum {
    BK_HID_PAR_HANDSHAKE_RSP_SUCCESS = 0,                 /*!< successful */
    BK_HID_PAR_HANDSHAKE_RSP_NOT_READY = 1,               /*!< not ready, device is too busy to accept data */
    BK_HID_PAR_HANDSHAKE_RSP_ERR_INVALID_REP_ID = 2,      /*!< invalid report ID */
    BK_HID_PAR_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ = 3,     /*!< device does not support the request */
    BK_HID_PAR_HANDSHAKE_RSP_ERR_INVALID_PARAM = 4,       /*!< parameter value is out of range or inappropriate */
    BK_HID_PAR_HANDSHAKE_RSP_ERR_UNKNOWN = 14,            /*!< device could not identify the error condition */
    BK_HID_PAR_HANDSHAKE_RSP_ERR_FATAL = 15,              /*!< restart is essential to resume functionality */
} bk_hidd_handshake_error_t;

/**
 * @brief HIDD message types
 */
typedef enum {
    BK_HIDD_MESSAGE_TYPE_HANDSHAKE = 0,                     /*!< handshake type */
    BK_HIDD_MESSAGE_TYPE_HID_CONTROL,                       /*!< hid control type */
    BK_HIDD_MESSAGE_TYPE_GET_REPORT = 4,                    /*!< get report */
    BK_HIDD_MESSAGE_TYPE_SET_REPORT,                        /*!< set report */
    BK_HIDD_MESSAGE_TYPE_GET_PROTOCOL,                      /*!< get protocol */
    BK_HIDD_MESSAGE_TYPE_SET_PROTOCOL,                      /*!< set protocol */
    BK_HIDD_MESSAGE_TYPE_GET_IDLE,                          /*!< get idle [DEPRECATED] */
    BK_HIDD_MESSAGE_TYPE_SET_IDLE,                          /*!< set idle [DEPRECATED] */
    BK_HIDD_MESSAGE_TYPE_DATA,                              /*!< data intr */
} bk_hidd_message_type_t;

/**
 * @brief HIDD report types
 */
typedef enum {
    BK_HIDD_REPORT_TYPE_RESERVED,                   /*!< reserved*/
    BK_HIDD_REPORT_TYPE_INPUT,                      /*!< input report */
    BK_HIDD_REPORT_TYPE_OUTPUT,                     /*!< output report */
    BK_HIDD_REPORT_TYPE_FEATURE,                    /*!< feature report */
} bk_hidd_report_type_t;

/**
 * @brief HIDD control operation
 */
typedef enum {
    BK_HIDD_CTRL_OP_NOP,                            /*!< no operation */
    BK_HIDD_CTRL_OP_HARD_RESET,                     /*!< hadr reset */
    BK_HIDD_CTRL_OP_SOFT_RESET,                     /*!< soft reset */
    BK_HIDD_CTRL_OP_SUSPEND,                        /*!< suspend */
    BK_HIDD_CTRL_OP_EXIT_SUSPEND,                   /*!< exit suspend */
} bk_hidd_ctrl_op_t;


/**
 * @brief HIDD connection state
 */
typedef enum {
    BK_HIDD_CONN_STATE_CONNECTED,                   /*!< HID connection established */
    BK_HIDD_CONN_STATE_CONNECTING,                  /*!< connection to remote Bluetooth device */
    BK_HIDD_CONN_STATE_DISCONNECTED,                /*!< connection released */
    BK_HIDD_CONN_STATE_DISCONNECTING,               /*!< disconnecting to remote Bluetooth device*/
    BK_HIDD_CONN_STATE_UNKNOWN,                     /*!< unknown connection state */
} bk_hidd_connection_state_t;

/**
 * @brief HID device protocol modes
 */
typedef enum {
    BK_HIDD_REPORT_MODE = 0x00,                     /*!< Report Protocol Mode */
    BK_HIDD_BOOT_MODE = 0x01,                       /*!< Boot Protocol Mode */
    BK_HIDD_UNSUPPORTED_MODE = 0xff,                /*!< unsupported */
} bk_hidd_protocol_mode_t;

/**
 * @brief HID Boot Protocol report IDs
 */
typedef enum {
    BK_HIDD_BOOT_REPORT_ID_KEYBOARD = 1,            /*!< report ID of Boot Protocol keyboard report */
    BK_HIDD_BOOT_REPORT_ID_MOUSE = 2,               /*!< report ID of Boot Protocol mouse report */
} bk_hidd_boot_report_id_t;

/**
 * @brief HID Boot Protocol report size including report ID
 */
enum {
    BK_HIDD_BOOT_REPORT_SIZE_KEYBOARD = 9,          /*!< report size of Boot Protocol keyboard report */
    BK_HIDD_BOOT_REPORT_SIZE_MOUSE = 4,             /*!< report size of Boot Protocol mouse report */
};

/**
 * @brief HID device characteristics for SDP server
 */
typedef struct {
    char *name;                                     /*!< service name */
    char *description;                              /*!< service description */
    char *provider;                                 /*!< provider name */
    uint8_t *subclass;                              /*!< HID device subclass */
    uint8_t *desc_list;                             /*!< HID descriptor list */
    int desc_list_len;                              /*!< size in bytes of HID descriptor list */
} bk_hidd_app_param_t;

/**
 * @brief HIDD Quality of Service parameters negotiated over L2CAP
 */
typedef struct {
    uint8_t service_type;                            /*!< the level of service, 0 indicates no traffic */
    uint32_t token_rate;                             /*!< token rate in bytes per second, 0 indicates "don't care" */
    uint32_t token_bucket_size;                      /*!< limit on the burstness of the application data */
    uint32_t peak_bandwidth;                         /*!< bytes per second, value 0 indicates "don't care" */
    uint32_t access_latency;                         /*!< maximum acceptable delay in microseconds */
    uint32_t delay_variation;                        /*!< the difference in microseconds between the max and min delay */
} bk_hidd_qos_param_t;

/**
 * @brief HID device callback function events
 */
typedef enum {
    BK_HIDD_INIT_EVT = 0,       /*!< When HID device is initialized, the event comes */
    BK_HIDD_DEINIT_EVT,         /*!< When HID device is deinitialized, the event comes */
    BK_HIDD_REGISTER_APP_EVT,   /*!< When HID device application registered, the event comes */
    BK_HIDD_UNREGISTER_APP_EVT, /*!< When HID device application unregistered, the event comes */
    BK_HIDD_OPEN_EVT,           /*!< When HID device connection to host opened, the event comes */
    BK_HIDD_CLOSE_EVT,          /*!< When HID device connection to host closed, the event comes */
    BK_HIDD_SEND_REPORT_EVT,    /*!< When HID device send report to lower layer, the event comes */
    BK_HIDD_REPORT_ERR_EVT,     /*!< When HID device report handshanke error to lower layer, the event comes */
    BK_HIDD_GET_REPORT_EVT,     /*!< When HID device receives GET_REPORT request from host, the event comes */
    BK_HIDD_SET_REPORT_EVT,     /*!< When HID device receives SET_REPORT request from host, the event comes */
    BK_HIDD_GET_PROTOCOL_EVT,   /*!< When HID device receives GET_PROTOCOL request from host, the event comes */
    BK_HIDD_SET_PROTOCOL_EVT,   /*!< When HID device receives SET_PROTOCOL request from host, the event comes */
    BK_HIDD_GET_IDLE_EVT,       /*!< When HID device receives GET_IDLE request from host, the event comes */
    BK_HIDD_SET_IDLE_EVT,       /*!< When HID device receives SET_IDLE request from host, the event comes */
    BK_HIDD_INTR_DATA_EVT,      /*!< When HID device receives DATA from host on intr, the event comes */
    BK_HIDD_CONTROL_EVT,        /*!< When HID device receives HID_CONTROL from host, the event comes */
    BK_HIDD_VC_UNPLUG_EVT,      /*!< When HID device initiates Virtual Cable Unplug, the event comes */
    BK_HIDD_API_ERR_EVT         /*!< When HID device has API error, the event comes */
} bk_hidd_cb_event_t;

typedef enum {
    BK_HIDD_SUCCESS,
    BK_HIDD_ERROR,         /*!< general BK HD error */
    BK_HIDD_NO_RES,        /*!< out of system resources */
    BK_HIDD_BUSY,          /*!< Temporarily can not handle this request. */
    BK_HIDD_NO_DATA,       /*!< No data. */
    BK_HIDD_NEED_INIT,     /*!< HIDD module shall init first */
    BK_HIDD_NEED_DEINIT,   /*!< HIDD module shall deinit first */
    BK_HIDD_NEED_REG,      /*!< HIDD module shall register first */
    BK_HIDD_NEED_DEREG,    /*!< HIDD module shall deregister first */
    BK_HIDD_NO_CONNECTION, /*!< connection may have been closed */
    BK_HIDD_NO_ACL_LINK,   /*!< Baseband acl link not exist */
    BK_HIDD_FAIL,          /*!< HIDD Fail */
} bk_hidd_status_t;

/**
 * @brief HID device callback parameters union
 */
typedef union {
    /**
     * @brief BK_HIDD_INIT_EVT
     */
    struct hidd_init_evt_param {
        bk_hidd_status_t status; /*!< operation status */
    } init;                       /*!< HIDD callback param of BK_HIDD_INIT_EVT */

    /**
     * @brief BK_HIDD_DEINIT_EVT
     */
    struct hidd_deinit_evt_param {
        bk_hidd_status_t status; /*!< operation status */
    } deinit;                     /*!< HIDD callback param of BK_HIDD_DEINIT_EVT */

    /**
     * @brief BK_HIDD_REGISTER_APP_EVT
     */
    struct hidd_register_app_evt_param {
        bk_hidd_status_t status; /*!< operation status */
        bool in_use;              /*!< indicate whether use virtual cable plug host address */
        bk_bd_addr_t bd_addr;    /*!< host address */
    } register_app;               /*!< HIDD callback param of BK_HIDD_REGISTER_APP_EVT */

    /**
     * @brief BK_HIDD_UNREGISTER_APP_EVT
     */
    struct hidd_unregister_app_evt_param {
        bk_hidd_status_t status; /*!< operation status         */
    } unregister_app;             /*!< HIDD callback param of BK_HIDD_UNREGISTER_APP_EVT */

    /**
     * @brief BK_HIDD_OPEN_EVT
     */
    struct hidd_open_evt_param {
        bk_hidd_status_t status;                /*!< operation status         */
        bk_hidd_connection_state_t conn_status; /*!< connection status */
        bk_bd_addr_t bd_addr;                   /*!< host address */
    } open;                                      /*!< HIDD callback param of BK_HIDD_OPEN_EVT */

    /**
     * @brief BK_HIDD_CLOSE_EVT
     */
    struct hidd_close_evt_param {
        bk_hidd_status_t status;                /*!< operation status         */
        bk_hidd_connection_state_t conn_status; /*!< connection status        */
    } close;                                     /*!< HIDD callback param of BK_HIDD_CLOSE_EVT */

    /**
     * @brief BK_HIDD_SEND_REPORT_EVT
     */
    struct hidd_send_report_evt_param {
        bk_hidd_status_t status;                /*!< operation status         */
        uint8_t report_id;                      /*!< report id         */
    } send_report;                              /*!< HIDD callback param of BK_HIDD_SEND_REPORT_EVT */

    /**
     * @brief BK_HIDD_REPORT_ERR_EVT
     */
    struct hidd_report_err_evt_param {
        bk_hidd_status_t status; /*!< operation status         */
        uint8_t reason;           /*!< lower layer failed reason(ref hiddefs.h)           */
    } report_err;                 /*!< HIDD callback param of BK_HIDD_REPORT_ERR_EVT */

    /**
     * @brief BK_HIDD_GET_REPORT_EVT
     */
    struct hidd_get_report_evt_param {
        bk_hidd_message_type_t message_type;    /*!< message type        */
        bk_hidd_report_type_t report_type;      /*!< report type        */
        uint8_t report_id;                      /*!< report id         */
        uint16_t buffer_size;                   /*!< buffer size         */
    } get_report;                               /*!< HIDD callback param of BK_HIDD_GET_REPORT_EVT */

    /**
     * @brief BK_HIDD_SET_REPORT_EVT
     */
    struct hidd_set_report_evt_param {
        bk_hidd_message_type_t message_type;    /*!< message type        */
        bk_hidd_report_type_t report_type;      /*!< report type        */
        uint8_t report_id;                      /*!< report id         */
        uint16_t len;                           /*!< set_report data length         */
        uint8_t *data;                          /*!< set_report data pointer         */
    } set_report;                               /*!< HIDD callback param of BK_HIDD_SET_REPORT_EVT */

    /**
     * @brief BK_HIDD_SET_PROTOCOL_EVT
     */
    struct hidd_set_protocol_evt_param {
        bk_hidd_protocol_mode_t protocol_mode; /*!< protocol mode        */
    } set_protocol;                             /*!< HIDD callback param of BK_HIDD_SET_PROTOCOL_EVT */

    /**
     * @brief BK_HIDD_GET_PROTOCOL_EVT
     */
    struct hidd_get_protocol_evt_param {
        bk_hidd_message_type_t message_type;    /*!< message type        */
    } get_protocol;                             /*!< HIDD callback param of BK_HIDD_GET_PROTOCOL_EVT */

     /**
     * @brief BK_HIDD_GET_IDLE_EVT
     */
    struct hidd_get_idle_evt_param {
        bk_hidd_message_type_t message_type;    /*!< message type        */
    } get_idle;                             /*!< HIDD callback param of BK_HIDD_GET_IDLE_EVT */


     /**
     * @brief BK_HIDD_SET_IDLE_EVT
     */
    struct hidd_set_idle_evt_param {
        bk_hidd_message_type_t message_type;    /*!< message type       */
        uint8_t idle;                           /*!< idle rate          */
    } set_idle;                                 /*!< HIDD callback param of BK_HIDD_SET_IDLE_EVT */

    /**
     * @brief BK_HIDD_INTR_DATA_EVT
     */
    struct hidd_intr_data_evt_param {
        bk_hidd_message_type_t message_type;    /*!< message type        */
        bk_hidd_report_type_t report_type;      /*!< report type        */
        uint8_t report_id;                      /*!< interrupt channel report id         */
        uint16_t len;                           /*!< interrupt channel report data length         */
        uint8_t *data;                          /*!< interrupt channel report data pointer         */
    } intr_data;                                /*!< HIDD callback param of BK_HIDD_INTR_DATA_EVT */

    /**
     * @brief BK_HIDD_CTRL_OP_EVT
     */
    struct hidd_ctrl_op_evt_param {
        bk_hidd_message_type_t message_type;    /*!< message type        */
        bk_hidd_ctrl_op_t op;                   /*!< ctral operation     */
    } ctrl_op;                                  /*!< HIDD callback param of BK_HIDD_INTR_DATA_EVT */
    
    /**
     * @brief BK_HIDD_VC_UNPLUG_EVT
     */
    struct hidd_vc_unplug_param {
        bk_hidd_status_t status;                /*!< operation status         */
        bk_hidd_connection_state_t conn_status; /*!< connection status        */
    } vc_unplug;                                 /*!< HIDD callback param of BK_HIDD_VC_UNPLUG_EVT */
} bk_hidd_cb_param_t;

/**
 * @brief           HID device callback function type.
 * @param           event: Event type
 * @param           param: Point to callback parameter, currently is union type
 */
typedef void (*bk_hd_cb_t)(bk_hidd_cb_event_t event, bk_hidd_cb_param_t *param);

/**
 * @brief           This function is called to init callbacks with HID device module.
 *
 * @param[in]       callback: pointer to the init callback function.
 *
 * @return
 *                  - BK_OK: success
 *                  - other: failed
 */
bk_err_t bk_bt_hid_device_register_callback(bk_hd_cb_t callback);

/**
 * @brief           Initializes HIDD interface. This function should be called after bk_bluedroid_init() and
 *                  bk_bluedroid_enable() success, and should be called after bk_bt_hid_device_register_callback.
 *                  When the operation is complete, the callback function will be called with BK_HIDD_INIT_EVT.
 *
 * @return
 *                  - BK_OK: success
 *                  - other: failed
 */
bk_err_t bk_bt_hid_device_init(void);

/**
 * @brief           De-initializes HIDD interface. This function should be called after bk_bluedroid_init() and
 *                  bk_bluedroid_enable() success, and should be called after bk_bt_hid_device_init(). When the
 *                  operation is complete, the callback function will be called with BK_HIDD_DEINIT_EVT.
 *
 * @return
 *                  - BK_OK: success
 *                  - other: failed
 */
bk_err_t bk_bt_hid_device_deinit(void);

/**
 * @brief           Registers HIDD parameters with SDP and sets l2cap Quality of Service. This function should be
 *                  called after bk_bluetooth_init() success, and should be called after
 *                  bk_bt_hid_device_init(). When the operation is complete, the callback function will be called
 *                  with BK_HIDD_REGISTER_APP_EVT.
 *
 * @param[in]       app_param: HIDD parameters
 *
 * @return
 *                  - BK_OK: success
 *                  - other: failed
 */
bk_err_t bk_bt_hid_device_register_app(bk_hidd_app_param_t *app_param);

/**
 * @brief           Removes HIDD parameters from SDP and resets l2cap Quality of Service. This function should be
 *                  called after bk_bluetooth_init() success, and should be called after
 *                  bk_bt_hid_device_init(). When the operation is complete, the callback function will be called
 *                  with BK_HIDD_UNREGISTER_APP_EVT.
 *
 * @return
 *                  - BK_OK: success
 *                  - other: failed
 */
bk_err_t bk_bt_hid_device_unregister_app(void);

/**
 * @brief           Connects to the peer HID Host with virtual cable. This function should be called after
 *                  bk_bluetooth_init() success, and should be called after bk_bt_hid_device_init().
 *                  When the operation is complete, the callback function will be called with BK_HIDD_OPEN_EVT.
 *
 * @param[in]       bd_addr: Remote host bluetooth device address.
 *
 * @return
 *                  - BK_OK: success
 *                  - other: failed
 */
bk_err_t bk_bt_hid_device_connect(bk_bd_addr_t bd_addr);

/**
 * @brief           Disconnects from the currently connected HID Host. This function should be called after
 *                  bk_bluetooth_init() success, and should be called after bk_bt_hid_device_init().
 *                  When the operation is complete, the callback function will be called with BK_HIDD_CLOSE_EVT.
 *
 * @note            The disconnect operation will not remove the virtually cabled device. If the connect request from the
 *                  different HID Host, it will reject the request.
 *
 * @return
 *                  - BK_OK: success
 *                  - other: failed
 */
bk_err_t bk_bt_hid_device_disconnect(void);

/**
 * @brief           Sends HID report to the currently connected HID Host. This function should be called after
 *                  bk_bluetooth_init() success, and should be called after bk_bt_hid_device_init().
 *                  When the operation is complete, the callback function will be called with BK_HIDD_SEND_REPORT_EVT.
 *
 * @param[in]       len: length of report
 * @param[in]       data: report data
 *
 * @return
 *                  - BK_OK: success
 *                  - other: failed
 */
bk_err_t bk_bt_hid_device_send_report(uint16_t len, uint8_t *data);

/**
 * @brief           Sends HID Handshake with error info for invalid set_report to the currently connected HID Host.
 *                  This function should be called after bk_bluetooth_init() success, and
 *                  should be called after bk_bt_hid_device_init(). When the operation is complete, the callback
 *                  function will be called with BK_HIDD_REPORT_ERR_EVT.
 *
 * @param[in]       message_type: type of hid message
 * @param[in]       res_status: response status
 * @param[in]       len: response data len 
 * @param[in]       data: response data
 *
 * @return
 *                  - BK_OK: success
 *                  - other: failed 
 */
bk_err_t bk_bt_hid_device_send_response(uint8_t message_type, uint8_t res_status, uint16_t len, uint8_t* data);

/**
 * @brief           Remove the virtually cabled device. This function should be called after bk_bluetooth_init()
 *                  success, and should be called after bk_bt_hid_device_init(). When the
 *                  operation is complete, the callback function will be called with BK_HIDD_VC_UNPLUG_EVT.
 *
 * @note            If the connection exists, then HID Device will send a `VIRTUAL_CABLE_UNPLUG` control command to
 *                  the peer HID Host, and the connection will be destroyed. If the connection does not exist, then HID
 *                  Device will only unplug on it's single side. Once the unplug operation is success, the related
 *                  pairing and bonding information will be removed, then the HID Device can accept connection request
 *                  from the different HID Host,
 *
 * @return          - BK_OK: success
 *                  - other: failed
 */
bk_err_t bk_bt_hid_device_virtual_cable_unplug(void);

#ifdef __cplusplus
}
#endif


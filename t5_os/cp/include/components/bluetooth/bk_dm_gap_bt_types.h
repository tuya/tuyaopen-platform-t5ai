#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// RSSI threshold
#define BK_BT_GAP_RSSI_HIGH_THRLD  -20             /*!< High RSSI threshold */
#define BK_BT_GAP_RSSI_LOW_THRLD   -45             /*!< Low RSSI threshold */

/// Class of device
typedef union {
    struct class_of_device {
        uint32_t      reserved_2: 2;                    /*!< undefined */
        uint32_t      minor: 6;                         /*!< minor class */
        uint32_t      major: 5;                         /*!< major class */
        uint32_t      service: 11;                      /*!< service class */
        uint32_t      reserved_8: 8;                    /*!< undefined */
    } cod_t;
    uint32_t cod;
} bk_bt_cod_t;

/// class of device settings
typedef enum {
    BK_BT_SET_COD_MAJOR_MINOR     = 0x01,          /*!< overwrite major, minor class */
    BK_BT_SET_COD_SERVICE_CLASS   = 0x02,          /*!< set the bits in the input, the current bit will remain */
    BK_BT_CLR_COD_SERVICE_CLASS   = 0x04,          /*!< clear the bits in the input, others will remain */
    BK_BT_SET_COD_ALL             = 0x08,          /*!< overwrite major, minor, set the bits in service class */
    BK_BT_INIT_COD                = 0x0a,          /*!< overwrite major, minor, and service class */
} bk_bt_cod_mode_t;

#define BK_BT_GAP_AFH_CHANNELS_LEN     10
typedef uint8_t bk_bt_gap_afh_channels[BK_BT_GAP_AFH_CHANNELS_LEN];

/// Bluetooth Device Property type
typedef enum {
    BK_BT_GAP_DEV_PROP_BDNAME = 1,                 /*!< Bluetooth device name, value type is int8_t [] */
    BK_BT_GAP_DEV_PROP_COD,                        /*!< Class of Device, value type is uint32_t */
    BK_BT_GAP_DEV_PROP_RSSI,                       /*!< Received Signal strength Indication, value type is int8_t, ranging from -128 to 127 */
    BK_BT_GAP_DEV_PROP_EIR,                        /*!< Extended Inquiry Response, value type is uint8_t [] */
} bk_bt_gap_dev_prop_type_t;

/// Maximum bytes of Bluetooth device name
#define BK_BT_GAP_MAX_BDNAME_LEN             (248)

/// Maximum size of EIR Significant part
#define BK_BT_GAP_EIR_DATA_LEN               (240)

/// Bluetooth Device Property Descriptor
typedef struct {
    bk_bt_gap_dev_prop_type_t type;                /*!< Device property type */
    int len;                                        /*!< Device property value length */
    void *val;                                      /*!< Device property value */
} bk_bt_gap_dev_prop_t;

/// Extended Inquiry Response data type
#define BK_BT_EIR_TYPE_FLAGS                   0x01      /*!< Flag with information such as BR/EDR and LE support */
#define BK_BT_EIR_TYPE_INCMPL_16BITS_UUID      0x02      /*!< Incomplete list of 16-bit service UUIDs */
#define BK_BT_EIR_TYPE_CMPL_16BITS_UUID        0x03      /*!< Complete list of 16-bit service UUIDs */
#define BK_BT_EIR_TYPE_INCMPL_32BITS_UUID      0x04      /*!< Incomplete list of 32-bit service UUIDs */
#define BK_BT_EIR_TYPE_CMPL_32BITS_UUID        0x05      /*!< Complete list of 32-bit service UUIDs */
#define BK_BT_EIR_TYPE_INCMPL_128BITS_UUID     0x06      /*!< Incomplete list of 128-bit service UUIDs */
#define BK_BT_EIR_TYPE_CMPL_128BITS_UUID       0x07      /*!< Complete list of 128-bit service UUIDs */
#define BK_BT_EIR_TYPE_SHORT_LOCAL_NAME        0x08      /*!< Shortened Local Name */
#define BK_BT_EIR_TYPE_CMPL_LOCAL_NAME         0x09      /*!< Complete Local Name */
#define BK_BT_EIR_TYPE_TX_POWER_LEVEL          0x0a      /*!< Tx power level, value is 1 octet ranging from  -127 to 127, unit is dBm*/
#define BK_BT_EIR_TYPE_URL                     0x24      /*!< Uniform resource identifier */
#define BK_BT_EIR_TYPE_MANU_SPECIFIC           0xff      /*!< Manufacturer specific data */
#define  BK_BT_EIR_TYPE_MAX_NUM                12        /*!< MAX number of EIR type */

typedef uint8_t bk_bt_eir_type_t;



/* BK_BT_EIR_FLAG bit definition */
#define BK_BT_EIR_FLAG_LIMIT_DISC         (0x01 << 0)
#define BK_BT_EIR_FLAG_GEN_DISC           (0x01 << 1)
#define BK_BT_EIR_FLAG_BREDR_NOT_SPT      (0x01 << 2)
#define BK_BT_EIR_FLAG_DMT_CONTROLLER_SPT (0x01 << 3)
#define BK_BT_EIR_FLAG_DMT_HOST_SPT       (0x01 << 4)

#define BK_BT_EIR_MAX_LEN                  240
/// EIR data content, according to "Supplement to the Bluetooth Core Specification"
typedef struct {
    bool                    fec_required;           /*!< FEC is required or not, true by default */
    bool                    include_txpower;        /*!< EIR data include TX power, false by default */
    bool                    include_uuid;           /*!< EIR data include UUID, false by default */
    uint8_t                 flag;                   /*!< EIR flags, see BK_BT_EIR_FLAG for details, EIR will not include flag if it is 0, 0 by default */
    uint16_t                manufacturer_len;       /*!< Manufacturer data length, 0 by default */
    uint8_t                 *p_manufacturer_data;   /*!< Manufacturer data point */
    uint16_t                url_len;                /*!< URL length, 0 by default */
    uint8_t                 *p_url;                 /*!< URL point */
} bk_bt_eir_data_t;

/// Major service class field of Class of Device, mutiple bits can be set
typedef enum {
    BK_BT_COD_SRVC_NONE                     =     0,    /*!< None indicates an invalid value */
    BK_BT_COD_SRVC_LMTD_DISCOVER            =   0x1,    /*!< Limited Discoverable Mode */
    BK_BT_COD_SRVC_POSITIONING              =   0x8,    /*!< Positioning (Location identification) */
    BK_BT_COD_SRVC_NETWORKING               =  0x10,    /*!< Networking, e.g. LAN, Ad hoc */
    BK_BT_COD_SRVC_RENDERING                =  0x20,    /*!< Rendering, e.g. Printing, Speakers */
    BK_BT_COD_SRVC_CAPTURING                =  0x40,    /*!< Capturing, e.g. Scanner, Microphone */
    BK_BT_COD_SRVC_OBJ_TRANSFER             =  0x80,    /*!< Object Transfer, e.g. v-Inbox, v-Folder */
    BK_BT_COD_SRVC_AUDIO                    = 0x100,    /*!< Audio, e.g. Speaker, Microphone, Headset service */
    BK_BT_COD_SRVC_TELEPHONY                = 0x200,    /*!< Telephony, e.g. Cordless telephony, Modem, Headset service */
    BK_BT_COD_SRVC_INFORMATION              = 0x400,    /*!< Information, e.g., WEB-server, WAP-server */
} bk_bt_cod_srvc_t;

typedef enum{
    BK_BT_PIN_TYPE_VARIABLE = 0,                       /*!< Refer to BTM_PIN_TYPE_VARIABLE */
    BK_BT_PIN_TYPE_FIXED    = 1,                       /*!< Refer to BTM_PIN_TYPE_FIXED */
} bk_bt_pin_type_t;

#define BK_BT_PIN_CODE_LEN        16                   /*!< Max pin code length */
typedef uint8_t bk_bt_pin_code_t[BK_BT_PIN_CODE_LEN]; /*!< Pin Code (upto 128 bits) MSB is 0 */

typedef enum {
    BK_BT_SP_IOCAP_MODE = 0,                            /*!< Set IO mode */
    //BK_BT_SP_OOB_DATA, //TODO                         /*!< Set OOB data */
} bk_bt_sp_param_t;

/** IO Capability Constants */
#define BK_BT_IO_CAP_OUT                      0        /*!< DisplayOnly */
#define BK_BT_IO_CAP_IO                       1        /*!< DisplayYesNo */
#define BK_BT_IO_CAP_IN                       2        /*!< KeyboardOnly */
#define BK_BT_IO_CAP_NONE                     3        /*!< NoInputNoOutput */
typedef uint8_t bk_bt_io_cap_t;                        /*!< Combination of the IO Capability */


/* BTM Power manager modes */
#define BK_BT_PM_MD_ACTIVE                 0x00        /*!< Active mode */
#define BK_BT_PM_MD_HOLD                   0x01        /*!< Hold mode */
#define BK_BT_PM_MD_SNIFF                  0x02        /*!< Sniff mode */
#define BK_BT_PM_MD_PARK                   0x03        /*!< Park state */
typedef uint8_t bk_bt_pm_mode_t;



/// Bits of major service class field
#define BK_BT_COD_SRVC_BIT_MASK              (0xffe000) /*!< Major service bit mask */
#define BK_BT_COD_SRVC_BIT_OFFSET            (13)       /*!< Major service bit offset */

/// Major device class field of Class of Device
typedef enum {
    BK_BT_COD_MAJOR_DEV_MISC                = 0,    /*!< Miscellaneous */
    BK_BT_COD_MAJOR_DEV_COMPUTER            = 1,    /*!< Computer */
    BK_BT_COD_MAJOR_DEV_PHONE               = 2,    /*!< Phone(cellular, cordless, pay phone, modem */
    BK_BT_COD_MAJOR_DEV_LAN_NAP             = 3,    /*!< LAN, Network Access Point */
    BK_BT_COD_MAJOR_DEV_AV                  = 4,    /*!< Audio/Video(headset, speaker, stereo, video display, VCR */
    BK_BT_COD_MAJOR_DEV_PERIPHERAL          = 5,    /*!< Peripheral(mouse, joystick, keyboard) */
    BK_BT_COD_MAJOR_DEV_IMAGING             = 6,    /*!< Imaging(printer, scanner, camera, display */
    BK_BT_COD_MAJOR_DEV_WEARABLE            = 7,    /*!< Wearable */
    BK_BT_COD_MAJOR_DEV_TOY                 = 8,    /*!< Toy */
    BK_BT_COD_MAJOR_DEV_HEALTH              = 9,    /*!< Health */
    BK_BT_COD_MAJOR_DEV_UNCATEGORIZED       = 31,   /*!< Uncategorized: device not specified */
} bk_bt_cod_major_dev_t;

/// Bits of major device class field
#define BK_BT_COD_MAJOR_DEV_BIT_MASK         (0x1f00) /*!< Major device bit mask */
#define BK_BT_COD_MAJOR_DEV_BIT_OFFSET       (8)      /*!< Major device bit offset */

/// Bits of minor device class field
#define BK_BT_COD_MINOR_DEV_BIT_MASK         (0xfc)   /*!< Minor device bit mask */
#define BK_BT_COD_MINOR_DEV_BIT_OFFSET       (2)      /*!< Minor device bit offset */

/// Bits of format type
#define BK_BT_COD_FORMAT_TYPE_BIT_MASK       (0x03)   /*!< Format type bit mask */
#define BK_BT_COD_FORMAT_TYPE_BIT_OFFSET     (0)      /*!< Format type bit offset */

/// Class of device format type 1
#define BK_BT_COD_FORMAT_TYPE_1              (0x00)

/** Bluetooth Device Discovery state */
typedef enum {
    BK_BT_GAP_DISCOVERY_STOPPED,                   /*!< Device discovery stopped */
    BK_BT_GAP_DISCOVERY_STARTED,                   /*!< Device discovery started */
} bk_bt_gap_discovery_state_t;

/// bt sniff config
typedef struct
{
    uint16_t sniff_max_interval;                   /// unit 0.625ms, range: 1.25ms ~ 40.9s
    uint16_t sniff_min_interval;                   /// unit 0.625ms, range: 1.25ms ~ 40.9s
    uint16_t sniff_attempt;                        /// unit 1.25ms, range: 1.25ms ~ 40.9s
    uint16_t sniff_timeout;                        /// unit 1.25ms, range: 0ms ~ 40.9s
}bk_bt_gap_sniff_config;

/// BT GAP callback events
typedef enum {
    BK_BT_GAP_DISC_RES_EVT = 0,                    /*!< Device discovery result event */
    BK_BT_GAP_DISC_STATE_CHANGED_EVT,              /*!< Discovery state changed event */
    BK_BT_GAP_RMT_SRVCS_EVT,                       /*!< Get remote services event */
    BK_BT_GAP_RMT_SRVC_REC_EVT,                    /*!< Get remote service record event */
    BK_BT_GAP_AUTH_CMPL_EVT,                       /*!< Authentication complete event */
    BK_BT_GAP_PIN_REQ_EVT,                         /*!< Legacy Pairing Pin code request */
    BK_BT_GAP_CFM_REQ_EVT,                         /*!< Security Simple Pairing User Confirmation request. */
    BK_BT_GAP_KEY_NOTIF_EVT,                       /*!< Security Simple Pairing Passkey Notification */
    BK_BT_GAP_KEY_REQ_EVT,                         /*!< Security Simple Pairing Passkey request */
    BK_BT_GAP_READ_RSSI_DELTA_EVT,                 /*!< Read rssi event */
    BK_BT_GAP_CONFIG_EIR_DATA_EVT,                 /*!< Config EIR data event */
    BK_BT_GAP_SET_AFH_CHANNELS_EVT,                /*!< Set AFH channels event */
    BK_BT_GAP_READ_REMOTE_NAME_EVT,                /*!< Read Remote Name event */
    BK_BT_GAP_MODE_CHG_EVT,
    BK_BT_GAP_REMOVE_BOND_DEV_COMPLETE_EVT,         /*!< remove bond device complete event */
    BK_BT_GAP_QOS_CMPL_EVT,                        /*!< QOS complete event */
    BK_BT_GAP_ACL_CONN_CMPL_STAT_EVT,              /*!< ACL connection complete status event */
    BK_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT,           /*!< ACL disconnection complete status event */
    BK_BT_GAP_LINK_KEY_NOTIF_EVT,                   /*!< Link Key Notification */
    BK_BT_GAP_LINK_KEY_REQ_EVT,                     /*!< Link Key request */
    BK_BT_GAP_SET_AUTO_SNIFF_CMPL_EVT,             /// set auto entry sniff req completed event
    BK_BT_GAP_EVT_MAX,
} bk_gap_bt_cb_event_t;

/** Inquiry Mode */
typedef enum {
    BK_BT_INQ_MODE_GENERAL_INQUIRY,                /*!< General inquiry mode */
    BK_BT_INQ_MODE_LIMITED_INQUIRY,                /*!< Limited inquiry mode */
} bk_bt_inq_mode_t;

/** Minimum and Maximum inquiry length*/
#define BK_BT_GAP_MIN_INQ_LEN                (0x01)  /*!< Minimum inquiry duration, unit is 1.28s */
#define BK_BT_GAP_MAX_INQ_LEN                (0x30)  /*!< Maximum inquiry duration, unit is 1.28s */

/// GAP state callback parameters
typedef union {
    /**
     * @brief BK_BT_GAP_DISC_RES_EVT
     */
    struct disc_res_param {
        bk_bd_addr_t bda;                     /*!< remote bluetooth device address*/
        int num_prop;                          /*!< number of properties got */
        bk_bt_gap_dev_prop_t *prop;           /*!< properties discovered from the new device */
    } disc_res;                                /*!< discovery result parameter struct */

    /**
     * @brief  BK_BT_GAP_DISC_STATE_CHANGED_EVT
     */
    struct disc_state_changed_param {
        bk_bt_gap_discovery_state_t state;    /*!< discovery state */
    } disc_st_chg;                             /*!< discovery state changed parameter struct */

    /**
     * @brief BK_BT_GAP_RMT_SRVCS_EVT
     */
    struct rmt_srvcs_param {
        bk_bd_addr_t bda;                     /*!< remote bluetooth device address*/
        bk_bt_status_t stat;                  /*!< service search status */
        int num_uuids;                         /*!< number of UUID in uuid_list */
        bk_bt_uuid_t *uuid_list;              /*!< list of service UUIDs of remote device */
    } rmt_srvcs;                               /*!< services of remote device parameter struct */

    /**
     * @brief BK_BT_GAP_RMT_SRVC_REC_EVT
     */
    struct rmt_srvc_rec_param {
        bk_bd_addr_t bda;                     /*!< remote bluetooth device address*/
        bk_bt_status_t stat;                  /*!< service search status */
    } rmt_srvc_rec;                            /*!< specific service record from remote device parameter struct */

    /**
     * @brief BK_BT_GAP_READ_RSSI_DELTA_EVT *
     */
    struct read_rssi_delta_param {
        bk_bd_addr_t bda;                     /*!< remote bluetooth device address*/
        bk_bt_status_t stat;                  /*!< read rssi status */
        int8_t rssi_delta;                     /*!< rssi delta value range -128 ~127, The value zero indicates that the RSSI is inside the Golden Receive Power Range, the Golden Receive Power Range is from BK_BT_GAP_RSSI_LOW_THRLD to BK_BT_GAP_RSSI_HIGH_THRLD */
    } read_rssi_delta;                         /*!< read rssi parameter struct */

    /**
     * @brief BK_BT_GAP_CONFIG_EIR_DATA_EVT *
     */
    struct config_eir_data_param {
        bk_bt_status_t stat;                                   /*!< config EIR status:
                                                                    BK_BT_STATUS_SUCCESS: config success
                                                                    BK_BT_STATUS_EIR_TOO_LARGE: the EIR data is more than 240B. The EIR may not contain the whole data.
                                                                    others: failed
                                                                */
        uint8_t eir_type_num;                                   /*!< the number of EIR types in EIR type */
        bk_bt_eir_type_t eir_type[BK_BT_EIR_TYPE_MAX_NUM];    /*!< EIR types in EIR type */
    } config_eir_data;                                          /*!< config EIR data */

    /**
     * @brief BK_BT_GAP_AUTH_CMPL_EVT
     */
    struct auth_cmpl_param {
        bk_bd_addr_t bda;                     /*!< remote bluetooth device address*/
        bk_bt_status_t stat;                  /*!< authentication complete status */
    } auth_cmpl;                               /*!< authentication complete parameter struct */

    /**
     * @brief BK_BT_GAP_PIN_REQ_EVT
     */
    struct pin_req_param {
        bk_bd_addr_t bda;                     /*!< remote bluetooth device address*/
        bool min_16_digit;                     /*!< TRUE if the pin returned must be at least 16 digits */
    } pin_req;                                 /*!< pin request parameter struct */

    /**
     * @brief BK_BT_GAP_CFM_REQ_EVT
     */
    struct cfm_req_param {
        bk_bd_addr_t bda;                     /*!< remote bluetooth device address*/
        uint32_t num_val;                      /*!< the numeric value for comparison. */
    } cfm_req;                                 /*!< confirm request parameter struct */

    /**
     * @brief BK_BT_GAP_KEY_NOTIF_EVT
     */
    struct key_notif_param {
        bk_bd_addr_t bda;                     /*!< remote bluetooth device address*/
        uint32_t passkey;                      /*!< the numeric value for passkey entry. */
    } key_notif;                               /*!< passkey notif parameter struct */

    /**
     * @brief BK_BT_GAP_KEY_REQ_EVT
     */
    struct key_req_param {
        bk_bd_addr_t bda;                     /*!< remote bluetooth device address*/
    } key_req;                                 /*!< passkey request parameter struct */

    /**
     * @brief BK_BT_GAP_SET_AFH_CHANNELS_EVT
     */
    struct set_afh_channels_param {
        bk_bt_status_t stat;                  /*!< set AFH channel status */
    } set_afh_channels;                        /*!< set AFH channel parameter struct */

    /**
     * @brief BK_BT_GAP_READ_REMOTE_NAME_EVT
     */
    struct read_rmt_name_param {
        bk_bt_status_t stat;                  /*!< read Remote Name status */
        uint8_t rmt_name[BK_BT_GAP_MAX_BDNAME_LEN + 1]; /*!< Remote device name */
        bk_bd_addr_t bda;                     /*!< remote bluetooth device address*/
    } read_rmt_name;                        /*!< read Remote Name parameter struct */

    /**
     * @brief BK_BT_GAP_MODE_CHG_EVT
     */
    struct mode_chg_param {
        bk_bd_addr_t bda;                      /*!< remote bluetooth device address*/
        bk_bt_pm_mode_t mode;                  /*!< PM mode*/
    } mode_chg;                                 /*!< mode change event parameter struct */

    /**
     * @brief BK_BT_GAP_REMOVE_BOND_DEV_COMPLETE_EVT
     */
    struct bt_remove_bond_dev_cmpl_evt_param {
        bk_bd_addr_t bda;                          /*!< remote bluetooth device address*/
        bk_bt_status_t status;                     /*!< Indicate the remove bond device operation success status */
    }remove_bond_dev_cmpl;                           /*!< Event parameter of BK_BT_GAP_REMOVE_BOND_DEV_COMPLETE_EVT */

    /**
     * @brief BK_BT_GAP_QOS_CMPL_EVT
     */
    struct qos_cmpl_param {
        bk_bt_status_t stat;                  /*!< QoS status */
        bk_bd_addr_t bda;                     /*!< remote bluetooth device address*/
        uint32_t t_poll;                       /*!< poll interval, the maximum time between transmissions
                                                    which from the master to a particular slave on the ACL
                                                    logical transport. unit is 0.625ms. */
    } qos_cmpl;                                /*!< QoS complete parameter struct */

    /**
     * @brief BK_BT_GAP_ACL_CONN_CMPL_STAT_EVT
     */
    struct acl_conn_cmpl_stat_param {
        bk_bt_status_t stat;                  /*!< ACL connection status */
        uint16_t handle;                       /*!< ACL connection handle */
        bk_bd_addr_t bda;                     /*!< remote bluetooth device address */
        uint32_t cod;                         /*!< Class of Device for remote bluetooth device, it is only valid
                                                    when paged by remote bluetooth device*/
    } acl_conn_cmpl_stat;                      /*!< ACL connection complete status parameter struct */

    /**
     * @brief BK_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT
     */
    struct acl_disconn_cmpl_stat_param {
        bk_bt_status_t reason;                /*!< ACL disconnection reason */
        uint16_t handle;                       /*!< ACL connection handle */
        bk_bd_addr_t bda;                     /*!< remote bluetooth device address */
    } acl_disconn_cmpl_stat;                   /*!< ACL disconnection complete status parameter struct */

    /**
     * @brief BK_BT_GAP_LINK_KEY_NOTIF_EVT
     */
    struct link_key_notif_param {
        bk_bd_addr_t bda;                     /*!< remote bluetooth device address*/
        uint8_t link_key[16];                      /*!< Link Key for the associated bda. */
    } link_key_notif;                               /*!< link key notif parameter struct */

    /**
     * @brief BK_BT_GAP_LINK_KEY_REQ_EVT
     */
    struct link_key_req_param {
        bk_bd_addr_t bda;                     /*!< remote bluetooth device address*/
    } link_key_req;                                 /*!< link key request parameter struct */

    /**
     * @brief BK_BT_GAP_SET_AUTO_SNIFF_CMPL_EVT
     */
    struct auto_sniff_cmpl_param
    {
        bk_bd_addr_t bda;                    /// peer addr
        bk_bt_status_t result;
    } auto_sniff_cmpl;
} bk_bt_gap_cb_param_t;

/**
 * @brief           bluetooth GAP callback function type
 *
 * @param           event : Event type
 *
 * @param           param : Pointer to callback parameter
 */
typedef void (* bk_bt_gap_cb_t)(bk_gap_bt_cb_event_t event, bk_bt_gap_cb_param_t *param);

#ifdef __cplusplus
}
#endif


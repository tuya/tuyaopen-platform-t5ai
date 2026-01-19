#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif



/**
 * @defgroup dm_gap_v1 GAP API
 * @{
 */

/**
 * @defgroup dm_gap_defines_v1 Defines
 * @{
 */

typedef enum
{
    BK_LE_KEY_NONE                    = 0,           /*!< No encryption key */
    BK_LE_KEY_PENC                    = (1 << 0),    /*!< encryption key, encryption information of peer device */
    BK_LE_KEY_PID                     = (1 << 1),    /*!< identity key of the peer device */
    BK_LE_KEY_PCSRK                   = (1 << 2),    /*!< peer SRK */
    BK_LE_KEY_PLK                     = (1 << 3),    /*!< Link key*/ //br edr key
    BK_LE_KEY_LLK                     = (BK_LE_KEY_PLK << 4),      /*!< link key*/ //br edr key
    BK_LE_KEY_LENC                    = (BK_LE_KEY_PENC << 4),     /*!< master role security information:div */
    BK_LE_KEY_LID                     = (BK_LE_KEY_PID << 4),      /*!< master device ID key */
    BK_LE_KEY_LCSRK                   = (BK_LE_KEY_PCSRK << 4),    /*!< local CSRK has been deliver to peer */
} bk_le_key_enum_t;

/// see bk_le_key_enum_t
typedef uint8_t bk_ble_key_type_t;

/// see bk_le_key_enum_t
typedef uint8_t bk_ble_key_mask_t;

typedef enum
{
    BK_LE_AUTH_NO_BOND                = 0x00,        /*!< 0  no bondingv*/
    BK_LE_AUTH_BOND                   = 0x01,        /*!< 1 << 0 device in the bonding with peer */
    BK_LE_AUTH_REQ_MITM               = (1 << 2),    /*!< 1 << 2 man in the middle attack */
    BK_LE_AUTH_REQ_BOND_MITM          = (BK_LE_AUTH_BOND | BK_LE_AUTH_REQ_MITM), /*!< 0101 banding with man in the middle attack */
    BK_LE_AUTH_REQ_SC_ONLY            = (1 << 3),                                  /*!< 1 << 3  secure connection */
    BK_LE_AUTH_REQ_SC_BOND            = (BK_LE_AUTH_BOND | BK_LE_AUTH_REQ_SC_ONLY),            /*!< 1001 secure connection with band*/
    BK_LE_AUTH_REQ_SC_MITM            = (BK_LE_AUTH_REQ_MITM | BK_LE_AUTH_REQ_SC_ONLY),        /*!< 1100 secure conn with MITM */
    BK_LE_AUTH_REQ_SC_MITM_BOND       = (BK_LE_AUTH_REQ_MITM | BK_LE_AUTH_REQ_SC_ONLY | BK_LE_AUTH_BOND),   /*!< 1101 SC with MITM and Bonding*/
} bk_le_auth_enum_t;

/// see bk_le_auth_enum_t
typedef uint8_t   bk_ble_auth_req_t;

#define BK_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE 0    /*!< authentication disable*/
#define BK_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_ENABLE  1    /*!< authentication enable*/

/**
 * @brief IO capability.
 *
 */
typedef enum
{
    BK_IO_CAP_DISPLAY_ONLY                      = 0,   /*!< output: display, input: none */
    BK_IO_CAP_DISPLAY_YESNO                     = 1,   /*!< output: display, input: yes or no */
    BK_IO_CAP_KEYBOARD_ONLY                     = 2,   /*!< output: none, input: keyboard. note: when both local and peer are keyboard only, both side should input same passkey which negotiated in other way, such as oral speak */
    BK_IO_CAP_NONE                              = 3,   /*!< NoInputNoOutput */
    BK_IO_CAP_KEYBOARD_DISPLAY                  = 4,   /*!< output: display, input: keyboard */
} bk_io_cap_enum_t;

/// GAP BLE callback event type
typedef enum
{
    //BLE_42_FEATURE_SUPPORT
    BK_BLE_GAP_ADV_DATA_SET_COMPLETE_EVT        = 0,       /*!< When advertising data set complete, the event comes */
    BK_BLE_GAP_SCAN_RSP_DATA_SET_COMPLETE_EVT,             /*!< When scan response data set complete, the event comes */
    BK_BLE_GAP_SCAN_PARAM_SET_COMPLETE_EVT,                /*!< When scan parameters set complete, the event comes */
    BK_BLE_GAP_SCAN_RESULT_EVT,                            /*!< When one scan result ready, the event comes each time */
    BK_BLE_GAP_ADV_DATA_RAW_SET_COMPLETE_EVT,              /*!< When raw advertising data set complete, the event comes */
    BK_BLE_GAP_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT,         /*!< When raw advertising data set complete, the event comes */
    BK_BLE_GAP_ADV_START_COMPLETE_EVT,                     /*!< When start advertising complete, the event comes */
    BK_BLE_GAP_SCAN_START_COMPLETE_EVT,                    /*!< When start scan complete, the event comes */
    //BLE_INCLUDED
    BK_BLE_GAP_AUTH_CMPL_EVT = 8,                          /*!< Authentication complete indication, param is bk_ble_sec_t. */
    BK_BLE_GAP_KEY_EVT,                                    /*!< BLE key event for peer device keys, param is bk_ble_sec_t */
    BK_BLE_GAP_SEC_REQ_EVT,                                /*!< BLE security request, param is bk_ble_sec_t. */
    BK_BLE_GAP_PASSKEY_NOTIF_EVT,                          /*!< passkey notification event, param is bk_ble_sec_t*/
    BK_BLE_GAP_PASSKEY_REQ_EVT,                            /*!< passkey request event, param is bk_ble_sec_t */
    BK_BLE_GAP_OOB_REQ_EVT,                                /*!< OOB request event, param is bk_ble_sec_t */
    BK_BLE_GAP_LOCAL_IR_EVT,                               /*!< BLE local IR (Encryption Root value used to generate identity resolving key) event */
    BK_BLE_GAP_LOCAL_ER_EVT,                               /*!< BLE local ER (identity Root 128-bit random static value used to generate Long Term Key) event */

    BK_BLE_GAP_NC_REQ_EVT,                                 /*!< Numeric Comparison request event, param is bk_ble_sec_t */


    BK_BLE_GAP_BOND_KEY_REQ_EVT,                           /*!< the mapping relation of param.ble_key.key_type is : when BK_LE_KEY_LENC,
                                                                app need reply BK_LE_KEY_LENC with bk_ble_pair_key_reply according compare param.ble_key.lenc_key */
    BK_BLE_GAP_BOND_KEY_GENERATE_EVT,                      /*!< app layer should save to storage for next power on. */
    BK_BLE_GAP_BOND_DEV_LIST_OPERATEION_COMPLETE_EVT,


    //BLE_42_FEATURE_SUPPORT
    BK_BLE_GAP_ADV_STOP_COMPLETE_EVT,                      /*!< When stop adv complete, the event comes */
    BK_BLE_GAP_SCAN_STOP_COMPLETE_EVT,                     /*!< When stop scan complete, the event comes */
    //BLE_INCLUDED
    BK_BLE_GAP_SET_STATIC_RAND_ADDR_EVT,                   /*!< When set the static rand address complete, the event comes */
    BK_BLE_GAP_UPDATE_CONN_PARAMS_EVT,                     /*!< When update connection parameters complete, the event comes */
    BK_BLE_GAP_SET_PKT_LENGTH_COMPLETE_EVT,                /*!< When set pkt length complete, the event comes */
    BK_BLE_GAP_SET_LOCAL_PRIVACY_COMPLETE_EVT,             /*!< When  Enable/disable privacy on the local device complete, the event comes */
    BK_BLE_GAP_READ_RSSI_COMPLETE_EVT,                     /*!< When read the rssi complete, the event comes */
    BK_BLE_GAP_UPDATE_WHITELIST_COMPLETE_EVT,              /*!< When add or remove whitelist complete, the event comes */
    //BLE_42_FEATURE_SUPPORT
    BK_BLE_GAP_UPDATE_DUPLICATE_EXCEPTIONAL_LIST_COMPLETE_EVT,  /*!< When update duplicate exceptional list complete, the event comes */
    //BLE_INCLUDED
    BK_BLE_GAP_SET_CHANNELS_COMPLETE_EVT,                           /*!< When setting BLE channels complete, the event comes */
    //BLE_50_FEATURE_SUPPORT
    BK_BLE_GAP_READ_PHY_COMPLETE_EVT,                           /*!< when reading phy complete, this event comes */
    BK_BLE_GAP_SET_PREFERRED_DEFAULT_PHY_COMPLETE_EVT,          /*!< when preferred default phy complete, this event comes */
    BK_BLE_GAP_SET_PREFERRED_PHY_COMPLETE_EVT,                  /*!< when preferred phy complete , this event comes */
    BK_BLE_GAP_EXT_ADV_SET_RAND_ADDR_COMPLETE_EVT,              /*!< when extended set random address complete, the event comes */
    BK_BLE_GAP_EXT_ADV_PARAMS_SET_COMPLETE_EVT,                 /*!< when extended advertising parameter complete, the event comes */
    BK_BLE_GAP_EXT_ADV_DATA_SET_COMPLETE_EVT,                   /*!< when extended advertising data complete, the event comes */
    BK_BLE_GAP_EXT_SCAN_RSP_DATA_SET_COMPLETE_EVT,              /*!< when extended scan response data complete, the event comes */
    BK_BLE_GAP_EXT_ADV_DATA_RAW_SET_COMPLETE_EVT,               /*!< when raw extended advertising data complete, the event comes */
    BK_BLE_GAP_EXT_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT,          /*!< when raw extended scan response data complete, the event comes */
    BK_BLE_GAP_EXT_ADV_START_COMPLETE_EVT,                      /*!< when extended advertising start complete, the event comes */
    BK_BLE_GAP_EXT_ADV_STOP_COMPLETE_EVT,                       /*!< when extended advertising stop complete, the event comes */
    BK_BLE_GAP_EXT_ADV_SET_REMOVE_COMPLETE_EVT,                 /*!< when extended advertising set remove complete, the event comes */
    BK_BLE_GAP_EXT_ADV_SET_CLEAR_COMPLETE_EVT,                  /*!< when extended advertising set clear complete, the event comes */
    BK_BLE_GAP_PERIODIC_ADV_SET_PARAMS_COMPLETE_EVT,            /*!< when periodic advertising parameter complete, the event comes */
    BK_BLE_GAP_PERIODIC_ADV_DATA_SET_COMPLETE_EVT,              /*!< when periodic advertising data complete, the event comes */
    BK_BLE_GAP_PERIODIC_ADV_START_COMPLETE_EVT,                 /*!< when periodic advertising start complete, the event comes */
    BK_BLE_GAP_PERIODIC_ADV_STOP_COMPLETE_EVT,                  /*!< when periodic advertising stop complete, the event comes */
    BK_BLE_GAP_PERIODIC_ADV_CREATE_SYNC_COMPLETE_EVT,           /*!< when periodic advertising create sync complete, the event comes */
    BK_BLE_GAP_PERIODIC_ADV_SYNC_CANCEL_COMPLETE_EVT,           /*!< when extended advertising sync cancel complete, the event comes */
    BK_BLE_GAP_PERIODIC_ADV_SYNC_TERMINATE_COMPLETE_EVT,        /*!< when extended advertising sync terminate complete, the event comes */
    BK_BLE_GAP_PERIODIC_ADV_ADD_DEV_COMPLETE_EVT,               /*!< when extended advertising add device complete , the event comes */
    BK_BLE_GAP_PERIODIC_ADV_REMOVE_DEV_COMPLETE_EVT,            /*!< when extended advertising remove device complete, the event comes */
    BK_BLE_GAP_PERIODIC_ADV_CLEAR_DEV_COMPLETE_EVT,             /*!< when extended advertising clear device, the event comes */
    BK_BLE_GAP_EXT_SCAN_PARAMS_SET_COMPLETE_EVT,                /*!< when extended scan parameter complete, the event comes */
    BK_BLE_GAP_EXT_SCAN_START_COMPLETE_EVT,                     /*!< when extended scan start complete, the event comes */
    BK_BLE_GAP_EXT_SCAN_STOP_COMPLETE_EVT,                      /*!< when extended scan stop complete, the event comes */
    BK_BLE_GAP_PREFER_EXT_CONN_PARAMS_SET_COMPLETE_EVT,         /*!< when extended prefer connection parameter set complete, the event comes */
    BK_BLE_GAP_PHY_UPDATE_COMPLETE_EVT,                         /*!< when ble phy update complete, the event comes */
    BK_BLE_GAP_EXT_ADV_REPORT_EVT,                              /*!< when extended advertising report complete, the event comes */
    BK_BLE_GAP_SCAN_TIMEOUT_EVT,                                /*!< when scan timeout complete, the event comes */
    BK_BLE_GAP_ADV_TERMINATED_EVT,                              /*!< when advertising terminate data complete, the event comes */
    BK_BLE_GAP_SCAN_REQ_RECEIVED_EVT,                           /*!< when scan req received complete, the event comes */
    BK_BLE_GAP_CHANNEL_SELECT_ALGORITHM_EVT,                    /*!< when channel select algorithm complete, the event comes */
    BK_BLE_GAP_PERIODIC_ADV_REPORT_EVT,                         /*!< when periodic report advertising complete, the event comes */
    BK_BLE_GAP_PERIODIC_ADV_SYNC_LOST_EVT,                      /*!< when periodic advertising sync lost complete, the event comes */
    BK_BLE_GAP_PERIODIC_ADV_SYNC_ESTAB_EVT,                     /*!< when periodic advertising sync establish complete, the event comes */

    BK_BLE_GAP_CONNECT_COMPLETE_EVT,                            /*!< when ble connection complete, the event comes */
    BK_BLE_GAP_DISCONNECT_COMPLETE_EVT,                         /*!< when ble disconnect complete, the event comes */
    BK_BLE_GAP_CONNECT_CANCEL_EVT,                              /*!< when ble connect cancel complete, the event comes */
    BK_BLE_GAP_UPDATE_CONN_PARAMS_REQ_EVT,                      /*!< when peer req update param, the event comes */
    BK_BLE_GAP_SET_SECURITY_PARAMS_COMPLETE_EVT,                /*!< when set security params complete, the event comes */
    BK_BLE_GAP_GENERATE_RPA_COMPLETE_EVT,                       /*!< when generate rpa complete, the event comes */
#if 0
    //BLE_INCLUDED
    BK_BLE_GAP_SC_OOB_REQ_EVT,                                  /*!< Secure Connection OOB request event */
    BK_BLE_GAP_SC_CR_LOC_OOB_EVT,                               /*!< Secure Connection create OOB data complete event */
    BK_BLE_GAP_GET_DEV_NAME_COMPLETE_EVT,                       /*!< When getting BT device name complete, the event comes */
    //BLE_FEAT_PERIODIC_ADV_SYNC_TRANSFER
    BK_BLE_GAP_PERIODIC_ADV_RECV_ENABLE_COMPLETE_EVT,           /*!< when set periodic advertising receive enable complete, the event comes */
    BK_BLE_GAP_PERIODIC_ADV_SYNC_TRANS_COMPLETE_EVT,            /*!< when periodic advertising sync transfer complete, the event comes */
    BK_BLE_GAP_PERIODIC_ADV_SET_INFO_TRANS_COMPLETE_EVT,        /*!< when periodic advertising set info transfer complete, the event comes */
    BK_BLE_GAP_SET_PAST_PARAMS_COMPLETE_EVT,                    /*!< when set periodic advertising sync transfer params complete, the event comes */
    BK_BLE_GAP_PERIODIC_ADV_SYNC_TRANS_RECV_EVT,                /*!< when periodic advertising sync transfer received, the event comes */
#endif

    BK_BLE_GAP_EVT_MAX,                                         /*!< when maximum advertising event complete, the event comes */
} bk_ble_gap_cb_event_t;

#define BK_BLE_GAP_CHANNELS_LEN     5                           /*!< channel length*/
typedef uint8_t bk_ble_gap_channels[BK_BLE_GAP_CHANNELS_LEN];

/// Advertising data maximum length
#define BK_BLE_ADV_DATA_LEN_MAX               31
/// Scan response data maximum length
#define BK_BLE_SCAN_RSP_DATA_LEN_MAX          31

/// The type of advertising data(not adv_type)
typedef enum
{
    BK_BLE_AD_TYPE_FLAG               = 0x01,
    BK_BLE_AD_TYPE_16SRV_PART         = 0x02,
    BK_BLE_AD_TYPE_16SRV_CMPL         = 0x03,
    BK_BLE_AD_TYPE_32SRV_PART         = 0x04,
    BK_BLE_AD_TYPE_32SRV_CMPL         = 0x05,
    BK_BLE_AD_TYPE_128SRV_PART        = 0x06,
    BK_BLE_AD_TYPE_128SRV_CMPL        = 0x07,
    BK_BLE_AD_TYPE_NAME_SHORT         = 0x08,
    BK_BLE_AD_TYPE_NAME_CMPL          = 0x09,
    BK_BLE_AD_TYPE_TX_PWR             = 0x0A,
    BK_BLE_AD_TYPE_DEV_CLASS          = 0x0D,
    BK_BLE_AD_TYPE_SM_TK              = 0x10,
    BK_BLE_AD_TYPE_SM_OOB_FLAG        = 0x11,
    BK_BLE_AD_TYPE_INT_RANGE          = 0x12,
    BK_BLE_AD_TYPE_SOL_SRV_UUID       = 0x14,
    BK_BLE_AD_TYPE_128SOL_SRV_UUID    = 0x15,
    BK_BLE_AD_TYPE_SERVICE_DATA       = 0x16,
    BK_BLE_AD_TYPE_PUBLIC_TARGET      = 0x17,
    BK_BLE_AD_TYPE_RANDOM_TARGET      = 0x18,
    BK_BLE_AD_TYPE_APPEARANCE         = 0x19,
    BK_BLE_AD_TYPE_ADV_INT            = 0x1A,
    BK_BLE_AD_TYPE_LE_DEV_ADDR        = 0x1B,
    BK_BLE_AD_TYPE_LE_ROLE            = 0x1C,
    BK_BLE_AD_TYPE_SPAIR_C256         = 0x1D,
    BK_BLE_AD_TYPE_SPAIR_R256         = 0x1E,
    BK_BLE_AD_TYPE_32SOL_SRV_UUID     = 0x1F,
    BK_BLE_AD_TYPE_32SERVICE_DATA     = 0x20,
    BK_BLE_AD_TYPE_128SERVICE_DATA    = 0x21,
    BK_BLE_AD_TYPE_LE_SECURE_CONFIRM  = 0x22,
    BK_BLE_AD_TYPE_LE_SECURE_RANDOM   = 0x23,
    BK_BLE_AD_TYPE_URI                = 0x24,
    BK_BLE_AD_TYPE_INDOOR_POSITION    = 0x25,
    BK_BLE_AD_TYPE_TRANS_DISC_DATA    = 0x26,
    BK_BLE_AD_TYPE_LE_SUPPORT_FEATURE = 0x27,
    BK_BLE_AD_TYPE_CHAN_MAP_UPDATE    = 0x28,

    BK_BLE_AD_TYPE_MANU               = 0xFF,
} bk_ble_adv_data_type;

/// Advertising mode
typedef enum
{
    /// adv that can be scan and connect
    ADV_TYPE_IND                = 0x00,
    /// adv that can be connect by special device(white list). This send adv more frequently.
    ADV_TYPE_DIRECT_IND_HIGH    = 0x01,
    /// adv that can be scan
    ADV_TYPE_SCAN_IND           = 0x02,
    /// adv that can't be scan and connect
    ADV_TYPE_NONCONN_IND        = 0x03,
    /// adv that can be connect by special device(white list). This send adv more few.
    ADV_TYPE_DIRECT_IND_LOW     = 0x04,
} bk_ble_adv_type_t;

/// Advertising channel mask
typedef enum
{
    /// channel 37
    BK_ADV_CHNL_37     = 0x01,
    /// channel 38
    BK_ADV_CHNL_38     = 0x02,
    /// channel 39
    BK_ADV_CHNL_39     = 0x04,
    /// channel all
    BK_ADV_CHNL_ALL    = 0x07,
} bk_ble_adv_channel_t;

/// Advertising filter
typedef enum
{
    ///Allow both scan and connection requests from anyone
    ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY  = 0x00,
    ///Allow both scan req from White List devices only and connection req from anyone
    ADV_FILTER_ALLOW_SCAN_WLST_CON_ANY,
    ///Allow both scan req from anyone and connection req from White List devices only
    ADV_FILTER_ALLOW_SCAN_ANY_CON_WLST,
    ///Allow scan and connection requests from White List devices only
    ADV_FILTER_ALLOW_SCAN_WLST_CON_WLST,
    ///Enumeration end value for advertising filter policy value check
} bk_ble_adv_filter_t;



/// ble encrypt method
typedef enum
{
    BK_BLE_SEC_ENCRYPT = 1,            /*!< If the device has already
                                           bonded, the stack will used Long Term Key (LTK) to encrypt with the remote device directly.
                                           Else if the device hasn't bonded, the stack will used the default authentication request
                                           used the bk_ble_gap_set_security_param function set by the user. */
    BK_BLE_SEC_ENCRYPT_NO_MITM,        /*!< If the device has been already
                                           bonded, the stack will check the LTK (Long Term Key) Whether the authentication request has been met, and if met, use the LTK
                                           to encrypt with the remote device directly, else re-pair with the remote device.
                                           Else if the device hasn't been bonded, the stack will use NO MITM authentication request in the current link instead of
                                           using the authreq in the bk_ble_gap_set_security_param function set by the user. */
    BK_BLE_SEC_ENCRYPT_MITM,           /*!< If the device has been already
                                           bonded, the stack will check the LTK (Long Term Key) whether the authentication request has been met, and if met, use the LTK
                                           to encrypt with the remote device directly, else re-pair with the remote device.
                                           Else if the device hasn't been bonded, the stack will use MITM authentication request in the current link instead of
                                           using the authreq in the bk_ble_gap_set_security_param function set by the user. */
} bk_ble_sec_act_t;

/// ble sm option
typedef enum
{
    /// Authentication requirements of local device, see bk_ble_auth_req_t
    BK_BLE_SM_AUTHEN_REQ_MODE,
    /// The IO capability of local device, see bk_io_cap_enum_t
    BK_BLE_SM_IOCAP_MODE,
    /// Initiator Key Distribution/Generation, use BK_BLE_SM_SET_LOCAL_REMOTE_KEY instead !!!
    BK_BLE_SM_SET_INIT_KEY,
    /// Responder Key Distribution/Generation, use BK_BLE_SM_SET_LOCAL_REMOTE_KEY instead !!!
    BK_BLE_SM_SET_RSP_KEY,
    /// Maximum Encryption key size to support
    BK_BLE_SM_MAX_KEY_SIZE,
    /// Minimum Encryption key size requirement from Peer
    BK_BLE_SM_MIN_KEY_SIZE,
    /// Set static Passkey
    BK_BLE_SM_SET_STATIC_PASSKEY,
    /// Reset static Passkey
    BK_BLE_SM_CLEAR_STATIC_PASSKEY,
    /// Accept only specified SMP Authentication requirement
    BK_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH,
    /// Enable/Disable OOB support
    BK_BLE_SM_OOB_SUPPORT,
    /// set ER, 16 bytes
    BK_BLE_SM_SET_ER,
    /// set IR, 16 bytes
    BK_BLE_SM_SET_IR,
    /// local and remote init key, first 4 bits means local, last 4 bits means remote
    BK_BLE_SM_SET_LOCAL_REMOTE_KEY,
} bk_ble_sm_param_t;

/// the key distribution mask type
typedef enum
{
    BK_BLE_KEY_DISTR_ENC_KEY_MASK    = (1 << 0),         /// Used to exchange the encryption key in the init key & response key, not used now.
    BK_BLE_KEY_DISTR_ID_KEY_MASK     = (1 << 1),         /// Used to exchange the IRK key in the init key & response key, not used now.
    BK_BLE_KEY_DISTR_CSR_KEY_MASK    = (1 << 2),         /// Used to exchange the CSRK key in the init key & response key, not used now.
    BK_BLE_KEY_DISTR_LINK_KEY_MASK   = (1 << 3),         /*!< Used to exchange the link key(this key just used in the BLE & BR/EDR coexist mode) in the init key & response key,
                                                              must used with BK_LE_AUTH_REQ_SC_MITM_BOND.
                                                            */
} bk_ble_key_distr_mask_t;

#if (BLE_42_FEATURE_SUPPORT == TRUE)
/// Advertising parameters
typedef struct
{
    uint16_t                adv_int_min;        /*!< Minimum advertising interval for
                                                  undirected and low duty cycle directed advertising.
                                                  Range: 0x0020 to 0x4000 Default: N = 0x0800 (1.28 second)
                                                  Time = N * 0.625 msec Time Range: 20 ms to 10.24 sec */
    uint16_t                adv_int_max;        /*!< Maximum advertising interval for
                                                  undirected and low duty cycle directed advertising.
                                                  Range: 0x0020 to 0x4000 Default: N = 0x0800 (1.28 second)
                                                  Time = N * 0.625 msec Time Range: 20 ms to 10.24 sec Advertising max interval */
    bk_ble_adv_type_t      adv_type;           /*!< Advertising type */
    bk_ble_addr_type_t     own_addr_type;      /*!< Owner bluetooth device address type */
    bk_bd_addr_t           peer_addr;          /*!< Peer device bluetooth device address */
    bk_ble_addr_type_t     peer_addr_type;     /*!< Peer device bluetooth device address type, only support public address type and random address type */
    bk_ble_adv_channel_t   channel_map;        /*!< Advertising channel map */
    bk_ble_adv_filter_t    adv_filter_policy;  /*!< Advertising filter policy */
} bk_ble_adv_params_t;

/// Advertising data content, according to "Supplement to the Bluetooth Core Specification"
typedef struct
{
    bool                    set_scan_rsp;           /*!< Set this advertising data as scan response or not*/
    bool                    include_name;           /*!< Advertising data include device name or not */
    bool                    include_txpower;        /*!< Advertising data include TX power */
    int                     min_interval;           /*!< Advertising data show slave preferred connection min interval.
                                                    The connection interval in the following manner:
                                                    connIntervalmin = Conn_Interval_Min * 1.25 ms
                                                    Conn_Interval_Min range: 0x0006 to 0x0C80
                                                    Value of 0xFFFF indicates no specific minimum.
                                                    Values not defined above are reserved for future use.*/

    int                     max_interval;           /*!< Advertising data show slave preferred connection max interval.
                                                    The connection interval in the following manner:
                                                    connIntervalmax = Conn_Interval_Max * 1.25 ms
                                                    Conn_Interval_Max range: 0x0006 to 0x0C80
                                                    Conn_Interval_Max shall be equal to or greater than the Conn_Interval_Min.
                                                    Value of 0xFFFF indicates no specific maximum.
                                                    Values not defined above are reserved for future use.*/

    int                     appearance;             /*!< External appearance of device */
    uint16_t                manufacturer_len;       /*!< Manufacturer data length */
    uint8_t                 *p_manufacturer_data;   /*!< Manufacturer data point */
    uint16_t                service_data_len;       /*!< Service data length */
    uint8_t                 *p_service_data;        /*!< Service data point */
    uint16_t                service_uuid_len;       /*!< Service uuid length */
    uint8_t                 *p_service_uuid;        /*!< Service uuid array point */
    uint8_t                 flag;                   /*!< Advertising flag of discovery mode, see BLE_ADV_DATA_FLAG detail */
} bk_ble_adv_data_t;

#endif // #if (BLE_42_FEATURE_SUPPORT == TRUE)

/// Ble scan type
typedef enum
{
    BLE_SCAN_TYPE_PASSIVE   =   0x0,            /*!< Passive scan */
    BLE_SCAN_TYPE_ACTIVE    =   0x1,            /*!< Active scan */
} bk_ble_scan_type_t;

/// Ble scan filter type
typedef enum
{
    BLE_SCAN_FILTER_ALLOW_ALL           = 0x0,  /*!< Accept all :
                                                  1. advertisement packets except directed advertising packets not addressed to this device (default). */
    BLE_SCAN_FILTER_ALLOW_ONLY_WLST     = 0x1,  /*!< Accept only :
                                                  1. advertisement packets from devices where the advertiser’s address is in the White list.
                                                  2. Directed advertising packets which are not addressed for this device shall be ignored. */
    BLE_SCAN_FILTER_ALLOW_UND_RPA_DIR   = 0x2,  /*!< Accept all :
                                                  1. undirected advertisement packets, and
                                                  2. directed advertising packets where the initiator address is a resolvable private address, and
                                                  3. directed advertising packets addressed to this device. */
    BLE_SCAN_FILTER_ALLOW_WLIST_RPA_DIR = 0x3,  /*!< Accept all :
                                                  1. advertisement packets from devices where the advertiser’s address is in the White list, and
                                                  2. directed advertising packets where the initiator address is a resolvable private address, and
                                                  3. directed advertising packets addressed to this device.*/
} bk_ble_scan_filter_t;

/// Ble scan duplicate type
typedef enum
{
    BLE_SCAN_DUPLICATE_DISABLE           = 0x0,  /*!< the Link Layer should generate advertising reports to the host for each packet received */
    BLE_SCAN_DUPLICATE_ENABLE            = 0x1,  /*!< the Link Layer should filter out duplicate advertising reports to the Host */
    BLE_SCAN_DUPLICATE_MAX               = 0x2,  /*!< 0x02 – 0xFF, Reserved for future use */
} bk_ble_scan_duplicate_t;
#if (BLE_42_FEATURE_SUPPORT == TRUE)
/// Ble scan parameters
typedef struct
{
    bk_ble_scan_type_t     scan_type;              /*!< Scan type */
    bk_ble_addr_type_t     own_addr_type;          /*!< Owner address type */
    bk_ble_scan_filter_t   scan_filter_policy;     /*!< Scan filter policy */
    uint16_t                scan_interval;          /*!< Scan interval. This is defined as the time interval from
                                                      when the Controller started its last LE scan until it begins the subsequent LE scan.
                                                      Range: 0x0004 to 0x4000 Default: 0x0010 (10 ms)
                                                      Time = N * 0.625 msec
                                                      Time Range: 2.5 msec to 10.24 seconds*/
    uint16_t                scan_window;            /*!< Scan window. The duration of the LE scan. LE_Scan_Window
                                                      shall be less than or equal to LE_Scan_Interval
                                                      Range: 0x0004 to 0x4000 Default: 0x0010 (10 ms)
                                                      Time = N * 0.625 msec
                                                      Time Range: 2.5 msec to 10240 msec */
    bk_ble_scan_duplicate_t  scan_duplicate;       /*!< The Scan_Duplicates parameter controls whether the Link Layer should filter out
                                                        duplicate advertising reports (BLE_SCAN_DUPLICATE_ENABLE) to the Host, or if the Link Layer should generate
                                                        advertising reports for each packet received */
} bk_ble_scan_params_t;
#endif // #if (BLE_42_FEATURE_SUPPORT == TRUE)



/// create connection param
typedef struct
{
    uint16_t scan_interval;                 /*!< scan interval, unit in 0.625ms */
    uint16_t scan_window;                   /*!< scan window, unit in 0.625ms */
    uint8_t initiator_filter_policy;        /*!< policy that how to use white list. 0 means not used, 1 means only connect device that in white list */
    bk_ble_addr_type_t local_addr_type;     /*!< local address type */
    bk_bd_addr_t peer_addr;                 /*!< peer addr */
    bk_ble_addr_type_t peer_addr_type;      /*!< Peer address type, only BLE_ADDR_TYPE_PUBLIC or BLE_ADDR_TYPE_RANDOM available */
    uint16_t conn_interval_min;             /*!< conn min interval, unit in 1.25ms, Range: 7.5ms to 4s */
    uint16_t conn_interval_max;             /*!< conn max interval, unit in 1.25ms, Range: 7.5ms to 4s */
    uint16_t conn_latency;                  /*!< latency Range: 0x0000 to 0x01F3 */
    uint16_t supervision_timeout;           /*!< conn timeout, unit in 10ms Range: 100ms to 32s */
    uint16_t min_ce;                        /*!< minimum length of connection event recommended, unit in 0.625ms Range: 0x0000 to 0xFFFF */
    uint16_t max_ce;                        /*!< maximum length of connection event recommended, unit in 0.625ms Range: 0x0000 to 0xFFFF */
} bk_gap_create_conn_params_t;

/// connection parameters information
typedef struct
{
    uint16_t interval;                  /*!< connection interval */
    uint16_t latency;                   /*!< Slave latency for the connection in number of connection events. Range: 0x0000 to 0x01F3 */
    uint16_t timeout;                   /*!< Supervision timeout for the LE Link. Range: 0x000A to 0x0C80.
                                                      Mandatory Range: 0x000A to 0x0C80 Time = N * 10 msec
                                                      Time Range: 100 msec to 32 seconds */
} bk_gap_conn_params_t;

/// Connection update parameters
typedef struct
{
    bk_bd_addr_t bda;                              /*!< Bluetooth device address */
    uint16_t min_int;                               /*!< Min connection interval */
    uint16_t max_int;                               /*!< Max connection interval */
    uint16_t latency;                               /*!< Slave latency for the connection in number of connection events. Range: 0x0000 to 0x01F3 */
    uint16_t timeout;                               /*!< Supervision timeout for the LE Link. Range: 0x000A to 0x0C80.
                                                      Mandatory Range: 0x000A to 0x0C80 Time = N * 10 msec
                                                      Time Range: 100 msec to 32 seconds */
} bk_ble_conn_update_params_t;

/**
* @brief BLE pkt date length keys
*/
typedef struct
{
    uint16_t rx_len;                   /*!< pkt rx data length value */
    uint16_t tx_len;                   /*!< pkt tx data length value */
} bk_ble_pkt_data_length_params_t;

typedef enum
{
    BK_BLE_SECURITY_LEVEL_1 = 1,
    BK_BLE_SECURITY_LEVEL_2,
    BK_BLE_SECURITY_LEVEL_3,
    BK_BLE_SECURITY_LEVEL_4,
}bk_ble_security_level_t;

/**
* @brief BLE encryption keys
*/
typedef struct
{
    bk_bt_octet16_t     ltk;          /*!< The long term key*/
    bk_bt_octet8_t      rand;         /*!< The random number*/
    uint16_t             ediv;         /*!< The ediv value*/
    uint8_t              sec_level;    /*!< The security level of the security link, see bk_ble_security_level_t*/
    uint8_t              key_size;     /*!< The key size(7~16) of the security link*/
} bk_ble_penc_keys_t;                 /*!< The key type*/

/**
* @brief  BLE CSRK keys
*/
typedef struct
{
    uint32_t            counter;      /*!< The counter */
    bk_bt_octet16_t    csrk;         /*!< The csrk key */
    uint8_t             sec_level;    /*!< The security level see bk_ble_security_level_t */
} bk_ble_pcsrk_keys_t;               /*!< The pcsrk key type */

/**
* @brief  BLE pid keys
*/
typedef struct
{
    bk_bt_octet16_t          irk;           /*!< The irk value */
    bk_ble_addr_type_t       addr_type;     /*!< The address type */
    bk_bd_addr_t             static_addr;   /*!< The static address */
} bk_ble_pid_keys_t;                        /*!< The pid key type */

/**
* @brief  BLE Encryption reproduction keys
*/
typedef struct
{
    bk_bt_octet16_t  ltk;                  /*!< The long term key */
    bk_bt_octet8_t      rand;         /*!< The random number*/
    uint16_t          div;                  /*!< The div value */
    uint8_t           key_size;             /*!< The key size of the security link */
    uint8_t           sec_level;            /*!< The security level of the security link see bk_ble_security_level_t*/
} bk_ble_lenc_keys_t;                      /*!< The  key type */

/**
* @brief  BLE SRK keys
*/
typedef struct
{
    uint32_t          counter;              /*!< The counter value */
    uint16_t          div;                  /*!< The div value */// not used
    uint8_t           sec_level;            /*!< The security level of the security link see bk_ble_security_level_t*/
    bk_bt_octet16_t  csrk;                 /*!< The csrk key value */
} bk_ble_lcsrk_keys_t;                       /*!< The csrk key type */

typedef struct
{
    uint8_t key[16];                   /// linkey, for br edr
} bk_ble_link_keys_t;

/**
* @brief  Structure associated with BK_KEY_NOTIF_EVT
*/
typedef struct
{
    bk_bd_addr_t  bd_addr;        /*!< peer address */
    uint32_t       passkey;        /*!< the passkey or numeric value for comparison. If just_works, do not show this number to UI */
} bk_ble_sec_key_notif_t;         /*!< BLE key notify type*/

/**
* @brief  Structure of the security request
*/
typedef struct
{
    bk_bd_addr_t  bd_addr;        /*!< peer address */
    bk_ble_addr_type_t addr_type;
} bk_ble_sec_req_t;               /*!< BLE security request type*/

/**
* @brief  union type of the security key value
*/
typedef union
{
    bk_ble_penc_keys_t   penc_key;       /*!< received peer encryption key */
    bk_ble_pcsrk_keys_t  pcsrk_key;      /*!< received peer device SRK */
    bk_ble_pid_keys_t    pid_key;        /*!< peer device ID key */

    bk_ble_lenc_keys_t   lenc_key;       /*!< local encryption reproduction keys LTK = = d1(ER,DIV,0)*/
    bk_ble_lcsrk_keys_t  lcsrk_key;      /*!< local device CSRK = d1(ER,DIV,1)*/
} bk_ble_key_value_t;                    /*!< ble key value type*/

/**
* @brief  struct type of the bond key information value
*/
typedef struct
{
    bk_ble_key_mask_t    key_mask;       /*!< the key mask to indicate witch key is present */

    bk_ble_penc_keys_t   penc_key;       /*!< received peer encryption key */
    bk_ble_pid_keys_t    pid_key;        /*!< peer device ID key */
    bk_ble_pcsrk_keys_t  pcsrk_key;      /*!< received peer device SRK */

    bk_ble_lenc_keys_t   lenc_key;       /*!< local encryption reproduction keys LTK = = d1(ER,DIV,0)*/
    bk_ble_pid_keys_t    lid_key;        /*!< local device ID key */
    bk_ble_lcsrk_keys_t  lcsrk_key;      /*!< local device CSRK = d1(ER,DIV,1)*/
    bk_ble_link_keys_t   llink_key;      /// local linkkey, for br edr
} bk_ble_bond_key_info_t;                /*!< ble bond key information value type */

/**
* @brief  struct type of the bond device value
*/
typedef struct
{
    bk_bd_addr_t  bd_addr;               /*!< peer nominal address */
    bk_ble_addr_type_t addr_type;        /// peer address type
    bk_ble_bond_key_info_t bond_key;     /*!< the bond key information */
} bk_ble_bond_dev_t;                     /*!< the ble bond device type */

/**
* @brief  structure type of the ble local keys value
*/
typedef struct
{
    bk_bt_octet16_t       ir;                  /*!< the 16 bits of the ir value */
    bk_bt_octet16_t       er;                 /*!< the 16 bits of the er value */
    bk_bt_octet16_t       dhk;                 /*!< the 16 bits of the dh key value */
} bk_ble_local_keys_t;


/**
  * @brief Structure associated with BK_AUTH_CMPL_EVT
  */
typedef struct
{
    bk_bd_addr_t         bd_addr;               /*!< BD address peer device. */
//    bool                  key_present;           /*!< Valid link key value in key element */            //internal type in bluedroid and not used in api !!!
//    bk_link_key_t          key;                   /*!< Link key associated with peer device. */         //internal type in bluedroid and not used in api !!!
//    uint8_t               key_type;              /*!< The type of Link Key */                           //internal type in bluedroid and not used in api !!!
    bool                  success;               /*!< TRUE of authentication succeeded, FALSE if failed. */
    uint8_t               fail_reason;           /*!< The HCI reason/error code for when success=FALSE */
    bk_ble_addr_type_t   addr_type;             /*!< Peer device address type */
    bk_bt_dev_type_t     dev_type;              /*!< Device type */
    bk_ble_auth_req_t    auth_mode;             /*!< authentication mode */
} bk_ble_auth_cmpl_t;                           /*!< The ble authentication complete cb type */

/**
  * @brief union associated with ble security
  */
typedef union
{
    bk_ble_sec_key_notif_t    key_notif;      /*!< passkey notification/req or number compare notification */
    bk_ble_sec_req_t          ble_req;        /*!< BLE SMP related request*/
    //bk_ble_sec_key_t          ble_key;        /*!< BLE SMP keys used when pairing */
    bk_ble_local_keys_t    ble_local_keys;    /*!< BLE local event */
    bk_ble_auth_cmpl_t        auth_cmpl;      /*!< Authentication complete indication. */
} bk_ble_sec_t;                               /*!< BLE security type */


typedef struct
{
    bk_bd_addr_t peer_addr;
    bk_ble_key_mask_t key_type;
} bk_ble_bond_key_req_t;

/// bond dev list operation
typedef enum
{
    BK_GAP_BOND_DEV_LIST_OPERATION_ADD,
    BK_GAP_BOND_DEV_LIST_OPERATION_REMOVE,
    BK_GAP_BOND_DEV_LIST_OPERATION_CLEAN,
}bk_gap_bond_dev_list_operation_t;

#if (BLE_42_FEATURE_SUPPORT == TRUE)
/// Sub Event of BK_BLE_GAP_SCAN_RESULT_EVT
typedef enum
{
    BK_GAP_SEARCH_INQ_RES_EVT             = 0,      /*!< Inquiry result for a peer device. */
    BK_GAP_SEARCH_INQ_CMPL_EVT            = 1,      /*!< Inquiry complete. */
    BK_GAP_SEARCH_DISC_RES_EVT            = 2,      /*!< Discovery result for a peer device. */
    BK_GAP_SEARCH_DISC_BLE_RES_EVT        = 3,      /*!< Discovery result for BLE GATT based service on a peer device. */
    BK_GAP_SEARCH_DISC_CMPL_EVT           = 4,      /*!< Discovery complete. */
    BK_GAP_SEARCH_DI_DISC_CMPL_EVT        = 5,      /*!< Discovery complete. */
    BK_GAP_SEARCH_SEARCH_CANCEL_CMPL_EVT  = 6,      /*!< Search cancelled */
    BK_GAP_SEARCH_INQ_DISCARD_NUM_EVT     = 7,      /*!< The number of pkt discarded by flow control */
} bk_gap_search_evt_t;
#endif // #if (BLE_42_FEATURE_SUPPORT == TRUE)
/**
 * @brief Ble scan result event type, to indicate the
 *        result is scan response or advertising data or other
 */
typedef enum
{
    BK_BLE_EVT_CONN_ADV         = 0x00,        /*!< Connectable undirected advertising (ADV_IND) */
    BK_BLE_EVT_CONN_DIR_ADV     = 0x01,        /*!< Connectable directed advertising (ADV_DIRECT_IND) */
    BK_BLE_EVT_DISC_ADV         = 0x02,        /*!< Scannable undirected advertising (ADV_SCAN_IND) */
    BK_BLE_EVT_NON_CONN_ADV     = 0x03,        /*!< Non connectable undirected advertising (ADV_NONCONN_IND) */
    BK_BLE_EVT_SCAN_RSP         = 0x04,        /*!< Scan Response (SCAN_RSP) */
} bk_ble_evt_type_t;

/// white list op
typedef enum
{
    BK_BLE_WHITELIST_REMOVE     = 0X00,    /*!< remove mac from whitelist */
    BK_BLE_WHITELIST_ADD        = 0X01,    /*!< add address to whitelist */
    BK_BLE_WHITELIST_CLEAN,                /*!< clean whitelist */
} bk_ble_wl_operation_t;

#if (BLE_50_FEATURE_SUPPORT == TRUE)
/// ext adv prop
typedef enum
{
    BK_BLE_GAP_SET_EXT_ADV_PROP_NONCONN_NONSCANNABLE_UNDIRECTED     = (0 << 0), /*!< Non-Connectable and Non-Scannable Undirected advertising */
    BK_BLE_GAP_SET_EXT_ADV_PROP_CONNECTABLE                         = (1 << 0), /*!< Connectable advertising */
    BK_BLE_GAP_SET_EXT_ADV_PROP_SCANNABLE                           = (1 << 1), /*!< Scannable advertising */
    BK_BLE_GAP_SET_EXT_ADV_PROP_DIRECTED                            = (1 << 2), /*!< Directed advertising */
    BK_BLE_GAP_SET_EXT_ADV_PROP_HD_DIRECTED                         = (1 << 3), /*!< High Duty Cycle Directed Connectable advertising (<= 3.75 ms Advertising Interval) */
    BK_BLE_GAP_SET_EXT_ADV_PROP_LEGACY                              = (1 << 4), /*!< Use legacy advertising PDUs */
    BK_BLE_GAP_SET_EXT_ADV_PROP_ANON_ADV                            = (1 << 5), /*!< Omit advertiser's address from all PDUs ("anonymous advertising") */
    BK_BLE_GAP_SET_EXT_ADV_PROP_INCLUDE_TX_PWR                      = (1 << 6), /*!< Include TxPower in the extended header of the advertising PDU */
    BK_BLE_GAP_SET_EXT_ADV_PROP_MASK                                = (0x7F),   /*!< Reserved for future use */
} bk_ble_gap_set_ext_adv_prop_t;
/*!<  If extended advertising PDU types are being used (bit 4 = 0) then:
    The advertisement shall not be both connectable and scannable.
    High duty cycle directed connectable advertising (<= 3.75 ms advertising interval) shall not be used (bit 3 = 0)
*/
/*!< ADV_IND */
#define BK_BLE_GAP_SET_EXT_ADV_PROP_LEGACY_IND        (BK_BLE_GAP_SET_EXT_ADV_PROP_LEGACY |\
                                                       BK_BLE_GAP_SET_EXT_ADV_PROP_CONNECTABLE |\
                                                       BK_BLE_GAP_SET_EXT_ADV_PROP_SCANNABLE)
/*!< ADV_DIRECT_IND (low duty cycle) */
#define BK_BLE_GAP_SET_EXT_ADV_PROP_LEGACY_LD_DIR     (BK_BLE_GAP_SET_EXT_ADV_PROP_LEGACY |\
                                                       BK_BLE_GAP_SET_EXT_ADV_PROP_CONNECTABLE |\
                                                       BK_BLE_GAP_SET_EXT_ADV_PROP_DIRECTED)
/*!< ADV_DIRECT_IND (high duty cycle) */
#define BK_BLE_GAP_SET_EXT_ADV_PROP_LEGACY_HD_DIR     (BK_BLE_GAP_SET_EXT_ADV_PROP_LEGACY |\
                                                       BK_BLE_GAP_SET_EXT_ADV_PROP_CONNECTABLE |\
                                                       BK_BLE_GAP_SET_EXT_ADV_PROP_HD_DIRECTED)
/*!< ADV_SCAN_IND */
#define BK_BLE_GAP_SET_EXT_ADV_PROP_LEGACY_SCAN       (BK_BLE_GAP_SET_EXT_ADV_PROP_LEGACY |\
                                                       BK_BLE_GAP_SET_EXT_ADV_PROP_SCANNABLE)
/*!< ADV_NONCONN_IND */
#define BK_BLE_GAP_SET_EXT_ADV_PROP_LEGACY_NONCONN    (BK_BLE_GAP_SET_EXT_ADV_PROP_LEGACY)

/// such as BK_BLE_GAP_SET_EXT_ADV_PROP_CONNECTABLE and BK_BLE_GAP_SET_EXT_ADV_PROP_LEGACY_IND
typedef uint16_t bk_ble_ext_adv_type_mask_t;

typedef enum
{
    BK_BLE_GAP_PHY_1M = 1,                              /*!< Secondery Advertisement PHY is LE1M */
    BK_BLE_GAP_PHY_2M = 2,                              /*!< Secondery Advertisement PHY is LE2M */
    BK_BLE_GAP_PHY_CODED = 3,                           /*!< Secondery Advertisement PHY is LE Coded */
} bk_ble_gap_phy_enum_t;

/// see bk_ble_gap_phy_enum_t
typedef uint8_t bk_ble_gap_phy_t;

///ble gap noprefer phy
typedef enum
{
    BK_BLE_GAP_NO_PREFER_TRANSMIT_PHY = (1 << 0),     /*!< No Prefer TX PHY supported by controller */
    BK_BLE_GAP_NO_PREFER_RECEIVE_PHY = (1 << 1),      /*!< No Prefer RX PHY supported by controller */
} bk_ble_gap_noprefer_enum_t;

typedef uint8_t bk_ble_gap_all_phys_t;

/// Primary phy only support 1M and LE coded phy
typedef enum
{
    BK_BLE_GAP_PRI_PHY_1M = BK_BLE_GAP_PHY_1M,     /*!< Primary Phy is LE1M */
    BK_BLE_GAP_PRI_PHY_CODED = BK_BLE_GAP_PHY_CODED,  /*!< Primary Phy is LE CODED */
} bk_ble_gap_pri_phy_enum_t;

///see bk_ble_gap_pri_phy_enum_t
typedef uint8_t bk_ble_gap_pri_phy_t;

typedef enum
{
    BK_BLE_GAP_PHY_1M_PREF_MASK    = (1 << 0), /*!< The Host prefers use the LE1M transmitter or reciever PHY */
    BK_BLE_GAP_PHY_2M_PREF_MASK    = (1 << 1), /*!< The Host prefers use the LE2M transmitter or reciever PHY */
    BK_BLE_GAP_PHY_CODED_PREF_MASK = (1 << 2), /*!< The Host prefers use the LE CODED transmitter or reciever PHY */
} bk_ble_gap_phy_pref_enum_t;

/// see bk_ble_gap_phy_pref_enum_t
typedef uint8_t bk_ble_gap_phy_mask_t;

/// ble gap coded phy pref param
typedef enum
{
    BK_BLE_GAP_PHY_OPTIONS_NO_PREF                 = 0, /*!< The Host has no preferred coding when transmitting on the LE Coded PHY */
    BK_BLE_GAP_PHY_OPTIONS_PREF_S2_CODING          = 1, /*!< The Host prefers that S=2 coding be used when transmitting on the LE Coded PHY */
    BK_BLE_GAP_PHY_OPTIONS_PREF_S8_CODING          = 2, /*!< The Host prefers that S=8 coding be used when transmitting on the LE Coded PHY */
} bk_ble_gap_coded_phy_pref_enum_t;

/// see bk_ble_gap_coded_phy_pref_enum_t
typedef uint16_t bk_ble_gap_prefer_phy_options_t;

/// ble gap ext scan cfg
typedef enum
{
    BK_BLE_GAP_EXT_SCAN_CFG_UNCODE_MASK          = 0x01, /*!< Scan Advertisements on the LE1M PHY */
    BK_BLE_GAP_EXT_SCAN_CFG_CODE_MASK            = 0x02, /*!< Scan advertisements on the LE coded PHY */
} bk_ble_gap_ext_scan_cfg_enum_t;

/// see bk_ble_gap_ext_scan_cfg_enum_t
typedef uint8_t bk_ble_ext_scan_cfg_mask_t;

/// Advertising data seg.
typedef enum
{
    BK_BLE_GAP_EXT_ADV_DATA_COMPLETE             = 0x00, /*!< extended advertising data compete */
    BK_BLE_GAP_EXT_ADV_DATA_INCOMPLETE           = 0x01, /*!< extended advertising data incomplete */
    BK_BLE_GAP_EXT_ADV_DATA_TRUNCATED            = 0x02, /*!< extended advertising data truncated mode */
} bk_ble_gap_ext_adv_data_seg_enum_t;

/// see bk_ble_gap_ext_adv_data_seg_enum_t
typedef uint8_t bk_ble_gap_ext_adv_data_status_t;

/// Advertising SYNC policy
typedef enum
{
    BK_BLE_GAP_SYNC_POLICY_BY_ADV_INFO      = 0, /*!< sync policy by advertising info */
    BK_BLE_GAP_SYNC_POLICY_BY_PERIODIC_LIST = 1, /*!< periodic advertising sync policy */
} bk_ble_gap_sync_policy_enum_t;

/// see bk_ble_gap_sync_policy_enum_t
typedef uint8_t bk_ble_gap_sync_t;

/// Advertising report
typedef enum
{
    BK_BLE_ADV_REPORT_EXT_ADV_IND                   = (1 << 0), /*!< advertising report with extended advertising indication type */
    BK_BLE_ADV_REPORT_EXT_SCAN_IND                  = (1 << 1), /*!< advertising report with extended scan indication type */
    BK_BLE_ADV_REPORT_EXT_DIRECT_ADV                = (1 << 2), /*!< advertising report with extended direct advertising indication type */
    BK_BLE_ADV_REPORT_EXT_SCAN_RSP                  = (1 << 3), /*!< advertising report with extended scan response indication type */

    BK_BLE_LEGACY_ADV_TYPE_IND                      = (0x13), /*!< advertising report with legacy advertising indication type */
    BK_BLE_LEGACY_ADV_TYPE_DIRECT_IND               = (0x15), /*!< advertising report with legacy direct indication type */
    BK_BLE_LEGACY_ADV_TYPE_SCAN_IND                 = (0x12), /*!< advertising report with legacy scan indication type */
    BK_BLE_LEGACY_ADV_TYPE_NONCON_IND               = (0x10), /*!< advertising report with legacy non connectable indication type */
    BK_BLE_LEGACY_ADV_TYPE_SCAN_RSP_TO_ADV_IND      = (0x1b), /*!< advertising report with legacy scan response indication type */
    BK_BLE_LEGACY_ADV_TYPE_SCAN_RSP_TO_ADV_SCAN_IND = (0x1a), /*!< advertising report with legacy advertising with scan response indication type */
} bk_ble_adv_report_ext_type_enum_t;

/// see bk_ble_adv_report_ext_type_enum_t
typedef uint8_t bk_ble_gap_adv_type_t;

/// Extend advertising tx power, range: [-127, +126] dBm
#define EXT_ADV_TX_PWR_NO_PREFERENCE                      (127) /*!< host has no preference for tx power */

/**
* @brief ext adv parameters
*/
typedef struct
{
    bk_ble_ext_adv_type_mask_t type;   /*!< ext adv type */
    uint32_t interval_min;              /*!< ext adv minimum interval */
    uint32_t interval_max;              /*!< ext adv maximum interval */
    bk_ble_adv_channel_t channel_map;  /*!< ext adv channel map */
    bk_ble_addr_type_t own_addr_type;  /*!< ext adv own address type */
    bk_ble_addr_type_t peer_addr_type; /*!< ext adv peer address type, only BLE_ADDR_TYPE_PUBLIC and BLE_ADDR_TYPE_RANDOM is valid */
    bk_bd_addr_t peer_addr;            /*!< ext adv peer address */
    bk_ble_adv_filter_t filter_policy; /*!< ext adv filter policy */
    int8_t tx_power;                    /*!< ext adv tx power */
    bk_ble_gap_pri_phy_t primary_phy;  /*!< ext adv primary phy */
    uint8_t max_skip;                   /*!< ext adv maximum skip */
    bk_ble_gap_phy_t secondary_phy;    /*!< ext adv secondary phy */
    uint8_t sid;                        /*!< ext adv sid */
    bool scan_req_notif;                /*!< ext adv scan request event notify */
} bk_ble_gap_ext_adv_params_t;

/**
* @brief ext scan config
*/
typedef struct
{
    bk_ble_scan_type_t scan_type; /*!< ext scan type */
    uint16_t scan_interval;        /*!< ext scan interval */
    uint16_t scan_window;          /*!< ext scan window */
} bk_ble_ext_scan_cfg_t;

/**
* @brief ext scan parameters
*/
typedef struct
{
    bk_ble_addr_type_t own_addr_type;        /*!< ext scan own address type */
    bk_ble_scan_filter_t filter_policy;      /*!< ext scan filter policy */
    bk_ble_scan_duplicate_t  scan_duplicate; /*!< ext scan duplicate scan */
    bk_ble_ext_scan_cfg_mask_t cfg_mask;     /*!< ext scan config mask */
    bk_ble_ext_scan_cfg_t uncoded_cfg;       /*!< ext scan uncoded config parameters */
    bk_ble_ext_scan_cfg_t coded_cfg;         /*!< ext scan coded config parameters */
} bk_ble_ext_scan_params_t;

/**
* @brief create extend connection parameters
*/
typedef struct
{
    uint16_t scan_interval;       /*!< init scan interval */
    uint16_t scan_window;         /*!< init scan window */
    uint16_t interval_min;        /*!< minimum interval */
    uint16_t interval_max;        /*!< maximum interval */
    uint16_t latency;             /*!< ext scan type */
    uint16_t supervision_timeout; /*!< connection supervision timeout */
    uint16_t min_ce_len;          /*!< minimum ce length */
    uint16_t max_ce_len;          /*!< maximum ce length */
} bk_ble_gap_conn_params_t;

/**
* @brief extend adv enable parameters
*/
typedef struct
{
    uint8_t instance;        /*!< advertising handle */
    int duration;            /*!< advertising duration */
    int max_events;          /*!< maximum number of extended advertising events */
} bk_ble_gap_ext_adv_t;

/**
* @brief periodic adv parameters
*/
typedef struct
{
    uint16_t interval_min;     /*!< periodic advertising minimum interval */
    uint16_t interval_max;     /*!< periodic advertising maximum interval */
    uint8_t  properties;       /*!< periodic advertising properties */
} bk_ble_gap_periodic_adv_params_t;

/**
* @brief periodic adv sync parameters
*/
typedef struct
{
    bk_ble_gap_sync_t filter_policy;   /*!< periodic advertising sync filter policy */
    uint8_t sid;                        /*!< periodic advertising sid */
    bk_ble_addr_type_t addr_type;      /*!< periodic advertising address type */
    bk_bd_addr_t addr;                 /*!< periodic advertising address */
    uint16_t skip;                      /*!< the maximum number of periodic advertising events that can be skipped */
    uint16_t sync_timeout;              /*!< synchronization timeout */
} bk_ble_gap_periodic_adv_sync_params_t;

/**
* @brief extend adv report parameters
*/
typedef struct
{
    // uint8_t props;
    // uint8_t legacy_event_type;
    bk_ble_gap_adv_type_t event_type;              /*!< extend advertising type */
    uint8_t addr_type;                              /*!< extend advertising address type */
    bk_bd_addr_t addr;                             /*!< extend advertising address */
    bk_ble_gap_pri_phy_t primary_phy;              /*!< extend advertising primary phy */
    bk_ble_gap_phy_t secondly_phy;                 /*!< extend advertising secondary phy */
    uint8_t sid;                                    /*!< extend advertising sid */
    uint8_t tx_power;                               /*!< extend advertising tx power */
    int8_t rssi;                                    /*!< extend advertising rssi */
    uint16_t per_adv_interval;                      /*!< periodic advertising interval */
    uint8_t dir_addr_type;                          /*!< direct address type */
    bk_bd_addr_t dir_addr;                         /*!< direct address */
    bk_ble_gap_ext_adv_data_status_t data_status;  /*!< data type */
    uint8_t adv_data_len;                           /*!< extend advertising data length */
    uint8_t adv_data[251];                          /*!< extend advertising data */
} bk_ble_gap_ext_adv_reprot_t;

/**
* @brief periodic adv report parameters
*/
typedef struct
{
    uint16_t sync_handle;                          /*!< periodic advertising train handle */
    uint8_t tx_power;                              /*!< periodic advertising tx power*/
    int8_t rssi;                                   /*!< periodic advertising rssi */
    bk_ble_gap_ext_adv_data_status_t data_status; /*!< periodic advertising data type*/
    uint8_t data_length;                           /*!< periodic advertising data length */
    uint8_t data[251];                             /*!< periodic advertising data */
} bk_ble_gap_periodic_adv_report_t;

/**
* @brief perodic adv sync establish parameters
*/
typedef struct
{
    uint8_t status;                               /*!< periodic advertising sync status */
    uint16_t sync_handle;                         /*!< periodic advertising train handle */
    uint8_t sid;                                  /*!< periodic advertising sid */
    bk_ble_addr_type_t addr_type;                /*!< periodic advertising address type */
    bk_bd_addr_t adv_addr;                       /*!< periodic advertising address */
    bk_ble_gap_phy_t adv_phy;                    /*!< periodic advertising adv phy type */
    uint16_t period_adv_interval;                 /*!< periodic advertising interval */
    uint8_t adv_clk_accuracy;                     /*!< periodic advertising clock accuracy */
} bk_ble_gap_periodic_adv_sync_estab_t;

#endif //#if (BLE_50_FEATURE_SUPPORT == TRUE)

/**
 * @brief Gap callback parameters union
 */
typedef union
{
    bk_ble_sec_t ble_security;                     /*!< ble gap security union type */
    /**
     * @brief BK_BLE_GAP_SET_STATIC_RAND_ADDR_EVT
     */
    struct ble_set_rand_cmpl_evt_param
    {
        bk_bt_status_t status;                     /*!< Indicate set static rand address operation success status */
    } set_rand_addr_cmpl;                           /*!< Event parameter of BK_BLE_GAP_SET_STATIC_RAND_ADDR_EVT */
    /**
     * @brief BK_BLE_GAP_UPDATE_CONN_PARAMS_EVT
     */
    struct ble_update_conn_params_evt_param
    {
        bk_bt_status_t status;                    /*!< Indicate update connection parameters success status */
        bk_bd_addr_t bda;                         /*!< Bluetooth device address */
        uint16_t min_int;                          /*!< Min connection interval */
        uint16_t max_int;                          /*!< Max connection interval */
        uint16_t latency;                          /*!< Slave latency for the connection in number of connection events. Range: 0x0000 to 0x01F3 */
        uint16_t conn_int;                         /*!< Current connection interval */
        uint16_t timeout;                          /*!< Supervision timeout for the LE Link. Range: 0x000A to 0x0C80.
                                                     Mandatory Range: 0x000A to 0x0C80 Time = N * 10 msec */
    } update_conn_params;                          /*!< Event parameter of BK_BLE_GAP_UPDATE_CONN_PARAMS_EVT */
    /**
     * @brief BK_BLE_GAP_SET_PKT_LENGTH_COMPLETE_EVT
     */
    struct ble_pkt_data_length_cmpl_evt_param
    {
        bk_bt_status_t status;                     /*!< Indicate the set pkt data length operation success status */
        bk_ble_pkt_data_length_params_t params;    /*!<  pkt data length value */
    } pkt_data_length_cmpl;                          /*!< Event parameter of BK_BLE_GAP_SET_PKT_LENGTH_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_SET_LOCAL_PRIVACY_COMPLETE_EVT
     */
    struct ble_local_privacy_cmpl_evt_param
    {
        bk_bt_status_t status;                     /*!< Indicate the set local privacy operation success status */
    } local_privacy_cmpl;                           /*!< Event parameter of BK_BLE_GAP_SET_LOCAL_PRIVACY_COMPLETE_EVT */


    /**
     * @brief BK_BLE_GAP_BOND_KEY_REQ_EVT
     */
    struct ble_bond_dev_key_req_evt_param
    {
        bk_ble_bond_key_req_t key_req;               /*!< bond device Structure */
    } bond_key_req_evt;                            /*!< Event parameter of BK_BLE_GAP_BOND_KEY_REQ_EVT */

    /**
     * @brief BK_BLE_GAP_BOND_KEY_GENERATE_EVT
     */
    struct ble_bond_dev_key_evt_param
    {
        bk_ble_bond_dev_t bond_dev;               /*!< bond device Structure */
    } bond_dev_key_generate_evt;                            /*!< Event parameter of BK_BLE_GAP_BOND_KEY_GENERATE_EVT */

    /**
     * @brief BK_BLE_GAP_BOND_DEV_LIST_OPERATEION_COMPLETE_EVT
     */
    struct ble_bond_dev_list_evt_param
    {
        bk_bt_status_t status;
        bk_gap_bond_dev_list_operation_t op;               /*!< operation id */
    } bond_dev_list_op_cmpl;                            /*!< Event parameter of BK_BLE_GAP_BOND_DEV_LIST_OPERATEION_COMPLETE_EVT */

    /**
     * @brief BK_BLE_GAP_READ_RSSI_COMPLETE_EVT
     */
    struct ble_read_rssi_cmpl_evt_param
    {
        bk_bt_status_t status;                     /*!< Indicate the read adv tx power operation success status */
        int8_t rssi;                                /*!< The ble remote device rssi value, the range is from -127 to 20, the unit is dbm,
                                                         if the RSSI cannot be read, the RSSI metric shall be set to 127. */
        bk_bd_addr_t remote_addr;                  /*!< The remote device address */
    } read_rssi_cmpl;                               /*!< Event parameter of BK_BLE_GAP_READ_RSSI_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_UPDATE_WHITELIST_COMPLETE_EVT
     */
    struct ble_update_whitelist_cmpl_evt_param
    {
        bk_bt_status_t status;                     /*!< Indicate the add or remove whitelist operation success status */
        bk_ble_wl_operation_t wl_operation;        /*!< The value is BK_BLE_WHITELIST_ADD if add address to whitelist operation success, BK_BLE_WHITELIST_REMOVE if remove address from the whitelist operation success */
    } update_whitelist_cmpl;                        /*!< Event parameter of BK_BLE_GAP_UPDATE_WHITELIST_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_SET_CHANNELS_COMPLETE_EVT
      */
    struct ble_set_channels_cmpl_evt_param
    {
        bk_bt_status_t status;                       /*!< BLE set channel status */
    } ble_set_channels;                             /*!< Event parameter of BK_BLE_GAP_SET_CHANNELS_COMPLETE_EVT */

#if (BLE_50_FEATURE_SUPPORT == TRUE)
    /**
     * @brief BK_BLE_GAP_READ_PHY_COMPLETE_EVT
     */
    struct ble_read_phy_cmpl_evt_param
    {
        bk_bt_status_t status;                   /*!< read phy complete status */
        bk_bd_addr_t bda;                        /*!< read phy address */
        bk_ble_gap_phy_t tx_phy;                 /*!< tx phy type */
        bk_ble_gap_phy_t rx_phy;                 /*!< rx phy type */
    } read_phy;                                   /*!< Event parameter of BK_BLE_GAP_READ_PHY_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_SET_PREFERRED_DEFAULT_PHY_COMPLETE_EVT
     */
    struct ble_set_perf_def_phy_cmpl_evt_param
    {
        bk_bt_status_t status;                     /*!< Indicate perf default phy set status */
    } set_perf_def_phy;                             /*!< Event parameter of BK_BLE_GAP_SET_PREFERRED_DEFAULT_PHY_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_SET_PREFERRED_PHY_COMPLETE_EVT
     */
    struct ble_set_perf_phy_cmpl_evt_param
    {
        bk_bt_status_t status;                     /*!< Indicate perf phy set status */
    } set_perf_phy;                                 /*!< Event parameter of BK_BLE_GAP_SET_PREFERRED_PHY_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_EXT_ADV_SET_RAND_ADDR_COMPLETE_EVT
     */
    struct ble_adv_set_rand_addr_cmpl_evt_param
    {
        bk_bt_status_t status;                      /*!< Indicate extend advertising random address set status */
    } adv_set_rand_addr;                         /*!< Event parameter of BK_BLE_GAP_EXT_ADV_SET_RAND_ADDR_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_EXT_ADV_PARAMS_SET_COMPLETE_EVT
     */
    struct ble_adv_params_set_cmpl_evt_param
    {
        bk_bt_status_t status;                     /*!< Indicate extend advertising parameters set status */
    } adv_params_set;                           /*!< Event parameter of BK_BLE_GAP_EXT_ADV_PARAMS_SET_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_EXT_ADV_DATA_SET_COMPLETE_EVT
     */
    struct ble_adv_data_set_cmpl_evt_param
    {
        bk_bt_status_t status;                      /*!< Indicate extend advertising data set status */
    } adv_data_set;                              /*!< Event parameter of BK_BLE_GAP_EXT_ADV_DATA_SET_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_EXT_SCAN_RSP_DATA_SET_COMPLETE_EVT
     */
    struct ble_adv_scan_rsp_set_cmpl_evt_param
    {
        bk_bt_status_t status;                      /*!< Indicate extend advertising scan response data set status */
    } scan_rsp_data_set;                                  /*!< Event parameter of BK_BLE_GAP_EXT_SCAN_RSP_DATA_SET_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_EXT_ADV_DATA_RAW_SET_COMPLETE_EVT
     */
    struct ble_adv_data_raw_set_cmpl_evt_param
    {
        bk_bt_status_t status;                      /*!< Indicate extend advertising data set status */
    } adv_data_raw_set;                              /*!< Event parameter of BK_BLE_GAP_EXT_ADV_DATA_SET_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_EXT_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT
     */
    struct ble_scan_rsp_data_raw_set_cmpl_evt_param
    {
        bk_bt_status_t status;                      /*!< Indicate extend advertising scan response data set status */
    } scan_rsp_data_raw_set;                                  /*!< Event parameter of BK_BLE_GAP_EXT_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_EXT_ADV_START_COMPLETE_EVT
     */
    struct ble_adv_start_cmpl_evt_param
    {
        bk_bt_status_t status;                     /*!< Indicate advertising start operation success status */
    } adv_start;                                /*!< Event parameter of BK_BLE_GAP_EXT_ADV_START_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_EXT_ADV_STOP_COMPLETE_EVT
     */
    struct ble_adv_stop_cmpl_evt_param
    {
        bk_bt_status_t status;                     /*!< Indicate advertising stop operation success status */
    } adv_stop;                                 /*!< Event parameter of BK_BLE_GAP_EXT_ADV_STOP_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_EXT_ADV_SET_REMOVE_COMPLETE_EVT
     */
    struct ble_adv_set_remove_cmpl_evt_param
    {
        bk_bt_status_t status;                     /*!< Indicate advertising stop operation success status */
    } adv_remove;                               /*!< Event parameter of BK_BLE_GAP_ADV_SET_REMOVE_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_EXT_ADV_SET_CLEAR_COMPLETE_EVT
     */
    struct ble_adv_set_clear_cmpl_evt_param
    {
        bk_bt_status_t status;                     /*!< Indicate advertising stop operation success status */
    } adv_clear;                                /*!< Event parameter of BK_BLE_GAP_ADV_SET_CLEAR_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_PERIODIC_ADV_SET_PARAMS_COMPLETE_EVT
     */
    struct ble_periodic_adv_set_params_cmpl_param
    {
        bk_bt_status_t status;                    /*!< Indicate periodic advertisingparameters set status */
    } peroid_adv_set_params;                       /*!< Event parameter of BK_BLE_GAP_PERIODIC_ADV_SET_PARAMS_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_PERIODIC_ADV_DATA_SET_COMPLETE_EVT
     */
    struct ble_periodic_adv_data_set_cmpl_param
    {
        bk_bt_status_t status;                    /*!< Indicate periodic advertising data set status */
    } period_adv_data_set;                         /*!< Event parameter of BK_BLE_GAP_PERIODIC_ADV_DATA_SET_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_PERIODIC_ADV_START_COMPLETE_EVT
     */
    struct ble_periodic_adv_start_cmpl_param
    {
        bk_bt_status_t status;                   /*!< Indicate periodic advertising start status */
    } period_adv_start;                           /*!< Event parameter of BK_BLE_GAP_PERIODIC_ADV_START_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_PERIODIC_ADV_STOP_COMPLETE_EVT
     */
    struct ble_periodic_adv_stop_cmpl_param
    {
        bk_bt_status_t status;                  /*!< Indicate periodic advertising stop status */
    } period_adv_stop;                           /*!< Event parameter of BK_BLE_GAP_PERIODIC_ADV_STOP_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_PERIODIC_ADV_CREATE_SYNC_COMPLETE_EVT
     */
    struct ble_period_adv_create_sync_cmpl_param
    {
        bk_bt_status_t status;                  /*!< Indicate periodic advertising create sync status */
    } period_adv_create_sync;                    /*!< Event parameter of BK_BLE_GAP_PERIODIC_ADV_CREATE_SYNC_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_PERIODIC_ADV_SYNC_CANCEL_COMPLETE_EVT
     */
    struct ble_period_adv_sync_cancel_cmpl_param
    {
        bk_bt_status_t status;                  /*!< Indicate periodic advertising sync cancel status */
    } period_adv_sync_cancel;                    /*!< Event parameter of BK_BLE_GAP_PERIODIC_ADV_SYNC_CANCEL_COMPLETE_EVT */
    /**
    * @brief BK_BLE_GAP_PERIODIC_ADV_SYNC_TERMINATE_COMPLETE_EVT
    */
    struct ble_period_adv_sync_terminate_cmpl_param
    {
        bk_bt_status_t status;                  /*!< Indicate periodic advertising sync terminate status */
    } period_adv_sync_term;                      /*!< Event parameter of BK_BLE_GAP_PERIODIC_ADV_SYNC_TERMINATE_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_PERIODIC_ADV_ADD_DEV_COMPLETE_EVT
     */
    struct ble_period_adv_add_dev_cmpl_param
    {
        bk_bt_status_t status;                 /*!< Indicate periodic advertising device list add status */
    } period_adv_add_dev;                       /*!< Event parameter of BK_BLE_GAP_PERIODIC_ADV_ADD_DEV_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_PERIODIC_ADV_REMOVE_DEV_COMPLETE_EVT
     */
    struct ble_period_adv_remove_dev_cmpl_param
    {
        bk_bt_status_t status;                /*!< Indicate periodic advertising device list remove status */
    } period_adv_remove_dev;                   /*!< Event parameter of BK_BLE_GAP_PERIODIC_ADV_REMOVE_DEV_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_PERIODIC_ADV_CLEAR_DEV_COMPLETE_EVT
     */
    struct ble_period_adv_clear_dev_cmpl_param
    {
        bk_bt_status_t status;               /*!< Indicate periodic advertising device list clean status */
    } period_adv_clear_dev;                   /*!< Event parameter of BK_BLE_GAP_PERIODIC_ADV_CLEAR_DEV_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_SCAN_PARAMS_SET_COMPLETE_EVT
     */
    struct ble_scan_params_set_cmpl_param
    {
        bk_bt_status_t status;              /*!< Indicate extend advertising parameters set status */
    } scan_params_set;                   /*!< Event parameter of BK_BLE_GAP_SCAN_PARAMS_SET_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_SCAN_START_COMPLETE_EVT
     */
    struct ble_scan_start_cmpl_param
    {
        bk_bt_status_t status;             /*!< Indicate extend advertising start status */
    } scan_start;                       /*!< Event parameter of BK_BLE_GAP_SCAN_START_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_SCAN_STOP_COMPLETE_EVT
     */
    struct ble_scan_stop_cmpl_param
    {
        bk_bt_status_t status;            /*!< Indicate extend advertising stop status */
    } scan_stop;                       /*!< Event parameter of BK_BLE_GAP_SCAN_STOP_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_PREFER_CONN_PARAMS_SET_COMPLETE_EVT
     */
    struct ble_conn_params_set_cmpl_param
    {
        bk_bt_status_t status;            /*!< Indicate extend connection parameters set status */
    } conn_params_set;                 /*!< Event parameter of BK_BLE_GAP_PREFER_CONN_PARAMS_SET_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_ADV_TERMINATED_EVT
     */
    struct ble_adv_terminate_param
    {
        uint8_t status;                   /*!< Indicate adv terminate status */
        /*  status 0x3c indicates that advertising for a fixed duration completed or,
            for directed advertising, that advertising completed without a connection
            being created;
            status 0x00 indicates that advertising successfully ended with a connection being created.
        */
        uint8_t adv_instance;           /*!< extend advertising handle */
        uint16_t conn_idx;              /*!< connection index */
        uint8_t completed_event;        /*!< the number of completed extend advertising events */
    } adv_terminate;                    /*!< Event parameter of BK_BLE_GAP_ADV_TERMINATED_EVT */
    /**
     * @brief BK_BLE_GAP_SCAN_REQ_RECEIVED_EVT
     */
    struct ble_scan_req_received_param
    {
        uint8_t adv_instance;                /*!< extend advertising handle */
        bk_ble_addr_type_t scan_addr_type;  /*!< scanner address type */
        bk_bd_addr_t scan_addr;             /*!< scanner address */
    } scan_req_received;                     /*!< Event parameter of BK_BLE_GAP_SCAN_REQ_RECEIVED_EVT */
    /**
     * @brief BK_BLE_GAP_CHANNEL_SELECT_ALGORITHM_EVT
     */
    struct ble_channel_sel_alg_param
    {
        uint16_t conn_handle;              /*!< connection handle */
        uint8_t channel_sel_alg;           /*!< channel selection algorithm */
    } channel_sel_alg;                     /*!< Event parameter of BK_BLE_GAP_CHANNEL_SELECT_ALGORITHM_EVT */
    /**
     * @brief BK_BLE_GAP_PERIODIC_ADV_SYNC_LOST_EVT
     */
    struct ble_periodic_adv_sync_lost_param
    {
        uint16_t sync_handle;                 /*!< sync handle */
    } periodic_adv_sync_lost;                 /*!< Event parameter of BK_BLE_GAP_PERIODIC_ADV_SYNC_LOST_EVT */
    /**
     * @brief BK_BLE_GAP_PERIODIC_ADV_SYNC_ESTAB_EVT
     */
    struct ble_periodic_adv_sync_estab_param
    {
        uint8_t status;                      /*!< periodic advertising sync status */
        uint16_t sync_handle;                /*!< periodic advertising sync handle */
        uint8_t sid;                         /*!< periodic advertising sid */
        bk_ble_addr_type_t adv_addr_type;   /*!< periodic advertising address type */
        bk_bd_addr_t adv_addr;              /*!< periodic advertising address */
        bk_ble_gap_phy_t adv_phy;           /*!< periodic advertising phy type */
        uint16_t period_adv_interval;        /*!< periodic advertising interval */
        uint8_t adv_clk_accuracy;            /*!< periodic advertising clock accuracy */
    } periodic_adv_sync_estab;               /*!< Event parameter of BK_BLE_GAP_PERIODIC_ADV_SYNC_ESTAB_EVT */
    /**
     * @brief BK_BLE_GAP_PHY_UPDATE_COMPLETE_EVT
     */
    struct ble_phy_update_cmpl_param
    {
        bk_bt_status_t status;             /*!< phy update status */
        bk_bd_addr_t bda;                  /*!< address */
        bk_ble_gap_phy_t tx_phy;           /*!< tx phy type */
        bk_ble_gap_phy_t rx_phy;           /*!< rx phy type */
    } phy_update;                           /*!< Event parameter of BK_BLE_GAP_PHY_UPDATE_COMPLETE_EVT */
    /**
     * @brief BK_BLE_GAP_EXT_ADV_REPORT_EVT
     */
    struct ble_ext_adv_report_param
    {
        bk_ble_gap_ext_adv_reprot_t params;   /*!< extend advertising report parameters */
    } ext_adv_report;                          /*!< Event parameter of BK_BLE_GAP_EXT_ADV_REPORT_EVT */
    /**
     * @brief BK_BLE_GAP_PERIODIC_ADV_REPORT_EVT
     */
    struct ble_periodic_adv_report_param
    {
        bk_ble_gap_periodic_adv_report_t params; /*!< periodic advertising report parameters */
    } period_adv_report;                          /*!< Event parameter of BK_BLE_GAP_PERIODIC_ADV_REPORT_EVT */

    /**
     * @brief BK_BLE_GAP_CONNECT_COMPLETE_EVT
     */
    struct ble_connect_complete_param
    {
        bk_bt_status_t status;
        bk_bd_addr_t remote_bda;       /*!< Remote bluetooth device address */
        bk_ble_addr_type_t remote_bda_type;
        uint8_t link_role;              /*!< Link role : master role = 0  ; slave role = 1*/
        uint16_t conn_intv;
        uint16_t conn_latency;
        uint16_t supervision_timeout;
        uint16_t hci_handle;            /// hci handle, valid only when status == 0
    } connect_complete;                          /*!< Event parameter of BK_BLE_GAP_CONNECT_COMPLETE_EVT */

    /**
     * @brief BK_BLE_GAP_DISCONNECT_COMPLETE_EVT
     */
    struct ble_disconnect_complete_param
    {
        bk_bt_status_t status;
        bk_bd_addr_t remote_bda;       /*!< Remote bluetooth device address */
        bk_ble_addr_type_t remote_bda_type;
        uint8_t reason;
        uint16_t hci_handle;
    } disconnect_complete;                          /*!< Event parameter of BK_BLE_GAP_DISCONNECT_COMPLETE_EVT */

    /**
     * @brief BK_BLE_GAP_CONNECT_CANCEL_EVT
     */
    struct ble_connect_cancel_param
    {
        bk_bt_status_t status;
    } connect_cancel;                          /*!< Event parameter of BK_BLE_GAP_CONNECT_CANCEL_EVT */

    /**
     * @brief BK_BLE_GAP_UPDATE_CONN_PARAMS_REQ_EVT
     */
    struct ble_conntection_update_param_req
    {
        uint8_t can_modify;                 /*!< if true, param can be modify before accept*/
        bk_ble_conn_update_params_t param;
        uint8_t accept;
    } update_conn_param_req;                /*!< Event parameter of BK_BLE_GAP_UPDATE_CONN_PARAMS_REQ_EVT */


    /**
     * @brief BK_BLE_GAP_SET_SECURITY_PARAMS_COMPLETE_EVT
     */
    struct ble_set_security_params_cmpl
    {
        bk_bt_status_t status;
        bk_ble_sm_param_t param;
    } set_security_params_cmpl;                /*!< Event parameter of BK_BLE_GAP_SET_SECURITY_PARAMS_COMPLETE_EVT */

    /**
     * @brief BK_BLE_GAP_GENERATE_RPA_COMPLETE_EVT
     */
    struct ble_generate_rpa_cmpl
    {
        bk_bt_status_t status;
        bk_bd_addr_t             addr;
        bk_ble_addr_type_t       addr_type;
    } generate_rpa_cmpl;                /*!< Event parameter of BK_BLE_GAP_GENERATE_RPA_COMPLETE_EVT */

#endif // #if (BLE_50_FEATURE_SUPPORT == TRUE)
} bk_ble_gap_cb_param_t;

/**
 * @brief GAP callback function type, todo: modify docx
 * @param event : Event type
 * @param param : Point to callback parameter, currently is union type
 */
typedef void (* bk_ble_gap_cb_t)(bk_ble_gap_cb_event_t event, bk_ble_gap_cb_param_t *param);

/// @}

/// @}

#ifdef __cplusplus
}
#endif


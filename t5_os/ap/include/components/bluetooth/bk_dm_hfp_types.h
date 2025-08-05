// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/* features masks of AG */
#define BK_HF_CLIENT_PEER_FEAT_3WAY       0x01        /* Three-way calling */
#define BK_HF_CLIENT_PEER_FEAT_ECNR       0x02        /* Echo cancellation and/or noise reduction */
#define BK_HF_CLIENT_PEER_FEAT_VREC       0x04        /* Voice recognition */
#define BK_HF_CLIENT_PEER_FEAT_INBAND     0x08        /* In-band ring tone */
#define BK_HF_CLIENT_PEER_FEAT_VTAG       0x10        /* Attach a phone number to a voice tag */
#define BK_HF_CLIENT_PEER_FEAT_REJECT     0x20        /* Ability to reject incoming call */
#define BK_HF_CLIENT_PEER_FEAT_ECS        0x40        /* Enhanced Call Status */
#define BK_HF_CLIENT_PEER_FEAT_ECC        0x80        /* Enhanced Call Control */
#define BK_HF_CLIENT_PEER_FEAT_EXTERR    0x100        /* Extended error codes */
#define BK_HF_CLIENT_PEER_FEAT_CODEC     0x200        /* Codec Negotiation */
/* HFP 1.7+ */
#define BK_HF_CLIENT_PEER_FEAT_HF_IND    0x400        /* HF Indicators */
#define BK_HF_CLIENT_PEER_FEAT_ESCO_S4   0x800        /* eSCO S4 Setting Supported */

/* CHLD feature masks of AG */
#define BK_HF_CLIENT_CHLD_FEAT_REL           0x01       /* 0  Release waiting call or held calls */
#define BK_HF_CLIENT_CHLD_FEAT_REL_ACC       0x02       /* 1  Release active calls and accept other waiting or held call */
#define BK_HF_CLIENT_CHLD_FEAT_REL_X         0x04       /* 1x Release specified active call only */
#define BK_HF_CLIENT_CHLD_FEAT_HOLD_ACC      0x08       /* 2  Active calls on hold and accept other waiting or held call */
#define BK_HF_CLIENT_CHLD_FEAT_PRIV_X        0x10       /* 2x Request private mode with specified call(put the rest on hold) */
#define BK_HF_CLIENT_CHLD_FEAT_MERGE         0x20       /* 3  Add held call to multiparty */
#define BK_HF_CLIENT_CHLD_FEAT_MERGE_DETACH  0x40       /* 4  Connect two calls and leave(disconnect from multiparty) */


/// Bluetooth HFP RFCOMM connection and service level connection status
typedef enum {
    BK_HF_CLIENT_CONNECTION_STATE_DISCONNECTED = 0,     /*!< RFCOMM data link channel released */
    BK_HF_CLIENT_CONNECTION_STATE_CONNECTING,           /*!< connecting remote device on the RFCOMM data link*/
    BK_HF_CLIENT_CONNECTION_STATE_CONNECTED,            /*!< RFCOMM connection established */
    BK_HF_CLIENT_CONNECTION_STATE_SLC_CONNECTED,        /*!< service level connection established */
    BK_HF_CLIENT_CONNECTION_STATE_DISCONNECTING,        /*!< disconnecting with remote device on the RFCOMM dat link*/
} bk_hf_client_connection_state_t;

/// Bluetooth HFP audio connection status
typedef enum {
    BK_HF_CLIENT_AUDIO_STATE_DISCONNECTED = 0,          /*!< audio connection released */
    BK_HF_CLIENT_AUDIO_STATE_CONNECTING,                /*!< audio connection has been initiated */
    BK_HF_CLIENT_AUDIO_STATE_CONNECTED,                 /*!< audio connection is established */
} bk_hf_client_audio_state_t;

/// HF CLIENT callback events
typedef enum {
    BK_HF_CLIENT_CONNECTION_STATE_EVT = 0,          /*!< connection state changed event */
    BK_HF_CLIENT_AUDIO_STATE_EVT,                   /*!< audio connection state change event */
    BK_HF_CLIENT_BVRA_EVT,                          /*!< voice recognition state change event */
    BK_HF_CLIENT_CIND_CALL_EVT,                     /*!< call indication */
    BK_HF_CLIENT_CIND_CALL_SETUP_EVT,               /*!< call setup indication */
    BK_HF_CLIENT_CIND_CALL_HELD_EVT,                /*!< call held indication */
    BK_HF_CLIENT_CIND_SERVICE_AVAILABILITY_EVT,     /*!< network service availability indication */
    BK_HF_CLIENT_CIND_SIGNAL_STRENGTH_EVT,          /*!< signal strength indication */
    BK_HF_CLIENT_CIND_ROAMING_STATUS_EVT,           /*!< roaming status indication */
    BK_HF_CLIENT_CIND_BATTERY_LEVEL_EVT,            /*!< battery level indication */
    BK_HF_CLIENT_COPS_CURRENT_OPERATOR_EVT,         /*!< current operator information */
    BK_HF_CLIENT_BTRH_EVT,                          /*!< call response and hold event */
    BK_HF_CLIENT_CLIP_EVT,                          /*!< Calling Line Identification notification */
    BK_HF_CLIENT_CCWA_EVT,                          /*!< call waiting notification */
    BK_HF_CLIENT_CLCC_EVT,                          /*!< list of current calls notification */
    BK_HF_CLIENT_VOLUME_CONTROL_EVT,                /*!< audio volume control command from AG, provided by +VGM or +VGS message */
    BK_HF_CLIENT_AT_RESPONSE_EVT,                   /*!< AT command response event */
    BK_HF_CLIENT_CNUM_EVT,                          /*!< subscriber information response from AG */
    BK_HF_CLIENT_BSIR_EVT,                          /*!< setting of in-band ring tone */
    BK_HF_CLIENT_BINP_EVT,                          /*!< requested number of last voice tag from AG */
    BK_HF_CLIENT_RING_IND_EVT,                      /*!< ring indication event */
} bk_hf_client_cb_event_t;

/// Codec Type
typedef enum {
    CODEC_VOICE_CVSD = 0,     /*!< cvsd */
    CODEC_VOICE_MSBC,         /*!< msbc*/
} bk_hf_codec_type_t;

/// voice recognition state
typedef enum {
    BK_HF_VR_STATE_DISABLED = 0,           /*!< voice recognition disabled */
    BK_HF_VR_STATE_ENABLED,                /*!< voice recognition enabled */
} bk_hf_vr_state_t;

/// +CIND call status indicator values
typedef enum {
    BK_HF_CALL_STATUS_NO_CALLS = 0,                  /*!< no call in progress  */
    BK_HF_CALL_STATUS_CALL_IN_PROGRESS = 1,          /*!< call is present(active or held) */
} bk_hf_call_status_t;

/// +CIND call setup status indicator values
typedef enum {
    BK_HF_CALL_SETUP_STATUS_IDLE = 0,                /*!< no call setup in progress */
    BK_HF_CALL_SETUP_STATUS_INCOMING = 1,            /*!< incoming call setup in progress */
    BK_HF_CALL_SETUP_STATUS_OUTGOING_DIALING = 2,    /*!< outgoing call setup in dialing state */
    BK_HF_CALL_SETUP_STATUS_OUTGOING_ALERTING = 3,   /*!< outgoing call setup in alerting state */
} bk_hf_call_setup_status_t;

/// +CIND call held indicator values
typedef enum {
    BK_HF_CALL_HELD_STATUS_NONE = 0,                 /*!< no calls held */
    BK_HF_CALL_HELD_STATUS_HELD_AND_ACTIVE = 1,      /*!< both active and held call */
    BK_HF_CALL_HELD_STATUS_HELD = 2,                 /*!< call on hold, no active call*/
} bk_hf_call_held_status_t;

/// +CIND network service availability status
typedef enum
{
    BK_HF_NETWORK_STATE_NOT_AVAILABLE = 0,
    BK_HF_NETWORK_STATE_AVAILABLE
} bk_hf_network_state_t;

/// +CIND roaming status indicator values
typedef enum {
    BK_HF_ROAMING_STATUS_INACTIVE = 0,               /*!< roaming is not active */
    BK_HF_ROAMING_STATUS_ACTIVE,                     /*!< a roaming is active */
} bk_hf_roaming_status_t;

/// +BTRH response and hold result code
typedef enum {
    BK_HF_BTRH_STATUS_HELD = 0,       /*!< incoming call is put on held in AG */
    BK_HF_BTRH_STATUS_ACCEPTED,       /*!< held incoming call is accepted in AG */
    BK_HF_BTRH_STATUS_REJECTED,       /*!< held incoming call is rejected in AG */
} bk_hf_btrh_status_t;


/// +CLCC status of the call
typedef enum {
    BK_HF_CURRENT_CALL_STATUS_ACTIVE = 0,            /*!< active */
    BK_HF_CURRENT_CALL_STATUS_HELD = 1,              /*!< held */
    BK_HF_CURRENT_CALL_STATUS_DIALING = 2,           /*!< dialing (outgoing calls only) */
    BK_HF_CURRENT_CALL_STATUS_ALERTING = 3,          /*!< alerting (outgoing calls only) */
    BK_HF_CURRENT_CALL_STATUS_INCOMING = 4,          /*!< incoming (incoming calls only) */
    BK_HF_CURRENT_CALL_STATUS_WAITING = 5,           /*!< waiting (incoming calls only) */
    BK_HF_CURRENT_CALL_STATUS_HELD_BY_RBK_HOLD = 6, /*!< call held by response and hold */
} bk_hf_current_call_status_t;

/// +CLCC direction of the call
typedef enum {
    BK_HF_CURRENT_CALL_DIRECTION_OUTGOING = 0,       /*!< outgoing */
    BK_HF_CURRENT_CALL_DIRECTION_INCOMING = 1,       /*!< incoming */
} bk_hf_current_call_direction_t;

/// +CLCC multi-party call flag
typedef enum {
    BK_HF_CURRENT_CALL_MPTY_TYPE_SINGLE = 0,         /*!< not a member of a multi-party call */
    BK_HF_CURRENT_CALL_MPTY_TYPE_MULTI = 1,          /*!< member of a multi-party call */
} bk_hf_current_call_mpty_type_t;

/// Bluetooth HFP audio volume control target
typedef enum {
    BK_HF_VOLUME_CONTROL_TARGET_SPK = 0,             /*!< speaker */
    BK_HF_VOLUME_CONTROL_TARGET_MIC,                 /*!< microphone */
} bk_hf_volume_control_target_t;

/// AT+CHLD command values
typedef enum {
    BK_HF_CHLD_TYPE_REL = 0,               /*!< <0>, Terminate all held or set UDUB("busy") to a waiting call */
    BK_HF_CHLD_TYPE_REL_ACC,               /*!< <1>, Terminate all active calls and accepts a waiting/held call */
    BK_HF_CHLD_TYPE_HOLD_ACC,              /*!< <2>, Hold all active calls and accepts a waiting/held call */
    BK_HF_CHLD_TYPE_MERGE,                 /*!< <3>, Add all held calls to a conference */
    BK_HF_CHLD_TYPE_MERGE_DETACH,          /*!< <4>, connect the two calls and disconnects the subscriber from both calls */
} bk_hf_chld_type_t;

/// AT+BTRH response and hold action code
typedef enum {
    BK_HF_BTRH_CMD_HOLD = 0,          /*!< put the incoming call on hold */
    BK_HF_BTRH_CMD_ACCEPT = 1,        /*!< accept a held incoming call */
    BK_HF_BTRH_CMD_REJECT = 2,        /*!< reject a held incoming call */
} bk_hf_btrh_cmd_t;

/* AT response code - OK/Error */
typedef enum {
    BK_HF_AT_RESPONSE_CODE_OK = 0,         /*!< acknowledges execution of a command line */
    BK_HF_AT_RESPONSE_CODE_ERR,            /*!< command not accepted */
    BK_HF_AT_RESPONSE_CODE_NO_CARRIER,     /*!< connection terminated */
    BK_HF_AT_RESPONSE_CODE_BUSY,           /*!< busy signal detected */
    BK_HF_AT_RESPONSE_CODE_NO_ANSWER,      /*!< connection completion timeout */
    BK_HF_AT_RESPONSE_CODE_DELAYED,        /*!< delayed */
    BK_HF_AT_RESPONSE_CODE_BLACKLISTED,    /*!< blacklisted */
    BK_HF_AT_RESPONSE_CODE_CME,            /*!< CME error */
} bk_hf_at_response_code_t;

/* AT response code - OK/Error */
typedef enum {
    BK_HF_AT_RESPONSE_ERROR = 0,
    BK_HF_AT_RESPONSE_OK
} bk_hf_at_response_t;

/// Extended Audio Gateway Error Result Code Response
typedef enum {
    BK_HF_CME_AG_FAILURE = 0,                    /*!< ag failure */
    BK_HF_CME_NO_CONNECTION_TO_PHONE = 1,        /*!< no connection to phone */
    BK_HF_CME_OPERATION_NOT_ALLOWED = 3,         /*!< operation not allowed */
    BK_HF_CME_OPERATION_NOT_SUPPORTED = 4,       /*!< operation not supported */
    BK_HF_CME_PH_SIM_PIN_REQUIRED = 5,           /*!< PH-SIM PIN Required */
    BK_HF_CME_SIM_NOT_INSERTED = 10,             /*!< SIM not inserted */
    BK_HF_CME_SIM_PIN_REQUIRED = 11,             /*!< SIM PIN required */
    BK_HF_CME_SIM_PUK_REQUIRED = 12,             /*!< SIM PUK required */
    BK_HF_CME_SIM_FAILURE = 13,                  /*!< SIM failure */
    BK_HF_CME_SIM_BUSY = 14,                     /*!< SIM busy */
    BK_HF_CME_INCORRECT_PASSWORD = 16,           /*!< incorrect password */
    BK_HF_CME_SIM_PIN2_REQUIRED = 17,            /*!< SIM PIN2 required */
    BK_HF_CME_SIM_PUK2_REQUIRED = 18,            /*!< SIM PUK2 required */
    BK_HF_CME_MEMORY_FULL = 20,                  /*!< memory full */
    BK_HF_CME_INVALID_INDEX = 21,                /*!< invalid index */
    BK_HF_CME_MEMORY_FAILURE = 23,              /*!< memory failure */
    BK_HF_CME_TEXT_STRING_TOO_LONG = 24,         /*!< test string too long */
    BK_HF_CME_INVALID_CHARACTERS_IN_TEXT_STRING = 25,  /*!< invalid characters in text string */
    BK_HF_CME_DIAL_STRING_TOO_LONG = 26,         /*!< dial string too long*/
    BK_HF_CME_INVALID_CHARACTERS_IN_DIAL_STRING = 27,  /*!< invalid characters in dial string */
    BK_HF_CME_NO_NETWORK_SERVICE = 30,           /*!< no network service */
    BK_HF_CME_NETWORK_TIMEOUT = 31,              /*!< network timeout */
    BK_HF_CME_NETWORK_NOT_ALLOWED = 32,          /*!< network not allowed --emergency calls only */
} bk_hf_cme_err_t;

/// +CNUM service type of the phone number
typedef enum {
    BK_HF_SUBSCRIBER_SERVICE_TYPE_UNKNOWN = 0,      /*!< unknown */
    BK_HF_SUBSCRIBER_SERVICE_TYPE_VOICE,            /*!< voice service */
    BK_HF_SUBSCRIBER_SERVICE_TYPE_FAX,              /*!< fax service */
} bk_hf_subscriber_service_type_t;

/// in-band ring tone state
typedef enum {
    BK_HF_CLIENT_IN_BAND_RINGTONE_NOT_PROVIDED = 0,
    BK_HF_CLIENT_IN_BAND_RINGTONE_PROVIDED,
} bk_hf_client_in_band_ring_state_t;


/// HFP client callback parameters
typedef struct{
    uint8_t remote_bda[6];                          /*!< remote bluetooth device address */
    union
    {
        /**
         * @brief  BK_HF_CLIENT_CONNECTION_STATE_EVT
         */
        struct hf_client_conn_stat_param
        {
            bk_hf_client_connection_state_t state;   /*!< HF connection state */
            uint32_t peer_feat;                      /*!< AG supported features */
            uint32_t chld_feat;                      /*!< AG supported features on call hold and multiparty services */
        } conn_state;                                 /*!< HF callback param of BK_HF_CLIENT_CONNECTION_STATE_EVT */

        /**
         * @brief BK_HF_CLIENT_AUDIO_STATE_EVT
         */
        struct hf_client_audio_stat_param
        {
            bk_hf_client_audio_state_t state;        /*!< audio connection state */
            bk_hf_codec_type_t codec_type;           /*!< cvsd or msbc */
        } audio_state;                                /*!< HF callback param of BK_HF_CLIENT_AUDIO_STATE_EVT */

        /**
         * @brief BK_HF_CLIENT_BVRA_EVT
         */
        struct hf_client_bvra_param
        {
            bk_hf_vr_state_t value;                 /*!< voice recognition state */
        } bvra;                                      /*!< HF callback param of BK_HF_CLIENT_BVRA_EVT */

        /**
         * @brief BK_HF_CLIENT_CIND_CALL_EVT
         */
        struct hf_client_call_ind_param
        {
            bk_hf_call_status_t status;             /*!< call status indicator */
        } call;                                      /*!< HF callback param of BK_HF_CLIENT_CIND_CALL_EVT */

        /**
         * @brief BK_HF_CLIENT_CIND_CALL_SETUP_EVT
         */
        struct hf_client_call_setup_ind_param
        {
            bk_hf_call_setup_status_t status;       /*!< call setup status indicator */
        } call_setup;                                /*!< HF callback param of BK_HF_CLIENT_BVRA_EVT */

        /**
         * @brief BK_HF_CLIENT_CIND_CALL_HELD_EVT
         */
        struct hf_client_call_held_ind_param
        {
            bk_hf_call_held_status_t status;        /*!< bluetooth proprietary call hold status indicator */
        } call_held;                                 /*!< HF callback param of BK_HF_CLIENT_CIND_CALL_HELD_EVT */

        /**
         * @brief BK_HF_CLIENT_CIND_SERVICE_AVAILABILITY_EVT
         */
        struct hf_client_service_availability_param
        {
            bk_hf_network_state_t status;           /*!< service availability status */
        } service_availability;                      /*!< HF callback param of BK_HF_CLIENT_CIND_SERVICE_AVAILABILITY_EVT */


        /**
         * @brief BK_HF_CLIENT_CIND_SIGNAL_STRENGTH_EVT
         */
        struct hf_client_signal_strength_ind_param
        {
            int value;                               /*!< signal strength value, ranges from 0 to 5 */
        } signal_strength;                           /*!< HF callback param of BK_HF_CLIENT_CIND_SIGNAL_STRENGTH_EVT */

        /**
         * @brief BK_HF_CLIENT_CIND_ROAMING_STATUS_EVT
         */
        struct hf_client_network_roaming_param
        {
            bk_hf_roaming_status_t status;          /*!< roaming status */
        } roaming;                                   /*!< HF callback param of BK_HF_CLIENT_CIND_ROAMING_STATUS_EVT */

        /**
         * @brief BK_HF_CLIENT_CIND_BATTERY_LEVEL_EVT
         */
        struct hf_client_battery_level_ind_param
        {
            int value;                               /*!< battery charge value, ranges from 0 to 5 */
        } battery_level;                             /*!< HF callback param of BK_HF_CLIENT_CIND_BATTERY_LEVEL_EVT */

        /**
         * @brief BK_HF_CLIENT_COPS_CURRENT_OPERATOR_EVT
         */
        struct hf_client_current_operator_param
        {
            const char *name;                        /*!< name of the network operator */
        } cops;                                      /*!< HF callback param of BK_HF_CLIENT_COPS_CURRENT_OPERATOR_EVT */

        /**
         * @brief BK_HF_CLIENT_BTRH_EVT
         */
        struct hf_client_btrh_param
        {
            bk_hf_btrh_status_t status;             /*!< call hold and response status result code */
        } btrh;                                      /*!< HF callback param of BK_HF_CLIENT_BRTH_EVT */

        /**
         * @brief BK_HF_CLIENT_CLIP_EVT
         */
        struct hf_client_clip_param
        {
            const char *name;                        /*!< name string of the call */
            const char *number;                      /*!< phone number string of call */
        } clip;                                      /*!< HF callback param of BK_HF_CLIENT_CLIP_EVT */

        /**
         * @brief BK_HF_CLIENT_CCWA_EVT
         */
        struct hf_client_ccwa_param
        {
            const char *number;                      /*!< phone number string of waiting call */
            const char *name;                        /*!< name string of waiting call */
        } ccwa;                                      /*!< HF callback param of BK_HF_CLIENT_BVRA_EVT */

        /**
         * @brief BK_HF_CLIENT_CLCC_EVT
         */
        struct hf_client_clcc_param
        {
            int idx;                                 /*!< numbering(starting with 1) of the call */
            bk_hf_current_call_direction_t dir;     /*!< direction of the call */
            bk_hf_current_call_status_t status;     /*!< status of the call */
            bk_hf_current_call_mpty_type_t mpty;    /*!< multi-party flag */
            char *number;                            /*!< phone number(optional) */
        } clcc;                                      /*!< HF callback param of BK_HF_CLIENT_CLCC_EVT */

        /**
         * @brief BK_HF_CLIENT_VOLUME_CONTROL_EVT
         */
        struct hf_client_volume_control_param
        {
            bk_hf_volume_control_target_t type;     /*!< volume control target, speaker or microphone */
            int volume;                              /*!< gain, ranges from 0 to 15 */
        } volume_control;                            /*!< HF callback param of BK_HF_CLIENT_VOLUME_CONTROL_EVT */

        /**
         * @brief BK_HF_CLIENT_AT_RESPONSE_EVT
         */
        struct hf_client_at_response_param
        {
            bk_hf_at_response_code_t code;          /*!< AT response code */
            bk_hf_cme_err_t cme;                    /*!< Extended Audio Gateway Error Result Code */
            uint16_t asso_cmd;
        } at_response;                               /*!< HF callback param of BK_HF_CLIENT_AT_RESPONSE_EVT */

        /**
         * @brief BK_HF_CLIENT_CNUM_EVT
         */
        struct hf_client_cnum_param
        {
            const char *number;                      /*!< phone number string */
            bk_hf_subscriber_service_type_t type;   /*!< service type that the phone number relates to */
        } cnum;                                      /*!< HF callback param of BK_HF_CLIENT_CNUM_EVT */

        /**
         * @brief BK_HF_CLIENT_BSIR_EVT
         */
        struct hf_client_bsirparam
        {
            bk_hf_client_in_band_ring_state_t state;  /*!< setting state of in-band ring tone */
        } bsir;                                        /*!< HF callback param of BK_HF_CLIENT_BSIR_EVT */

        /**
         * @brief bk_HF_CLIENT_BINP_EVT
         */
        struct hf_client_binp_param
        {
            const char *number;                      /*!< phone number corresponding to the last voice tag in the HF */
        } binp;                                      /*!< HF callback param of BK_HF_CLIENT_BINP_EVT */

    };
}bk_hf_client_cb_param_t;                      /*!< HFP client callback parameters */

/**
 * @brief           HFP client callback function type
 *
 * @param           event : Event type
 *
 * @param           param : Pointer to callback parameter
 */
typedef void (* bk_bt_hf_client_cb_t)(bk_hf_client_cb_event_t event, bk_hf_client_cb_param_t *param);

/**
 * @brief           HFP client incoming data callback function
 *
 * @param[in]       buf : pointer to the data(payload of HCI synchronous data packet) received from HFP AG device
 *
 * @param[in]       len : size(in bytes) in buf
 */
typedef void (* bk_bt_hf_client_data_cb_t)(const uint8_t *buf, uint16_t len);


#ifdef __cplusplus
}
#endif


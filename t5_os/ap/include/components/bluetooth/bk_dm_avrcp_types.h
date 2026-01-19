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

#include "components/bluetooth/bk_dm_bluetooth_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup dm_avrcp_types AVRCP DEFINES
 * @{
 */

/// AVRCP event notification ids
typedef enum {
    BK_AVRCP_RN_PLAY_STATUS_CHANGE = 0x01,        /*!< track status change, eg. from playing to paused */
    BK_AVRCP_RN_TRACK_CHANGE = 0x02,              /*!< new track is loaded */
    BK_AVRCP_RN_TRACK_REACHED_END = 0x03,         /*!< current track reached end */
    BK_AVRCP_RN_TRACK_REACHED_START = 0x04,       /*!< current track reached start position */
    BK_AVRCP_RN_PLAY_POS_CHANGED = 0x05,          /*!< track playing position changed */
    BK_AVRCP_RN_BATTERY_STATUS_CHANGE = 0x06,     /*!< battery status changed */
    BK_AVRCP_RN_SYSTEM_STATUS_CHANGE = 0x07,      /*!< system status changed */
    BK_AVRCP_RN_APP_SETTING_CHANGE = 0x08,        /*!< application settings changed */
    BK_AVRCP_RN_NOW_PLAYING_CHANGE = 0x09,        /*!< now playing content changed */
    BK_AVRCP_RN_AVAILABLE_PLAYERS_CHANGE = 0x0a,  /*!< available players changed */
    BK_AVRCP_RN_ADDRESSED_PLAYER_CHANGE = 0x0b,   /*!< the addressed player changed */
    BK_AVRCP_RN_UIDS_CHANGE = 0x0c,               /*!< UIDs changed */
    BK_AVRCP_RN_VOLUME_CHANGE = 0x0d,             /*!< volume changed locally on TG */
    BK_AVRCP_RN_MAX_EVT
} bk_avrcp_rn_event_ids_t;

/// AVRCP Controller callback events
typedef enum {
    BK_AVRCP_CT_CONNECTION_STATE_EVT = 0,            /*!< connection state changed event */
    BK_AVRCP_CT_PASSTHROUGH_RSP_EVT = 1,             /*!< passthrough response event */
    BK_AVRCP_CT_GET_ELEM_ATTR_RSP_EVT = 2,                /*!< get elem attr response event */
    BK_AVRCP_CT_PLAY_STATUS_RSP_EVT = 3,             /*!< play status response event */ //not used
    BK_AVRCP_CT_CHANGE_NOTIFY_EVT = 4,               /*!< notification event */
    BK_AVRCP_CT_REMOTE_FEATURES_EVT = 5,             /*!< feature of remote device indication event */
    BK_AVRCP_CT_GET_RN_CAPABILITIES_RSP_EVT = 6,     /*!< supported notification events capability of peer device */
    BK_AVRCP_CT_SET_ABSOLUTE_VOLUME_RSP_EVT = 7,     /*!< set absolute volume response event */
    BK_AVRCP_CT_SET_PLAYER_APP_SETTING_RSP_EVT,          ///set player app setting rsp event
} bk_avrcp_ct_cb_event_t;

/// AVCTP response codes
typedef enum {
    BK_AVRCP_RSP_NOT_IMPL   = 8,                  /*!< not implemented */
    BK_AVRCP_RSP_ACCEPT     = 9,                  /*!< accept */
    BK_AVRCP_RSP_REJECT     = 10,                 /*!< reject */
    BK_AVRCP_RSP_IN_TRANS   = 11,                 /*!< in transition */
    BK_AVRCP_RSP_IMPL_STBL  = 12,                 /*!< implemented/stable */
    BK_AVRCP_RSP_CHANGED    = 13,                 /*!< changed */
    BK_AVRCP_RSP_INTERIM    = 15,                 /*!< interim */
} bk_avrcp_rsp_t;

/// AVRCP battery status
typedef enum {
    BK_AVRCP_BATT_NORMAL       = 0,               /*!< normal state */
    BK_AVRCP_BATT_WARNING      = 1,               /*!< unable to operate soon */
    BK_AVRCP_BATT_CRITICAL     = 2,               /*!< cannot operate any more */
    BK_AVRCP_BATT_EXTERNAL     = 3,               /*!< plugged to external power supply */
    BK_AVRCP_BATT_FULL_CHARGE  = 4,               /*!< when completely charged from external power supply */
} bk_avrcp_batt_stat_t;

/// AVRCP current status of playback
typedef enum {
    BK_AVRCP_PLAYBACK_STOPPED = 0,                /*!< stopped */
    BK_AVRCP_PLAYBACK_PLAYING = 1,                /*!< playing */
    BK_AVRCP_PLAYBACK_PAUSED = 2,                 /*!< paused */
    BK_AVRCP_PLAYBACK_FWD_SEEK = 3,               /*!< forward seek */
    BK_AVRCP_PLAYBACK_REV_SEEK = 4,               /*!< reverse seek */
    BK_AVRCP_PLAYBACK_ERROR = 0xFF,               /*!< error */
} bk_avrcp_playback_stat_t;

/// AVRCP passthrough command code
typedef enum {
    BK_AVRCP_PT_CMD_SELECT         =   0x00,    /*!< select */
    BK_AVRCP_PT_CMD_UP             =   0x01,    /*!< up */
    BK_AVRCP_PT_CMD_DOWN           =   0x02,    /*!< down */
    BK_AVRCP_PT_CMD_LEFT           =   0x03,    /*!< left */
    BK_AVRCP_PT_CMD_RIGHT          =   0x04,    /*!< right */
    BK_AVRCP_PT_CMD_RIGHT_UP       =   0x05,    /*!< right-up */
    BK_AVRCP_PT_CMD_RIGHT_DOWN     =   0x06,    /*!< right-down */
    BK_AVRCP_PT_CMD_LEFT_UP        =   0x07,    /*!< left-up */
    BK_AVRCP_PT_CMD_LEFT_DOWN      =   0x08,    /*!< left-down */
    BK_AVRCP_PT_CMD_ROOT_MENU      =   0x09,    /*!< root menu */
    BK_AVRCP_PT_CMD_SETUP_MENU     =   0x0A,    /*!< setup menu */
    BK_AVRCP_PT_CMD_CONT_MENU      =   0x0B,    /*!< contents menu */
    BK_AVRCP_PT_CMD_FAV_MENU       =   0x0C,    /*!< favorite menu */
    BK_AVRCP_PT_CMD_EXIT           =   0x0D,    /*!< exit */
    BK_AVRCP_PT_CMD_0              =   0x20,    /*!< 0 */
    BK_AVRCP_PT_CMD_1              =   0x21,    /*!< 1 */
    BK_AVRCP_PT_CMD_2              =   0x22,    /*!< 2 */
    BK_AVRCP_PT_CMD_3              =   0x23,    /*!< 3 */
    BK_AVRCP_PT_CMD_4              =   0x24,    /*!< 4 */
    BK_AVRCP_PT_CMD_5              =   0x25,    /*!< 5 */
    BK_AVRCP_PT_CMD_6              =   0x26,    /*!< 6 */
    BK_AVRCP_PT_CMD_7              =   0x27,    /*!< 7 */
    BK_AVRCP_PT_CMD_8              =   0x28,    /*!< 8 */
    BK_AVRCP_PT_CMD_9              =   0x29,    /*!< 9 */
    BK_AVRCP_PT_CMD_DOT            =   0x2A,    /*!< dot */
    BK_AVRCP_PT_CMD_ENTER          =   0x2B,    /*!< enter */
    BK_AVRCP_PT_CMD_CLEAR          =   0x2C,    /*!< clear */
    BK_AVRCP_PT_CMD_CHAN_UP        =   0x30,    /*!< channel up */
    BK_AVRCP_PT_CMD_CHAN_DOWN      =   0x31,    /*!< channel down */
    BK_AVRCP_PT_CMD_PREV_CHAN      =   0x32,    /*!< previous channel */
    BK_AVRCP_PT_CMD_SOUND_SEL      =   0x33,    /*!< sound select */
    BK_AVRCP_PT_CMD_INPUT_SEL      =   0x34,    /*!< input select */
    BK_AVRCP_PT_CMD_DISP_INFO      =   0x35,    /*!< display information */
    BK_AVRCP_PT_CMD_HELP           =   0x36,    /*!< help */
    BK_AVRCP_PT_CMD_PAGE_UP        =   0x37,    /*!< page up */
    BK_AVRCP_PT_CMD_PAGE_DOWN      =   0x38,    /*!< page down */
    BK_AVRCP_PT_CMD_POWER          =   0x40,    /*!< power */
    BK_AVRCP_PT_CMD_VOL_UP         =   0x41,    /*!< volume up */
    BK_AVRCP_PT_CMD_VOL_DOWN       =   0x42,    /*!< volume down */
    BK_AVRCP_PT_CMD_MUTE           =   0x43,    /*!< mute */
    BK_AVRCP_PT_CMD_PLAY           =   0x44,    /*!< play */
    BK_AVRCP_PT_CMD_STOP           =   0x45,    /*!< stop */
    BK_AVRCP_PT_CMD_PAUSE          =   0x46,    /*!< pause */
    BK_AVRCP_PT_CMD_RECORD         =   0x47,    /*!< record */
    BK_AVRCP_PT_CMD_REWIND         =   0x48,    /*!< rewind */
    BK_AVRCP_PT_CMD_FAST_FORWARD   =   0x49,    /*!< fast forward */
    BK_AVRCP_PT_CMD_EJECT          =   0x4A,    /*!< eject */
    BK_AVRCP_PT_CMD_FORWARD        =   0x4B,    /*!< forward */
    BK_AVRCP_PT_CMD_BACKWARD       =   0x4C,    /*!< backward */
    BK_AVRCP_PT_CMD_ANGLE          =   0x50,    /*!< angle */
    BK_AVRCP_PT_CMD_SUBPICT        =   0x51,    /*!< subpicture */
    BK_AVRCP_PT_CMD_F1             =   0x71,    /*!< F1 */
    BK_AVRCP_PT_CMD_F2             =   0x72,    /*!< F2 */
    BK_AVRCP_PT_CMD_F3             =   0x73,    /*!< F3 */
    BK_AVRCP_PT_CMD_F4             =   0x74,    /*!< F4 */
    BK_AVRCP_PT_CMD_F5             =   0x75,    /*!< F5 */
    BK_AVRCP_PT_CMD_VENDOR         =   0x7E,    /*!< vendor unique */
} bk_avrcp_pt_cmd_t;

/// AVRCP passthrough command state
typedef enum {
    BK_AVRCP_PT_CMD_STATE_PRESSED = 0,           /*!< key pressed */
    BK_AVRCP_PT_CMD_STATE_RELEASED = 1           /*!< key released */
} bk_avrcp_pt_cmd_state_t;


/// AVRC media attribute id
typedef enum {
    BK_AVRCP_MEDIA_ATTR_ID_TITLE = 1,                  /*!< title of the playing track */
    BK_AVRCP_MEDIA_ATTR_ID_ARTIST,                 /*!< track artist */
    BK_AVRCP_MEDIA_ATTR_ID_ALBUM,                  /*!< album name */
    BK_AVRCP_MEDIA_ATTR_ID_TRACK_NUM,              /*!< track position on the album */
    BK_AVRCP_MEDIA_ATTR_ID_NUM_TRACKS,             /*!< number of tracks on the album */
    BK_AVRCP_MEDIA_ATTR_ID_GENRE,                  /*!< track genre */
    BK_AVRCP_MEDIA_ATTR_ID_PLAYING_TIME,           /*!< total album playing time in miliseconds */
    BK_AVRCP_MEDIA_ATTR_ID_DEFAULT_COVER_ART,      ///BIP image
} bk_avrcp_media_attr_id_t;

/// AVRC player setting ids
typedef enum {
    BK_AVRCP_PS_EQUALIZER = 0x01,                 /*!< equalizer, on or off */
    BK_AVRCP_PS_REPEAT_MODE = 0x02,               /*!< repeat mode */
    BK_AVRCP_PS_SHUFFLE_MODE = 0x03,              /*!< shuffle mode */
    BK_AVRCP_PS_SCAN_MODE = 0x04,                 /*!< scan mode on or off */
    BK_AVRCP_PS_MAX_ATTR
} bk_avrcp_ps_attr_ids_t;

/// AVRCP notification parameters
typedef union
{
    uint8_t volume;                          /*!< response data for BK_AVRCP_RN_VOLUME_CHANGE, ranges 0..127 */
    bk_avrcp_playback_stat_t playback;       /*!< response data for BK_AVRCP_RN_PLAY_STATUS_CHANGE */
    uint8_t elm_id[8];                       /*!< response data for BK_AVRCP_RN_TRACK_CHANGE */
    uint32_t play_pos;                       /*!< response data for BK_AVRCP_RN_PLAY_POS_CHANGED, in millisecond */
    bk_avrcp_batt_stat_t batt;               /*!< response data for BK_AVRCP_RN_BATTERY_STATUS_CHANGE */

    struct                                   /*!< response data for BK_AVRCP_RN_ADDRESSED_PLAYER_CHANGE */
    {
        uint16_t player_id;
        uint16_t uid;
    };

    uint16_t uid_counter;                    /*!< response data for BK_AVRCP_RN_UIDS_CHANGE */
    uint8_t system_status;                   /*!< response data for BK_AVRCP_RN_SYSTEM_STATUS_CHANGE */

    struct                                   /*!< response data for BK_AVRCP_RN_APP_SETTING_CHANGE */
    {
        uint8_t player_app_set_count;
        struct
        {
            uint8_t attr_id;
            uint8_t value_id;
        }player_app_set_array[BK_AVRCP_PS_MAX_ATTR];
    };

} bk_avrcp_rn_param_t;

/// AVRCP target notification event capability bit mask
typedef struct {
    uint16_t bits;                                /*!< bit mask representation of supported event_ids */
} bk_avrcp_rn_evt_cap_mask_t;

/// AVRCP controller callback parameters
typedef union {
    /**
     * @brief BK_AVRCP_CT_CONNECTION_STATE_EVT
     */
    struct avrcp_ct_conn_state_param {
        uint8_t connected;                          /*!< whether AVRCP connection is set up */
        uint8_t remote_bda[6];                /*!< remote bluetooth device address */
    } conn_state;                                 /*!< AVRCP connection status */

    /**
     * @brief BK_AVRCP_CT_PASSTHROUGH_RSP_EVT
     */
    struct avrcp_ct_psth_rsp_param {
        uint8_t tl;                              /*!< transaction label, 0 to 15 */
        uint8_t key_code;                        /*!< passthrough command code, see bk_avrcp_pt_cmd_t */
        uint8_t key_state;                       /*!< 0 for PRESSED, 1 for RELEASED */
        bk_avrcp_rsp_t rsp_code;                 /*!< response code */
        uint8_t remote_bda[6];                /*!< remote bluetooth device address */
    } psth_rsp;                                  /*!< passthrough command response */

    /**
     * @brief BK_AVRCP_CT_CHANGE_NOTIFY_EVT
     */
    struct avrcp_ct_change_notify_param {
        uint8_t event_id;                        /*!< id of AVRCP event notification */
        bk_avrcp_rn_param_t event_parameter;     /*!< event notification parameter */
        uint8_t remote_bda[6];                /*!< remote bluetooth device address */
    } change_ntf;                                /*!< notifications */

    /**
     * @brief BK_AVRCP_CT_GET_RN_CAPABILITIES_RSP_EVT
     */
    struct avrcp_ct_get_rn_caps_rsp_param {
        uint8_t cap_count;                       /*!< number of items provided in event or company_id according to cap_id used */
        bk_avrcp_rn_evt_cap_mask_t evt_set;      /*!< supported event_ids represented in bit-mask */
        uint8_t remote_bda[6];                /*!< remote bluetooth device address */
    } get_rn_caps_rsp;                           /*!< get supported event capabilities response from AVRCP target */

    /**
     * @brief BK_AVRCP_CT_SET_ABSOLUTE_VOLUME_RSP_EVT
     */
    struct avrcp_ct_set_volume_rsp_param {
        uint8_t status;                          /// 0 means success
        uint8_t volume;                          /*!< the volume which has actually been set, range is 0 to 0x7f, means 0% to 100% */
        uint8_t remote_bda[6];                /*!< remote bluetooth device address */
    } set_volume_rsp;                            /*!< set absolute volume response event */

    /**
     * @brief BK_AVRCP_CT_SET_PLAYER_APP_SETTING_RSP_EVT
     */
    struct avrcp_ct_set_player_app_setting_rsp_param {
        uint8_t status;                          /// 0 means success
        uint8_t remote_bda[6];                /*!< remote bluetooth device address */
    } set_player_app_setting_rsp;                            /*!< set player app setting response event */

    /**
     * @brief BK_AVRCP_CT_GET_ELEM_ATTR_RSP_EVT
     */
    struct avrcp_ct_elem_attr_rsp_param {
        uint8_t status;                          /// 0 means success
        uint8_t attr_count;                      ///attr_array count
        struct
        {
            uint32_t attr_id;                         /*!< elem attribute id, see bk_avrcp_media_attr_id_t */
            uint16_t attr_text_charset;              /// attr text charset, see http://www.iana.org/assignments/character-sets
            uint32_t attr_length;                         /*!< attribute character length */
            uint8_t *attr_text;                      /*!< attribute itself */
        }*attr_array;                                ///elem array
        uint8_t remote_bda[6];                /*!< remote bluetooth device address */
    } elem_attr_rsp;                                  /*!< metadata attributes response */

} bk_avrcp_ct_cb_param_t;



/// AVRC notification response type
typedef enum
{
    BK_AVRCP_RN_RSP_INTERIM,     /*!< initial response to RegisterNotification, should be sent T_mtp(1000ms) from receiving the command */
    BK_AVRCP_RN_RSP_CHANGED,     /*!< final response to RegisterNotification command */
} bk_avrcp_rn_rsp_t;

/// AVRC target notification event notification capability
typedef enum
{
    BK_AVRCP_RN_CAP_API_METHOD_ALLOWED = 0,              /*!< all of the notification events that can possibly be supported, immutable */
    BK_AVRCP_RN_CAP_API_METHOD_CURRENT_ENABLE = 1,            /*!< notification events selectively supported according to the current configuration */
    BK_AVRCP_RN_CAP_API_METHOD_MAX,
} bk_avrcp_rn_cap_api_method_t;

/// AVRC passthrough command filter
typedef enum {
    BK_AVRCP_PSTH_FILTER_METHOD_ALLOWED,       /*!< all of the PASSTHROUGH commands that can possibly be used, immutable */
    BK_AVRCP_PSTH_FILTER_METHOD_CURRENT_ENABLE,     /*!< PASSTHROUGH commands selectively supported according to the current configuration */
    BK_AVRCP_PSTH_FILTER_METHOD_MAX,
} bk_avrcp_psth_filter_t;

/// AVRC passthrough command bit mask
typedef struct {
    uint16_t bits[8];                           /*!< bit mask representation of PASSTHROUGH commands */
} bk_avrcp_psth_bit_mask_t;

typedef enum {
    BK_AVRCP_BIT_MASK_OP_TEST = 0,      /*!< operation code to test a specific bit */
    BK_AVRCP_BIT_MASK_OP_SET = 1,       /*!< operation code to set a specific bit  */
    BK_AVRCP_BIT_MASK_OP_CLEAR = 2,     /*!< operation code to clear a specific bit */
} bk_avrcp_bit_mask_op_t;

/// AVRC Target callback events
typedef enum
{
    BK_AVRCP_TG_CONNECTION_STATE_EVT,          /*!< connection state changed event */
    BK_AVRCP_TG_PASSTHROUGH_CMD_EVT,           /*!< passthrough command event */
    BK_AVRCP_TG_SET_ABSOLUTE_VOLUME_CMD_EVT,   /*!< set absolute volume command from remote device */
    BK_AVRCP_TG_REGISTER_NOTIFICATION_EVT,     /*!< register notification event */
} bk_avrcp_tg_cb_event_t;

/// AVRC target callback parameters
typedef union
{
    /**
     * @brief BK_AVRCP_TG_CONNECTION_STATE_EVT
     */
    struct avrcp_tg_conn_stat_param
    {
        bool connected;                          /*!< whether AVRC connection is set up */
        uint8_t remote_bda[6];                /*!< remote bluetooth device address */
    } conn_stat;                                 /*!< AVRC connection status */

    /**
     * @brief BK_AVRCP_TG_PASSTHROUGH_CMD_EVT
     */
    struct avrcp_tg_psth_cmd_param
    {
        uint8_t key_code;                        /*!< passthrough command code */
        uint8_t key_state;                       /*!< 0 for PRESSED, 1 for RELEASED */
        uint8_t remote_bda[6];                /*!< remote bluetooth device address */
    } psth_cmd;                                  /*!< passthrough command */

    /**
     * @brief BK_AVRCP_TG_SET_ABSOLUTE_VOLUME_CMD_EVT
     */
    struct avrcp_tg_set_abs_vol_param
    {
        uint8_t volume;                          /*!< volume ranges from 0 to 127 */
        uint8_t remote_bda[6];                /*!< remote bluetooth device address */
    } set_abs_vol;                               /*!< set absolute volume command targeted on audio sink */

    /**
     * @brief BK_AVRCP_TG_REGISTER_NOTIFICATION_EVT
     */
    struct avrcp_tg_reg_ntf_param
    {
        uint8_t event_id;                        /*!< event id of bk_avrcp_rn_event_ids_t */
        uint32_t event_parameter;                /*!< event notification parameter, used with BK_AVRCP_RN_PLAY_POS_CHANGED only now, means timer in second */
        uint8_t remote_bda[6];                /*!< remote bluetooth device address */
    } reg_ntf;                                   /*!< register notification */

} bk_avrcp_tg_cb_param_t;


/**
 * @brief           AVRCP controller callback function type
 *
 * @param           event : Event type
 *
 * @param           param : Pointer to callback parameter union
 */
typedef void (* bk_avrcp_ct_cb_t)(bk_avrcp_ct_cb_event_t event, bk_avrcp_ct_cb_param_t *param);


/**
 * @brief           AVRCP target callback function type
 *
 * @param           event : Event type
 *
 * @param           param : Pointer to callback parameter union
 */
typedef void (* bk_avrcp_tg_cb_t)(bk_avrcp_tg_cb_event_t event, bk_avrcp_tg_cb_param_t *param);
///@}

#ifdef __cplusplus
}
#endif


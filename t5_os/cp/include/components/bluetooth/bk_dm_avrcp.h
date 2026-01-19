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

#include "bk_dm_bluetooth_types.h"
#include "bk_dm_avrcp_types.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup dm_avrcp AVRCP API
 * @{
 */

/**
 *
 * @brief           Initialize the bluetooth AVRCP controller module
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 *
 */
bt_err_t bk_bt_avrcp_ct_init(void);

/**
 *
 * @brief           Deinitialize the bluetooth AVRCP controller module
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 *
 */
bt_err_t bk_bt_avrcp_ct_deinit(void);

/**
 * @brief           Register application callbacks to AVRCP module.
 *
 * @param[in]       callback: AVRCP controller callback function
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 *
 */
bt_err_t bk_bt_avrcp_ct_register_callback(bk_avrcp_ct_cb_t callback);

/**
 * @brief           Send GetCapabilities PDU to AVRCP target to retrieve remote device's supported
 *                  notification event_ids. This function should be called after
 *                  BK_AVRCP_CT_CONNECTION_STATE_EVT is received and AVRCP connection is established.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 */
bt_err_t bk_bt_avrcp_ct_send_get_rn_capabilities_cmd(uint8_t *remote_bda);

/**
 * @brief           Send register notification command to AVRCP target. This function should be called after
 *                  BK_AVRCP_CT_CONNECTION_STATE_EVT is received and AVRCP connection is established.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 *
 * @param[in]       event_id : id of events, e.g. BK_AVRCP_RN_PLAY_STATUS_CHANGE, BK_AVRCP_RN_TRACK_CHANGE, etc.
 *
 * @param[in]       event_parameter : playback interval for BK_AVRCP_RN_PLAY_POS_CHANGED in second;
 *                                    For other events , value of this parameter is ignored.
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 */
bt_err_t bk_bt_avrcp_ct_send_register_notification_cmd(uint8_t *remote_bda, uint8_t event_id, uint32_t event_parameter);

/**
 * @brief           Send passthrough command to AVRCP target. This function should be called after
 *                  BK_AVRCP_CT_CONNECTION_STATE_EVT is received and AVRCP connection is established.
 *
 * @param[in]       remote_bda: remote bluetooth device address.
 *
 * @param[in]       key_code : passthrough command code, e.g. BK_AVRCP_PT_CMD_PLAY, BK_AVRCP_PT_CMD_STOP, etc.
 *
 * @param[in]       key_state : passthrough command key state, BK_AVRCP_PT_CMD_STATE_PRESSED or
 *                  BK_AVRCP_PT_CMD_STATE_RELEASED
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 */
bt_err_t bk_bt_avrcp_ct_send_passthrough_cmd(uint8_t *remote_bda, uint8_t key_code, uint8_t key_state);

/**
 * @brief           Send set absolute volume command to AVRCP target. This function should be called after
 *                  BK_AVRCP_CT_CONNECTION_STATE_EVT is received and AVRCP connection is established.
 *
 * @param[in]       remote_bda: remote bluetooth device address.
 *
 * @param[in]       volume : volume, 0 to 0x7f, means 0% to 100%
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 */
bt_err_t bk_bt_avrcp_ct_send_absolute_volume_cmd(uint8_t *remote_bda, uint8_t volume);


/**
 *
 * @brief           Send player application settings command to AVRCP target. This function should be called
 *                  after BK_AVRCP_CT_CONNECTION_STATE_EVT is received and AVRCP connection is established.
 *
 * @param[in]       remote_bda: remote bluetooth device address.
 * @param[in]       attr_id : player application setting attribute IDs from one of bk_avrcp_ps_attr_ids_t
 * @param[in]       value_id : attribute value defined for the specific player application setting attribute
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 */
bt_err_t bk_bt_avrcp_ct_send_set_player_app_setting_cmd(uint8_t *remote_bda, uint8_t attr_id, uint8_t value_id);


/**
 * @brief           Send get media attr command to AVRCP target. This function should be called after
 *                  BK_AVRCP_CT_CONNECTION_STATE_EVT is received and AVRCP connection is established.
 *
 * @param[in]       remote_bda: remote bluetooth device address.
 * @param[in]       media_attr_id_mask : mask of bk_avrcp_media_attr_id_t, such as (1 << BK_AVRCP_MEDIA_ATTR_ID_TITLE) | (1 << BK_AVRCP_MEDIA_ATTR_ID_ARTIST).
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 */
bt_err_t bk_bt_avrcp_ct_send_get_elem_attribute_cmd(uint8_t *remote_bda, uint32_t media_attr_id_mask);

/**
 *
 * @brief           Initialize the bluetooth AVRCP target module
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 *
 */
bt_err_t bk_bt_avrcp_tg_init(void);


/**
 *
 * @brief           Deinitialize the bluetooth AVRCP target module
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 *
 */
bt_err_t bk_bt_avrcp_tg_deinit(void);

/**
 * @brief           Register application callbacks to AVRCP target module.
 *
 * @param[in]       callback: AVRCP target callback function
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 *
 */
bt_err_t bk_bt_avrcp_tg_register_callback(bk_avrcp_tg_cb_t callback);

/**
 *
 * @brief           Get the requested event notification capabilies on local AVRC target. The capability is returned
 *                  in a bit mask representation in evt_set. This function should be called after bk_avrcp_tg_init().
 *
 *                  For capability type "BK_AVRCP_RN_CAP_API_METHOD_ALLOWED, the retrieved event set is constant and
 *                  it covers all of the notification events that can possibly be supported with current
 *                  implementation.
 *
 *                  For capability type BK_AVRCP_RN_CAP_API_METHOD_CURRENT_ENABLE, the event set covers the notification
 *                  events selected to be supported under current configuration, The configuration can be
 *                  changed using bk_avrcp_tg_set_rn_evt_cap().
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 */
bt_err_t bk_bt_avrcp_tg_get_rn_evt_cap(bk_avrcp_rn_cap_api_method_t cap, bk_avrcp_rn_evt_cap_mask_t *evt_set);

/**
 *
 * @brief           Set the event notification capabilities on local AVRCP target. The capability is given in a
 *                  bit mask representation in evt_set and must be a subset of allowed event IDs with current
 *                  implementation. This function should be called after bk_avrcp_tg_init().
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 *
 */
bt_err_t bk_bt_avrcp_tg_set_rn_evt_cap(const bk_avrcp_rn_evt_cap_mask_t *evt_set);


/**
 * @brief           Operate on the type bk_avrcp_rn_evt_cap_mask_t with regard to a specific event.
 *
 * @param[in]       op: operation requested on the bit mask field
 *
 * @param[in]       events: pointer to event notification capability bit mask structure
 *
 * @param[in]       event_id: notification event code
 *
 * @return          For operation BK_AVRCP_BIT_MASK_OP_SET or BK_AVRCP_BIT_MASK_OP_CLEAR, return
 *                  true for a successful operation, otherwise return false.
 *
 *                  For operation BK_AVRCP_BIT_MASK_OP_TEST, return true if the corresponding bit
 *                  is set, otherwise false.
 *
 */
bool bk_bt_avrcp_rn_evt_bit_mask_operation(bk_avrcp_bit_mask_op_t op, bk_avrcp_rn_evt_cap_mask_t *events, bk_avrcp_rn_event_ids_t event_id);

/**
 *
 * @brief           Send RegisterNotification Response to remote AVRCP controller. Local event notification
 *                  capability can be set using bk_avrcp_tg_set_rn_evt_cap(), in a bit mask representation
 *                  in evt_set. This function should be called after bk_avrcp_tg_init().
 *
 * @param[in]       addr: peer addr
 *
 * @param[in]       event_id: notification event ID that remote AVRCP CT registers
 *
 * @param[in]       rsp: notification response code
 *
 * @param[in]       param: parameters included in the specific notification
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 *
 */
bt_err_t bk_bt_avrcp_tg_send_rn_rsp(uint8_t *addr, bk_avrcp_rn_event_ids_t event_id, bk_avrcp_rn_rsp_t rsp,
                                    bk_avrcp_rn_param_t *param);


/**
 *
 * @brief           Get the current filter of remote passthrough commands on AVRC target. Filter is given by
 *                  filter type and bit mask for the passthrough commands. This function should be called
 *                  after bk_bt_avrcp_tg_init().
 *                  For filter type BK_AVRCP_PSTH_FILTER_METHOD_ALLOWED, the retrieved command set is constant and
 *                  it covers all of the passthrough commands that can possibly be supported.
 *                  For filter type BK_AVRCP_PSTH_FILTER_METHOD_CURRENT_ENABLE, the retrieved command set covers the
 *                  passthrough commands selected to be supported according to current configuration. The
 *                  configuration can be changed using bk_bt_avrcp_tg_set_psth_cmd_filter().
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 */
bt_err_t bk_bt_avrcp_tg_get_psth_cmd_filter(bk_avrcp_psth_filter_t filter, bk_avrcp_psth_bit_mask_t *cmd_set);

/**
 *
 * @brief           Set the filter of remote passthrough commands on AVRC target. Filter is given by
 *                  filter type and bit mask for the passthrough commands. This function should be called
 *                  after bk_bt_avrcp_tg_init().
 *
 *                  If filter type is BK_AVRCP_PSTH_FILTER_METHOD_ALLOWED, the passthrough commands which
 *                  are set "1" as given in cmd_set will generate BK_AVRCP_CT_PASSTHROUGH_RSP_EVT callback
 *                  event and are auto-accepted in the protocol stack, other commands are replied with response
 *                  type "NOT IMPLEMENTED" (8). The set of supported commands should be a subset of allowed
 *                  command set. The allowed command set can be retrieved using bk_bt_avrcp_tg_get_psth_cmd_filter()
 *                  with filter type "BK_AVRCP_PSTH_FILTER_METHOD_CURRENT_ENABLE".
 *
 *                  Filter type "BK_AVRCP_PSTH_FILTER_METHOD_ALLOWED" does not apply to this function.
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 *
 */
bt_err_t bk_bt_avrcp_tg_set_psth_cmd_filter(bk_avrcp_psth_filter_t filter, const bk_avrcp_psth_bit_mask_t *cmd_set);

/**
 * @brief           Operate on the type bk_avrcp_psth_bit_mask_t with regard to a specific PASSTHROUGH command.
 *
 * @param[in]       op: operation requested on the bit mask field
 *
 * @param[in]       psth: pointer to passthrough command bit mask structure
 *
 * @param[in]       cmd: passthrough command code
 *
 * @return          For operation BK_AVRCP_BIT_MASK_OP_SET or BK_AVRCP_BIT_MASK_OP_CLEAR, return
 *                  true for a successful operation, otherwise return false.
 *                  For operation BK_AVRCP_BIT_MASK_OP_TEST, return true if the corresponding bit
 *                  is set, otherwise false.
 *
 */
bool bk_bt_avrcp_psth_bit_mask_operation(bk_avrcp_bit_mask_op_t op, bk_avrcp_psth_bit_mask_t *psth, bk_avrcp_pt_cmd_t cmd);

/**
 *
 * @brief           Connect to remote bluetooth AVRCP. This API must be called after
 *                  bk_bt_avrcp_ct_init() or bk_bt_avrcp_tg_init.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  -  others: fail
 *
 */
bt_err_t bk_bt_avrcp_connect(uint8_t *remote_bda);

///@}

#ifdef __cplusplus
}
#endif



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
#include "bk_dm_hfp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief           Register application callback function to HFP client module.
*
* @param[in]       callback: HFP client event callback function
*
* @return
*                  - BK_ERR_BT_SUCCESS: success
*                  - others: fail
*/
bt_err_t bk_bt_hf_client_register_callback(bk_bt_hf_client_cb_t callback);

/**
 *
 * @brief     Initialize the bluetooth HFP client module.
 *
 * @param[in]   msbc_supported: 1 means msbc is supported, 0 means msbc is not supported.
 *
 * @return
 *            - BK_ERR_BT_SUCCESS: the initialization request is successfully
 *            - others: fail
 */
bt_err_t bk_bt_hf_client_init(uint8_t msbc_supported);

/**
 *
 * @brief     De-initialize for HFP client module.
 *
 * @return
 *            - BK_ERR_BT_SUCCESS: success
 *            - others: fail
 */
bt_err_t bk_bt_hf_client_deinit(void);

/**
 * @brief           Register HFP client data input function; 
 *
 * @param[in]       callback: HFP client incoming data callback function
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: the initialization request is successfully
 *                  - others: fail
 */
bt_err_t bk_bt_hf_client_register_data_callback(bk_bt_hf_client_data_cb_t callback);

/**
 * @brief           Send voice data to HFP AG; 
 *
 * @param[in]       remote_bda: remote bluetooth device address
 *
 * @param[in]       buf : pointer to the data
 *
 * @param[in]       len : size(in bytes) in buf
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: the initialization request is successfully
 *                  - others: fail
 */
bt_err_t bk_bt_hf_client_voice_out_write(uint8_t *remote_bda, uint8_t *buf, uint16_t len);

/**
 *
 * @brief           Establish a Service Level Connection to remote bluetooth HFP audio gateway(AG) device.
 *                  This function must be called after bk_bt_hf_client_init().
 *
 * @param[in]       remote_bda: remote bluetooth device address
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: connect request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_client_connect(uint8_t *remote_bda);

/**
 *
 * @brief           Disconnect from the remote HFP audio gateway.
 *                  This function must be called after bk_bt_hf_client_init().
 *
 * @param[in]       remote_bda: remote bluetooth device address
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: connect request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_client_disconnect(uint8_t *remote_bda);

/**
 *
 * @brief           Create audio connection with remote HFP AG.
 *                  As a precondition to use this API, Service Level Connection shall exist with AG.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 * @return
 *                  - BK_ERR_BT_SUCCESS: connect request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_client_connect_audio(uint8_t *remote_bda);

/**
 *
 * @brief           Release the established audio connection with remote HFP AG.
 *                  As a precondition to use this API, Service Level Connection shall exist with AG.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 * @return
 *                  - BK_ERR_BT_SUCCESS: disconnect request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_client_disconnect_audio(uint8_t *remote_bda);

/**
 *
 * @brief           Enable voice recognition in the AG.
 *                  As a precondition to use this API, Service Level Connection shall exist with AG.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_client_start_voice_recognition(uint8_t *remote_bda);

/**
 *
 * @brief           Disable voice recognition in the AG.
 *                  As a precondition to use this API, Service Level Connection shall exist with AG.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_client_stop_voice_recognition(uint8_t *remote_bda);

/**
 *
 * @brief           Volume synchronization with AG.
 *                  As a precondition to use this API, Service Level Connection shall exist with AG.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 * @param[in]       type: volume control target, speaker or microphone
 * @param[in]       volume: gain of the speaker of microphone, ranges 0 to 15
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_client_volume_update(uint8_t *remote_bda, bk_hf_volume_control_target_t type, uint8_t volume);

/**
 *
 * @brief           Place a call with a specified number, if number is NULL, last called number is called.
 *                  As a precondition to use this API, Service Level Connection shall exist with AG.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 * @param[in]       number: number string of the call. If NULL, the last number is called(aka re-dial)
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_client_dial(uint8_t *remote_bda, const char *number);

/**
 *
 * @brief           Place a call with number specified by location(speed dial).
 *                  As a precondition to use this API, Service Level Connection shall exist with AG.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 * @param[in]       location: location of the number in the memory
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_client_dial_memory(uint8_t *remote_bda, int location);

/**
 *
 * @brief           Send call hold and multiparty commands(Use AT+CHLD).
 *                  As a precondition to use this API, Service Level Connection shall exist with AG.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 * @param[in]       chld: AT+CHLD call hold and multiparty handling AT command.
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_client_send_chld_cmd(uint8_t *remote_bda, bk_hf_chld_type_t chld);

/**
 *
 * @brief           Send response and hold action command(Send AT+BTRH command)
 *                  As a precondition to use this API, Service Level Connection shall exist with AG.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 * @param[in]       btrh: response and hold action to send
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_client_send_btrh_cmd(uint8_t *remote_bda, bk_hf_btrh_cmd_t btrh);

/**
 *
 * @brief           Answer an incoming call(send ATA command).
 *                  As a precondition to use this API, Service Level Connection shall exist with AG.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_client_answer_call(uint8_t *remote_bda);

/**
 *
 * @brief           Reject an incoming call(send AT+CHUP command).
 *                  As a precondition to use this API, Service Level Connection shall exist with AG.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_client_reject_call(uint8_t *remote_bda);

/**
 *
 * @brief           Query list of current calls in AG(send AT+CLCC command).
 *                  As a precondition to use this API, Service Level Connection shall exist with AG.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_client_query_current_calls(uint8_t *remote_bda);

/**
 *
 * @brief           Query the name of currently selected network operator in AG(use AT+COPS commands).
 *                  As a precondition to use this API, Service Level Connection shall exist with AG.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_client_query_current_operator_name(uint8_t *remote_bda);

/**
 *
 * @brief           Get subscriber information number from AG(send AT+CNUM command)
 *                  As a precondition to use this API, Service Level Connection shall exist with AG
 *
 * @param[in]       remote_bda: remote bluetooth device address
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_client_retrieve_subscriber_info(uint8_t *remote_bda);

/**
 *
 * @brief           Transmit DTMF codes during an ongoing call(use AT+VTS commands)
 *                  As a precondition to use this API, Service Level Connection shall exist with AG.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 * @param[in]       code: string of the dtmf code
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_client_send_dtmf(uint8_t *remote_bda, const char *code);

/**
 *
 * @brief           Request a phone number from AG corresponding to last voice tag recorded (send AT+BINP command).
 *                  As a precondition to use this API, Service Level Connection shall exist with AG.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_client_request_last_voice_tag_number(uint8_t *remote_bda);

/**
 *
 * @brief           Disable echo cancellation and noise reduction in the AG (use AT+NREC=0 command).
 *                  As a precondition to use this API, Service Level Connection shall exist with AG
 *
 * @param[in]       remote_bda: remote bluetooth device address
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_client_send_nrec(uint8_t *remote_bda);

/**
 *
 * @brief           This API allows user to redial the last number dialed (use AT+BLDN command).
 *                  As a precondition to use this API, Service Level Connection shall exist with AG
 *
 * @param[in]       remote_bda: remote bluetooth device address
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_client_redial(uint8_t *remote_bda);

/**
 *
 * @brief           This API allows user to send custom AT Command.
 *                  As a precondition to use this API, Service Level Connection shall exist with AG.
 *
 * @param[in]       remote_bda: remote bluetooth device address
 * @param[in]       atcmd: string of the custom atcmd.
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: request is sent to lower layer
 *                  - others: fail
 */
bt_err_t bk_bt_hf_client_send_custom_cmd(uint8_t *remote_bda, const char *atcmd);

#ifdef __cplusplus
}
#endif


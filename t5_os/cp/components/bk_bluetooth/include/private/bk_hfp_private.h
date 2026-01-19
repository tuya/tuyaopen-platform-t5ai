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

#include "components/bluetooth/bk_dm_hfp.h"

#ifdef __cplusplus
extern "C" {
#endif

bt_err_t bt_hf_client_register_callback_internal(bk_bt_hf_client_cb_t callback);
bt_err_t bt_hf_client_init_internal(uint8_t msbc_supported);
bt_err_t bt_hf_client_deinit_internal(void);
bt_err_t bt_hf_client_register_data_callback_internal(bk_bt_hf_client_data_cb_t callback);
bt_err_t bt_hf_client_voice_out_write_internal(uint8_t *remote_bda, uint8_t *data, uint16_t len);
bt_err_t bt_hf_client_connect_internal(uint8_t *remote_bda);
bt_err_t bt_hf_client_disconnect_internal(uint8_t *remote_bda);
bt_err_t bt_hf_client_connect_audio_internal(uint8_t *remote_bda);
bt_err_t bt_hf_client_disconnect_audio_internal(uint8_t *remote_bda);
bt_err_t bt_hf_client_start_voice_recognition_internal(uint8_t *remote_bda);
bt_err_t bt_hf_client_stop_voice_recognition_internal(uint8_t *remote_bda);
bt_err_t bt_hf_client_volume_update_internal(uint8_t *remote_bda, bk_hf_volume_control_target_t type, uint8_t volume);
bt_err_t bt_hf_client_dial_internal(uint8_t *remote_bda, const char *number);
bt_err_t bt_hf_client_dial_memory_internal(uint8_t *remote_bda, int location);
bt_err_t bt_hf_client_send_chld_cmd_internal(uint8_t *remote_bda, bk_hf_chld_type_t chld);
bt_err_t bt_hf_client_send_btrh_cmd_internal(uint8_t *remote_bda, bk_hf_btrh_cmd_t btrh);
bt_err_t bt_hf_client_answer_call_internal(uint8_t *remote_bda);
bt_err_t bt_hf_client_reject_call_internal(uint8_t *remote_bda);
bt_err_t bt_hf_client_query_current_calls_internal(uint8_t *remote_bda);
bt_err_t bt_hf_client_query_current_operator_name_internal(uint8_t *remote_bda);
bt_err_t bt_hf_client_retrieve_subscriber_info_internal(uint8_t *remote_bda);
bt_err_t bt_hf_client_send_dtmf_internal(uint8_t *remote_bda, const char *code);
bt_err_t bt_hf_client_request_last_voice_tag_number_internal(uint8_t *remote_bda);
bt_err_t bt_hf_client_send_nrec_internal(uint8_t *remote_bda);
bt_err_t bt_hf_client_redial_internal(uint8_t *remote_bda);
bt_err_t bt_hf_client_send_custom_cmd_internal(uint8_t *remote_bda, const char *atcmd);


#ifdef __cplusplus
}
#endif

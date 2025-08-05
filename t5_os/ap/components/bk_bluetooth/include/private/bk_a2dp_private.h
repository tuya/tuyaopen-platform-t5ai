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

#include "components/bluetooth/bk_dm_a2dp.h"

#ifdef __cplusplus
extern "C" {
#endif

bt_err_t bt_a2dp_register_callback_internal(bk_bt_a2dp_cb_t callback);
bt_err_t bt_a2dp_sink_init_internal(uint8_t aac_supported);
bt_err_t bt_a2dp_sink_deinit_internal(void);
bt_err_t bt_a2dp_sink_register_data_callback_internal(bk_bt_sink_data_cb_t callback);
bt_err_t bt_a2dp_sink_connect_internal(uint8_t *remote_bda);
bt_err_t bt_a2dp_sink_disconnect_internal(uint8_t *remote_bda);
bt_err_t bt_a2dp_sink_set_delay_value_internal(uint16_t delay_value);
bt_err_t bt_a2dp_sink_get_delay_value_internal(void);


#ifdef __cplusplus
}
#endif

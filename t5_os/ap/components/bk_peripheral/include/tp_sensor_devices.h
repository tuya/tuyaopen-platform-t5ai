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

#include "driver/tp_types.h"


#ifdef __cplusplus
extern "C" {
#endif


#if CONFIG_TP_GT911
    extern const tp_sensor_config_t tp_sensor_gt911;
#endif
#if CONFIG_TP_GT1151
    extern const tp_sensor_config_t tp_sensor_gt1151;
#endif
#if CONFIG_TP_FT6336
    extern const tp_sensor_config_t tp_sensor_ft6336;
#endif
#if CONFIG_TP_HY4633
    extern const tp_sensor_config_t tp_sensor_hy4633;
#endif
#if CONFIG_TP_CST816D
    extern const tp_sensor_config_t tp_sensor_cst816d;
#endif

void tp_sensor_devices_init(void);

#ifdef __cplusplus
}
#endif

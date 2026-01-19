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

#include <driver/int.h>
#include <os/mem.h>
#include <os/os.h>
#include <os/str.h>
#include "driver/tp.h"
#include "tp_sensor_devices.h"


const tp_sensor_config_t *tp_sensor_configs[] =
{
#if CONFIG_TP_GT911
	&tp_sensor_gt911,
#endif

#if CONFIG_TP_GT1151
	&tp_sensor_gt1151,
#endif

#if CONFIG_TP_FT6336
	&tp_sensor_ft6336,
#endif

#if CONFIG_TP_HY4633
	&tp_sensor_hy4633,
#endif

#if CONFIG_TP_CST816D
	&tp_sensor_cst816d,
#endif
};

void tp_sensor_devices_init(void)
{
	bk_tp_set_sensor_devices_list(&tp_sensor_configs[0], sizeof(tp_sensor_configs) / sizeof(tp_sensor_config_t *));
}




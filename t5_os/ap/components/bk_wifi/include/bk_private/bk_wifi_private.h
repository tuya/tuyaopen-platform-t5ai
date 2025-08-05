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

#include "bk_private/bk_wifi.h"
#include "bk_private/bk_wifi_types.h"


struct scan_cfg_scan_param_tag{
	u8 set_param;     /**< 1:indicates set scan param;0:not set scan para;*/
	u8 scan_type;     /**< passive scan:1, active scan:0*/
	u8 chan_cnt;     /**< scan channel cnt*/
	u8 chan_nb[WIFI_2BAND_MAX_CHAN_NUM];     /**< scan channel number 2.4g+5g*/
	u32 duration;     /**< scan duration,us*/
};
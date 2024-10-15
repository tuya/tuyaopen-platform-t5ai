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

#include <stdint.h>
#include "device.h"
#include "uart_hal.h"

#ifndef __ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

struct uart_dev_config {
    uint32_t id;
    uart_hal_t *hal;
    uart_config_t config;
};

struct uart_driver_api {
    int32_t (*poll_in)(struct device *dev, unsigned char *p_char);
    int32_t (*poll_rx_ready)(struct device *dev);
    unsigned char (*poll_out)(struct device *dev, unsigned char out_char);
};

#ifdef __cplusplus
}
#endif
#endif /* __ASSEMBLY__ */


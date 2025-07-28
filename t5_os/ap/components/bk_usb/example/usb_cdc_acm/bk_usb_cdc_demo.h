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

#include <components/log.h>
#include <components/usb.h>
#include <components/usb_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (CONFIG_CPU_CNT > 1)

#if (CONFIG_SYS_CPU1 && CONFIG_USB_CDC && CONFIG_USB_CDC_ACM_DEMO)
#define USB_CDC_CP1_IPC 1
#else
#define USB_CDC_CP1_IPC 0
#endif

#if (CONFIG_SYS_CPU0 && !CONFIG_USB_CDC && CONFIG_USB_CDC_ACM_DEMO)
#define USB_CDC_CP0_IPC 1
#else
#define USB_CDC_CP0_IPC 0
#endif

#endif

typedef struct {
	uint8_t  type;
	uint32_t data;
}cdc_msg_t;

typedef struct
{
	uint32_t port_idx;

	uint32_t tx_len;
	uint32_t tx_data;

	uint32_t rx_len;
	uint32_t rx_data;

	uint32_t cmd_len;
	uint32_t state;

	void (*bk_cdc_acm_bulkin_cb)(void);
	void (*bk_cdc_acm_bulkout_cb)(void * p);
}IPC_CDC_DATA_t;

typedef enum
{
	CDC_STATUS_IDLE,       //0
	CDC_STATUS_OPEN,       //1
	CDC_STATUS_INIT,       //2
	CDC_STATUS_OUT,        //3
	CDC_STATUS_STOP,       //4
	CDC_STATUS_REOPEN,     //5
	CDC_STATUS_CLOSE,      //6
	CDC_STATUS_ABNORMAL,   //7
	CDC_STATUS_EXIT,
} E_CDC_STATUS_T;


typedef struct {

	uint32_t port_idx;
	uint32_t port_status;

}bk_cdc_hub_status;



#ifdef __cplusplus
}
#endif

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

#define USB_CDC_ACM_DEV_NUM_MAX  (4)
#define USB_CDC_DATA_DEV_NUM_MAX (4)

#define CDC_TX_MAX_SIZE     512
#define CDC_RX_MAX_SIZE     512

#define CDC_EXTX_MAX_SIZE     2048

typedef enum
{
	CDC_STATUS_OPEN = 0,
	CDC_STATUS_CLOSE,
	CDC_STATUS_CONN,         /// 2
	CDC_STATUS_DISCON,       /// 3
	CDC_STATUS_BULKOUT_DATA,
	CDC_STATUS_BULKIN_DATA,
} E_CDC_STATUS_T;

typedef struct {
	uint8_t  type;
	uint32_t data;
	uint32_t *param;
}BK_USB_MODEM_CDC_MSG_T;

typedef enum
{
	/* cp0 ---> cp1 */
	CPU0_OPEN_USB_CDC = 0,
	CPU0_CLOSE_USB_CDC,
	CPU0_INIT_USB_CDC_PARAM,

	CPU0_BULKOUT_USB_CDC_CMD,
	CPU0_BULKOUT_USB_CDC_DATA,

	/* cp1 ---> cp0 */
	CPU1_UPDATE_USB_CDC_STATE,
	CPU1_UPLOAD_USB_CDC_CMD,   ///6
	CPU1_UPLOAD_USB_CDC_DATA,  ///7
}IPC_CDC_SUBMSG_TYPE_T;

typedef struct
{
	uint32_t dev_cnt;
	E_CDC_STATUS_T status;
}CDC_STATUS_t;

typedef struct{
  void (*bk_modem_usbh_conn_ind)(uint32_t cnt);
  void (*bk_modem_usbh_disconn_ind)(void);
  void (*bk_modem_usbh_bulkin_ind)(uint8_t *p_rx, uint32_t l_rx);
}bk_modem_usbh_if_t;

extern const bk_modem_usbh_if_t bk_modem_usbh_if;
void bk_cdc_acm_state_notify(CDC_STATUS_t * dev_state);
uint8_t *bk_usb_cdc_modem_get_rxpuf(void);
void bk_cdc_acm_bulkin_data(uint8_t *pbuf, uint16_t len);
void bk_usb_cdc_open(void);
void bk_usb_cdc_close(void);
void bk_usb_cdc_modem(void);
int32_t bk_cdc_acm_modem_write(char *p_tx, uint32_t l_tx);

#ifdef __cplusplus
}
#endif

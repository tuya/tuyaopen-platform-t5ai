#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "wdrv_main.h"
#include "wdrv_cntrl.h"

extern void wdrv_rx_handle_msg(wdrv_rx_msg *msg);
extern void wdrv_rxdata_process(struct pbuf *p);
extern uint8_t wdrv_recv_buffer(void *param, uint32_t *payload);
extern void wdrv_rx_confirm_tx_msg(wdrv_rx_msg *msg);

#ifdef __cplusplus
}
#endif

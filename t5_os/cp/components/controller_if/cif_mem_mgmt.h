#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "cif_mem_mgmt.h"

enum cmd_buf_type
{
    CMD_BUF_LONG = 0,
    CMD_BUF_SHORT = 1,
    CMD_BUF_MAX
};


extern struct cif_rx_bank_t * cif_rxbank_ptr;
extern uint8_t* cif_get_event_buffer(uint16_t size);
extern void cif_free_cmd_buffer(uint8_t* buf);
extern void cif_tx_event_buffer_init();
extern void cif_save_buffer_addr(void *node);
void cif_free_rx_buf(uint32_t buf);
extern void cif_free_ap_txbuf(struct pbuf * pbuf);
extern uint8_t* cif_maclloc_rx_buf();
void cif_rxbank_check();
uint8_t cif_get_event_short_buf_cnt();
uint8_t cif_get_event_long_buf_cnt();
#ifdef __cplusplus
}
#endif

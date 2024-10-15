/*
 * bl_tra_hcit.h
 *
 *  Created on: 2017-5-5
 *      Author: gang.cheng
 */

#ifndef BL_TRA_HCIT_H_
#define BL_TRA_HCIT_H_

#include <stdint.h>

/*
 * HCI transport type bytes
 */

enum
{
    TRA_HCIT_COMMAND = 1,
    TRA_HCIT_ACLDATA = 2,
    TRA_HCIT_SCODATA = 3,
    TRA_HCIT_EVENT   = 4
};


#define	BK_DEBUG_HCI_OPCODE1	0XE0
#define	BK_DEBUG_HCI_OPCODE2 	0XFC


/*
 * The state variable for the receive state machine and the
 * associated states used whilst decoding a received PDU from the host
 */
enum
    {
        TRA_HCIT_STATE_RX_TYPE,
        TRA_HCIT_STATE_RX_COMMAND_OPCODE1,
        TRA_HCIT_STATE_RX_COMMAND_OPCODE2,
        TRA_HCIT_STATE_RX_COMMAND_LENGTH,
        TRA_HCIT_STATE_RX_DATA_START,
        TRA_HCIT_STATE_RX_COMMAND_CMD,
        TRA_HCIT_STATE_RX_SUB_COMMAND_LENGTH1,
        TRA_HCIT_STATE_RX_SUB_COMMAND_LENGTH2,
        TRA_HCIT_STATE_RX_FLASH_DATA_START,
        TRA_HCIT_STATE_RX_DATA_CONT,
        TRA_HCIT_STATE_RX_DATA_COMMIT,
        TRA_HCIT_STATE_RX_EVENT_OPCODE,
        TRA_HCIT_STATE_RX_EVENT_LENGTH,
        TRA_HCIT_STATE_RX_DATA_WHEN_NO_BUFFER,
        TRA_HCIT_STATE_RX_DATA_PRO,
    };

enum
    {
        TRA_HCIT_STATE_RX_NOPAGE,
        TRA_HCIT_STATE_RX_START,
        TRA_HCIT_STATE_RX_PAGE_0,
        TRA_HCIT_STATE_RX_PAGE_1,
        TRA_HCIT_STATE_RX_PAGE_2,
        TRA_HCIT_STATE_RX_PAGE_3,
        TRA_HCIT_STATE_RX_PAGE_4,
		TRA_HCIT_STATE_RX_PAGE_5,
		TRA_HCIT_STATE_RX_PAGE_6,
		TRA_HCIT_STATE_RX_PAGE_7,
		TRA_HCIT_STATE_RX_PAGE_8,
		TRA_HCIT_STATE_RX_PAGE_9,
		TRA_HCIT_STATE_RX_PAGE_10,
		TRA_HCIT_STATE_RX_PAGE_11,
		TRA_HCIT_STATE_RX_PAGE_12,
		TRA_HCIT_STATE_RX_PAGE_13,
		TRA_HCIT_STATE_RX_PAGE_14,
		TRA_HCIT_STATE_RX_PAGE_15,
		TRA_HCIT_STATE_RX_FLASH_DATA_INT_END,
		TRA_HCIT_STATE_RX_FLASH_DATA_COMMIT,
		TRA_HCIT_STATE_RX_FLASH_DATA_COMMIT_ERROR,
    };


extern uint32_t tra_hcit_rx_head_buf_array[2];
extern uint32_t tra_hcit_tx_head_buf_array[2];
extern uint8_t  Longtrx_pdu_buf[4200];
extern uint8_t  Shorttrx_pdu_buf[300];
extern uint32_t flash_page_cnt;

void TRAhcit_Transmit(uint8_t *data, uint32_t pdu_len, uint8_t *header,
                      uint8_t head_len );
uint8_t *TRAhcit_Generic_Get_Rx_Buf(uint8_t ptype, int size, uint8_t *hci_header);
void TRAhcit_Generic_Commit_Rx_Buf(uint8_t buffer_type);
void TRAhcit_Rx_Char(uint8_t ch,uint8_t uart_status);
void TRAhcit_Tx_PDU(void);

void TRAhci_Rx_state_Clear(void);

uint8_t TRAhci_get_rx_done_state(void);

void TRAhci_set_rx_done_state(uint8_t state);

uint8_t TRAhci_get_rx_state(void);

void TRAhci_set_rx_state(uint8_t state);



#endif /* BL_TRA_HCIT_H_ */

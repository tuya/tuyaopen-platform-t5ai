/*
 * bl_tra_hcit.c
 *
 *  Created on: 2017-5-5
 *      Author: gang.cheng
 */

#include "type.h"
#include "bl_tra_hcit.h"
#include "system.h"
#include "bl_uart.h"
#include <string.h>
/*
 * Transmit variables associated with the actual data being transmitted
 *     tra_hcit_tx_pdu_buf         Pointer to the buffer being transmitted
 *     tra_hcit_tx_pdu_length      Remaining Length of PDU being transmitted
 *     tra_hcit_tx_original_length Original PDU length for Queue acknowledge
 */
static volatile uint8_t  *tra_hcit_tx_pdu_buf;
static volatile uint32_t tra_hcit_tx_pdu_length;
static uint32_t tra_hcit_tx_original_length;

/*
 * Variables used while transmitting characters (used both for header and PDU)
 *      tra_hcit_cur_tx_pointer    Pointer to the buffer being transmitted
 *      tra_hcit_cur_tx_length     Remaining Length of buffer (header/PDU)
 */
static volatile uint8_t  *tra_hcit_cur_tx_pointer;
static volatile uint32_t tra_hcit_cur_tx_length;


/*
 * The state variable for the transmit state machine and the
 * associated transmit states used whilst encoding a transmit PDU to the host
 */
enum
    {
        TRA_HCIT_STATE_TX_HEADER,
        TRA_HCIT_STATE_TX_BODY
    };
static uint8_t tra_hcit_tx_state;


/*
 * Receive specific state machine variables
 *
 * 4 byte array to store the HCI command/ACL/SCO header,
 * This header is stripped off in the tra_hcit.c module
 * rather than the main hci control logic
 */

uint32_t tra_hcit_rx_head_buf_array[2];
uint8_t* tra_hcit_rx_head_buf = (uint8_t*)tra_hcit_rx_head_buf_array;

uint32_t tra_hcit_tx_head_buf_array[2];


uint8_t  Longtrx_pdu_buf[4200];

uint8_t  Shorttrx_pdu_buf[300];

/*
 * Receive variables associated with the actual data being received
 * tra_hcit_rx_pdu_buf             Pointer to write received characters to
 * tra_hcit_rx_pdu_length          Current Length of received PDU
 * tra_hcit_rx_head_length         Header received length
 * tra_hcit_rx_type                PDU Type: HCI command|ACL Data|SCO Data
 */
volatile uint8_t* tra_hcit_rx_pdu_buf;
volatile uint16_t tra_hcit_rx_pdu_length;
uint16_t 				bk_hcit_rx_pdu_length;
static volatile uint16_t tra_hcit_remain_rx_pdu_length = 0; /* TP/CON/SLA/BV-04 only for input data length bigger then L2cap mtu size (27 bytes) */
static uint8_t tra_hcit_rx_head_length;
static uint8_t tra_hcit_rx_type;
static uint8_t tra_hcit_hardware_error_event_flag;

/*
 * Common structure (generic HCI transport structure above)
 * Any accesses to members of the common structure below should
 * collapse into additions from the base of the tra_hcit_info
 * structure. NOTE: The common structure _must_ be the first member of
 * this or any other transport type.
 */





static uint8_t tra_hcit_rx_state;


/*
 * HCIT_poll_tx
 *
 * paramters: buf = double pointer to buffer (See block transmit function)
 *            length = length pointer
 * returns: void
 *
 * This function waits until the UART is ready to receive another character and
 * then writes this character to hte register
 */

void HCIT_poll_tx(volatile uint8_t **buf, volatile uint32_t *length)
{
    uint32_t count = *length;
    /*
     * Once the transmitter is empty, fill the FIFO with characters
     */
  //  if(count >= (HCIT_TX_FIFO_LENGTH))
  //      count = HCIT_TX_FIFO_LENGTH;

    uart_send((unsigned char *)(*buf), count);
    (*length)-=count;
    (*buf)+=count;
}

/*****************************************************************************
 * Function: TRAhcit_Tx_Block
 * Parameters:
 *      data  :         pointer to the data
 *      length:         length of data
 *      queue :         queue ID to commit to (e.g. HCI_COMMAND_QUEUE )
 *      device index :  device index to commit to.
 *
 * This function transmits a queued PDU over the hci uart interface.
 * It initially sets up pointers from supplied info
 * The transmission continues character
 * by character (or in blocks determined by the Tx FIFO length, if
 * available), through the end of transmission interrupt.
 ****************************************************************************/
/*void TRAhcit_Transmit(u_int8 *data, u_int32 pdu_len, u_int8 *header,
                      u_int8 head_len,u8 tx_state ,u8 tx_init_en)
*/

void TRAhcit_Transmit(uint8_t *data, uint32_t pdu_len, uint8_t *header,
                      uint8_t head_len )
{


    /*
     * Store the pointer to the header
     */

    tra_hcit_cur_tx_pointer = header;

    /*
     * Number of header bytes to be transmitted
     */

    tra_hcit_cur_tx_length = head_len;

    /*
     * Store the pdu buffer and length
     */

    tra_hcit_tx_pdu_buf = data;
    tra_hcit_tx_pdu_length = pdu_len;

    /*
     * Original length of PDU (minus header) for ACKing the PDU from the queue
     */
    tra_hcit_tx_original_length = pdu_len;

    tra_hcit_tx_state = TRA_HCIT_STATE_TX_HEADER;//TRA_HCIT_STATE_TX_HEADER;

    TRAhcit_Tx_PDU();
}


/*****************************************************************************
 * Function: TRAhcit_Tx_PDU(void)
 * parameters: void.
 *
 * This function sends the PDU checking after each byte if it was the last
 ****************************************************************************/
void TRAhcit_Tx_PDU(void)
{
    /*
     * Otherwise, we must poll spinning
     * NOTE: The goto is used here for more compact code
     */

 do_remainder:

    while(tra_hcit_cur_tx_length)
    {

            HCIT_poll_tx(&tra_hcit_cur_tx_pointer, &tra_hcit_cur_tx_length);
    }
    if(tra_hcit_tx_state == TRA_HCIT_STATE_TX_HEADER)
    {
        tra_hcit_cur_tx_pointer = tra_hcit_tx_pdu_buf;
        tra_hcit_cur_tx_length = tra_hcit_tx_pdu_length;
        tra_hcit_tx_state = TRA_HCIT_STATE_TX_BODY;
        goto do_remainder;
    }

}

void TRAhcit_Tx_data_int(uint8_t *data,uint32_t len)
{
    /*
     * Otherwise, we must poll spinning
     * NOTE: The goto is used here for more compact code
     */

 do_remainder:

    while(tra_hcit_cur_tx_length)
    {

            HCIT_poll_tx(&tra_hcit_cur_tx_pointer, &tra_hcit_cur_tx_length);
    }
    if(tra_hcit_tx_state == TRA_HCIT_STATE_TX_HEADER)
    {
        tra_hcit_cur_tx_pointer = tra_hcit_tx_pdu_buf;
        tra_hcit_cur_tx_length = tra_hcit_tx_pdu_length;
        tra_hcit_tx_state = TRA_HCIT_STATE_TX_BODY;
        goto do_remainder;
    }

}


void TRAhci_Rx_state_Clear()
{
	tra_hcit_rx_state = TRA_HCIT_STATE_RX_TYPE;
}


uint32_t flash_page_cnt = 0;
//extern unsigned char uart_rx_end_flag ;
/****************************************************************************
 * Function: TRAhcit_Rx_Char
 * Paramters: ch : character received from the UART driver
 *
 * This function is called by the individual UARTs receive interrupt handler
 * When a full HCI packet has been decoded, this will call the HCI generic
 * commit function to validate the entry just placed on the queue.
 ****************************************************************************/
void TRAhcit_Rx_Char(uint8_t ch,uint8_t uart_status)
{



    /*
     * Receive state machine. Stores 3/4 header bytes seperately until
     * the length has been received and decoded. At that point a buffer is
     * obtained from the generic transport mechanism and the PDU is enqueued.
     * This function uses early returns at the end of each state block. This is
     * for speed in exiting, as this function currently runs at the highest
     * interrupt priority, and should take as little time as possible
     */

    /*
     * NOTE:
     * This module could employ an optional watchdog timer of one second
     * to reset the state to TRA_HCIT_STATE_RX_PTYPE in case of a loss of
     * synchronisation.
     */
#if 0
	if(uart_status == 1 )
	{
		/*
		 * Time to commit, set new state and continue
		 */
		flash_page_cnt = 0;
		tra_hcit_rx_state = TRA_HCIT_STATE_RX_DATA_COMMIT;
		goto _TRA_HCIT_STATE_RX_DATA_COMMIT;
	}
#endif

    switch(tra_hcit_rx_state)
    {
    case TRA_HCIT_STATE_RX_DATA_CONT:
    	_TRA_HCIT_STATE_RX_DATA_CONT:

            /*
             * Copy in the received character into the newly allocated buffer
             */
            *tra_hcit_rx_pdu_buf = ch;

        /*
         * Increase pointer to next byte, and denote one less byte to go
         */
        tra_hcit_rx_pdu_buf++;
        flash_page_cnt++;
        uart_rx_index++;
   //     if(uart_status == 0)
        {
        	 tra_hcit_rx_pdu_length--;
        	 bk_hcit_rx_pdu_length = tra_hcit_rx_pdu_length;
        }


        if((flash_page_cnt > 0x5) && (((flash_page_cnt - 5) % 0x100 )== 0))
        {
        	uart_rx_done_state =  (flash_page_cnt - 5) / 0x100 + 1;

        }


        if((!tra_hcit_rx_pdu_length) ||(uart_status == 1) )
        {
            /*
             * Time to commit, set new state and continue
             */
        	flash_page_cnt = 0;
            tra_hcit_rx_state = TRA_HCIT_STATE_RX_DATA_COMMIT;
            goto _TRA_HCIT_STATE_RX_DATA_COMMIT;
        }
        return;

    case TRA_HCIT_STATE_RX_TYPE:
        /* _TRA_HCIT_STATE_RX_TYPE: */

        /*
         * New HCI Transport packet, must first determine type
         * Note: tra_hcit_rx_type is assigned immediate (faster than = ch)
         */
        switch(ch)
        {
        case TRA_HCIT_COMMAND:

        	tra_hcit_rx_head_buf_array[0] = 0;
        	tra_hcit_rx_head_buf_array[1] = 0;
        	tra_hcit_rx_pdu_length = 0;
        	bk_hcit_rx_pdu_length = tra_hcit_rx_pdu_length;
        	tra_hcit_rx_head_buf[0] = ch;
            tra_hcit_rx_type = TRA_HCIT_COMMAND;
            tra_hcit_rx_state = TRA_HCIT_STATE_RX_COMMAND_OPCODE1;

            tra_hcit_hardware_error_event_flag=0;
            uart_rx_done_state = TRA_HCIT_STATE_RX_NOPAGE ;
            uart_rx_index = 1;
            return;

        default:
        	tra_hcit_rx_head_buf_array[0] = 0;
        	tra_hcit_rx_head_buf_array[1] = 0;
        	tra_hcit_rx_pdu_length = 0;
        	bk_hcit_rx_pdu_length = tra_hcit_rx_pdu_length;
            tra_hcit_rx_state = TRA_HCIT_STATE_RX_TYPE;
            uart_rx_index = 0;
            uart_rx_done_state = TRA_HCIT_STATE_RX_NOPAGE;
            /*
             * Error Handle hook
             */
            if (tra_hcit_hardware_error_event_flag==0)
            {


                tra_hcit_hardware_error_event_flag=1;
            }
            break;
        }
        break;

    case TRA_HCIT_STATE_RX_COMMAND_OPCODE1:
        /* _TRA_HCIT_STATE_RX_COMMAND_OPCODE1: */


        if(BK_DEBUG_HCI_OPCODE1 == ch)
        {
        	tra_hcit_rx_head_buf[1] = ch;
        	 tra_hcit_rx_state = TRA_HCIT_STATE_RX_COMMAND_OPCODE2;
        	 uart_rx_index++;
        }else if(TRA_HCIT_COMMAND == ch)
        {
        	tra_hcit_rx_state = TRA_HCIT_STATE_RX_COMMAND_OPCODE1;
        	 uart_rx_index = 1;
        }else
        {
        	tra_hcit_rx_state = TRA_HCIT_STATE_RX_TYPE;
        }

        return;

    case TRA_HCIT_STATE_RX_COMMAND_OPCODE2:
        /* _TRA_HCIT_STATE_RX_COMMAND_OPCODE2: */


        if(BK_DEBUG_HCI_OPCODE2 == ch)
        {
        	tra_hcit_rx_head_buf[2] = ch;
        	 tra_hcit_rx_state = TRA_HCIT_STATE_RX_COMMAND_LENGTH;
        	 uart_rx_index++;
        }else if(TRA_HCIT_COMMAND == ch)
        {
        	tra_hcit_rx_state = TRA_HCIT_STATE_RX_COMMAND_OPCODE1;
        	uart_rx_index = 1;
        }else
        {
        	tra_hcit_rx_state = TRA_HCIT_STATE_RX_TYPE;
        }

        return;

    case TRA_HCIT_STATE_RX_COMMAND_LENGTH:
        /* _TRA_HCIT_STATE_RX_COMMAND_LENGTH: */

        /*
         * Final state in decoding header of received HCI command
         * Note: Keep track of the head length (in case PDU is zero length)
         */
        tra_hcit_rx_head_buf[3] = ch;
        tra_hcit_rx_pdu_length = ch;
        bk_hcit_rx_pdu_length = tra_hcit_rx_pdu_length;
        tra_hcit_rx_head_length = 3;
        uart_rx_index++;

        /*
         * Handle zero length commands, in particular the HCI Reset command
         * There will be no more characters for this command
         * - so re-run state machine straight away
         */

        if (tra_hcit_rx_pdu_length == 0xff)
        {
        	tra_hcit_rx_state = TRA_HCIT_STATE_RX_COMMAND_CMD;
        }else
        {

        	 tra_hcit_rx_state = TRA_HCIT_STATE_RX_DATA_START;
        }
        if (tra_hcit_rx_pdu_length==0)
	   {
		   goto _TRA_HCIT_STATE_RX_DATA_START;
	   }
        return;

    case TRA_HCIT_STATE_RX_COMMAND_CMD:

    	uart_rx_index++;
    	tra_hcit_rx_head_buf[4] = ch;

    	 tra_hcit_rx_state = TRA_HCIT_STATE_RX_SUB_COMMAND_LENGTH1;

    	return;

    case TRA_HCIT_STATE_RX_SUB_COMMAND_LENGTH1:
    	   set_5mstime_cnt(0);
    	   uart_rx_index++;
        	tra_hcit_rx_head_buf[5] = ch;

        	 tra_hcit_rx_state = TRA_HCIT_STATE_RX_SUB_COMMAND_LENGTH2;

        	return;

    case TRA_HCIT_STATE_RX_SUB_COMMAND_LENGTH2:
    	uart_rx_index++;
           	tra_hcit_rx_head_buf[6] = ch;

           	tra_hcit_rx_pdu_length = (tra_hcit_rx_head_buf[5] | (tra_hcit_rx_head_buf[6] << 8));
           	bk_hcit_rx_pdu_length = tra_hcit_rx_pdu_length;
           	tra_hcit_rx_state = TRA_HCIT_STATE_RX_FLASH_DATA_START;

           	return;
    case TRA_HCIT_STATE_RX_DATA_START:
    _TRA_HCIT_STATE_RX_DATA_START:


        /*
         * This function should return a buffer for writing the remainder of
         * the PDU into.
         * HCI commands will have their headers copied in automatically,
         * will be stored in members of the queue descriptor.
         */



        tra_hcit_rx_pdu_buf = (volatile uint8_t*)Longtrx_pdu_buf;


    	uart_rx_done_state = TRA_HCIT_STATE_RX_START;

        if(!tra_hcit_rx_pdu_length)    /* Zero length command or data */
        {
            tra_hcit_rx_state = TRA_HCIT_STATE_RX_DATA_COMMIT;
            goto _TRA_HCIT_STATE_RX_DATA_COMMIT;
        }

        tra_hcit_rx_state = TRA_HCIT_STATE_RX_DATA_CONT;
        /*
         * The goto avoids duplication of the code in DATA_CONT - space saving
         */
        goto _TRA_HCIT_STATE_RX_DATA_CONT;


        /*
         * When we get here, just keep receiving until the remaining length runs out
         */

    case TRA_HCIT_STATE_RX_FLASH_DATA_START:
//_TRA_HCIT_STATE_RX_FLASH_DATA_START:


           /*
            * This function should return a buffer for writing the remainder of
            * the PDU into.
            * HCI commands will have their headers copied in automatically,
            * will be stored in members of the queue descriptor.
            */

    		uart_rx_done_state = TRA_HCIT_STATE_RX_START;


		    bl_memcpy(Longtrx_pdu_buf,(uint8_t *)&(tra_hcit_rx_head_buf[4]),3);
           tra_hcit_rx_pdu_buf = (volatile uint8_t*)&(Longtrx_pdu_buf[3]);


           if(!tra_hcit_rx_pdu_length)    /* Zero length command or data */
           {
               tra_hcit_rx_state = TRA_HCIT_STATE_RX_DATA_COMMIT;
               goto _TRA_HCIT_STATE_RX_DATA_COMMIT;
           }

           tra_hcit_rx_state = TRA_HCIT_STATE_RX_DATA_CONT;
           /*
            * The goto avoids duplication of the code in DATA_CONT - space saving
            */
           goto _TRA_HCIT_STATE_RX_DATA_CONT;


           /*
            * When we get here, just keep receiving until the remaining length runs out
            */

    case TRA_HCIT_STATE_RX_DATA_COMMIT:
    _TRA_HCIT_STATE_RX_DATA_COMMIT:

        /*
         * Finished reading the pdu, therefore commit entry
         */


        tra_hcit_rx_state = TRA_HCIT_STATE_RX_DATA_PRO;//TRA_HCIT_STATE_RX_DATA_PRO;

        uart_rx_done_state = TRA_HCIT_STATE_RX_FLASH_DATA_COMMIT;

        if(tra_hcit_rx_pdu_length != 0)
        {
        	tra_hcit_rx_pdu_length = 0;
        	bk_hcit_rx_pdu_length = tra_hcit_rx_pdu_length;
        //	uart_rx_done_state = TRA_HCIT_STATE_RX_FLASH_DATA_COMMIT_ERROR;
        }


        return;

    default:    /* Error Handle hook */
        return;
    }
    return;
}






uint8_t TRAhci_get_rx_done_state(void)
{
	return uart_rx_done_state;
}

void TRAhci_set_rx_done_state(uint8_t state)
{
	uart_rx_done_state = state;
}


uint8_t TRAhci_get_rx_state(void)
{
	return tra_hcit_rx_state;
}

void TRAhci_set_rx_state(uint8_t state)
{
	tra_hcit_rx_state = state;
}







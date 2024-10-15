#include "system.h"
#include "bl_config.h"
#include "bl_tra_hcit.h"
#include "bl_uart.h"
#define TX_FIFO_THRD                0x40
#define RX_FIFO_THRD                0x40


volatile char uart_rx_done_state = 0;
volatile unsigned long uart_rx_index = 0;
volatile unsigned long uart_rx_index_cur = 0;

unsigned char uart_rx_end_flag = 0;



char buf[128 / 2];
u16 COUNT;
u32 g_baud_rate;

uint32_t uart_get_clock_freq(void)
{
#if CONFIG_XTAL_40M
	if (AON_PMU_IS_XTAL_40M) {
		return UART_CLOCK_FREQ_40M;
	} else {
		return UART_CLOCK_FREQ_26M;
	}
#else
	return UART_CLOCK_FREQ_26M;
#endif
}

void boot_uart_init(uint32_t baud_rate,uint8_t rx_fifo_thr)
{
    uint32_t baud_divisor;
    g_baud_rate = baud_rate;
    baud_divisor           = UART_CLOCK_FREQ/baud_rate;
    baud_divisor           = baud_divisor-1;

    COUNT = UART_TX_FIFO_COUNT;

    REG_APB3_UART_CFG      = (   (DEF_STOP_BIT    << sft_UART_CONF_STOP_LEN)
                                 | (DEF_PARITY_MODE << sft_UART_CONF_PAR_MODE)
                                 | (DEF_PARITY_EN   << sft_UART_CONF_PAR_EN)
                                 | (DEF_DATA_LEN    << sft_UART_CONF_UART_LEN)
                                 | (baud_divisor    << sft_UART_CONF_CLK_DIVID)
                                 | (DEF_IRDA_MODE   << sft_UART_CONF_IRDA)
                                 | (DEF_RX_EN       << sft_UART_CONF_RX_ENABLE)
                                 | (DEF_TX_EN       << sft_UART_CONF_TX_ENABLE));

    REG_APB3_UART_FIFO_THRESHOLD = ((rx_fifo_thr << sft_UART_FIFO_CONF_RX_FIFO)
                                    | (TX_FIFO_THRD << sft_UART_FIFO_CONF_TX_FIFO));
    REG_APB3_UART_INT_ENABLE=0;             /* Disable UART Interrupts */
    REG_APB3_UART_INT_ENABLE = bit_UART_INT_RX_NEED_READ | bit_UART_INT_RX_STOP_END ; //enable Rx interrupt

}


void boot_uart_init_tx(uint32_t baud_rate,uint8_t rx_fifo_thr)
{
    uint32_t baud_divisor;
    g_baud_rate = baud_rate;
    baud_divisor           = UART_CLOCK_FREQ/baud_rate;
    baud_divisor           = baud_divisor-1;

    COUNT = UART_TX_FIFO_COUNT;

    REG_APB3_UART_CFG      = (   (DEF_STOP_BIT    << sft_UART_CONF_STOP_LEN)
                                 | (DEF_PARITY_MODE << sft_UART_CONF_PAR_MODE)
                                 | (DEF_PARITY_EN   << sft_UART_CONF_PAR_EN)
                                 | (DEF_DATA_LEN    << sft_UART_CONF_UART_LEN)
                                 | (baud_divisor    << sft_UART_CONF_CLK_DIVID)
                                 | (DEF_IRDA_MODE   << sft_UART_CONF_IRDA)
//                                 | (DEF_RX_EN       << sft_UART_CONF_RX_ENABLE)
                                 | (DEF_TX_EN       << sft_UART_CONF_TX_ENABLE));

    REG_APB3_UART_FIFO_THRESHOLD = ((rx_fifo_thr << sft_UART_FIFO_CONF_RX_FIFO)
                                    | (TX_FIFO_THRD << sft_UART_FIFO_CONF_TX_FIFO));
    REG_APB3_UART_INT_ENABLE=0;             /* Disable UART Interrupts */
//    REG_APB3_UART_INT_ENABLE = bit_UART_INT_RX_NEED_READ | bit_UART_INT_RX_STOP_END ; //enable Rx interrupt

}



void debug_uart_send_poll( uint8_t *buff, int len )
{
//	return;
    while (len--) {
        while(!UART_TX_WRITE_READY);
        UART_WRITE_BYTE(*buff++);
    }
}
void debug2_uart_send_poll( UINT8 *buff, int len )
{
    while (len--) {
        while(!UART_TX_WRITE_READY);
        UART_WRITE_BYTE(*buff++);
    }
}


int bl_printf(const char *fmt, ...)
{
	return 0;
    PRINT_BUF_PREPARE(rc, buf, fmt);
    debug_uart_send_poll((uint8_t *)&buf[0], rc);
    return rc;
}

int ad_printf(const char *fmt, ...)
{
		return 0;
    PRINT_BUF_PREPARE(rc, buf, fmt);
    debug_uart_send_poll((uint8_t *)&buf[0], rc);
    return rc;
}


void uart_send(unsigned char *buff, int len)
{
	 while (len--) {
	        while(!UART_TX_WRITE_READY);
	        UART_WRITE_BYTE(*buff++);
	    }
}


extern volatile uint8_t* tra_hcit_rx_pdu_buf;
extern volatile uint16_t tra_hcit_rx_pdu_length;
extern uint32_t flash_page_cnt ;

#if 0
	
void UART_InterruptHandler(void) {
    uint32_t status;
    uint8_t value = 0;
    uint16_t read_cnt = 0;
  //  uint16_t read_static_flag =0;
#ifdef UART_TIME_CHECK
    uint8_t ch ;
#endif

   // u_int8 data_cnt = 0;
    status = REG_APB3_UART_INT_FLAG;
#if GPIO_DEBUG
    gpio_output(5,1);
#endif

    read_cnt = 0;
 //   read_static_flag = 1;


if(status & (bit_UART_INT_RX_NEED_READ)) {

#ifdef UART_TIME_CHECK
    	ch = 0x1;
    	//data_cnt = 0;
    	uart_send(&ch,1);
#endif
    	uart_rx_end_flag = UART_RX_START;
    	if(TRA_HCIT_STATE_RX_DATA_CONT == TRAhci_get_rx_state() )
    	{
#ifdef UART_TIME_CHECK
    		ch = 0x2;
    		uart_send(&ch,1);
#endif


    		 uart_static_read85();

    		 if(tra_hcit_rx_pdu_length >= RX_FIFO_THR_COUNT) //read_cnt
    		 			{
    		 				uart_rx_index +=RX_FIFO_THR_COUNT;
    		 				tra_hcit_rx_pdu_length-=RX_FIFO_THR_COUNT;

    		 				flash_page_cnt+= RX_FIFO_THR_COUNT;
    		 			}else
    		 			{
    		 				uart_rx_index+= tra_hcit_rx_pdu_length;
    		 				flash_page_cnt  += tra_hcit_rx_pdu_length;
    		 				tra_hcit_rx_pdu_length = 0;
    		 			}

    		if(flash_page_cnt > 0x105)
			{
				uart_rx_done_state =  (flash_page_cnt - 5) / 0x100 + 1;


			}

    	}else
    	{
#ifdef UART_TIME_CHECK
    		ch = 0x3;
    		uart_send(&ch,1);
#endif
    		while ((UART_RX_READ_READY))
    		{
				value=UART_READ_BYTE();
				TRAhcit_Rx_Char(value,0);
				if(TRA_HCIT_STATE_RX_DATA_CONT == TRAhci_get_rx_state())
				{
#ifdef UART_TIME_CHECK
					ch = 0x4;
					 uart_send(&ch,1);
#endif

#if 1
					 while (UART_RX_READ_READY)
					 {

						   *tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
						   read_cnt++;

					}
#else



#endif


					if(tra_hcit_rx_pdu_length >= read_cnt)
					{
						uart_rx_index +=read_cnt;
						tra_hcit_rx_pdu_length-=read_cnt;

						flash_page_cnt+= read_cnt;
						read_cnt = 0;
					}else
					{
						uart_rx_index+= tra_hcit_rx_pdu_length;
						flash_page_cnt  += tra_hcit_rx_pdu_length;
						tra_hcit_rx_pdu_length = 0;
						read_cnt = 0;
					}

#ifdef UART_TIME_CHECK
					ch = 0x5;
					uart_send(&ch,1);
#endif
					if(flash_page_cnt > 0x105)
					{
						uart_rx_done_state =  (flash_page_cnt - 5) / 0x100 + 1;

					}
				}
			}
#ifdef UART_TIME_CHECK
    		ch = 0x6;
    		uart_send(&ch,1);
#endif

    	}


#ifdef UART_TIME_CHECK
    	ch = 0x7;
        uart_send(&ch,1);
#endif
}




    if(status & (bit_UART_INT_RX_STOP_END)) {
#ifdef UART_TIME_CHECK
    	ch = 0x1;
    	//data_cnt = 0;
    	uart_send(&ch,1);
#endif
    	uart_rx_end_flag = UART_RX_START;
    	if(TRA_HCIT_STATE_RX_DATA_CONT == TRAhci_get_rx_state() )
    	{
#ifdef UART_TIME_CHECK
    		ch = 0x2;
    		uart_send(&ch,1);
#endif

#if 1
    		while (UART_RX_READ_READY)
    		{

				   *tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
				   read_cnt++;


			}
#else


#endif

    		if(tra_hcit_rx_pdu_length >= read_cnt)
			{
				uart_rx_index +=read_cnt;
				tra_hcit_rx_pdu_length-=read_cnt;

				flash_page_cnt+= read_cnt;
			}else
			{
				uart_rx_index+= tra_hcit_rx_pdu_length;
				flash_page_cnt  += tra_hcit_rx_pdu_length;
				tra_hcit_rx_pdu_length = 0;
			}

    		if(flash_page_cnt > 0x105)
			{
				uart_rx_done_state =  (flash_page_cnt - 5) / 0x100 + 1;

			}

    	}else
    	{
#ifdef UART_TIME_CHECK
    		ch = 0x3;
    		uart_send(&ch,1);
#endif
    		while ((UART_RX_READ_READY))
    		{

				value=UART_READ_BYTE();
				TRAhcit_Rx_Char(value,0);
				if(TRA_HCIT_STATE_RX_DATA_CONT == TRAhci_get_rx_state())
				{
#ifdef UART_TIME_CHECK
					ch = 0x4;
					 uart_send(&ch,1);
#endif

#if 1
					 while (UART_RX_READ_READY)
					 {

						   *tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
						   read_cnt++;

					}
#else



#endif


					if(tra_hcit_rx_pdu_length >= read_cnt)
					{
						uart_rx_index +=read_cnt;
						tra_hcit_rx_pdu_length-=read_cnt;

						flash_page_cnt+= read_cnt;
						read_cnt = 0;
					}else
					{
						uart_rx_index+= tra_hcit_rx_pdu_length;
						flash_page_cnt  += tra_hcit_rx_pdu_length;
						tra_hcit_rx_pdu_length = 0;
						read_cnt = 0;
					}

#ifdef UART_TIME_CHECK
					ch = 0x5;
					uart_send(&ch,1);
#endif
					if(flash_page_cnt > 0x105)
					{
						uart_rx_done_state =  (flash_page_cnt - 5) / 0x100 + 1;

					}
				}
			}
#ifdef UART_TIME_CHECK
    		ch = 0x6;
    		uart_send(&ch,1);
#endif

    	}


#ifdef UART_TIME_CHECK
    	ch = 0x7;
        uart_send(&ch,1);
#endif
    }


    if(status & bit_UART_INT_RX_STOP_END)
    {
#ifdef UART_TIME_CHECK
    	 ch = 0x55;
    	 uart_send(&ch,1);
#endif
    	uart_rx_end_flag = UART_RX_END;

#ifdef UART_TIME_CHECK
    	uart_send(&tra_hcit_rx_pdu_length,4);
#endif
    	 if((!tra_hcit_rx_pdu_length)  )
    	 {
    	            /*
    	             * Time to commit, set new state and continue
    	             */
#ifdef UART_TIME_CHECK
    		 	 	 ch = 0xaa;
    		    	 uart_send(&ch,1);
#endif
    	        	flash_page_cnt = 0;
    	        	if(TRAhci_get_rx_state() != TRA_HCIT_STATE_RX_NOPAGE)
    	        	{
#ifdef UART_TIME_CHECK
    	        		ch = 0xbb;
    	        		uart_send(&ch,1);
#endif
    	        		TRAhci_set_rx_state(TRA_HCIT_STATE_RX_DATA_COMMIT);
    	        		TRAhcit_Rx_Char(value,0);
    	        	}



    	   }



    }
  //  bl_printf("bit_UART_INT_RX_STOP_END status1 = %x\r\n",status);



#ifdef UART_TIME_CHECK
    	 uart_send(&uart_rx_index,4);
    	 uart_send(&uart_rx_done_state,1);
#endif
    REG_APB3_UART_INT_FLAG = status;
#if GPIO_DEBUG
    gpio_output(5,0);
#endif
}

#endif



void uart_static_read85(void)
{
			*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();

    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
			*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
			*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
			*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
			*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();//10

    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();

    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();//20

    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();

    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();//30

    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();

    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();//40

    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();

    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();//50


    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();

    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();//60

    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();

    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();//70


    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();

    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();//80


    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
    		*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();//85
	/*
			*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
			*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
			*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
			*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
			*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();//90


			*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
			*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
			*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
			*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
			*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();//95


			*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
			*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
			*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
			*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();
			*tra_hcit_rx_pdu_buf++ = UART_READ_BYTE();//100
*/

}

// EOF




#ifndef _ATSVR_PORT_H_
#define _ATSVR_PORT_H_
#include <os/os.h>


#define ATSVR_QUEUE_MAX_NB              6
#define AT_GETCHAR_TIMEOUT            (120000)


#if(CONFIG_DEFAULT_AT_PORT == 0)
#define AT_UART_PORT_CFG              	  0
#elif (CONFIG_DEFAULT_AT_PORT == 1)
#define AT_UART_PORT_CFG              	  1
#elif (CONFIG_DEFAULT_AT_PORT == 2)
#define AT_UART_PORT_CFG              	  2
#endif



#define AT_CTS_RTS_SOFTWARE_CFG         1
#define AT_UART_SEND_DATA_INTTRRUPT_PROTECT   0

#define RX_RB_LENGTH  128

#define IN
#define OUT
#define CONST const
#define STRING

 struct atsvr_cmd_parameter
 {
	   char 	*rsp_buff;
	   char 	*cmd_buff;
	   int	   cmd_data_len;
	   int	   out_buf_size;
 };


 struct atsvr_port_env{
	int atsvr_uart_id;
};

extern void atsvr_port_env_init(void);
extern int atsvr_handle_shell_input(char *inbuf, int in_buf_size, char * outbuf, int out_buf_size);

extern int atsvr_msg_get_input(char *inbuf, int in_buf_size, char * outbuf, int out_buf_size);


#endif


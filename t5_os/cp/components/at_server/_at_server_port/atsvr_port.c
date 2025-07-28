#include "atsvr_port.h"
#include "atsvr_core.h"
#include "at_server.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "_at_server.h"

#include <os/os.h>
#include <common/bk_include.h>
#include "uart_pub.h"
#include <driver/uart.h>
#include <driver/hal/hal_uart_types.h>
#include <os/mem.h>

extern _at_svr_ctrl_env_t _at_svr_env;

#if CONFIG_AT_DATA_MODE
	char* at_data_temp   = NULL;
	int   at_data_offset = 0;
#endif

void *at_malloc(unsigned int size)
{
	return os_malloc(size);
}

void at_free(void *p)
{
	os_free(p);
}

int atsvr_port_send_msg_queue(atsvr_msg_t *sd_atsvrmsg,unsigned int timeout)
{
	if(_at_svr_env.msg_queue != NULL){
		ATSVRLOG("[ATSVR]msg_queue is not NULL\r\n");
		int ret = rtos_push_to_queue(&_at_svr_env.msg_queue,sd_atsvrmsg,timeout);
		if(ret != kNoErr){
			ATSVRLOGW("[ATSVR]msg_queue push error:%d\r\n",ret);
			return -1;
		}
		return 0;
	}else{
		ATSVRLOGW("[ATSVR]atsvr_port.msg_queue is invaild\r\n");
		return -1;
	}
}
extern int handle_shell_input(char *inbuf, int in_buf_size, char * outbuf, int out_buf_size);
#if CONFIG_AT_DATA_MODE// at socket
int atsvr_handle_data_input(char *inbuf, int in_buf_size, char * outbuf, int out_buf_size);
#endif
int atsvr_msg_get_input(char *inbuf, int in_buf_size, char * outbuf, int out_buf_size)
{
	_at_svr_ctrl_env_t* penv = &_at_svr_env;
	if (inbuf == NULL){
		ATSVRLOGW("inbuf_null\r\n");
		return 0;
	}
	int bp = 0;
#if CONFIG_AT_DATA_MODE// at socket
	ATSVRLOG("current state:%d\r\n",get_atsvr_work_state());
	if(ATSVR_WK_DATA_HANDLE != get_atsvr_work_state()){
		ATSVRLOGD("atsvr cmd mode\r\n");
		goto cmd_mode;
	}
	else{
		ATSVRLOGD("atsvr data mode\r\n");
		atsvr_handle_data_input(inbuf,in_buf_size,outbuf,out_buf_size);
		return 0;
	}
cmd_mode:
#endif
	penv->atsvr_mode = false;
	while (true)
	{
		if(bp == 0) 
		{
			if(inbuf[0] != 'A') 
			{
				ATSVRLOG("cli command\r\n");
				handle_shell_input(inbuf,in_buf_size,outbuf,out_buf_size);
				break;
			}
		}
		else if(bp == 1)
		{
			if(inbuf[1] != 'T') 
			{
				ATSVRLOG("cli command\r\n");
				handle_shell_input(inbuf,in_buf_size,outbuf,out_buf_size);
				break;
			}
		
			else
			{
				ATSVRLOG("atsvr command\r\n");
				atsvr_handle_shell_input(inbuf,in_buf_size,outbuf,out_buf_size);
				break;
			}
		}
		bp++;
	}
	return 0;
}

void atsvr_event_handler(atsvr_msg_t *msg)
{
	for(int i = 0; i < ATSVR_MAX_COMMANDS; i++)
	{
		if(0 == strcmp(msg->evt_type,_at_svr_env.cmd_type[i]))
		{
			
			if(ATSVR_OK!=_at_svr_env.cmd_event_cb[i](msg))
			{
				ATSVRLOGW("[ATSVR]untreated event\r\n");
				return;
			}
			else
				return;
		}
	}
}
static beken_thread_t atsvr_handle_thread_handle = NULL;
static beken_semaphore_t wait_atsvr_handle_semaphore = NULL;

static void atsvr_handle_input_proxy(beken_thread_arg_t arg)
{
	set_atsvr_work_state(ATSVR_WK_DOING);

	struct atsvr_cmd_parameter *proxy = (struct atsvr_cmd_parameter *)(int)arg;
	atsvr_msg_t msg;
	msg.type = ATSVR_MSG_STREAM;
	msg.sub_type = ATSVR_SUBMSG_NONE;
	msg.len = proxy->cmd_data_len+1;
	msg.msg_param =at_malloc(msg.len);
	memcpy((char*)msg.msg_param,proxy->cmd_buff,proxy->cmd_data_len);

	char* ptr;
	ptr = (char*)msg.msg_param;
	ptr[proxy->cmd_data_len] = '\0';
	atsvr_msg_handler(&msg);
	at_free(msg.msg_param);

#ifndef CONFIG_AT_DATA_MODE
	set_atsvr_work_state(ATSVR_WK_IDLE);
#else
	if(ATSVR_WK_DATA_HANDLE!= get_atsvr_work_state())
		set_atsvr_work_state(ATSVR_WK_IDLE);
#endif
	rtos_set_semaphore(&wait_atsvr_handle_semaphore);
	rtos_delete_thread(NULL);
}

int atsvr_handle_shell_input(char *inbuf, int in_buf_size, char * outbuf, int out_buf_size)
{
	struct atsvr_cmd_parameter cmd_par = {0};
	_at_svr_ctrl_env_t* penv = &_at_svr_env;
	int err,ret;

	/*if former AT Cmd is in working*/
	if(get_atsvr_work_state()!=ATSVR_WK_IDLE)
	{
		atsvr_output_msg("BUSY...\r\n");
		return 0;
	}

	cmd_par.rsp_buff = outbuf;
	cmd_par.cmd_buff = inbuf;
	cmd_par.cmd_data_len = in_buf_size;
	cmd_par.out_buf_size = out_buf_size;
	penv->atsvr_mode = true;

	rtos_init_semaphore(&wait_atsvr_handle_semaphore,1);
	ret = rtos_create_thread(&atsvr_handle_thread_handle,
							4,
							"atsvr_cmd_handle",
							(beken_thread_function_t)atsvr_handle_input_proxy,
							1024*4,
							(beken_thread_arg_t)(&cmd_par));
	if (ret != kNoErr) {
		ATSVRLOGE("Error: Failed to create atsvr_handle_thread_handle thread: %d\r\n",ret);
		BK_ASSERT(0);
	}

	err = rtos_get_semaphore(&wait_atsvr_handle_semaphore,BEKEN_WAIT_FOREVER);

	if(err){
		ATSVRLOGW("get wait_atsvr_handle_semaphore fail\r\n");
	}
	rtos_deinit_semaphore(&wait_atsvr_handle_semaphore);
	return 0;
}

#if CONFIG_AT_DATA_MODE// at socket
extern beken_queue_t g_at_sal_queue;

int atsvr_handle_data_input(char *inbuf, int in_buf_size, char * outbuf, int out_buf_size)
{
	atsvr_msg_t atsvrmsg = {0};
	if(ATSVR_WK_DATA_HANDLE == get_atsvr_work_state())// this cmd is TCP/UDP Data
	{
		if(at_data_temp == NULL)
		{
			ATSVRLOGD("atsvr_lwip_data_offset:%d,in_buf_size:%d!\r\n",at_data_offset,in_buf_size);
			if(at_data_offset != in_buf_size){
				ATSVRLOGW("packet data len is wrong!\r\n");
				set_atsvr_work_state(ATSVR_WK_IDLE);
				return -1;
			}
			else{
				at_data_temp = at_malloc(at_data_offset); //total size
				os_memcpy(at_data_temp,inbuf,in_buf_size);
				atsvrmsg.sub_type = ATSVR_SUBMSG_NONE;
				atsvrmsg.addition_infor = ATSVR_SEND_DATA_READY;
				atsvrmsg.msg_param = at_data_temp;
				if(g_at_sal_queue != NULL){
					ATSVRLOG("[ATSVR]g_at_sal_queue is not NULL\r\n");
					int ret = rtos_push_to_queue(&g_at_sal_queue,&atsvrmsg,0);
					if(ret != kNoErr){
						ATSVRLOGW("[ATSVR]g_at_sal_queue push error:%d\r\n",ret);
						set_atsvr_work_state(ATSVR_WK_IDLE);
						return -1;
					}
					ATSVRLOGD("[ATSVR]g_at_sal_queue push ok:%d\r\n",ret);
					return 0;
				}else{
					ATSVRLOGW("[ATSVR]g_at_sal_queue is invaild\r\n");
					set_atsvr_work_state(ATSVR_WK_IDLE);
					return -1;
				}
				return 0;
			}
		}
		else
		{
			ATSVRLOGW("wrong packet come in!\r\n");
			set_atsvr_work_state(ATSVR_WK_IDLE);
			return -1;
		}
		
	}
	else
	{
		ATSVRLOGW("wrong atsvr status!\r\n");
		set_atsvr_work_state(ATSVR_WK_IDLE);
	}
	return 0;
}

#endif



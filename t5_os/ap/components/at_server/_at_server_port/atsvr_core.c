#include "atsvr_core.h"
#include "atsvr_port.h"
#include "at_server.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"


int atsvr_send_msg_queue(atsvr_msg_t *sd_atsvrmsg,unsigned int timeout)
{
	return atsvr_port_send_msg_queue(sd_atsvrmsg,timeout);
}

int atsvr_send_msg_queue_block(atsvr_msg_t *sd_atsvrmsg)
{
	return atsvr_port_send_msg_queue(sd_atsvrmsg,0xFFFFFFFF);
}

int __weak__ atsvr_port_send_msg_queue(atsvr_msg_t *sd_atsvrmsg,unsigned int timeout)
{
	return -1;
}

void atsvr_msg_handler(atsvr_msg_t *msg)
{
	int ret;

	ATSVRLOG("[ATSVR]msg type:%d\r\n", msg->type);
	switch( msg->type ) {
		case ATSVR_MSG_STREAM:
			{
				ret = atsvr_input_msg_analysis_handler((char *)msg->msg_param,msg->len);
				if(ret != 0){
					ATSVRLOGW("[ATSVR]analysis stream error\r\n");
				}
			}
			break;
		case ATSVR_MSG_INIT:
			{
				at_server_init();
			}
			break;
		case ATSVR_MSG_EVENT:
			{
				atsvr_event_handler(msg);
			}
			break;
		default:
			break;
	}

	if(msg->sub_type == ATSVR_SUBMSG_ATFREE ) {
		if( msg->msg_param ) {
			at_free(msg->msg_param);
		}
	}
}

void atsvr_event_sender(char* event,int sub_evt_id, int msg_len,void *msg,bool free)
{
	atsvr_msg_t sdmsg;
	int ret;
	sdmsg.type = ATSVR_MSG_EVENT;
	if(free == true)
		sdmsg.sub_type = ATSVR_SUBMSG_ATFREE;
	else
		sdmsg.sub_type = ATSVR_SUBMSG_NONE;
	sdmsg.evt_type = event;
	sdmsg.addition_infor = sub_evt_id;
	if(NULL != msg)
	{
		sdmsg.len = msg_len;
		sdmsg.msg_param = at_malloc(msg_len);
		memcpy((char*)sdmsg.msg_param,(char*)msg,msg_len);
	}
	else
	{
		sdmsg.len = 0;
		sdmsg.msg_param = NULL;
	}	
	ret = atsvr_send_msg_queue(&sdmsg,0);
	if( ret != 0 ) {
		ATSVRLOGW("wlan_event(%x) notice error\r\n",event);
	}
}

#if CONFIG_AT_DATA_MODE
void atsvr_socket_event_sender(int module_id,int sub_evt_id, int msg_len,void *msg)
{
	atsvr_msg_t sdmsg;
	int ret;
	sdmsg.type = ATSVR_MSG_EVENT;
	sdmsg.evt_type = "at_sal";
	sdmsg.sub_type = ATSVR_SUBMSG_ATFREE;
	sdmsg.addition_infor = sub_evt_id;
	sdmsg.data_module_id = module_id;
	if(NULL != msg)
	{
		sdmsg.len = msg_len;
		sdmsg.msg_param = at_malloc(msg_len);
		memcpy((char*)sdmsg.msg_param,(char*)msg,msg_len);
	}
	else
	{
		sdmsg.len = 0;
		sdmsg.msg_param = NULL;
	}	
	ret = atsvr_send_msg_queue(&sdmsg,0);
	if( ret != 0 ) {
		ATSVRLOGW("atsvr socket mode  notice error\r\n");
	}
}
#endif


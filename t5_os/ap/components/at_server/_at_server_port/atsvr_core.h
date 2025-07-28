#ifndef _ATSVR_CORE_H_
#define _ATSVR_CORE_H_

#include "_at_server.h"

typedef enum
{
	ATSVR_SEND_DATA_UNKNOWN = 0,
	ATSVR_SEND_DATA_READY,
	ATSVR_SEND_DATA_PACKET_REMAIN,
	ATSVR_SEND_DATA_DONE,
}atsvr_data_mode_status_t;


extern void atsvr_msg_handler(atsvr_msg_t *msg);

extern int atsvr_send_msg_queue(atsvr_msg_t *sd_atsvrmsg,unsigned int timeout);
extern int atsvr_send_msg_queue_block(atsvr_msg_t *sd_atsvrmsg);
extern int atsvr_port_send_msg_queue(atsvr_msg_t *sd_atsvrmsg,unsigned int timeout);

extern void atsvr_event_handler(atsvr_msg_t *msg);
extern void atsvr_event_sender(char* event,int sub_evt_id, int msg_len,void *msg,bool free);
#if CONFIG_AT_DATA_MODE
extern void atsvr_socket_event_sender(int module_id,int sub_evt_id, int msg_len,void *msg);
#endif
#endif


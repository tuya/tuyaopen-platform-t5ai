#ifndef _AT_SERVER_H_
#define _AT_SERVER_H_
#include "_at_server.h"

extern char *get_atsvr_version(void);

extern void set_atsvr_echo_mode(_atsvr_echo_t echo);
extern _atsvr_echo_t get_atsvr_echo_mode(void);

extern int atsvr_register_command(const struct _atsvr_command *command);
extern int atsvr_register_commands(const struct _atsvr_command *commands,int num_commands, char* tag,atsvr_event_cb evecb);

extern void atsvr_notice_ready(void);
extern void atsvr_cmd_rsp_ok(void);
extern void atsvr_cmd_rsp_error(void);
//extern void atsvr_cmd_analysis_notice_error(void);
extern void atsvr_output_msg(char *msg);
extern _atsvr_work_st get_atsvr_work_state(void);
extern void set_atsvr_work_state(_atsvr_work_st wk_st);

extern int atsvr_input_msg_analysis_handler(char *msg,unsigned int msg_len);
extern unsigned int atsvr_input_msg_get(char *data,unsigned int data_len);

extern void at_server_init(void);

extern void atsvr_cmd_init(void);
extern void atsvr_extern_cmd_init(void);

extern void *at_malloc(unsigned int size);
extern void at_free(void *p);


#ifndef ATSVR_SIZEOF_OUTPUT_STRRING
#define ATSVR_SIZEOF_OUTPUT_STRRING(string)   atsvr_output_msg(string);
#endif

#ifndef ATSVR_OUTPUT_STRRING
#define ATSVR_OUTPUT_STRRING(string)   atsvr_output_msg(string);
#endif


#endif

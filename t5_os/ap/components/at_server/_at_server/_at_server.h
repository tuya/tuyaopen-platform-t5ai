#ifndef __AT_SERVER_H_
#define __AT_SERVER_H_

#include "_at_svr_opts.h"


typedef enum{
	ATSVR_OK = 0,
	ATSVR_SEVERE_ERR = -2,
	ATSVR_ERROR = -1,

	ATSVR_GENERAL = 1,
	ATSVR_NnMemoryErr = 2,	
	ATSVR_INREGULAR_CMD = 3,
	ATSVR_DATA_PARSE_ERROR = 4,
	ATSVR_INCOMPLETE_CMD = 5,
	ATSVR_CMD_NULL = 6,

}_atsvr_st;



typedef enum{
	ATSVR_ECHO_NONE = 0,
	ATSVR_ECHO_NORMAL,
	ATSVR_ECHO_ALL,
}_atsvr_echo_t;

typedef enum{
	ATSVR_WK_IDLE = 0,
	ATSVR_WK_DOING = 1,
	ATSVR_WK_DATA_HANDLE = 2,
	ATSVR_WK_DONE = 3,
}_atsvr_work_st;


typedef enum{

	_ATSVR_SETUP_DONE = 0,
	_ATSVR_SETUP_ERROR = 1,
	_ATSVR_SETUP_TIMEOUT = 2,
	_ATSVR_SETUP_MAX,
}_atsvr_setup_st;

typedef enum{

	_ATSVR_CPU0 = 0,
	_ATSVR_CPU1 = 1,
	_ATSVR_CPU_MAX,
}_atsvr_cpuid_st;

typedef enum{
	ATSVR_MSG_INIT = 0,
	ATSVR_MSG_STREAM,
	ATSVR_MSG_EVENT,
	ATSVR_MSG_BACKUP,
	ATSVR_MSG_TRANSFER,//for single port used by Log and AT

}atsvr_msgtype_e;

typedef enum{
	ATSVR_SUBMSG_NONE = 0x00,
	ATSVR_SUBMSG_ATFREE,
}atsvr_subtype_e;
	
typedef struct{
	atsvr_msgtype_e	type;
	atsvr_subtype_e sub_type;
	char*			evt_type;
	int 			addition_infor;
#if CONFIG_AT_DATA_MODE
	int 			data_module_id;
#endif
	unsigned int len;
	void *msg_param;
}atsvr_msg_t;

typedef enum{
	ATSVR_MB_OK,
	ATSVR_MB_ERROR,
	ATSVR_MB_ANALILYZE_ERROR,
	ATSVR_MB_TIMEOUT,
	ATSVR_MB_MAX
}asvr_mailbox_msg;


typedef int (*atsvr_handler)(int sync,int argc, char **argv);


typedef int (*atsvr_event_cb)(atsvr_msg_t *amsg);



/* Structure for registering at server commands */
struct _atsvr_command{
	const char 			*name;
	const char 			*help;
	atsvr_handler 		query_func;
	atsvr_handler 		setup_func;
	bool          		sync;
	unsigned int   		sync_time;
	bool   				timeout;
	timer_handler_t     timeout_func;  
	bool           		bassert;
};




typedef struct{
	_atsvr_echo_t echo;
	_atsvr_work_st wk_st;
	beken_timer_t* atsvr_timer;
	beken_queue_t msg_queue;
	const struct _atsvr_command* commands[ATSVR_MAX_COMMANDS];
	unsigned int num_commands[ATSVR_MAX_COMMANDS];
	char* cmd_type[ATSVR_MAX_COMMANDS];
	atsvr_event_cb cmd_event_cb[ATSVR_MAX_COMMANDS];//for event cb,if NULL,set it NULL
	unsigned int cmds_reg_index;// for register	the num which has registerd command table 
	unsigned int cmd_index;
	bool       setup_done;
	_atsvr_setup_st   setup_status;
	bool atsvr_mode;
	_atsvr_cpuid_st priority_cpu;
	bool cpu1_ready;
	bool core_sel_model;
}_at_svr_ctrl_env_t;

typedef _at_svr_ctrl_env_t _atsvr_env_t;

#ifndef _ATSVR_SIZEOF_OUTPUT_STRING
#define _ATSVR_SIZEOF_OUTPUT_STRING(env,string)   _atsvr_output_msg(env,string,ATSVR_SIZEOF_STR_LEN_NOEOF(string));
#endif

#ifndef _ATSVR_OUTPUT_STRING
#define _ATSVR_OUTPUT_STRING(env,string)   _atsvr_output_msg(env,string,strlen(string));
#endif

#ifndef atsvr_islower
#define atsvr_islower(c) ((c) >= 'a' && (c) <= 'z')
#endif

#ifndef _ATSVR_DEFAULT_TIME
#define _ATSVR_DEFAULT_TIME  15*1000
#endif


extern char *_get_atsvr_version(void);

extern void _set_atsvr_echo_mode(_atsvr_env_t *env,_atsvr_echo_t echo);
extern _atsvr_echo_t _get_atsvr_echo_mode(_atsvr_env_t *env);

extern int _atsvr_register_command(_atsvr_env_t *env,const struct _atsvr_command *command);
extern int _atsvr_unregister_command(_atsvr_env_t *env,const struct _atsvr_command *command);
extern int _atsvr_register_commands(_atsvr_env_t *env,const struct _atsvr_command *commands_table,int num_commands, char *tag,atsvr_event_cb evecb);
extern int _atsvr_unregister_commands(_atsvr_env_t *env,const struct _atsvr_command *commands,int num_commands);

extern int _atsvr_input_msg_analysis_handler(_atsvr_env_t *env,char *msg,unsigned int msg_len);

extern void _atsvr_notice_ready();
//extern void _atsvr_cmd_analysis_notice_error();
extern void _atsvr_cmd_rsp_ok(_atsvr_env_t *env);
extern void _atsvr_cmd_rsp_error(_atsvr_env_t *env);
extern void _atsvr_cmd_rsp_timeout(_atsvr_env_t *env);
extern void _atsvr_output_msg(char *msg);
extern void _atsvr_output_help(const char *msg);
extern _atsvr_work_st _get_atsvr_work_state(_atsvr_env_t *env);
extern void _set_atsvr_work_state(_atsvr_env_t *env,_atsvr_work_st wk_st);
extern int _atsvr_def_config(_atsvr_env_t *env);

extern int _atsvc_command_handle(_atsvr_env_t *env, char argc,char **argv,int len);


#define ATSVR_CMD_HADLER(name_st,help_str,q_func,s_func,is_sync,sync_to,to,to_func,is_assert){\
.name = name_st,.help = help_str,.query_func = q_func,.setup_func = s_func,.sync = is_sync,\
.sync_time = sync_to, .timeout = to,.timeout_func = to_func,.bassert = is_assert\
}


#endif


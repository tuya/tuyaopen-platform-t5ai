#include "at_server.h"
#include "_at_server.h"
#include "_atsvr_func.h"
#include "atsvr_core.h"
#include "atsvr_port.h"
#include <driver/hal/hal_uart_types.h>
#if CONFIG_MISC_AT_ENABLE 
#include "bk_system_at.h"
#endif


_at_svr_ctrl_env_t _at_svr_env = {0};
extern void atsvr_task( void *para );


char *get_atsvr_version(void)
{
	return _get_atsvr_version();
}

void set_atsvr_echo_mode(_atsvr_echo_t echo)
{
	_set_atsvr_echo_mode(&_at_svr_env,(_atsvr_echo_t)echo);
}

_atsvr_echo_t get_atsvr_echo_mode(void)
{
	return (_atsvr_echo_t)_get_atsvr_echo_mode(&_at_svr_env);
}

int atsvr_register_command(const struct _atsvr_command *command)
{
	return _atsvr_register_command(&_at_svr_env,command);
}

int atsvr_register_commands(const struct _atsvr_command *commands,int num_commands, char* tag,atsvr_event_cb evecb)
{
	return _atsvr_register_commands(&_at_svr_env,commands,num_commands,tag,evecb);
}

void atsvr_notice_ready(void)
{
	_atsvr_notice_ready();
}

void atsvr_cmd_rsp_ok(void)
{
	_atsvr_cmd_rsp_ok(&_at_svr_env);
	set_atsvr_work_state(ATSVR_WK_IDLE);
}

void atsvr_cmd_rsp_error(void)
{
	_atsvr_cmd_rsp_error(&_at_svr_env);
	set_atsvr_work_state(ATSVR_WK_IDLE);
}

void atsvr_output_msg(char *msg)
{
	_atsvr_output_msg(msg);
}

_atsvr_work_st get_atsvr_work_state(void)
{
	return (_atsvr_work_st)_get_atsvr_work_state(&_at_svr_env);
}

void set_atsvr_work_state(_atsvr_work_st wk_st)
{
	_set_atsvr_work_state(&_at_svr_env,(_atsvr_work_st)wk_st);
}

int atsvr_input_msg_analysis_handler(char *msg,unsigned int msg_len)
{
	int ret = _atsvr_input_msg_analysis_handler(&_at_svr_env,msg,msg_len);
	return ret;
}


void atsvr_def_cmd_init(void)
{
	_atsvr_def_cmd_init(&_at_svr_env);
}


void atsvr_cmd_init(void)
{
#if ATSVR_INTERNAL_CMD
	extern _at_svr_ctrl_env_t _at_svr_env;
	_atsvr_def_cmd_init(&_at_svr_env);
#endif
	atsvr_extern_cmd_init();
}

void atsvr_extern_cmd_init(void)
{
#if (CONFIG_MISC_AT_ENABLE) 
	at_misc_cmd_init();
#endif
}

void at_server_init(void)
{
	//atsvr_port_env_init();
	_atsvr_def_config(&_at_svr_env);
	atsvr_cmd_init();
}

static void atsvr_handler_main( UINT32 data )
{
	unsigned int timeout = BEKEN_NEVER_TIMEOUT;
	atsvr_msg_t atsvrmsg;
	int ret;

	while (1)
	{
		ret = rtos_pop_from_queue(&_at_svr_env.msg_queue,&atsvrmsg,timeout);
		if(ret == kNoErr){
			atsvr_msg_handler(&atsvrmsg);
		}
	}
	rtos_delete_thread(NULL);
}

int atsvr_app_init(void)
{
	int ret;
	beken_thread_t handler_thread = NULL;
	beken_thread_t msg_thread = NULL;

	//at_server_init();
	if(_at_svr_env.msg_queue == NULL){
		ret = rtos_init_queue(&_at_svr_env.msg_queue,"atsvr-q",sizeof(atsvr_msg_t),ATSVR_QUEUE_MAX_NB);
		if(ret){
			ATSVRLOGW("Error: Failed to create atsvr-q (ret:%d)\r\n",ret);
			goto init_general_err;
		}
	}
#if (CONFIG_UART_PRINT_PORT != AT_UART_PORT_CFG)//temporary modify,cannot use for multicore
#if CONFIG_FREERTOS_SMP
	ret = rtos_smp_create_thread(&msg_thread,
							 BEKEN_DEFAULT_WORKER_PRIORITY,
							 "atsvr_task",
							 (beken_thread_function_t)atsvr_task,
							 1024*3,
							 0);
#else
	ret = rtos_create_thread(&msg_thread,
							 BEKEN_DEFAULT_WORKER_PRIORITY,
							 "atsvr_task",
							 (beken_thread_function_t)atsvr_task,
							 1024*3,
							 0);
#endif
	if (ret != kNoErr) {
		ATSVRLOGW("Error: Failed to create at thread: %d\r\n",ret);
		goto init_general_err;
	}
#endif
#if CONFIG_FREERTOS_SMP
	ret = rtos_smp_create_thread(&handler_thread,
								BEKEN_DEFAULT_WORKER_PRIORITY,
								"atsvr-handler",
								(beken_thread_function_t)atsvr_handler_main,
								(1024 * 3),
								0);
#else
	ret = rtos_create_thread(&handler_thread,
							BEKEN_DEFAULT_WORKER_PRIORITY,
							"atsvr-handler",
							(beken_thread_function_t)atsvr_handler_main,
							(1024 * 3),
							0);
#endif
	if (ret != kNoErr)
	{
		ATSVRLOGW("Error: Failed to create at thread: %d\r\n",ret);
		goto init_general_err;
	}

	ATSVRLOG("AT TASK Create OK!\r\n");
	return kNoErr;
init_general_err:
	if( msg_thread != NULL ) {
		rtos_delete_thread(&msg_thread);
	}
	if( handler_thread != NULL ) {
		rtos_delete_thread(&handler_thread);
	}
	if(_at_svr_env.msg_queue != NULL){
		ret = rtos_deinit_queue( &_at_svr_env.msg_queue );
		_at_svr_env.msg_queue = NULL;
	}
	return kGeneralErr;
}


#include "_atsvr_func.h"
#include "_at_server.h"
#include "_atsvr_func.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys_driver.h"
#include "../../include/os/os.h"

extern _at_svr_ctrl_env_t _at_svr_env;
extern volatile const uint8_t build_version[];

static int _atsvr_at_command(int sync,int argc, char **argv)
{
	_at_svr_ctrl_env_t *p_env = &_at_svr_env;
	if(argc != 0)
		_atsvr_cmd_rsp_error(p_env);
	else
		_atsvr_cmd_rsp_ok(p_env);
	return 0;
}

static int _atsvr_at_echo_command(int sync,int argc, char **argv)
{
	_at_svr_ctrl_env_t *p_env = &_at_svr_env;

	if(argc != 1){
		_atsvr_cmd_rsp_error(p_env);
		return -1;
	}
	int en = atoi(argv[0]);
	if(en == 0){
		_set_atsvr_echo_mode(p_env,ATSVR_ECHO_NONE);
	}else if(en == 1){
		_set_atsvr_echo_mode(p_env,ATSVR_ECHO_NORMAL);
	}else if(en == 2){
		_set_atsvr_echo_mode(p_env,ATSVR_ECHO_ALL);
	}else{
		if(strcmp(argv[0],"en") == 0){
			_set_atsvr_echo_mode(p_env,ATSVR_ECHO_NORMAL);
		}else if(strcmp(argv[0],"EN") == 0){
			_set_atsvr_echo_mode(p_env,ATSVR_ECHO_NORMAL);
		}else if(strcmp(argv[0],"dis") == 0){
			_set_atsvr_echo_mode(p_env,ATSVR_ECHO_NONE);
		}else if(strcmp(argv[0],"DIS") == 0){
			_set_atsvr_echo_mode(p_env,ATSVR_ECHO_NONE);
		}else{
			_atsvr_cmd_rsp_error(p_env);
			return 0;
		}
	}
	_atsvr_cmd_rsp_ok(p_env);
	return 0;
}

static int _atsvr_at_verion(int sync,int argc, char **argv)
{
	_at_svr_ctrl_env_t *p_env = &_at_svr_env;
	char output[150];
	int n;
	if(argc > 1){
		_atsvr_cmd_rsp_error(p_env);
		return -1;
	}
	else
	{
		n = snprintf(output,150,"EVT:version:%s\r\n",build_version);
		n += snprintf(output+n,150-n,"EVT:chip id:%x\r\n", sys_drv_get_chip_id());
		n += snprintf(output+n,150-n,"EVT:soc: %s\r\n",CONFIG_SOC_STR);
		n += snprintf(output+n,150 - n,ATSVR_CMD_RSP_SUCCEED);
		_atsvr_output_msg(output);
	}
	return 0;
}

static int _atsvr_at_atverion(int sync,int argc, char **argv)
{
	_at_svr_ctrl_env_t *p_env = &_at_svr_env;
	char output[150];
	int n;
	if(argc > 1){
		_atsvr_cmd_rsp_error(p_env);
		return -1;
	}
	else{
		n = snprintf(output,150,"EVT:AT-VERSION:%s\r\n",ATSVR_VERSION_NUM);
		n += snprintf(output+n,150 - n,ATSVR_CMD_RSP_SUCCEED);
		_atsvr_output_msg(output);
	}
	return 0;
}

static int _atsvr_help_command(int sync,int argc, char **argv)
{
	unsigned int i;
	int n;
	char resultbuf[128];
	extern _at_svr_ctrl_env_t _at_svr_env;
	_at_svr_ctrl_env_t *p_env = &_at_svr_env;
		/*full HELP print*/
		if(argc == 0)
		{
			for (i = 0; i < (p_env->cmds_reg_index); i++) 
			{
				const struct _atsvr_command **command = &(p_env->commands[i]);
				for(n = 0; n < (p_env->num_commands[i]); n++)
				{
					if ((*command)[n].name) {
						snprintf(resultbuf,128,
							"CMDRSP:cmd:%s,help:%s\r\n\r\n",(*command)[n].name,
							(*command)[n].help ? (*command)[n].help : "");
						_atsvr_output_msg(resultbuf);
					}
				}
			}
			_atsvr_cmd_rsp_ok(p_env);
		}
		else if(argc > 0)
		/*single module HELP print*/
		{
			char* name = argv[0];
			int index = 0;
			for(int m = 0; m< p_env->cmds_reg_index; m++)
			{
				if(0==strcmp(p_env->cmd_type[m],name))
				{
					index = m;
					break;
				}
			}
			if(index == 0)
			{
				_atsvr_cmd_rsp_error(p_env);
				return -1;
			}
			const struct _atsvr_command **command = &(p_env->commands[index]);
				for(n = 0; n < p_env->num_commands[index]; n++)
				{
					if ((*command)[n].name) {
						snprintf(resultbuf,128,
							"CMDRSP:cmd:%s,help:%s\r\n",(*command)[n].name,
							(*command)[n].help ? (*command)[n].help : "");
						_atsvr_output_msg(resultbuf);
					}
				}
				_atsvr_cmd_rsp_ok(p_env);
		}
		else 
		{
			_atsvr_cmd_rsp_error(p_env);
		}
	return 0;
}

const struct _atsvr_command _atsvc_cmds_table[] = {
	ATSVR_CMD_HADLER("AT","AT",NULL,_atsvr_at_command,false,0,0,NULL,false),
	ATSVR_CMD_HADLER("AT+HELP","AT+HELP",NULL,_atsvr_help_command,false,0,0,NULL,false),
	ATSVR_CMD_HADLER("AT+VERSION","get firmware version:AT+VERSION",NULL,_atsvr_at_verion,false,0,0,NULL,false),
	ATSVR_CMD_HADLER("AT+ATVERSION","get AT version:AT+ATVERSION",NULL,_atsvr_at_atverion,false,0,0,NULL,false),
};

void _atsvr_def_cmd_init(_atsvr_env_t *env)
{
	_atsvr_register_commands(env,_atsvc_cmds_table, sizeof(_atsvc_cmds_table) / sizeof(struct _atsvr_command),"at_core",NULL);
}


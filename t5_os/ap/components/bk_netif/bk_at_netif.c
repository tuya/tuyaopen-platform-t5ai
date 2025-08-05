#include <common/bk_typedef.h>

//#include "common.h"
#include <../../../include/os/os.h>


#include "atsvr_unite.h"
#include "bk_at_netif.h"




/*global variable defination*/

#define TAG "AT_NETIF"

static int netif_at_demo(int sync,int argc, char **argv)
{

	BK_LOGD(TAG,"This is NETIF AT DEMO\r\n");
	return 0;
}


const struct _atsvr_command netif_cmds_table[] = {
	/*STA*/
	ATSVR_CMD_HADLER("AT+NETIF","AT+NETIF",NULL,netif_at_demo,false,0,0,NULL,false),

};

void netif_at_cmd_init(void)
{
	int ret;
	ret = atsvr_register_commands(netif_cmds_table, sizeof(netif_cmds_table) / sizeof(netif_cmds_table[0]),"netif",NULL);
	if(0 == ret)
		BK_LOGD(TAG,"NETIF AT CMDS INIT OK\r\n");
}


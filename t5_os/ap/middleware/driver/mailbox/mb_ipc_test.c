// Copyright 2020-2024 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <string.h>

#include <os/os.h>
#include <driver/mailbox_channel.h>
#include <driver/mb_ipc.h>
#include <driver/mb_ipc_port_cfg.h>

#include <os/rtos_ext.h>

#define TEST_SERVER_SEND

#define SVR_CONNECT_MAX           3
/* connection ID bits, 8 bits for max 8 connections. */
#define SVR_CONNECT_FLAGS         ((0x01 << (SVR_CONNECT_MAX)) - 1)   //  0x00FF

enum
{
	//   cpu0 servers
	CPU0_SERVER_ID_START_test = IPC_SVR_ID_START(0),
	reserved_s1, // for flash server.
	TEST_SVR1_ID,
	
	//   cpu1 servers
	CPU1_SERVER_ID_START_test = IPC_SVR_ID_START(1),
	TEST_SVR2_ID,
	
	//   cpu2 servers
	CPU2_SERVER_ID_START_test = IPC_SVR_ID_START(2),
	TEST_SVR3_ID,
	
} ;

enum
{
	//   cpu1 clients
	CPU1_CLIENT_ID_START_test = IPC_CLIENT_ID_START(1),
	reserved_c1,
	TEST1_CLIENT_ID,
	TEST2_CLIENT_ID,
	TEST3_CLIENT_ID,

} ;

static const char  *  test_svr_name[] = {"svr1", "svr2"};
static const u8       svr_id[] = {TEST_SVR1_ID, TEST_SVR2_ID, TEST_SVR3_ID};
static const u8       clnt_id[] = {TEST1_CLIENT_ID, TEST2_CLIENT_ID, TEST3_CLIENT_ID};
static const char  *  test_client_name[] = {"client1", "client2", "client3"};

volatile int    client_start = 0;

static rtos_event_ext_t  svr1_event;

int mb_ipc_get_connection(u32 handle, u8 *src, u8 * dst);

/* ========================================================
 *
 *        SERVER-1
 *
 ==========================================================*/
 
static u32 svr_rx_callback(u32 handle, u32 connect_id)
{
	u32  connect_flag;

	if(connect_id >= SVR_CONNECT_MAX)
		return 0;

	connect_flag = 0x01 << connect_id;

	rtos_set_event_ex(&svr1_event, connect_flag);

	return 0;	
}

static void svr_connect_handler(u32 handle)
{
	u32  cmd_id;

	int ret_val = mb_ipc_get_recv_event(handle, &cmd_id);
	if(ret_val != 0)  // failed
	{
		BK_LOGD(NULL, "get evt fail %x %d.\r\n", handle, ret_val);
		return;
	}

	if(cmd_id > MB_IPC_CMD_MAX)
	{
		BK_LOGD(NULL, "cmd-id error %d.\r\n", cmd_id);
		return;
	}
	
	u8  src =0, dst = 0;

	mb_ipc_get_connection(handle, &src, &dst);

	if(cmd_id == MB_IPC_DISCONNECT_CMD)
	{
		BK_LOGD(NULL, "disconnect 0x%x, %x-%x.\r\n", handle, src, dst);
	}
	else if(cmd_id == MB_IPC_CONNECT_CMD)
	{
		BK_LOGD(NULL, "connect 0x%x, %x-%x.\r\n", handle, src, dst);
	}
	else if(cmd_id == MB_IPC_SEND_CMD)
	{
		int   rem_len = mb_ipc_get_recv_data_len(handle);
		
		BK_LOGD(NULL, "==recv 0x%x, %x-%x,  %d bytes.\r\n", handle, src, dst, rem_len);

		u8       data_buff[32];
		int      read_len = 0;

		if(rem_len == 0)
		{
			u8       user_cmd;
			read_len = mb_ipc_recv(handle, &user_cmd, data_buff, 0, 0);
			if(read_len < 0)
			{
				BK_LOGD(NULL, "==recv cmd failed! %d\r\n", read_len);
			}
			else
			{
				BK_LOGD(NULL, "==recv cmd= %d\r\n", user_cmd);
			}
		}

		while(rem_len > 0)
		{
			read_len = mb_ipc_recv(handle, NULL, data_buff, sizeof(data_buff), 0);

			if(read_len < 0)
			{
				BK_LOGD(NULL, "==recv failed! %d\r\n", read_len);
				break;
			}
			else if(read_len > 0)
			{
				rem_len -= read_len;
			//	BK_LOGD(NULL, "==> recv %d bytes\r\n", read_len);
				// for(int i = 0; i < read_len; i++)
				{
				//	BK_LOG_RAW("%02x ", data_buff[i]);
				}
			
				if(read_len < sizeof(data_buff))
				{
				//	BK_LOGD(NULL, "recv complete!\r\n");
					break;
				}
			}
			else
			{
			//	BK_LOGD(NULL, "recv failed!\r\n");
				break;
			}
		}

		// send back the cmd.

		#ifdef TEST_SERVER_SEND
		int      send_result = mb_ipc_send(handle, 0x80+SRC_CPU, data_buff, 16, 500);

		BK_LOGD(NULL, "--->send  0x%x, %x-%x, result %d.\r\n", handle, src, dst, send_result);
		#endif

	}

}

void mb_ipc_test_svr1(void * param)
{
	u32  handle, svr_port;
	u32  connect_handle;
	
	rtos_init_event_ex(&svr1_event);

	svr_port = (u32)param;
	
	handle = mb_ipc_socket((u8)svr_port, svr_rx_callback);

	if(handle == 0)
	{
		BK_LOGE("ipc_svr", "create_socket failed.\r\n");

		rtos_deinit_event_ex(&svr1_event);
		rtos_delete_thread(NULL);
		
		return ;
	}

	while(1)
	{
		u32 event = rtos_wait_event_ex(&svr1_event, SVR_CONNECT_FLAGS, 1, 2000);

		if(event == 0)  // timeout.
		{
			continue;
		}

		for(int i = 0; i < SVR_CONNECT_MAX; i++)
		{
			if(event & (0x01 << i))
			{
				connect_handle = mb_ipc_server_get_connect_handle(handle, i);
				svr_connect_handler(connect_handle);
			}
		}
	}

	mb_ipc_server_close(handle, 1000);
	
	rtos_deinit_event_ex(&svr1_event);
	rtos_delete_thread(NULL);
}

/* ========================================================
 *
 *        client
 *
 *=======================================================*/

void mb_ipc_test_client(void * param)
{
	int   client_id = (int)param;
	
	// test svr-1

	// wait start.
	while(client_start == 0)
	{
		BK_LOGD(NULL, "==> client_start addr=0x%x.\r\n", &client_start);
		rtos_delay_milliseconds(500);
	}

	rtos_delay_milliseconds(2000);

	u32 handle = mb_ipc_socket(IPC_GET_ID_PORT(clnt_id[client_id]), NULL);
	if(handle == 0)
	{
		BK_LOGD(NULL, "client-%d create socket failed\r\n", client_id);
		goto client_exit;
	}
	
	int ret_val = mb_ipc_connect(handle, IPC_GET_ID_CPU(svr_id[client_id]), IPC_GET_ID_PORT(svr_id[client_id]), 500);

	if(ret_val != 0)
	{
		BK_LOGD(NULL, "client-%d connect failed %d\r\n", client_id, ret_val);
		goto client_exit;
	}

	u8  src =0, dst = 0;

	mb_ipc_get_connection(handle, &src, &dst);
	
	u8   test_data[80];
	for(int i = 0;  i < sizeof(test_data); i++)
	{
		test_data[i] = i;
	}

	for(int i = 0;  i < 100; i++)
	{
		u8   user_cmd = i & 0x1f;
		
		ret_val = mb_ipc_send(handle, user_cmd, test_data + user_cmd, 16 + user_cmd, 200);
		if(ret_val != 0)
		{
			BK_LOGD(NULL, "client-%d %x-%x handle-%x, send %d failed %d.\r\n", client_id, src, dst, handle, user_cmd, ret_val);
			i--;  // retry...
			rtos_delay_milliseconds(100);
			continue;
		}
		else
		{
			BK_LOGD(NULL, "client-%d send successfully, cmd=0x%02x.\r\n", client_id, user_cmd);
		}

		#ifdef TEST_SERVER_SEND
		u8       data_buff[32];
		int      read_len = 0;

		u32      time_out = 200;

		do
		{
			read_len = mb_ipc_recv(handle, &user_cmd, data_buff, sizeof(data_buff), time_out);

			if(read_len < 0)
			{
				BK_LOGD(NULL, "--recv failed! %d\r\n", read_len);
				break;
			}
			else if(read_len == 0)
			{
				// if it is the first loop!
				if((user_cmd == INVALID_USER_CMD_ID) && (time_out != 0))
				{
					BK_LOGD(NULL, "--recv cmd failed!\r\n");
				}

				BK_LOGD(NULL, "--recv complete! cmd= %d\r\n", user_cmd);
				break;
			}
			else // (read_len > 0)
			{
				BK_LOGD(NULL, "--recv %d bytes\r\n", read_len);
				for(int i = 0; i < read_len; i++)
				{
				//	BK_LOG_RAW("%02x ", data_buff[i]);
				}
			}
			
			time_out = 0;   // time_out = 0 for subsequent loops!
			
		} while(read_len == sizeof(data_buff));
		#endif

	//	rtos_delay_milliseconds(100);
	}

client_exit:

	if(handle != 0)
	{
		ret_val = mb_ipc_close(handle, 500);

		if(ret_val != 0)
		{
			BK_LOGD(NULL, "close failed, client-%d, %x-%x handle-%x, ret: %d.\r\n", client_id, src, dst, handle, ret_val);
		}
	}
	
	rtos_delete_thread(NULL);
	
	return ;
}

int mb_ipc_test_init(void)
{

	#if IPC_GET_ID_CPU(TEST_SVR2_ID) !=  MAILBOX_CPU1
	#error server cpu configuration error!
	#endif
	rtos_create_thread(NULL, 3, test_svr_name[0], mb_ipc_test_svr1, 2048, (void *)IPC_GET_ID_PORT(TEST_SVR2_ID));

	for(int i = 0; i < 3; i ++)  // create 3 client for each cpu.
	{
		rtos_create_thread(NULL, 4+i/2, test_client_name[i], mb_ipc_test_client, 2048, (void *) i);
	}

	return 0;
}


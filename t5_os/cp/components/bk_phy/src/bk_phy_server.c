// Copyright 2020-2021 Beken
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

#include <string.h>
#include <common/bk_include.h>
#include <os/mem.h>
#include <os/os.h>
#include "bk_phy_ipc.h"
#include <driver/mb_ipc.h>
#include <driver/mb_ipc_port_cfg.h>
#include <os/rtos_ext.h>
#include <components/sensor.h>

#define TAG                             "bk_phy_s"

#define LOCAL_TRACE_E                  (1)
#define LOCAL_TRACE_I                  (0)

#define TRACE_E(...)                   do { if(LOCAL_TRACE_E) BK_LOGE(__VA_ARGS__); } while(0)
#define TRACE_I(...)                   do { if(LOCAL_TRACE_I) BK_LOGD(__VA_ARGS__); } while(0)

#define PHY_SVR_PRIORITY               BEKEN_DEFAULT_WORKER_PRIORITY
#define PHY_SVR_STACK_SIZE             1024

#define PHY_SVR_CONNECT_MAX            2
#define PHY_SVR_CONNECT_EVENTS         ((0x01 << (PHY_SVR_CONNECT_MAX)) - 1)
#define PHY_SVR_QUIT_EVENT             (0x100)

#define PHY_SVR_EVENTS                 (PHY_SVR_CONNECT_EVENTS | PHY_SVR_QUIT_EVENT)

#define PHY_SVR_WAIT_TIME              50

static u8 s_phy_svr_init =             0;
static rtos_event_ext_t  phy_svr_event;

static uint32_t calc_crc32(uint32_t crc, const uint8_t *buf, int len)
{
    uint8_t i;

    while (len--) {
        crc ^= *buf++;
        for (i = 8; i > 0; i--) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

static void phy_error_handler(u32 handle, u8 user_cmd)
{
	phy_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(phy_cmd_t));
	cmd_buff.ret_status = BK_FAIL;
	mb_ipc_send(handle, user_cmd, (u8 *)&cmd_buff, sizeof(phy_cmd_t), PHY_SVR_WAIT_TIME);
}

static void bk_phy_get_current_temperature_handler(u32 handle, phy_cmd_t *cmd_buff)
{
	cmd_buff->ret_status = bk_sensor_get_current_temperature(&cmd_buff->param);
	int ret_val = mb_ipc_send(handle, PHY_CMD_GET_TEMP, (u8 *)cmd_buff, sizeof(phy_cmd_t), PHY_SVR_WAIT_TIME);
	if(ret_val != 0)
		TRACE_I(TAG, "0x%x, start: %d, %d.\r\n", handle, cmd_buff->ret_status, ret_val);
}

static void bk_phy_get_current_voltage_handler(u32 handle, phy_cmd_t *cmd_buff)
{
	cmd_buff->ret_status = bk_sensor_get_current_voltage(&cmd_buff->param);
	int ret_val = mb_ipc_send(handle, PHY_CMD_GET_VOLT, (u8 *)cmd_buff, sizeof(phy_cmd_t), PHY_SVR_WAIT_TIME);
	if(ret_val != 0)
		TRACE_I(TAG, "0x%x, start: %d, %d.\r\n", handle, cmd_buff->ret_status, ret_val);
}


static void phy_cmd_handler(u32 handle, u8 connect_id)
{
	phy_cmd_t cmd_buff;
	int recv_len = mb_ipc_get_recv_data_len(handle);
	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(phy_cmd_t));

	if(recv_len != sizeof(phy_cmd_t))
	{
		mb_ipc_recv(handle, &user_cmd, NULL, 0, 0);

		// if client wait_forever, then must send a response even if user_cmd == INVALID_USER_CMD_ID.
		// flash client does not wait forever, so only respond then cmd is valid.
		if(user_cmd != INVALID_USER_CMD_ID)
			phy_error_handler(handle, user_cmd);

		TRACE_I(TAG, "recv user_cmd=%d, data len failed! %d,cmd_buff_size=%d\r\n", user_cmd, recv_len,sizeof(phy_cmd_t));

		return;
	}

	recv_len = mb_ipc_recv(handle, &user_cmd, (u8 *)&cmd_buff, sizeof(phy_cmd_t), 0);

	if(recv_len != sizeof(phy_cmd_t))
	{
		// if client wait_forever, then must send a response even if user_cmd == INVALID_USER_CMD_ID.
		// flash client does not wait forever, so only respond then cmd is valid.
		if(user_cmd != INVALID_USER_CMD_ID)
			phy_error_handler(handle, user_cmd);

		TRACE_I(TAG, "recv user_cmd=%d, data len failed! %d,cmd_buff_size=%d\r\n", user_cmd, recv_len,sizeof(phy_cmd_t));

		return;
	}

	switch(user_cmd)
	{
		case PHY_CMD_GET_TEMP:
			bk_phy_get_current_temperature_handler(handle, &cmd_buff);
			break;
		case PHY_CMD_GET_VOLT:
			bk_phy_get_current_voltage_handler(handle, &cmd_buff);
			break;
		default:
			phy_error_handler(handle, user_cmd);
			TRACE_I(TAG, "0x%x, unknown cmd%d!\r\n", handle, user_cmd);
			break;
	}
}

static void phy_svr_connect_handler(u32 handle, u8 connect_id)
{
	u32  cmd_id;

	int ret_val = mb_ipc_get_recv_event(handle, &cmd_id);

	if(ret_val != 0)  // failed
	{
		TRACE_I(TAG, "get evt fail %x %d.\r\n", handle, ret_val);
		return;
	}

	if(cmd_id > MB_IPC_CMD_MAX)
	{
		TRACE_I(TAG, "cmd-id error %d.\r\n", cmd_id);
		return;
	}

	u8  src =0, dst = 0;

	extern int mb_ipc_get_connection(u32 handle, u8 *src, u8 * dst);

	mb_ipc_get_connection(handle, &src, &dst);

	if(cmd_id == MB_IPC_SEND_CMD)
	{
		phy_cmd_handler(handle, connect_id);
	}
	else  /* any other commands. */
	{
		TRACE_I(TAG, "cmd=%d, 0x%x, %x-%x.\r\n", cmd_id, handle, src, dst);
	}
}

static void phy_ipc_disconnect_handler(u32 handle)
{
	TRACE_I(TAG, "0x%x disconnected\r\n", handle);
}

static u32 phy_svr_rx_callback(u32 handle, u32 connect_id)
{
	u32  connect_flag;

	if(connect_id >= PHY_SVR_CONNECT_MAX)
		return 0;

	connect_flag = 0x01 << connect_id;

	rtos_set_event_ex(&phy_svr_event, connect_flag);

	return 0;
}

static void phy_svr_task(void *param)
{
	u32 event;
	u32  handle;
	u32  connect_handle;

	if(rtos_init_event_ex(&phy_svr_event) != BK_OK)
	{
		rtos_delete_thread(NULL);
		return;
	}

	handle = mb_ipc_socket(IPC_GET_ID_PORT(PHY_SERVER), phy_svr_rx_callback);

	if(handle == 0)
	{
		TRACE_I("ipc_svr", "create_socket failed.\r\n");

		rtos_deinit_event_ex(&phy_svr_event);
		rtos_delete_thread(NULL);

		return ;
	}

	s_phy_svr_init = 2;

	while(1)
	{
		event = rtos_wait_event_ex(&phy_svr_event, PHY_SVR_EVENTS, 1, BEKEN_WAIT_FOREVER);

		if(event == 0)  // timeout.
		{
			continue;
		}

		if(event & PHY_SVR_QUIT_EVENT)
		{
			TRACE_I(TAG, "saradc server task quit\r\n");
			break;
		}

		// Handle connection events
		for(int i = 0; i < PHY_SVR_CONNECT_MAX; i++)
		{
			if(event & (0x01 << i))
			{
				connect_handle = mb_ipc_server_get_connect_handle(handle, i);
				phy_svr_connect_handler(connect_handle, i);
			}
		}
	}
	mb_ipc_server_close(handle, PHY_SVR_WAIT_TIME);
	rtos_deinit_event_ex(&phy_svr_event);
	s_phy_svr_init = 0;
	rtos_delete_thread(NULL);
}

bk_err_t bk_phy_server_init(void)
{
	bk_err_t ret = BK_OK;

	if(s_phy_svr_init)
	{
		return BK_OK;
	}

	s_phy_svr_init = 1;

	#if IPC_GET_ID_CPU(PHY_SERVER) != 0   // != CPU0.
	#error server cpu configuration error!
	#endif

	// Create server task
	beken_thread_t phy_svr_thread = NULL;
	ret = rtos_create_thread(&phy_svr_thread,
							 PHY_SVR_PRIORITY,
							 "phy_svr",
							 phy_svr_task,
							 PHY_SVR_STACK_SIZE,
							 NULL);

	if(ret != BK_OK)
	{
		TRACE_I(TAG, "phy server task create failed\r\n");
		rtos_delete_thread(NULL);
		return ret;
	}

	TRACE_I(TAG, "phy server init ok\r\n");

	return BK_OK;
}

bk_err_t bk_phy_server_deinit(void)
{
    if(s_phy_svr_init != 2)
    {
        return BK_OK;
    }
    s_phy_svr_init = 1;

    rtos_set_event_ex(&phy_svr_event, PHY_SVR_QUIT_EVENT);

    TRACE_I(TAG, "phy server deinit ok\r\n");

    return BK_OK;
}

// eof
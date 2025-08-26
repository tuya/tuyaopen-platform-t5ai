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
#include <driver/adc.h>
#include <os/os.h>
#include "saradc_ipc.h"
#include <driver/mb_ipc.h>
#include <driver/mb_ipc_port_cfg.h>
#include <os/rtos_ext.h>
#include "adc_driver.h"
#include <modules/pm.h>

#define TAG		"saradc_s"

#define LOCAL_TRACE_E     (1)
#define LOCAL_TRACE_I     (0)

#define TRACE_E(...)        do { if(LOCAL_TRACE_E) BK_LOGE(__VA_ARGS__); } while(0)
#define TRACE_I(...)        do { if(LOCAL_TRACE_I) BK_LOGD(__VA_ARGS__); } while(0)

#define SARADC_SVR_PRIORITY               BEKEN_DEFAULT_WORKER_PRIORITY
#define SARADC_SVR_STACK_SIZE             1536

#define SARADC_SVR_CONNECT_MAX            2
#define SARADC_SVR_CONNECT_EVENTS         ((0x01 << (SARADC_SVR_CONNECT_MAX)) - 1)
#define SARADC_SVR_QUIT_EVENT             (0x100)

#define SARADC_SVR_EVENTS         (SARADC_SVR_CONNECT_EVENTS | SARADC_SVR_QUIT_EVENT)

#define SARADC_SVR_WAIT_TIME      50

static u8 s_saradc_svr_init = 0;
static rtos_event_ext_t  saradc_svr_event;
static u16 saradc_buff[32];

// CRC32 table for data integrity check
static uint32_t calc_crc32(uint32_t crc, const uint8_t *buf, int len)
{
	uint8_t i;
	while(len--){
		crc ^=*buf++;
		for(i = 8; i> 0; i--){
             if(crc & 0x80){
               crc = (crc << 1) ^ 0x31;
			 }else {
			   crc <<= 1;
			 }

		}
	}
	return crc;
}

static void saradc_error_handler(u32 handle, u8 user_cmd)
{
	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(saradc_cmd_t));
	cmd_buff.ret_status = BK_FAIL;
	mb_ipc_send(handle, user_cmd, (u8 *)&cmd_buff, sizeof(saradc_cmd_t), SARADC_SVR_WAIT_TIME);
}

static void saradc_acquire_handler(u32 handle, saradc_cmd_t *cmd_buff)
{
	cmd_buff->ret_status = bk_adc_acquire();
	int ret_val = mb_ipc_send(handle, SARADC_CMD_ACQUIRE, (u8 *)cmd_buff, sizeof(saradc_cmd_t), SARADC_SVR_WAIT_TIME);
	if(ret_val != 0)
		TRACE_I(TAG, "0x%x, acquire: %d, %d.\r\n", handle, cmd_buff->ret_status, ret_val);
}

static void saradc_init_handler(u32 handle, saradc_cmd_t *cmd_buff)
{
	cmd_buff->ret_status = bk_adc_init(cmd_buff->config.chan);
	int ret_val = mb_ipc_send(handle, SARADC_CMD_INIT, (u8 *)cmd_buff, sizeof(saradc_cmd_t), SARADC_SVR_WAIT_TIME);
	if(ret_val != 0)
		TRACE_I(TAG, "0x%x, init chan%d: %d, %d.\r\n", handle, cmd_buff->config.chan, cmd_buff->ret_status, ret_val);
}

static void saradc_enable_bypass_calibration_handler(u32 handle, saradc_cmd_t *cmd_buff)
{
	cmd_buff->ret_status = bk_adc_enable_bypass_clalibration();
	int ret_val = mb_ipc_send(handle, SARADC_CMD_ENABLE_BYPASS_CALIBRATION, (u8 *)cmd_buff, sizeof(saradc_cmd_t), SARADC_SVR_WAIT_TIME);
	if(ret_val != 0)
		TRACE_I(TAG, "0x%x, enable bypass calibration: %d, %d.\r\n", handle, cmd_buff->ret_status, ret_val);
}

static void saradc_start_handler(u32 handle, saradc_cmd_t *cmd_buff)
{
	cmd_buff->ret_status = bk_adc_start();
	int ret_val = mb_ipc_send(handle, SARADC_CMD_START, (u8 *)cmd_buff, sizeof(saradc_cmd_t), SARADC_SVR_WAIT_TIME);
	if(ret_val != 0)
		TRACE_I(TAG, "0x%x, start: %d, %d.\r\n", handle, cmd_buff->ret_status, ret_val);
}

static void saradc_read_raw_handler(u32 handle, saradc_cmd_t *cmd_buff, u8 connect_id)
{
	int read_status = BK_OK;
	u16 *read_buff = saradc_buff;

	if(cmd_buff->size > sizeof(saradc_buff))
	{
		saradc_error_handler(handle, SARADC_CMD_READ_RAW);
		TRACE_I(TAG, "%s @%d, 0x%x buffer overflow!\r\n", __FUNCTION__, __LINE__, handle);
		return;
	}

	read_status = bk_adc_read_raw(read_buff, cmd_buff->size, cmd_buff->timeout);

	cmd_buff->ret_status = read_status;
	//cmd_buff->buff = read_buff;
	memcpy(cmd_buff->buff, read_buff, cmd_buff->size * sizeof(u16));

	cmd_buff->crc = calc_crc32(0, (UINT8 *)read_buff, cmd_buff->size * sizeof(uint16_t));

	int ret_val = mb_ipc_send(handle, SARADC_CMD_READ_RAW, (u8 *)cmd_buff, sizeof(saradc_cmd_t), SARADC_SVR_WAIT_TIME);

	if(read_status != BK_OK)
	{
		TRACE_I(TAG, "%s @%d, 0x%x read failed!\r\n", __FUNCTION__, __LINE__, handle);
		return;
	}

	if(ret_val != 0)
	{
		(*read_buff) ^= 0x01;   // make crc not match
		TRACE_I(TAG, "%s @%d, 0x%x send failed, ret_val = %d!\r\n", __FUNCTION__, __LINE__, handle, ret_val);
		return;
	}

	u32 event = rtos_wait_event_ex(&saradc_svr_event, (0x01 << connect_id), 1, SARADC_SVR_WAIT_TIME);

	if(event == 0)
	{
		(*read_buff) ^= 0x01;   // make crc not match
		TRACE_I(TAG, "%s @%d, 0x%x read done failed!\r\n", __FUNCTION__, __LINE__, handle);
		return;
	}

	memset(cmd_buff, 0, sizeof(saradc_cmd_t));
	u8 user_cmd = INVALID_USER_CMD_ID;
	ret_val = mb_ipc_recv(handle, &user_cmd, (u8 *)cmd_buff, sizeof(saradc_cmd_t), SARADC_SVR_WAIT_TIME);

	if((ret_val != sizeof(saradc_cmd_t)) || (user_cmd != SARADC_CMD_READ_RAW_DONE))
	{
		TRACE_I(TAG, "%s @%d, 0x%x recv cmd%d, %d, 0x%x!\r\n", __FUNCTION__, __LINE__, handle, user_cmd, ret_val, event);
	}
	else
	{
		memset(cmd_buff, 0, sizeof(saradc_cmd_t));
		cmd_buff->ret_status = BK_OK;
		mb_ipc_send(handle, SARADC_CMD_READ_RAW_DONE, (u8 *)cmd_buff, sizeof(saradc_cmd_t), SARADC_SVR_WAIT_TIME);
	}
}

static void saradc_read_handler(u32 handle, saradc_cmd_t *cmd_buff, u8 connect_id)
{
	int read_status = BK_OK;
	uint16_t read_buff = 0;

	if(cmd_buff->size > sizeof(saradc_buff))
	{
		saradc_error_handler(handle, SARADC_CMD_READ);
		TRACE_I(TAG, "%s @%d, 0x%x buffer overflow!\r\n", __FUNCTION__, __LINE__, handle);
		return;
	}

	read_status = bk_adc_read(&read_buff, cmd_buff->timeout);
	cmd_buff->ret_status = read_status;
	cmd_buff->buff[0] = read_buff;
	cmd_buff->crc = calc_crc32(0, (UINT8 *)&read_buff, sizeof(uint16_t));

	int ret_val = mb_ipc_send(handle, SARADC_CMD_READ, (u8 *)cmd_buff, sizeof(saradc_cmd_t), SARADC_SVR_WAIT_TIME);

	if(read_status != BK_OK)
	{
		TRACE_I(TAG, "%s @%d, 0x%x read failed!\r\n", __FUNCTION__, __LINE__, handle);
		return;
	}

	if(ret_val != 0)
	{
		(read_buff) ^= 0x01;   // make crc not match
		TRACE_I(TAG, "%s @%d, 0x%x send failed, ret_val = %d!\r\n", __FUNCTION__, __LINE__, handle, ret_val);
		return;
	}

	u32 event = rtos_wait_event_ex(&saradc_svr_event, (0x01 << connect_id), 1, SARADC_SVR_WAIT_TIME);

	if(event == 0)
	{
		(read_buff) ^= 0x01;   // make crc not match
		TRACE_I(TAG, "%s @%d, 0x%x read done failed!\r\n", __FUNCTION__, __LINE__, handle);
		return;
	}

	memset(cmd_buff, 0, sizeof(saradc_cmd_t));
	u8 user_cmd = INVALID_USER_CMD_ID;
	ret_val = mb_ipc_recv(handle, &user_cmd, (u8 *)cmd_buff, sizeof(saradc_cmd_t), SARADC_SVR_WAIT_TIME);

	if((ret_val != sizeof(saradc_cmd_t)) || (user_cmd != SARADC_CMD_READ_DONE))
	{
		TRACE_I(TAG, "%s @%d, 0x%x recv12121 cmd%d, %d, 0x%x!\r\n", __FUNCTION__, __LINE__, handle, user_cmd, ret_val, event);
	}
	else
	{
		memset(cmd_buff, 0, sizeof(saradc_cmd_t));
		cmd_buff->ret_status = BK_OK;
		mb_ipc_send(handle, SARADC_CMD_READ_DONE, (u8 *)cmd_buff, sizeof(saradc_cmd_t), SARADC_SVR_WAIT_TIME);
	}
}

static void saradc_stop_handler(u32 handle, saradc_cmd_t *cmd_buff)
{
	cmd_buff->ret_status = bk_adc_stop();
	int ret_val = mb_ipc_send(handle, SARADC_CMD_STOP, (u8 *)cmd_buff, sizeof(saradc_cmd_t), SARADC_SVR_WAIT_TIME);
	if(ret_val != 0)
		TRACE_I(TAG, "0x%x, stop: %d, %d.\r\n", handle, cmd_buff->ret_status, ret_val);
}

static void saradc_deinit_handler(u32 handle, saradc_cmd_t *cmd_buff)
{
	cmd_buff->ret_status = bk_adc_deinit(cmd_buff->config.chan);
	int ret_val = mb_ipc_send(handle, SARADC_CMD_DEINIT, (u8 *)cmd_buff, sizeof(saradc_cmd_t), SARADC_SVR_WAIT_TIME);
	if(ret_val != 0)
		TRACE_I(TAG, "0x%x, deinit chan%d: %d, %d.\r\n", handle, cmd_buff->config.chan, cmd_buff->ret_status, ret_val);
}

static void saradc_release_handler(u32 handle, saradc_cmd_t *cmd_buff)
{
	cmd_buff->ret_status = bk_adc_release();
	int ret_val = mb_ipc_send(handle, SARADC_CMD_RELEASE, (u8 *)cmd_buff, sizeof(saradc_cmd_t), SARADC_SVR_WAIT_TIME);
	if(ret_val != 0)
		TRACE_I(TAG, "0x%x, release: %d, %d.\r\n", handle, cmd_buff->ret_status, ret_val);
}

static void saradc_set_config_handler(u32 handle, saradc_cmd_t *cmd_buff)
{
	cmd_buff->ret_status = bk_adc_set_config(&cmd_buff->config);
	int ret_val = mb_ipc_send(handle, SARADC_CMD_SET_CONFIG, (u8 *)cmd_buff, sizeof(saradc_cmd_t), SARADC_SVR_WAIT_TIME);
	if(ret_val != 0)
		TRACE_I(TAG, "0x%x, set config: %d, %d.\r\n", handle, cmd_buff->ret_status, ret_val);
}

static void saradc_en_handler(u32 handle, saradc_cmd_t *cmd_buff)
{
	cmd_buff->ret_status = bk_adc_en();
	int ret_val = mb_ipc_send(handle, SARADC_CMD_EN, (u8 *)cmd_buff, sizeof(saradc_cmd_t), SARADC_SVR_WAIT_TIME);
	if(ret_val != 0)
		TRACE_I(TAG, "0x%x, enable: %d, %d.\r\n", handle, cmd_buff->ret_status, ret_val);
}

static void saradc_data_calculate_handler(u32 handle, saradc_cmd_t *cmd_buff)
{
	float adc_calculate = 0.0;

	adc_calculate = bk_adc_data_calculate(cmd_buff->adc_cali_data, cmd_buff->config.chan);
	cmd_buff->ret_status = BK_OK;
	cmd_buff->adc_cali_data = (UINT16)(adc_calculate * 1000);
	int ret_val = mb_ipc_send(handle, SARADC_CMD_DATA_CALCULATE, (u8 *)cmd_buff, sizeof(saradc_cmd_t), SARADC_SVR_WAIT_TIME);
	if(ret_val != 0)
		TRACE_I(TAG, "0x%x, adc calculate: %d, %d.\r\n", handle, cmd_buff->ret_status, ret_val);
}

static void saradc_cmd_handler(u32 handle, u8 connect_id)
{
	saradc_cmd_t cmd_buff;
	int recv_len = mb_ipc_get_recv_data_len(handle);
	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(saradc_cmd_t));

	if(recv_len != sizeof(saradc_cmd_t))
	{
		mb_ipc_recv(handle, &user_cmd, NULL, 0, 0);  // data_buff == NULL or buff_len == 0 just discard all data.

		// if client wait_forever, then must send a response even if user_cmd == INVALID_USER_CMD_ID.
		// flash client does not wait forever, so only respond then cmd is valid.
		if(user_cmd != INVALID_USER_CMD_ID)
			saradc_error_handler(handle, user_cmd);

		TRACE_I(TAG, "recv user_cmd=%d, data len failed! %d,cmd_buff_size=%d\r\n", user_cmd, recv_len,sizeof(saradc_cmd_t));

		return;
	}

	recv_len = mb_ipc_recv(handle, &user_cmd, (u8 *)&cmd_buff, sizeof(saradc_cmd_t), 0);

	if(recv_len != sizeof(saradc_cmd_t))
	{
		// if client wait_forever, then must send a response even if user_cmd == INVALID_USER_CMD_ID.
		// flash client does not wait forever, so only respond then cmd is valid.
		if(user_cmd != INVALID_USER_CMD_ID)
			saradc_error_handler(handle, user_cmd);

		TRACE_I(TAG, "recv user_cmd=%d, data len failed! %d,cmd_buff_size=%d\r\n", user_cmd, recv_len,sizeof(saradc_cmd_t));

		return;
	}

	switch(user_cmd)
	{
		case SARADC_CMD_ACQUIRE:
			saradc_acquire_handler(handle, &cmd_buff);
			break;

		case SARADC_CMD_INIT:
			saradc_init_handler(handle, &cmd_buff);
			break;

		case SARADC_CMD_ENABLE_BYPASS_CALIBRATION:
			saradc_enable_bypass_calibration_handler(handle, &cmd_buff);
			break;

		case SARADC_CMD_START:
			saradc_start_handler(handle, &cmd_buff);
			break;

		case SARADC_CMD_READ_RAW:
			saradc_read_raw_handler(handle, &cmd_buff, connect_id);
			break;

		case SARADC_CMD_READ:
			saradc_read_handler(handle, &cmd_buff, connect_id);
			break;

		case SARADC_CMD_STOP:
			saradc_stop_handler(handle, &cmd_buff);
			break;

		case SARADC_CMD_DEINIT:
			saradc_deinit_handler(handle, &cmd_buff);
			break;

		case SARADC_CMD_RELEASE:
			saradc_release_handler(handle, &cmd_buff);
			break;

		case SARADC_CMD_SET_CONFIG:
			saradc_set_config_handler(handle, &cmd_buff);
			break;

		case SARADC_CMD_EN:
			saradc_en_handler(handle, &cmd_buff);
			break;

		case SARADC_CMD_DATA_CALCULATE:
			saradc_data_calculate_handler(handle, &cmd_buff);
			break;

		default:
			saradc_error_handler(handle, user_cmd);
			TRACE_I(TAG, "0x%x, unknown cmd%d!\r\n", handle, user_cmd);
			break;
	}
}

static void saradc_svr_connect_handler(u32 handle, u8 connect_id)
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
		bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_SARADC,0x0,0x0);
		saradc_cmd_handler(handle, connect_id);
		bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_SARADC,0x1,0x0);
	}
	else  /* any other commands. */
	{
		TRACE_I(TAG, "cmd=%d, 0x%x, %x-%x.\r\n", cmd_id, handle, src, dst);
	}
}

static void saradc_ipc_disconnect_handler(u32 handle)
{
	TRACE_I(TAG, "0x%x disconnected\r\n", handle);
}

static u32 saradc_svr_rx_callback(u32 handle, u32 connect_id)
{
	u32  connect_flag;

	if(connect_id >= SARADC_SVR_CONNECT_MAX)
		return 0;

	connect_flag = 0x01 << connect_id;

	rtos_set_event_ex(&saradc_svr_event, connect_flag);

	return 0;
}

static void saradc_svr_task(void *param)
{
	u32 event;
	u32  handle;
	u32  connect_handle;

	if(rtos_init_event_ex(&saradc_svr_event) != BK_OK)
	{
		rtos_delete_thread(NULL);
		return;
	}

	handle = mb_ipc_socket(IPC_GET_ID_PORT(SARADC_SERVER), saradc_svr_rx_callback);

	if(handle == 0)
	{
		TRACE_I("ipc_svr", "create_socket failed.\r\n");

		rtos_deinit_event_ex(&saradc_svr_event);
		rtos_delete_thread(NULL);

		return ;
	}

	s_saradc_svr_init = 2;

	while(1)
	{
		event = rtos_wait_event_ex(&saradc_svr_event, SARADC_SVR_EVENTS, 1, BEKEN_WAIT_FOREVER);

		if(event == 0)  // timeout.
		{
			continue;
		}

		if(event & SARADC_SVR_QUIT_EVENT)
		{
			TRACE_I(TAG, "saradc server task quit\r\n");
			break;
		}

		// Handle connection events
		for(int i = 0; i < SARADC_SVR_CONNECT_MAX; i++)
		{
			if(event & (0x01 << i))
			{
				connect_handle = mb_ipc_server_get_connect_handle(handle, i);
				saradc_svr_connect_handler(connect_handle, i);
			}
		}
	}
	mb_ipc_server_close(handle, SARADC_SVR_WAIT_TIME);
	rtos_deinit_event_ex(&saradc_svr_event);
	s_saradc_svr_init = 0;
	rtos_delete_thread(NULL);
}

bk_err_t bk_saradc_server_init(void)
{
	bk_err_t ret = BK_OK;

	if(s_saradc_svr_init)
	{
		return BK_OK;
	}

	s_saradc_svr_init = 1;

	#if IPC_GET_ID_CPU(SARADC_SERVER) != 0   // != CPU0.
	#error server cpu configuration error!
	#endif

	// Create server task
	beken_thread_t saradc_svr_thread = NULL;
	ret = rtos_create_thread(&saradc_svr_thread,
							 SARADC_SVR_PRIORITY,
							 "saradc_svr",
							 saradc_svr_task,
							 SARADC_SVR_STACK_SIZE,
							 NULL);

	if(ret != BK_OK)
	{
		TRACE_I(TAG, "saradc server task create failed\r\n");
		rtos_delete_thread(NULL);
		return ret;
	}

	TRACE_I(TAG, "saradc server init ok\r\n");

	return BK_OK;
}

bk_err_t bk_saradc_server_deinit(void)
{
	if(s_saradc_svr_init != 2)
	{
		return BK_OK;
	}
    s_saradc_svr_init = 1;

	rtos_set_event_ex(&saradc_svr_event, SARADC_SVR_QUIT_EVENT);

	TRACE_I(TAG, "saradc server deinit ok\r\n");

	return BK_OK;
}

// eof

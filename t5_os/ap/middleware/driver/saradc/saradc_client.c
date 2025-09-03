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
#include <driver/adc.h>
#include "saradc_ipc.h"
#include <driver/mb_ipc.h>
#include <driver/mb_ipc_port_cfg.h>

#define TAG		"saradc_c"

#define LOCAL_TRACE    (1)

#define SARADC_OPERATE_TIMEOUT         600
#define ADC_SAMPLE_CNT_DEFAULT         32

static bool s_saradc_client_init = false;
static uint32_t saradc_socket_handle = 0;
static beken_mutex_t saradc_mutex = NULL;

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

bk_err_t bk_saradc_driver_init(void)
{
	if (s_saradc_client_init)
	{
		return BK_OK;
	}

	extern bk_err_t mb_saradc_ipc_init(void);

	static u8 saradc_mb_init = 0;

	if(saradc_mb_init == 0)
	{
		bk_err_t ret_code = mb_saradc_ipc_init();

		if(ret_code != BK_OK)
			return ret_code;

		saradc_mb_init = 1;
	}

	if(!rtos_is_scheduler_started())
		return BK_FAIL; // delay initialization.

	int ret = rtos_init_mutex(&saradc_mutex);

	if(kNoErr != ret)
	{
		return BK_FAIL;
	}

	saradc_socket_handle = mb_ipc_socket(IPC_GET_ID_PORT(SARADC_CLIENT), NULL);
	if(saradc_socket_handle == 0)
	{
		BK_LOGI(TAG, "saradc_client create socket failed\r\n");
		goto init_fail_exit;
	}

	ret = mb_ipc_connect(saradc_socket_handle, IPC_GET_ID_CPU(SARADC_SERVER), IPC_GET_ID_PORT(SARADC_SERVER), 500);

	if(ret != 0)
	{
		BK_LOGI(TAG, "saradc_client connect failed %d\r\n", ret);
		goto init_fail_exit;
	}

	s_saradc_client_init = true;

	return BK_OK;

init_fail_exit:

	if(saradc_socket_handle != 0)
	{
		mb_ipc_close(saradc_socket_handle, SARADC_OPERATE_TIMEOUT);
		saradc_socket_handle = 0;
	}

	rtos_deinit_mutex(&saradc_mutex);
	saradc_mutex = NULL;

	return BK_FAIL;
}

bk_err_t bk_saradc_driver_deinit(void)
{
	if(!s_saradc_client_init)
		return BK_OK;

	if(saradc_socket_handle != 0)
	{
		mb_ipc_close(saradc_socket_handle, SARADC_OPERATE_TIMEOUT);
		saradc_socket_handle = 0;
	}

	rtos_deinit_mutex(&saradc_mutex);
	saradc_mutex = NULL;

	s_saradc_client_init = false;

	return BK_OK;
}

bk_err_t bk_adc_acquire(void)
{
	int ret_val = BK_FAIL;
	int line_num;

	if(bk_saradc_driver_init() != BK_OK)
		return BK_FAIL;

	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	rtos_lock_mutex(&saradc_mutex);

	int ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_ACQUIRE,
		(u8 *)&cmd_buff, sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto acquire_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto acquire_exit;
	}

	if(user_cmd != SARADC_CMD_ACQUIRE)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto acquire_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto acquire_exit;
	}

	ret_val = BK_OK;

acquire_exit:

	rtos_unlock_mutex(&saradc_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_adc_init(adc_chan_t adc_chan)
{
	int ret_val = BK_FAIL;
	int line_num;

	if(bk_saradc_driver_init() != BK_OK)
		return BK_FAIL;

	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));
	cmd_buff.config.chan = adc_chan;

	rtos_lock_mutex(&saradc_mutex);

	int ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_INIT,
		(u8 *)&cmd_buff, sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto init_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto init_exit;
	}

	if(user_cmd != SARADC_CMD_INIT)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto init_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto init_exit;
	}

	ret_val = BK_OK;

init_exit:

	rtos_unlock_mutex(&saradc_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_adc_enable_bypass_clalibration(void)
{
	int ret_val = BK_FAIL;
	int line_num;

	if(bk_saradc_driver_init() != BK_OK)
		return BK_FAIL;

	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	rtos_lock_mutex(&saradc_mutex);

	int ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_ENABLE_BYPASS_CALIBRATION,
		(u8 *)&cmd_buff, sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto bypass_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto bypass_exit;
	}

	if(user_cmd != SARADC_CMD_ENABLE_BYPASS_CALIBRATION)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto bypass_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto bypass_exit;
	}

	ret_val = BK_OK;

bypass_exit:

	rtos_unlock_mutex(&saradc_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_adc_start(void)
{
	int ret_val = BK_FAIL;
	int line_num;

	if(bk_saradc_driver_init() != BK_OK)
		return BK_FAIL;

	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	rtos_lock_mutex(&saradc_mutex);

	int ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_START,
		(u8 *)&cmd_buff, sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto start_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto start_exit;
	}

	if(user_cmd != SARADC_CMD_START)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto start_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto start_exit;
	}

	ret_val = BK_OK;

start_exit:

	rtos_unlock_mutex(&saradc_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

static bk_err_t saradc_read_raw_internal(uint16_t* buf, uint32_t size, uint32_t timeout)
{
	int ret_val = BK_FAIL;
	int line_num;

	if(size > 0xFFFF)
		return BK_FAIL;

	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));
	cmd_buff.size = size;
	cmd_buff.timeout = timeout;

	rtos_lock_mutex(&saradc_mutex);

	int ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_READ_RAW,
			(u8 *)&cmd_buff, sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto read_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
			sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto read_exit;
	}

	if(user_cmd != SARADC_CMD_READ_RAW)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto read_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto read_exit;
	}

	if(cmd_buff.size != size)
	{
		line_num = __LINE__;
		ret = cmd_buff.size;
		goto read_exit;
	}

	memcpy(buf, cmd_buff.buff, size * sizeof(uint16_t));

	u32 crc = calc_crc32(0, (const u8 *)buf, size * sizeof(uint16_t));

	if(cmd_buff.crc != crc)
	{
		line_num = __LINE__;
		ret = crc;
		goto read_exit;
	}

	ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_READ_RAW_DONE,
			(u8 *)&cmd_buff, sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto read_exit;
	}

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
			sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

#if LOCAL_TRACE
	if(ret != sizeof(cmd_buff))
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, __LINE__, ret);
#endif

	ret_val = BK_OK;

read_exit:

	rtos_unlock_mutex(&saradc_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_adc_read_raw(uint16_t* buf, uint32_t size, uint32_t timeout)
{
	int ret_val = BK_OK;
	u32 rd_len = 0;
	u32 max_size = ADC_SAMPLE_CNT_DEFAULT;

	if(bk_saradc_driver_init() != BK_OK)
		return BK_FAIL;

	while(size > max_size)
	{
		ret_val = saradc_read_raw_internal(buf + rd_len, max_size, timeout);

		if(ret_val != BK_OK)
			return ret_val;

		rd_len += max_size;
		size -= max_size;
	}

	ret_val = saradc_read_raw_internal(buf + rd_len, size, timeout);

	return ret_val;
}

bk_err_t bk_adc_stop(void)
{
	int ret_val = BK_FAIL;
	int line_num;

	if(bk_saradc_driver_init() != BK_OK)
		return BK_FAIL;

	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(saradc_cmd_t));

	rtos_lock_mutex(&saradc_mutex);

	int ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_STOP,
		(u8 *)&cmd_buff, sizeof(saradc_cmd_t), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto stop_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto stop_exit;
	}

	if(user_cmd != SARADC_CMD_STOP)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto stop_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto stop_exit;
	}

	ret_val = BK_OK;

stop_exit:

	rtos_unlock_mutex(&saradc_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_adc_deinit(adc_chan_t chan)
{
	int ret_val = BK_FAIL;
	int line_num;

	if(bk_saradc_driver_init() != BK_OK)
		return BK_FAIL;

	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));
	cmd_buff.config.chan = chan;

	rtos_lock_mutex(&saradc_mutex);

	int ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_DEINIT,
		(u8 *)&cmd_buff, sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto deinit_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto deinit_exit;
	}

	if(user_cmd != SARADC_CMD_DEINIT)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto deinit_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto deinit_exit;
	}

	ret_val = BK_OK;

deinit_exit:

	rtos_unlock_mutex(&saradc_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_adc_release(void)
{
	int ret_val = BK_FAIL;
	int line_num;

	if(bk_saradc_driver_init() != BK_OK)
		return BK_FAIL;

	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	rtos_lock_mutex(&saradc_mutex);

	int ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_RELEASE,
		(u8 *)&cmd_buff, sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto release_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto release_exit;
	}

	if(user_cmd != SARADC_CMD_RELEASE)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto release_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto release_exit;
	}

	ret_val = BK_OK;

release_exit:

	rtos_unlock_mutex(&saradc_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_adc_set_config(adc_config_t *config)
{
	int ret_val = BK_FAIL;
	int line_num;

	if(bk_saradc_driver_init() != BK_OK)
		return BK_FAIL;

	if(config == NULL)
		return BK_ERR_NULL_PARAM;

	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));
	memcpy(&cmd_buff.config, config, sizeof(adc_config_t));

	rtos_lock_mutex(&saradc_mutex);

	int ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_SET_CONFIG,
		(u8 *)&cmd_buff, sizeof(saradc_cmd_t), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto set_config_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(saradc_cmd_t));

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(saradc_cmd_t), SARADC_OPERATE_TIMEOUT);

	if(ret != sizeof(saradc_cmd_t))
	{
		line_num = __LINE__;
		goto set_config_exit;
	}

	if(user_cmd != SARADC_CMD_SET_CONFIG)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto set_config_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto set_config_exit;
	}

	ret_val = BK_OK;

set_config_exit:

	rtos_unlock_mutex(&saradc_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_adc_en(void)
{
	int ret_val = BK_FAIL;
	int line_num;

	if(bk_saradc_driver_init() != BK_OK)
		return BK_FAIL;

	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	rtos_lock_mutex(&saradc_mutex);

	int ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_EN,
		(u8 *)&cmd_buff, sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto en_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto en_exit;
	}

	if(user_cmd != SARADC_CMD_EN)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto en_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto en_exit;
	}

	ret_val = BK_OK;

en_exit:

	rtos_unlock_mutex(&saradc_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_adc_read(uint16_t* data, uint32_t timeout)
{
	int ret_val = BK_FAIL;
	int line_num;

	if(bk_saradc_driver_init() != BK_OK)
		return BK_FAIL;

	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));
	cmd_buff.timeout = timeout;

	rtos_lock_mutex(&saradc_mutex);

	int ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_READ,
		(u8 *)&cmd_buff, sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto read_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto read_exit;
	}

	if(user_cmd != SARADC_CMD_READ)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto read_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto read_exit;
	}

	*data = cmd_buff.buff[0];

	u32  crc = calc_crc32(0, (u8 *)&cmd_buff.buff[0], sizeof(uint16_t));

	if(cmd_buff.crc != crc)
	{
		line_num = __LINE__;
		ret = crc;

		goto read_exit;
	}

	ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_READ_DONE,
			(u8 *)&cmd_buff, sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto read_exit;
	}

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
			sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);  // it is just a handshake. every send cmd to server must have a recv.

#if LOCAL_TRACE
	if(ret != sizeof(cmd_buff))
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, __LINE__, ret);
#endif
	ret_val = BK_OK;

read_exit:

	rtos_unlock_mutex(&saradc_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_adc_single_read(uint16_t* data)
{
	int ret_val = BK_FAIL;
	int line_num;

	if(bk_saradc_driver_init() != BK_OK)
		return BK_FAIL;

	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	rtos_lock_mutex(&saradc_mutex);

	int ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_SINGLE_READ,
		(u8 *)&cmd_buff, sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto single_read_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto single_read_exit;
	}

	if(user_cmd != SARADC_CMD_SINGLE_READ)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto single_read_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto single_read_exit;
	}

	*data = cmd_buff.buff[0];
	ret_val = BK_OK;

single_read_exit:

	rtos_unlock_mutex(&saradc_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_adc_set_channel(adc_chan_t adc_chan)
{
	int ret_val = BK_FAIL;
	int line_num;

	if(bk_saradc_driver_init() != BK_OK)
		return BK_FAIL;

	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));
	cmd_buff.config.chan = adc_chan;

	rtos_lock_mutex(&saradc_mutex);

	int ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_SET_CHANNEL,
		(u8 *)&cmd_buff, sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto set_channel_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto set_channel_exit;
	}

	if(user_cmd != SARADC_CMD_SET_CHANNEL)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto set_channel_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto set_channel_exit;
	}

	ret_val = BK_OK;

set_channel_exit:

	rtos_unlock_mutex(&saradc_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		// BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_adc_set_mode(adc_mode_t adc_mode)
{
	int ret_val = BK_FAIL;
	int line_num;

	if(bk_saradc_driver_init() != BK_OK)
		return BK_FAIL;

	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));
	cmd_buff.config.adc_mode = adc_mode;

	rtos_lock_mutex(&saradc_mutex);

	int ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_SET_MODE,
		(u8 *)&cmd_buff, sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto set_mode_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto set_mode_exit;
	}

	if(user_cmd != SARADC_CMD_SET_MODE)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto set_mode_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto set_mode_exit;
	}

	ret_val = BK_OK;

set_mode_exit:

	rtos_unlock_mutex(&saradc_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

adc_mode_t bk_adc_get_mode(void)
{
	int line_num = 0XFF;
	adc_mode_t ret_mode = ADC_SINGLE_STEP_MODE;

	if(bk_saradc_driver_init() != BK_OK)
		return ret_mode;

	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	rtos_lock_mutex(&saradc_mutex);

	int ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_GET_MODE,
		(u8 *)&cmd_buff, sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto get_mode_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto get_mode_exit;
	}

	if(user_cmd != SARADC_CMD_GET_MODE)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto get_mode_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto get_mode_exit;
	}

	ret_mode = cmd_buff.config.adc_mode;

get_mode_exit:

	rtos_unlock_mutex(&saradc_mutex);

#if LOCAL_TRACE
	if(ret != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_mode;
}

bk_err_t bk_adc_set_clk(adc_src_clk_t src_clk, uint32_t adc_clk)
{
	int ret_val = BK_FAIL;
	int line_num;

	if(bk_saradc_driver_init() != BK_OK)
		return BK_FAIL;

	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));
	cmd_buff.config.src_clk = src_clk;
	cmd_buff.config.clk = adc_clk;

	rtos_lock_mutex(&saradc_mutex);

	int ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_SET_CLK,
		(u8 *)&cmd_buff, sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto set_clk_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto set_clk_exit;
	}

	if(user_cmd != SARADC_CMD_SET_CLK)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto set_clk_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto set_clk_exit;
	}

	ret_val = BK_OK;

set_clk_exit:

	rtos_unlock_mutex(&saradc_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_adc_set_sample_rate(uint32_t sample_rate)
{
	int ret_val = BK_FAIL;
	int line_num;

	if(bk_saradc_driver_init() != BK_OK)
		return BK_FAIL;

	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));
	cmd_buff.config.sample_rate = sample_rate;

	rtos_lock_mutex(&saradc_mutex);

	int ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_SET_SAMPLE_RATE,
		(u8 *)&cmd_buff, sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto set_sample_rate_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto set_sample_rate_exit;
	}

	if(user_cmd != SARADC_CMD_SET_SAMPLE_RATE)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto set_sample_rate_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto set_sample_rate_exit;
	}

	ret_val = BK_OK;

set_sample_rate_exit:

	rtos_unlock_mutex(&saradc_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_adc_set_filter(uint32_t adc_filter)
{
	int ret_val = BK_FAIL;
	int line_num;

	if(bk_saradc_driver_init() != BK_OK)
		return BK_FAIL;

	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));
	cmd_buff.config.adc_filter = adc_filter;

	rtos_lock_mutex(&saradc_mutex);

	int ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_SET_FILTER,
		(u8 *)&cmd_buff, sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto set_filter_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto set_filter_exit;
	}

	if(user_cmd != SARADC_CMD_SET_FILTER)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto set_filter_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto set_filter_exit;
	}

	ret_val = BK_OK;

set_filter_exit:

	rtos_unlock_mutex(&saradc_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_adc_set_steady_time(uint32_t steady_ctrl)
{
	int ret_val = BK_FAIL;
	int line_num;

	if(bk_saradc_driver_init() != BK_OK)
		return BK_FAIL;

	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));
	cmd_buff.config.steady_ctrl = steady_ctrl;

	rtos_lock_mutex(&saradc_mutex);

	int ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_SET_STEADY_TIME,
		(u8 *)&cmd_buff, sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto set_steady_time_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto set_steady_time_exit;
	}

	if(user_cmd != SARADC_CMD_SET_STEADY_TIME)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto set_steady_time_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto set_steady_time_exit;
	}

	ret_val = BK_OK;

set_steady_time_exit:

	rtos_unlock_mutex(&saradc_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_adc_set_sample_cnt(uint32_t sample_cnt)
{
	int ret_val = BK_FAIL;
	int line_num;

	if(bk_saradc_driver_init() != BK_OK)
		return BK_FAIL;

	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));
	cmd_buff.sample_cnt = ADC_SAMPLE_CNT_DEFAULT;

	rtos_lock_mutex(&saradc_mutex);

	int ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_SET_SAMPLE_CNT,
		(u8 *)&cmd_buff, sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto set_sample_cnt_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto set_sample_cnt_exit;
	}

	if(user_cmd != SARADC_CMD_SET_SAMPLE_CNT)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto set_sample_cnt_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto set_sample_cnt_exit;
	}

	ret_val = BK_OK;

set_sample_cnt_exit:

	rtos_unlock_mutex(&saradc_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_adc_set_saturate_mode(adc_saturate_mode_t saturate_mode)
{
	int ret_val = BK_FAIL;
	int line_num;

	if(bk_saradc_driver_init() != BK_OK)
		return BK_FAIL;

	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));
	cmd_buff.config.saturate_mode = saturate_mode;

	rtos_lock_mutex(&saradc_mutex);

	int ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_SET_SATURATE_MODE,
		(u8 *)&cmd_buff, sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto set_saturate_mode_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto set_saturate_mode_exit;
	}

	if(user_cmd != SARADC_CMD_SET_SATURATE_MODE)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto set_saturate_mode_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto set_saturate_mode_exit;
	}

	ret_val = BK_OK;

set_saturate_mode_exit:

	rtos_unlock_mutex(&saradc_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_adc_register_isr(adc_isr_t adc_isr, uint32_t param)
{
	int ret_val = BK_FAIL;
	int line_num;

	if(bk_saradc_driver_init() != BK_OK)
		return BK_FAIL;

	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));
	//cmd_buff.chan = adc_chan;
	cmd_buff.param = param;

	rtos_lock_mutex(&saradc_mutex);

	int ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_REGISTER_ISR,
		(u8 *)&cmd_buff, sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto register_isr_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(cmd_buff), SARADC_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto register_isr_exit;
	}

	if(user_cmd != SARADC_CMD_REGISTER_ISR)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto register_isr_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto register_isr_exit;
	}

	ret_val = BK_OK;

register_isr_exit:

	rtos_unlock_mutex(&saradc_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

UINT16 bk_adc_data_calculate(UINT16 adc_val, UINT8 adc_chan)
{
	int ret_val = BK_FAIL;
	int line_num = 0;
	UINT16 adc_calculate = 0;

	if(bk_saradc_driver_init() != BK_OK)
		return BK_FAIL;

	saradc_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));
	cmd_buff.adc_cali_data = adc_val;
	cmd_buff.config.chan = adc_chan;

	rtos_lock_mutex(&saradc_mutex);

	int ret = mb_ipc_send(saradc_socket_handle, SARADC_CMD_DATA_CALCULATE,
		(u8 *)&cmd_buff, sizeof(saradc_cmd_t), SARADC_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto adc_calculate_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(saradc_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(saradc_cmd_t), SARADC_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto adc_calculate_exit;
	}

	if(user_cmd != SARADC_CMD_DATA_CALCULATE)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto adc_calculate_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto adc_calculate_exit;
	}
	adc_calculate = cmd_buff.adc_cali_data;
	ret_val = BK_OK;

adc_calculate_exit:

	rtos_unlock_mutex(&saradc_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return adc_calculate;
}
// eof

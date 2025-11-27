// Copyright 2020-2025 Beken
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
#include "phy_ipc.h"
#include <driver/mb_ipc.h>
#include <driver/mb_ipc_port_cfg.h>

#define TAG                        "phy_client_c"

#define LOCAL_TRACE               (1)

#define PHY_OPERATE_TIMEOUT       600

static bool s_phy_client_init     = false;
static uint32_t phy_socket_handle = 0;
static beken_mutex_t phy_mutex    = NULL;

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

bk_err_t bk_phy_driver_init(void)
{
	if (s_phy_client_init)
	{
		return BK_OK;
	}

	extern bk_err_t mb_phy_ipc_init(void);
	static u8 phy_mb_init = 0;

	if(phy_mb_init == 0)
	{
		bk_err_t ret_code = mb_phy_ipc_init();

		if(ret_code != BK_OK)
			return ret_code;

		phy_mb_init = 1;
	}

	if(!rtos_is_scheduler_started())
		return BK_FAIL; // delay initialization.

	int ret = rtos_init_mutex(&phy_mutex);

	if(kNoErr != ret)
	{
		return BK_FAIL;
	}

	phy_socket_handle = mb_ipc_socket(IPC_GET_ID_PORT(PHY_CLIENT), NULL);
	if(phy_socket_handle == 0)
	{
		BK_LOGI(TAG, "phy_client create socket failed\r\n");
		goto init_fail_exit;
	}

	ret = mb_ipc_connect(phy_socket_handle, IPC_GET_ID_CPU(PHY_SERVER), IPC_GET_ID_PORT(PHY_SERVER), 500);
	if(ret != 0)
	{
		BK_LOGI(TAG, "phy_client connect failed %d\r\n", ret);
		goto init_fail_exit;
	}

	s_phy_client_init = true;

	return BK_OK;

init_fail_exit:

	if(phy_socket_handle != 0)
	{
		mb_ipc_close(phy_socket_handle, PHY_OPERATE_TIMEOUT);
		phy_socket_handle = 0;
	}

	rtos_deinit_mutex(&phy_mutex);
	phy_mutex = NULL;

	return BK_FAIL;
}

bk_err_t bk_phy_driver_deinit(void)
{
	if(!s_phy_client_init)
		return BK_OK;

	if(phy_socket_handle != 0)
	{
		mb_ipc_close(phy_socket_handle, PHY_OPERATE_TIMEOUT);
		phy_socket_handle = 0;
	}

	rtos_deinit_mutex(&phy_mutex);
	phy_mutex = NULL;

	s_phy_client_init = false;

	return BK_OK;
}

bk_err_t bk_phy_get_current_temperature(float *temperature)
{
	int ret_val = BK_FAIL;
	int line_num;

	if(bk_phy_driver_init() != BK_OK)
		return BK_FAIL;

	phy_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	rtos_lock_mutex(&phy_mutex);

	int ret = mb_ipc_send(phy_socket_handle, PHY_CMD_GET_TEMP,
		(u8 *)&cmd_buff, sizeof(phy_cmd_t), PHY_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto get_temp_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(phy_cmd_t));

	ret = mb_ipc_recv(phy_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(phy_cmd_t), PHY_OPERATE_TIMEOUT);

	if(ret != sizeof(phy_cmd_t))
	{
		line_num = __LINE__;
		goto get_temp_exit;
	}

	if(user_cmd != PHY_CMD_GET_TEMP)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto get_temp_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto get_temp_exit;
	}
    *temperature = cmd_buff.param;
	ret_val = BK_OK;

get_temp_exit:

	rtos_unlock_mutex(&phy_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_sensor_get_current_voltage(float *volt)
{
	int ret_val = BK_FAIL;
	int line_num;

	if(bk_phy_driver_init() != BK_OK)
		return BK_FAIL;

	phy_cmd_t cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	rtos_lock_mutex(&phy_mutex);

	int ret = mb_ipc_send(phy_socket_handle, PHY_CMD_GET_VOLT,
		(u8 *)&cmd_buff, sizeof(phy_cmd_t), PHY_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto get_temp_exit;
	}

	u8 user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(phy_cmd_t));

	ret = mb_ipc_recv(phy_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(phy_cmd_t), PHY_OPERATE_TIMEOUT);

	if(ret != sizeof(phy_cmd_t))
	{
		line_num = __LINE__;
		goto get_temp_exit;
	}

	if(user_cmd != PHY_CMD_GET_VOLT)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto get_temp_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;
		goto get_temp_exit;
	}
    *volt = cmd_buff.param;
	ret_val = BK_OK;

get_temp_exit:

	rtos_unlock_mutex(&phy_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGI(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}
// eof
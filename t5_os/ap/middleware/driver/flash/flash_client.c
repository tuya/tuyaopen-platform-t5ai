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
#include <driver/flash.h>
#include <driver/flash_partition.h>
#include "flash_ipc.h"
#include <driver/mb_ipc.h>
#include <driver/mb_ipc_port_cfg.h>
// Modified by TUYA Start
#if CONFIG_FLASH_BYPASS_OTP_OPERATION
#include "flash_bypass.h"
#endif
// Modified by TUYA End

#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif

#define TAG		"flash_c"

#define LOCAL_TRACE    (1)

#define FLASH_OPERATE_TIMEOUT         2000

static bool s_flash_client_init = false;

static uint32_t         flash_socket_handle = 0;
static beken_mutex_t    flash_mutex = NULL;

static const uint32_t crc32_table[] =
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t calc_crc32_xor(uint32_t crc, const void *buf, size_t size)
{
    const uint8_t *p;

    p = (const uint8_t *)buf;
    crc = crc ^ ~0U;

    while (size--) {
        crc = crc32_table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    }

    return crc ^ ~0U;
}

static uint32_t calc_crc32(uint32_t crc, const uint8_t *buf, int len)
{
	while(len--)
	{
		crc = (crc >> 8)^(crc32_table[(crc^*buf++)&0xff]);
	}

	return crc;
}

// #define FLASH_SVR_TEST
#ifdef FLASH_SVR_TEST
#include "flash_svr_test.c"
#endif

bk_err_t bk_flash_driver_init(void)
{
	if (s_flash_client_init)
	{
		return BK_OK;
	}

	extern bk_err_t    mb_flash_ipc_init(void);

	static u8   flash_mb_init = 0;

	if(flash_mb_init == 0)
	{
		bk_err_t ret_code = mb_flash_ipc_init();

		if(ret_code != BK_OK)
			return ret_code;

		flash_mb_init = 1;
	}

	if(!rtos_is_scheduler_started())
		return BK_FAIL; // delay initialization.

	int ret = rtos_init_mutex(&flash_mutex);

	if(kNoErr != ret)
	{
		return BK_FAIL;
	}

	flash_socket_handle = mb_ipc_socket(IPC_GET_ID_PORT(FLASH_CLIENT), NULL);
	if(flash_socket_handle == 0)
	{
		#if LOCAL_TRACE
		BK_LOGE(TAG, "flash-drv-client create socket failed\r\n");
		#endif

		goto init_fail_exit;
	}

	ret = mb_ipc_connect(flash_socket_handle, IPC_GET_ID_CPU(FLASH_SERVER), IPC_GET_ID_PORT(FLASH_SERVER), 500);

	if(ret != 0)
	{
		#if LOCAL_TRACE
		BK_LOGE(TAG, "flash-drv-client connect failed %d\r\n", ret);
		#endif

		goto init_fail_exit;
	}

	s_flash_client_init = true;

#if CONFIG_FLASH_TEST
	extern int bk_flash_register_cli_test_feature(void);
	//bk_flash_register_cli_test_feature();
#endif

	return BK_OK;

init_fail_exit:

	if(flash_socket_handle != 0)
	{
		mb_ipc_close(flash_socket_handle, FLASH_OPERATE_TIMEOUT);
		flash_socket_handle = 0;
	}

	rtos_deinit_mutex(&flash_mutex);
	flash_mutex = NULL;

	return BK_FAIL;
}

bk_err_t bk_flash_driver_deinit(void)
{
	if(!s_flash_client_init)
		return BK_OK;

	if(flash_socket_handle != 0)
	{
		mb_ipc_close(flash_socket_handle, FLASH_OPERATE_TIMEOUT);
		flash_socket_handle = 0;
	}

	rtos_deinit_mutex(&flash_mutex);
	flash_mutex = NULL;

	s_flash_client_init = false;

	return BK_OK;
}

bk_err_t bk_flash_erase_sector(uint32_t address)
{
	int  ret_val = BK_FAIL;
	int  line_num;

// Modified by TUYA Start
#if !LOCAL_TRACE
	(void)line_num;
#endif
// Modified by TUYA End

	if(bk_flash_driver_init() != BK_OK)
		return BK_FAIL;

	flash_cmd_t   cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	cmd_buff.addr = address;

	rtos_lock_mutex(&flash_mutex);

	mb_ipc_recv(flash_socket_handle, NULL, NULL, 0, 0);  // data_buff == NULL or buff_len == 0 just discard all data.

	int ret = mb_ipc_send(flash_socket_handle, FLASH_CMD_ERASE_SECTOR,
		(u8 *)&cmd_buff, sizeof(cmd_buff), FLASH_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto erase_sector_exit;
	}

	u8   user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(flash_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(cmd_buff), FLASH_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto erase_sector_exit;
	}

	if(user_cmd != FLASH_CMD_ERASE_SECTOR)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto erase_sector_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;

		goto erase_sector_exit;
	}

	ret_val = BK_OK;

erase_sector_exit:

	rtos_unlock_mutex(&flash_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGE(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_flash_erase_32k(uint32_t address)
{
	return BK_FAIL;
}

bk_err_t bk_flash_erase_block(uint32_t address)
{
	return BK_FAIL;
}

static bk_err_t flash_read_bytes(uint32_t address, uint8_t *user_buf, uint32_t size)
{
	int  ret_val = BK_FAIL;
	int  line_num;

// Modified by TUYA Start
#if !LOCAL_TRACE
	(void)line_num;
#endif
// Modified by TUYA End

	if(size > 0xFFFF)
		return BK_FAIL;

	flash_cmd_t   cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	cmd_buff.addr = address;
	cmd_buff.len  = size;

	rtos_lock_mutex(&flash_mutex);

	mb_ipc_recv(flash_socket_handle, NULL, NULL, 0, 0);  // data_buff == NULL or buff_len == 0 just discard all data.

	int ret = mb_ipc_send(flash_socket_handle, FLASH_CMD_READ,
			(u8 *)&cmd_buff, sizeof(cmd_buff), FLASH_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto read_exit;
	}

	u8   user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(flash_socket_handle, &user_cmd, (u8 *)&cmd_buff,
			sizeof(cmd_buff), FLASH_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto read_exit;
	}

	if(user_cmd != FLASH_CMD_READ)
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

	if(cmd_buff.len != size)
	{
		line_num = __LINE__;
		ret = cmd_buff.len;

		goto read_exit;
	}

	#if CONFIG_CACHE_ENABLE
	flush_dcache(cmd_buff.buff, size);
	#endif
	memcpy(user_buf, cmd_buff.buff, size);

	u32  crc = calc_crc32(0, (const u8 *)user_buf, size);

	if(cmd_buff.crc != crc)
	{
		line_num = __LINE__;
		ret = crc;

		goto read_exit;
	}

	ret = mb_ipc_send(flash_socket_handle, FLASH_CMD_READ_DONE,
			(u8 *)&cmd_buff, sizeof(cmd_buff), FLASH_OPERATE_TIMEOUT);

	if(ret != 0)
	{
		line_num = __LINE__;
		goto read_exit;
	}

	// u8   user_cmd = INVALID_USER_CMD_ID;

	// memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(flash_socket_handle, &user_cmd, (u8 *)&cmd_buff,
			sizeof(cmd_buff), FLASH_OPERATE_TIMEOUT);  // it is just a handshake. every send cmd to server must have a recv.

#if LOCAL_TRACE
	if(ret != sizeof(cmd_buff))
		BK_LOGE(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, __LINE__, ret);
#endif

	ret_val = BK_OK;

read_exit:

	rtos_unlock_mutex(&flash_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGE(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

static bk_err_t flash_read_bytes_retry(uint32_t address, uint8_t *user_buf, uint32_t size)
{
	int  try_cnt = 0;
	int  ret_val = BK_OK;

	while(1)
	{
		ret_val = flash_read_bytes(address, user_buf, size);

		if(ret_val == BK_OK)
			break;

		try_cnt++;
		if(try_cnt >= 2)
			return ret_val;
		else
		{
			rtos_delay_milliseconds(10);  // delay 10ms.
			continue;
		}
	}

	return ret_val;
}

bk_err_t bk_flash_read_bytes(uint32_t address, uint8_t *user_buf, uint32_t size)
{
	int  ret_val = BK_OK;
	u32  rd_len = 0;

	if(bk_flash_driver_init() != BK_OK)
		return BK_FAIL;

	while(size > FLASH_IPC_READ_SIZE)
	{
		ret_val = flash_read_bytes_retry(address + rd_len, user_buf + rd_len, FLASH_IPC_READ_SIZE);

		if(ret_val != BK_OK)
			return ret_val;

		rd_len += FLASH_IPC_READ_SIZE;
		size -= FLASH_IPC_READ_SIZE;
	}

	ret_val = flash_read_bytes_retry(address + rd_len, user_buf + rd_len, size);

	return ret_val;
}

static bk_err_t flash_write_bytes(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	int  ret_val = BK_FAIL;
	int  line_num;

// Modified by TUYA Start
#if !LOCAL_TRACE
	(void)line_num;
#endif
// Modified by TUYA End

	if(size > 0xFFFF)
		return BK_FAIL;

	flash_cmd_t   cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	cmd_buff.addr = address;
	cmd_buff.len  = size;
	cmd_buff.buff = (u8 *)user_buf;
	cmd_buff.crc  = calc_crc32(0, user_buf, size);

	rtos_lock_mutex(&flash_mutex);

	mb_ipc_recv(flash_socket_handle, NULL, NULL, 0, 0);  // data_buff == NULL or buff_len == 0 just discard all data.

	int ret = mb_ipc_send(flash_socket_handle, FLASH_CMD_WRITE,
		(u8 *)&cmd_buff, sizeof(cmd_buff), FLASH_OPERATE_TIMEOUT + size / 200);  // flash write speed: write done 200 bytes in 1ms.

	if(ret != 0)
	{
		line_num = __LINE__;
		goto write_exit;
	}

	u8   user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(flash_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(cmd_buff), FLASH_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto write_exit;
	}

	if(user_cmd != FLASH_CMD_WRITE)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto write_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;

		goto write_exit;
	}

	#if 0
	if(cmd_buff.len != size)
	{
		line_num = __LINE__;
		ret = cmd_buff.len;

		goto write_exit;
	}
	#endif

	ret_val = BK_OK;

write_exit:

	rtos_unlock_mutex(&flash_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGE(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

bk_err_t bk_flash_write_bytes(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	int  ret_val = BK_OK;
	u32  wr_len = 0;

	if(bk_flash_driver_init() != BK_OK)
		return BK_FAIL;

	while(size > FLASH_IPC_WRITE_SIZE)
	{
		ret_val = flash_write_bytes(address + wr_len, user_buf + wr_len, FLASH_IPC_WRITE_SIZE);

		if(ret_val != BK_OK)
			return ret_val;

		wr_len += FLASH_IPC_WRITE_SIZE;
		size -= FLASH_IPC_WRITE_SIZE;
	}

	ret_val = flash_write_bytes(address + wr_len, user_buf + wr_len, size);

	return ret_val;
}

uint32_t bk_flash_get_id(void)
{
	return 0;
}

flash_line_mode_t bk_flash_get_line_mode(void)
{
	return FLASH_LINE_MODE_TWO;
}

flash_protect_type_t bk_flash_get_protect_type(void)
{
	return FLASH_PROTECT_ALL;
}

bk_err_t bk_flash_set_protect_type(flash_protect_type_t type)
{
	return BK_OK;
}

bool bk_flash_is_driver_inited()
{
	return s_flash_client_init;
}

#include "flash_driver.h"

bk_err_t bk_flash_erase_fast(uint32_t erase_off, uint32_t len)
{
	int  ret_val = BK_FAIL;
	int  line_num;

// Modified by TUYA Start
#if !LOCAL_TRACE
	(void)line_num;
#endif
// Modified by TUYA End

	if(bk_flash_driver_init() != BK_OK)
		return BK_FAIL;

	flash_cmd_t   cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	cmd_buff.addr = erase_off;
	cmd_buff.len  = len;

	rtos_lock_mutex(&flash_mutex);

	mb_ipc_recv(flash_socket_handle, NULL, NULL, 0, 0);  // data_buff == NULL or buff_len == 0 just discard all data.

	int ret = mb_ipc_send(flash_socket_handle, FLASH_CMD_FAST_ERASE,
		(u8 *)&cmd_buff, sizeof(cmd_buff), FLASH_OPERATE_TIMEOUT * (1 + len / FLASH_SECTOR_SIZE));

	if(ret != 0)
	{
		line_num = __LINE__;
		goto erase_fast_exit;
	}

	u8   user_cmd = INVALID_USER_CMD_ID;

	memset(&cmd_buff, 0, sizeof(cmd_buff));

	ret = mb_ipc_recv(flash_socket_handle, &user_cmd, (u8 *)&cmd_buff,
		sizeof(cmd_buff), FLASH_OPERATE_TIMEOUT);

	if(ret != sizeof(cmd_buff))
	{
		line_num = __LINE__;
		goto erase_fast_exit;
	}

	if(user_cmd != FLASH_CMD_FAST_ERASE)
	{
		line_num = __LINE__;
		ret = user_cmd;
		goto erase_fast_exit;
	}

	if(cmd_buff.ret_status != BK_OK)
	{
		line_num = __LINE__;
		ret = cmd_buff.ret_status;

		goto erase_fast_exit;
	}

	ret_val = BK_OK;

erase_fast_exit:

	rtos_unlock_mutex(&flash_mutex);

#if LOCAL_TRACE
	if(ret_val != BK_OK)
		BK_LOGE(TAG, "%s @%d, data=%d.\r\n", __FUNCTION__, line_num, ret);
#endif

	return ret_val;
}

// Modified by TUYA Start
#if CONFIG_FLASH_BYPASS_OTP_OPERATION && (CONFIG_CPU_CNT > 1)

#ifndef FLASH_BYPASS_OTP_IPC_RETRY_MAX
#define FLASH_BYPASS_OTP_IPC_RETRY_MAX 8
#endif

bk_err_t flash_bypass_otp_operation(flash_bypass_otp_cmd_t cmd, flash_bypass_otp_ctrl_t *param)
{
	if(bk_flash_driver_init() != BK_OK)
		return BK_FAIL;

	for (uint8_t attempt = 0; attempt < FLASH_BYPASS_OTP_IPC_RETRY_MAX; attempt++)
	{
		int  ret_val = BK_FAIL;
		int  line_num = 0;
#if !LOCAL_TRACE
		(void)line_num;
#endif
		int  ret = 0;

		flash_bypass_otp_ipc_cmd_t ipc_cmd;
		uint8_t *data_buff = NULL;
		uint32_t data_len = 0;

		memset(&ipc_cmd, 0, sizeof(ipc_cmd));

		ipc_cmd.cmd = (uint8_t)cmd;
		ipc_cmd.otp_idx = param->otp_idx;
		ipc_cmd.addr_offset = param->addr_offset;
		ipc_cmd.read_len = param->read_len;
		ipc_cmd.write_len = param->write_len;

		if(cmd == FLASH_BYPASS_OTP_WRITE && param->write_buf != NULL && param->write_len > 0)
		{
			data_len = param->write_len;
			data_buff = (uint8_t *)os_malloc(data_len);
			if(data_buff == NULL)
			{
				return BK_FAIL;
			}
			memcpy(data_buff, param->write_buf, data_len);
		}

		rtos_lock_mutex(&flash_mutex);

		mb_ipc_recv(flash_socket_handle, NULL, NULL, 0, 0);  // discard all data

		if(cmd == FLASH_BYPASS_OTP_WRITE && data_buff != NULL && data_len > 0)
		{
			uint32_t total_len = sizeof(ipc_cmd) + data_len;
			uint8_t *combined_buff = (uint8_t *)os_malloc(total_len);
			if(combined_buff == NULL)
			{
				line_num = __LINE__;
				ret = BK_FAIL;
				goto bypass_otp_exit;
			}

			memcpy(combined_buff, &ipc_cmd, sizeof(ipc_cmd));
			memcpy(combined_buff + sizeof(ipc_cmd), data_buff, data_len);

			ret = mb_ipc_send(flash_socket_handle, FLASH_CMD_BYPASS_OTP_OPERATION,
				combined_buff, total_len, FLASH_OPERATE_TIMEOUT);

			os_free(combined_buff);

			if(ret != 0)
			{
				line_num = __LINE__;
				goto bypass_otp_exit;
			}
		}
		else
		{
			ret = mb_ipc_send(flash_socket_handle, FLASH_CMD_BYPASS_OTP_OPERATION,
				(u8 *)&ipc_cmd, sizeof(ipc_cmd), FLASH_OPERATE_TIMEOUT);

			if(ret != 0)
			{
				line_num = __LINE__;
				goto bypass_otp_exit;
			}
		}

		u8 user_cmd = INVALID_USER_CMD_ID;
		memset(&ipc_cmd, 0, sizeof(ipc_cmd));

		ret = mb_ipc_recv(flash_socket_handle, &user_cmd, (u8 *)&ipc_cmd,
			sizeof(ipc_cmd), FLASH_OPERATE_TIMEOUT);

		if(ret != sizeof(ipc_cmd))
		{
			line_num = __LINE__;
			goto bypass_otp_exit;
		}

		if(user_cmd != FLASH_CMD_BYPASS_OTP_OPERATION)
		{
			line_num = __LINE__;
			ret = user_cmd;
			goto bypass_otp_exit;
		}

		if(ipc_cmd.ret_status != BK_OK)
		{
			line_num = __LINE__;
			ret = ipc_cmd.ret_status;
			goto bypass_otp_exit;
		}

		if(cmd == FLASH_BYPASS_OTP_READ && param->read_buf != NULL && param->read_len > 0)
		{
			uint32_t read_len = param->read_len;
			uint8_t *read_buff = (uint8_t *)os_malloc(read_len);
			if(read_buff == NULL)
			{
				line_num = __LINE__;
				ret = BK_FAIL;
				goto bypass_otp_exit;
			}

			u8 data_cmd = INVALID_USER_CMD_ID;
			int data_recv_len = mb_ipc_recv(flash_socket_handle, &data_cmd, read_buff, read_len, FLASH_OPERATE_TIMEOUT);

			if(data_recv_len != read_len || data_cmd != FLASH_CMD_BYPASS_OTP_OPERATION)
			{
				line_num = __LINE__;
				ret = BK_FAIL;
				os_free(read_buff);
				goto bypass_otp_exit;
			}

			memcpy(param->read_buf, read_buff, read_len);
			os_free(read_buff);

			memset(&ipc_cmd, 0, sizeof(ipc_cmd));
			ipc_cmd.ret_status = BK_OK;
			ret = mb_ipc_send(flash_socket_handle, FLASH_CMD_BYPASS_OTP_OPERATION,
				(u8 *)&ipc_cmd, sizeof(ipc_cmd), FLASH_OPERATE_TIMEOUT);
			if(ret != 0)
			{
				line_num = __LINE__;
				goto bypass_otp_exit;
			}

			memset(&ipc_cmd, 0, sizeof(ipc_cmd));
			u8 final_cmd = INVALID_USER_CMD_ID;
			ret = mb_ipc_recv(flash_socket_handle, &final_cmd, (u8 *)&ipc_cmd,
				sizeof(ipc_cmd), FLASH_OPERATE_TIMEOUT);
			if(ret != sizeof(ipc_cmd) || final_cmd != FLASH_CMD_BYPASS_OTP_OPERATION)
			{
				line_num = __LINE__;
				#if LOCAL_TRACE
				BK_LOGE(TAG, "%s @%d, recv final ack failed, final_cmd=%d, ret=%d\r\n", __FUNCTION__, line_num, final_cmd, ret);
				#endif
			}
		}

		ret_val = BK_OK;

bypass_otp_exit:
		if(data_buff != NULL)
		{
			os_free(data_buff);
		}

		rtos_unlock_mutex(&flash_mutex);

		if(ret_val == BK_OK)
		{
#if LOCAL_TRACE
			if(attempt != 0)
				BK_LOGW(TAG, "%s retry success after %d attempt(s).\r\n", __FUNCTION__, attempt);
#endif
			return BK_OK;
		}

#if LOCAL_TRACE
		BK_LOGE(TAG, "%s retry=%d @%d, data=%d.\r\n", __FUNCTION__, attempt, line_num, ret);
#endif

		if(attempt + 1 < FLASH_BYPASS_OTP_IPC_RETRY_MAX)
			rtos_delay_milliseconds(5);
	}

	return BK_FAIL;
}
#endif
// Modified by TUYA End


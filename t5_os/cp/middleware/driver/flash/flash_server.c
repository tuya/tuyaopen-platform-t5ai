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
#include <driver/flash.h>
#include <driver/flash_partition.h>
#include <os/os.h>
#include "flash_ipc.h"
#include <driver/mb_ipc.h>
#include <driver/mb_ipc_port_cfg.h>
#include <os/rtos_ext.h>
#include "modules/pm.h"
#if CONFIG_FLASH_BYPASS_OTP_OPERATION
#include "flash_bypass.h"
#endif

#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif

#define TAG		"flash_s"

// #define DYNAMIC_FLASH_BUFFER

#define LOCAL_TRACE_E     (1)
#define LOCAL_TRACE_I     (0)

#define TRACE_E(...)        do { if(LOCAL_TRACE_E) BK_LOGE(__VA_ARGS__); } while(0)
#define TRACE_I(...)        do { if(LOCAL_TRACE_I) BK_LOGD(__VA_ARGS__); } while(0)

#define FLASH_SVR_PRIORITY               BEKEN_DEFAULT_WORKER_PRIORITY
#define FLASH_SVR_STACK_SIZE             1536

#define FLASH_SVR_CONNECT_MAX            2
/* connection ID bits, 8 bits for max 8 connections (0x01 ~ 0xFF). */
#define FLASH_SVR_CONNECT_EVENTS         ((0x01 << (FLASH_SVR_CONNECT_MAX)) - 1)   //  0x03
#define FLASH_SVR_QUIT_EVENT             (0x100)

#define FLASH_SVR_EVENTS         (FLASH_SVR_CONNECT_EVENTS | FLASH_SVR_QUIT_EVENT)

#define FLASH_SVR_WAIT_TIME      50
// #define FLASH_SVR_WAIT_TIME      200

static u8 s_flash_svr_init = 0;

static rtos_event_ext_t  flash_svr_event;

#ifndef  DYNAMIC_FLASH_BUFFER
static u8        flash_buff[MAX(FLASH_IPC_READ_SIZE, FLASH_IPC_WRITE_SIZE)];
#if CONFIG_FLASH_BYPASS_OTP_OPERATION
static u8        flash_bypass_buff[FLASH_IPC_BYPASS_OTP_SIZE];
#endif
#endif

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

/*
extern uint32_t calc_crc32_xor(uint32_t crc, const void *buf, size_t size);
extern uint32_t calc_crc32(uint32_t crc, const uint8_t *buf, int len); */

static void flash_error_handler(u32 handle, u8 user_cmd)
{
	flash_cmd_t   cmd_buff;

	memset(&cmd_buff, 0, sizeof(cmd_buff));
	cmd_buff.ret_status = BK_FAIL;
	mb_ipc_send(handle, user_cmd, (u8 *)&cmd_buff, sizeof(cmd_buff), FLASH_SVR_WAIT_TIME);
}

static void flash_erase_handler(u32 handle, flash_cmd_t *cmd_buff)
{
	cmd_buff->ret_status = bk_flash_erase_sector(cmd_buff->addr);

	int ret_val = mb_ipc_send(handle, FLASH_CMD_ERASE_SECTOR, (u8 *)cmd_buff, sizeof(flash_cmd_t), FLASH_SVR_WAIT_TIME);

	if(ret_val != 0)
		TRACE_E(TAG, "0x%x, erase-0x%x : %d, %d.\r\n", handle, cmd_buff->addr, cmd_buff->ret_status, ret_val);

	(void)ret_val;
}

static void flash_fast_erase_handler(u32 handle, flash_cmd_t *cmd_buff)
{
	cmd_buff->ret_status = bk_flash_erase_fast(cmd_buff->addr,cmd_buff->len);

	int ret_val = mb_ipc_send(handle, FLASH_CMD_FAST_ERASE, (u8 *)cmd_buff, sizeof(flash_cmd_t), FLASH_SVR_WAIT_TIME);

	if(ret_val != 0)
		TRACE_E(TAG, "0x%x, erase-0x%x : %d, %d.\r\n", handle, cmd_buff->addr, cmd_buff->ret_status, ret_val);

	(void)ret_val;
}

static void flash_read_handler(u32 handle, flash_cmd_t *cmd_buff, u8 connect_id)
{
	int      read_status = BK_OK;

	#ifdef  DYNAMIC_FLASH_BUFFER
	u8     * read_buff = os_malloc(cmd_buff->len);

	if(read_buff == NULL)
	{
		flash_error_handler(handle, FLASH_CMD_READ);

		TRACE_E(TAG, "%s @%d, 0x%x memory overrun!\r\n", __FUNCTION__, __LINE__, handle);

		return;
	}
	#else
	u8     * read_buff = flash_buff;

	if(cmd_buff->len > sizeof(flash_buff))
	{
		flash_error_handler(handle, FLASH_CMD_READ);

		TRACE_E(TAG, "%s @%d, 0x%x buffer overflow!\r\n", __FUNCTION__, __LINE__, handle);

		return;
	}
	#endif

	read_status = bk_flash_read_bytes(cmd_buff->addr, read_buff, cmd_buff->len);

	cmd_buff->ret_status = read_status;
	cmd_buff->buff = read_buff;
	cmd_buff->crc = calc_crc32(0, read_buff, cmd_buff->len);

	int     ret_val;

	ret_val = mb_ipc_send(handle, FLASH_CMD_READ, (u8 *)cmd_buff, sizeof(flash_cmd_t), FLASH_SVR_WAIT_TIME);

	if(read_status != BK_OK)
	{
		#ifdef  DYNAMIC_FLASH_BUFFER
		os_free(read_buff);
		#endif
		TRACE_E(TAG, "%s @%d, 0x%x read failed!\r\n", __FUNCTION__, __LINE__, handle);
		return;
	}

	if(ret_val != 0)
	{
		(*read_buff) ^= 0x01;   // make crc not match
		#ifdef  DYNAMIC_FLASH_BUFFER
		os_free(read_buff);
		#endif
		TRACE_E(TAG, "%s @%d, 0x%x send failed, ret_val = %d!\r\n", __FUNCTION__, __LINE__, handle, ret_val);
		return;
	}

	u32 event = rtos_wait_event_ex(&flash_svr_event, (0x01 << connect_id), 1, FLASH_SVR_WAIT_TIME);

	if(event == 0)
	{
		(*read_buff) ^= 0x01;   // make crc not match
		#ifdef  DYNAMIC_FLASH_BUFFER
		os_free(read_buff);
		#endif
		TRACE_E(TAG, "%s @%d, 0x%x read done failed!\r\n", __FUNCTION__, __LINE__, handle);
		return;
	}

	memset(cmd_buff, 0, sizeof(flash_cmd_t));

	u8     user_cmd = INVALID_USER_CMD_ID;

	ret_val = mb_ipc_recv(handle, &user_cmd, (u8 *)cmd_buff,  sizeof(flash_cmd_t), FLASH_SVR_WAIT_TIME);

	if((ret_val != sizeof(flash_cmd_t)) || (user_cmd != FLASH_CMD_READ_DONE))
	{
		TRACE_E(TAG, "%s @%d, 0x%x recv cmd%d, %d, 0x%x!\r\n", __FUNCTION__, __LINE__, handle, user_cmd, ret_val, event);
	}
	else
	{
		memset(cmd_buff, 0, sizeof(flash_cmd_t));
		cmd_buff->ret_status = BK_OK;

		// it is just a handshake. every recv from client must have a send.
		mb_ipc_send(handle, FLASH_CMD_READ_DONE, (u8 *)cmd_buff, sizeof(flash_cmd_t), FLASH_SVR_WAIT_TIME);
	}

	(*read_buff) ^= 0x01;   // make crc not match

	#ifdef  DYNAMIC_FLASH_BUFFER
	os_free(read_buff);
	#endif

	TRACE_I(TAG, "0x%x read done %d.\r\n", handle, user_cmd);

	return;
}

static void flash_write_handler(u32 handle, flash_cmd_t *cmd_buff)
{
	u16     cpy_len, write_len = 0;
	int     write_status = BK_OK;
	int     line_num;
	#ifdef  DYNAMIC_FLASH_BUFFER
	u8    * write_buff = os_malloc(cmd_buff->len);
	u32     buff_len = cmd_buff->len;

	if(write_buff == NULL)
	{
		flash_error_handler(handle, FLASH_CMD_WRITE);

		TRACE_E(TAG, "%s @%d, 0x%x memory overrun!\r\n", __FUNCTION__, __LINE__, handle);

		return;
	}
	#else
	u8    * write_buff = flash_buff;
	u32     buff_len = sizeof(flash_buff);
	#endif

	while(cmd_buff->len > write_len)
	{
		#if CONFIG_CACHE_ENABLE
		flush_dcache(cmd_buff->buff, cmd_buff->len);
		#endif

		cpy_len = MIN(buff_len, (cmd_buff->len - write_len));
		memcpy(write_buff, cmd_buff->buff + write_len, cpy_len);

		// check data validity after memcpy.
		if(cmd_buff->crc != calc_crc32(0, cmd_buff->buff, cmd_buff->len))
		{
			write_status = BK_FAIL;
			line_num = __LINE__;
			break;
		}

		write_status = bk_flash_write_bytes(cmd_buff->addr + write_len, (u8 *)write_buff, cpy_len);

		if(write_status != BK_OK)
		{
			line_num = __LINE__;
			break;
		}

		write_len += cpy_len;
	}

	cmd_buff->ret_status = write_status;

	int ret_val = mb_ipc_send(handle, FLASH_CMD_WRITE, (u8 *)cmd_buff, sizeof(flash_cmd_t), FLASH_SVR_WAIT_TIME);

	#ifdef  DYNAMIC_FLASH_BUFFER
	os_free(write_buff);
	#endif

	if(ret_val != 0)
		TRACE_E(TAG, "0x%x, write-0x%x : %d, %d.\r\n", handle, cmd_buff->addr, cmd_buff->ret_status, ret_val);

	if(write_status != BK_OK)
		TRACE_E(TAG, "%s @%d, data=%d wr_len=%d.\r\n", __FUNCTION__, line_num, write_status, write_len);

	(void)ret_val;
}

#if CONFIG_FLASH_BYPASS_OTP_OPERATION
static void flash_bypass_otp_handler(u32 handle, flash_bypass_otp_ipc_cmd_t *ipc_cmd, uint8_t *data_buff, u8 connect_id)
{
	flash_bypass_otp_ctrl_t otp_cfg;
	bk_err_t ret = BK_OK;

	memset(&otp_cfg, 0, sizeof(otp_cfg));

	otp_cfg.otp_idx = ipc_cmd->otp_idx;
	otp_cfg.addr_offset = ipc_cmd->addr_offset;
	otp_cfg.read_len = ipc_cmd->read_len;
	otp_cfg.write_len = ipc_cmd->write_len;

	if(ipc_cmd->cmd == FLASH_BYPASS_OTP_WRITE && data_buff != NULL && ipc_cmd->write_len > 0)
	{
		otp_cfg.write_buf = data_buff;
	}

	if(ipc_cmd->cmd == FLASH_BYPASS_OTP_READ && ipc_cmd->read_len > 0)
	{
		#ifdef DYNAMIC_FLASH_BUFFER
		otp_cfg.read_buf = (uint8_t *)os_malloc(ipc_cmd->read_len);
		#else
		if(ipc_cmd->read_len > sizeof(flash_bypass_buff))
		{
			flash_error_handler(handle, FLASH_CMD_BYPASS_OTP_OPERATION);
			TRACE_E(TAG, "%s @%d, buffer overflow!\r\n", __FUNCTION__, __LINE__);
			return;
		}
		otp_cfg.read_buf = flash_bypass_buff;
		#endif

		if(otp_cfg.read_buf == NULL)
		{
			flash_error_handler(handle, FLASH_CMD_BYPASS_OTP_OPERATION);
			TRACE_E(TAG, "%s @%d, memory overrun!\r\n", __FUNCTION__, __LINE__);
			return;
		}
	}

	ret = flash_bypass_otp_operation((flash_bypass_otp_cmd_t)ipc_cmd->cmd, &otp_cfg);

	ipc_cmd->ret_status = ret;

	int ret_val = mb_ipc_send(handle, FLASH_CMD_BYPASS_OTP_OPERATION, (u8 *)ipc_cmd, sizeof(flash_bypass_otp_ipc_cmd_t), FLASH_SVR_WAIT_TIME);

	if(ret_val != 0)
	{
		TRACE_E(TAG, "%s @%d, send response failed, ret_val=%d\r\n", __FUNCTION__, __LINE__, ret_val);
	}

	if(ipc_cmd->cmd == FLASH_BYPASS_OTP_READ && ret == BK_OK && otp_cfg.read_buf != NULL && ipc_cmd->read_len > 0)
	{
		ret_val = mb_ipc_send(handle, FLASH_CMD_BYPASS_OTP_OPERATION, otp_cfg.read_buf, ipc_cmd->read_len, FLASH_SVR_WAIT_TIME);

		if(ret_val != 0)
		{
			TRACE_E(TAG, "%s @%d, send read data failed, ret_val=%d\r\n", __FUNCTION__, __LINE__, ret_val);
			#ifdef DYNAMIC_FLASH_BUFFER
			if(otp_cfg.read_buf != NULL)
			{
				os_free(otp_cfg.read_buf);
			}
			#endif
			return;
		}

		u32 event = rtos_wait_event_ex(&flash_svr_event, (0x01 << connect_id), 1, FLASH_SVR_WAIT_TIME);

		if(event == 0)
		{
			TRACE_E(TAG, "%s @%d, wait for ack event timeout!\r\n", __FUNCTION__, __LINE__);
			#ifdef DYNAMIC_FLASH_BUFFER
			if(otp_cfg.read_buf != NULL)
			{
				os_free(otp_cfg.read_buf);
			}
			#endif
			return;
		}

		memset(ipc_cmd, 0, sizeof(flash_bypass_otp_ipc_cmd_t));
		u8 ack_cmd = INVALID_USER_CMD_ID;
		ret_val = mb_ipc_recv(handle, &ack_cmd, (u8 *)ipc_cmd, sizeof(flash_bypass_otp_ipc_cmd_t), FLASH_SVR_WAIT_TIME);

		if(ret_val != sizeof(flash_bypass_otp_ipc_cmd_t) || ack_cmd != FLASH_CMD_BYPASS_OTP_OPERATION)
		{
			TRACE_E(TAG, "%s @%d, recv ack failed, ack_cmd=%d, ret_val=%d, event=0x%x\r\n", __FUNCTION__, __LINE__, ack_cmd, ret_val, event);
		}
		else
		{
			memset(ipc_cmd, 0, sizeof(flash_bypass_otp_ipc_cmd_t));
			ipc_cmd->ret_status = BK_OK;
			ret_val = mb_ipc_send(handle, FLASH_CMD_BYPASS_OTP_OPERATION, (u8 *)ipc_cmd, sizeof(flash_bypass_otp_ipc_cmd_t), FLASH_SVR_WAIT_TIME);
			if(ret_val != 0)
			{
				TRACE_E(TAG, "%s @%d, send final ack failed, ret_val=%d\r\n", __FUNCTION__, __LINE__, ret_val);
			}
		}

		#ifdef DYNAMIC_FLASH_BUFFER
		if(otp_cfg.read_buf != NULL)
		{
			os_free(otp_cfg.read_buf);
		}
		#endif
	}

	#ifdef DYNAMIC_FLASH_BUFFER
	if(data_buff != NULL && ipc_cmd->cmd == FLASH_BYPASS_OTP_WRITE)
	{
		os_free(data_buff);
	}
	#endif
}
#endif

static void flash_cmd_handler(u32 handle, u8 connect_id)
{
	int recv_len = mb_ipc_get_recv_data_len(handle);
	u8 user_cmd = mb_ipc_get_recv_user_cmd(handle);

#if CONFIG_FLASH_BYPASS_OTP_OPERATION
	if(user_cmd == FLASH_CMD_BYPASS_OTP_OPERATION) {
		flash_bypass_otp_ipc_cmd_t ipc_cmd;

		recv_len = mb_ipc_recv(handle, &user_cmd, (u8 *)&ipc_cmd, sizeof(ipc_cmd), 0);
		if(recv_len != sizeof(ipc_cmd) || user_cmd != FLASH_CMD_BYPASS_OTP_OPERATION)
		{
			flash_error_handler(handle, user_cmd);
			TRACE_E(TAG, "recv bypass otp cmd failed! user_cmd=%d, recv_len=%d\r\n", user_cmd, recv_len);
			return;
		}

		flash_bypass_otp_handler(handle, &ipc_cmd, ipc_cmd.buf, connect_id);
		
		return;
	}
#endif

	flash_cmd_t   cmd_buff;
	memset(&cmd_buff, 0, sizeof(cmd_buff));

	if(recv_len != sizeof( cmd_buff))
	{
		mb_ipc_recv(handle, &user_cmd, NULL, 0, 0);  // data_buff == NULL or buff_len == 0 just discard all data.

		// if client wait_forever, then must send a response even if user_cmd == INVALID_USER_CMD_ID.
		// flash client does not wait forever, so only respond then cmd is valid.
		if(user_cmd != INVALID_USER_CMD_ID)
			flash_error_handler(handle, user_cmd);

		TRACE_E(TAG, "recv user_cmd=%d, data len failed! %d\r\n", user_cmd, recv_len);

		return;
	}

	recv_len = mb_ipc_recv(handle, &user_cmd, (u8 *)&cmd_buff, sizeof(cmd_buff), 0);

	if(recv_len != sizeof( cmd_buff))
	{
		// if client wait_forever, then must send a response even if user_cmd == INVALID_USER_CMD_ID.
		// flash client does not wait forever, so only respond then cmd is valid.
		if(user_cmd != INVALID_USER_CMD_ID)
			flash_error_handler(handle, user_cmd);

		TRACE_E(TAG, "recv user_cmd=%d, data len failed! %d\r\n", user_cmd, recv_len);

		return;
	}

	switch(user_cmd)
	{
		// flash driver cmds.
		// these flash driver cmds should be turned off when all APPs have been updated to use flash partition APIs.
		#if 1
		case FLASH_CMD_ERASE_SECTOR:
			flash_erase_handler(handle, &cmd_buff);
			break;

		case FLASH_CMD_READ:
			flash_read_handler(handle, &cmd_buff, connect_id);
			break;

		case FLASH_CMD_WRITE:
			flash_write_handler(handle, &cmd_buff);
			break;

		case FLASH_CMD_FAST_ERASE:
			flash_fast_erase_handler(handle, &cmd_buff);
			break;
		#endif

		default:
			flash_error_handler(handle, user_cmd);
			TRACE_E(TAG, "recv unknown user_cmd=%d\r\n", user_cmd);
			break;
	}

	return;
}

/* ========================================================
 *
 *        FLASH-SERVER
 *
 ==========================================================*/

static u32 flash_svr_rx_callback(u32 handle, u32 connect_id)
{
	u32  connect_flag;

	if(connect_id >= FLASH_SVR_CONNECT_MAX)
		return 0;

	connect_flag = 0x01 << connect_id;

	rtos_set_event_ex(&flash_svr_event, connect_flag);

	return 0;
}

static void flash_svr_connect_handler(u32 handle, u8 connect_id)
{
	u32  cmd_id;

	int ret_val = mb_ipc_get_recv_event(handle, &cmd_id);

	if(ret_val != 0)  // failed
	{
		TRACE_E(TAG, "get evt fail %x %d.\r\n", handle, ret_val);
		return;
	}

	if(cmd_id > MB_IPC_CMD_MAX && cmd_id != MB_IPC_SEND_CMD)
	{
		TRACE_I(TAG, "cmd-id %d (may be internal event, ignoring)\r\n", cmd_id);
		return;
	}

	u8  src =0, dst = 0;

	extern int mb_ipc_get_connection(u32 handle, u8 *src, u8 * dst);

	mb_ipc_get_connection(handle, &src, &dst);

	if(cmd_id == MB_IPC_SEND_CMD)
	{
		bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_FLASH_OP,0x0,0x0);
		flash_cmd_handler(handle, connect_id);
		bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_FLASH_OP,0x1,0x0);
	}
	#if 0
	else if(cmd_id == MB_IPC_DISCONNECT_CMD)
	{
		TRACE_I(TAG, "disconnect 0x%x, %x-%x.\r\n", handle, src, dst);
	}
	else if(cmd_id == MB_IPC_CONNECT_CMD)
	{
		TRACE_I(TAG, "connect 0x%x, %x-%x.\r\n", handle, src, dst);
	}
	#endif
	else  /* any other commands. */
	{
		TRACE_I(TAG, "cmd=%d, 0x%x, %x-%x.\r\n", cmd_id, handle, src, dst);
	}

}

static void flash_server_task(void * param)
{
	u32  handle;
	u32  connect_handle;

	if(rtos_init_event_ex(&flash_svr_event) != BK_OK)
	{
		rtos_delete_thread(NULL);
		return;
	}

	handle = mb_ipc_socket(IPC_GET_ID_PORT(FLASH_SERVER), flash_svr_rx_callback);

	if(handle == 0)
	{
		TRACE_E("ipc_svr", "create_socket failed.\r\n");

		rtos_deinit_event_ex(&flash_svr_event);
		rtos_delete_thread(NULL);

		return ;
	}

	s_flash_svr_init = 2;

	while(1)
	{
		u32 event = rtos_wait_event_ex(&flash_svr_event, FLASH_SVR_EVENTS, 1, BEKEN_WAIT_FOREVER);

		if(event == 0)  // timeout.
		{
			continue;
		}

		if(event & FLASH_SVR_QUIT_EVENT)
		{
			break;
		}

		for(int i = 0; i < FLASH_SVR_CONNECT_MAX; i++)
		{
			if(event & (0x01 << i))
			{
				connect_handle = mb_ipc_server_get_connect_handle(handle, i);
				flash_svr_connect_handler(connect_handle, i);
			}
		}
	}

	mb_ipc_server_close(handle, FLASH_SVR_WAIT_TIME);

	rtos_deinit_event_ex(&flash_svr_event);

	s_flash_svr_init = 0;

	rtos_delete_thread(NULL);
}

bk_err_t bk_flash_svr_init(void)
{
	if(s_flash_svr_init)
		return BK_OK;

	s_flash_svr_init = 1;

	#if IPC_GET_ID_CPU(FLASH_SERVER) != 0   // != CPU0.
	#error server cpu configuration error!
	#endif

	int ret_val;

	ret_val = rtos_create_thread(NULL, FLASH_SVR_PRIORITY, "flash_svr",
					flash_server_task, FLASH_SVR_STACK_SIZE, NULL);

	return ret_val;

}

bk_err_t bk_flash_svr_deinit(void)
{
	if(s_flash_svr_init != 2)
		return BK_OK;

	s_flash_svr_init = 1;

	rtos_set_event_ex(&flash_svr_event, FLASH_SVR_QUIT_EVENT);

	return BK_OK;
}


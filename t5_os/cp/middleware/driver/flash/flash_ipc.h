#ifndef _FLASH_IPC_H_
#define _FLASH_IPC_H_

#include <common/bk_include.h>

enum
{
	FLASH_CMD_ERASE_SECTOR = 0,
	FLASH_CMD_ERASE_32k,
	FLASH_CMD_ERASE_BLOCK,
	FLASH_CMD_READ,
	FLASH_CMD_READ_DONE,
	FLASH_CMD_WRITE,
	FLASH_CMD_FAST_ERASE,
	FLASH_CMD_BYPASS_OTP_OPERATION,
} ;

typedef struct
{
	u32     part_id  : 8;
	u32     addr     : 24;
	u8    * buff;
	u16     len;
	int16   ret_status;
	u32     crc;
} flash_cmd_t;

#define FLASH_IPC_READ_SIZE     0x200
#define FLASH_IPC_WRITE_SIZE    0x200

#if CONFIG_FLASH_BYPASS_OTP_OPERATION
#include "flash_bypass.h"

typedef struct {
	uint8_t  cmd;           // flash_bypass_otp_cmd_t
	uint8_t  otp_idx;
	uint16_t addr_offset;
	uint16_t read_len;
	uint16_t write_len;
	int16_t  ret_status;
	uint8_t *buf;
} flash_bypass_otp_ipc_cmd_t;

#define FLASH_IPC_BYPASS_OTP_SIZE  1024  
#endif

#endif //_FLASH_IPC_H_
// eof


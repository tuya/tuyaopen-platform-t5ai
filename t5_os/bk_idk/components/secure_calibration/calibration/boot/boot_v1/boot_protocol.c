// Copyright 2020-2022 Beken
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
#include <stdlib.h>
#include <string.h>
#include "system.h"
//#include "board.h"
#include "crc32.h"
#include "boot_protocol.h"
#include "driver_uart0.h"
//#include "driver_system.h"
//#include "driver_flash.h"
#include "hal_wdt.h"

#define RX_CMD_BUFF_SIZE		(4200)

enum
{
	FRM_TYPE_INVALID = 0,
	FRM_TYPE_BK_HCI,
	FRM_TYPE_PPP_LIKE,
};

typedef struct
{
	u8			cmd_hdr_0x01;
	u8			cmd_hdr_0xe0;
	u8			cmd_hdr_0xfc;
	u8			cmd_len;		// < 0xFF
	u8			cmd_id;
	u8			cmd_param[0];	// (cmd_len - 1) bytes.
} bk_hci_cmd_frm_t;

typedef struct
{
	u8			rsp_hdr_0x04;
	u8			rsp_hdr_0x0e;
	u8			rsp_len;
	u8			cmd_hdr_0x01;
	u8			cmd_hdr_0xe0;
	u8			cmd_hdr_0xfc;
	u8			cmd_id;
	u8			rsp_param[0];	// (rsp_len - 4) bytes.
} bk_hci_rsp_frm_t;

typedef struct
{
	u8			cmd_hdr_0x01;
	u8			cmd_hdr_0xe0;
	u8			cmd_hdr_0xfc;
	u8			cmd_len;		// 0xFF
	u8			cmd_id;			// 0xF4
	u8			flash_cmd_len_low;
	u8			flash_cmd_len_high;
	u8			flash_cmd_id;
	u8			flash_cmd_param[0];	// (flash_cmd_len - 1) bytes.
} bk_flash_cmd_frm_t;

typedef struct
{
	u8			rsp_hdr_0x04;
	u8			rsp_hdr_0x0e;
	u8			rsp_len;		// 0xFF
	u8			cmd_hdr_0x01;
	u8			cmd_hdr_0xe0;
	u8			cmd_hdr_0xfc;
	u8			cmd_id;			//0xF4
	u8			flash_rsp_len_low;
	u8			flash_rsp_len_high;
	u8			flash_cmd_id;
	u8			flash_cmd_result;
	u8			flash_rsp_param[0];	// (flash_rsp_len - 2) bytes.
} bk_flash_rsp_frm_t;

enum
{
	HCI_WAIT_0X01 = 0,
	HCI_WAIT_0XE0,
	HCI_WAIT_0XFC,
};

enum
{
	HCI_RX_CMD_LEN = 0,
	HCI_RX_CMD_ID,
	HCI_RX_CMD_PARAM,
	HCI_RX_FLASH_CMD_LEN_LOW,
	HCI_RX_FLASH_CMD_LEN_HIGH,
	HCI_RX_FLASH_CMD_ID,
};

enum
{
	CMD_TYPE_INVALID = 0,
	CMD_TYPE_COMMON,
	CMD_TYPE_FLASH,
};

typedef struct
{
	u8		frm_type;
	u8		frm_done;

	/* for hci frame type. */
	u8		hci_hdr_state;
	u8		hci_rx_state;

	/*for ppp-like frame type. */
	//u8		ppp_frm_crc8;
	//u8		ppp_esc_next;
	//u8		ppp_rx_state;

	u8		cmd_type;		// COMMON_CMD_xx or FLASH_CMD_xx.
	u16		cmd_len;
	u16		cmd_id;
	u32		cmd_param[RX_CMD_BUFF_SIZE / sizeof(u32)];		// buffer align with 4-bytes.
	u16		write_idx;

	u16		read_idx;	// used to concurrent process for cmd_rx & sector_write.
	u8		status;		// for concurrent process of sector_write.
} rx_frm_ctrl_t;
#if 0
enum
{	
    STATUS_OK = 0,
    FLASH_STATUS_BUSY = 1,
    FLASH_OP_T_OUT = 3,
    PACK_LEN_ERROR = 4,
    PARAM_ERROR = 6,
    UNKNOW_CMD = 7,
};
#endif

#if 0
enum
{	
    STATUS_OK = 0,
    FLASH_STATUS_BUSY = 1,
    SPI_OP_T_OUT = 2,
    FLASH_OP_T_OUT = 3,
    PACK_LEN_ERROR = 4,

    PACK_PAYLOAD_LACK = 5,

    PARAM_ERROR = 6,

    UNKNOW_CMD = 7,
};
 #endif
 
#define     REG_XVR_BASE_ADDR                   (0x4a800000)
#define     BK_SPI_BASE_ADDR                    (0x44860000)
#define     BK_GPIO_BASE_ADDR                   (0x44000400)

static rx_frm_ctrl_t	rx_frm_ctrl = {0};
rx_link_buf_t			rx_link_buf;
extern unsigned int flash_id;

typedef int(*key_process_callback)(void*,u16);
key_process_callback aes_key_data_process;


u32 common_cmd_process(rx_frm_ctrl_t * frm_ctrl);
u32 flash_cmd_process(rx_frm_ctrl_t * frm_ctrl);
u32 flash_cmd_sector_write(rx_frm_ctrl_t * frm_ctrl);
u32 flash_cmd_sector_write_done(rx_frm_ctrl_t * frm_ctrl);
static int security_key_process_register_callback(void*data, u16 len, key_process_callback do_key_process);
static int security_aes_image_process_register_callback(key_process_callback do_key_process);

/*==============================================================================================
 *               ================                        ================ 
 *               ----------------     Data Link Layer    ----------------
 *               ================                        ================ 
 *==============================================================================================*/

static void reset_rx_ctrl(rx_frm_ctrl_t * frm_ctrl)
{
	// reset link type. -- link layer reset.
	frm_ctrl->frm_type = FRM_TYPE_INVALID;
	frm_ctrl->frm_done = 0;

	// reset HCI link state.  -- HCI link reset.
	frm_ctrl->hci_hdr_state = HCI_WAIT_0X01;
	frm_ctrl->hci_rx_state = HCI_RX_CMD_LEN;

	// reset PPP-like link state.  -- PPP-like link reset.
	//frm_ctrl->ppp_esc_next = 0;
	//frm_ctrl->ppp_rx_state = PPP_WAIT_SYNC;

	// reset app cmd type.  -- app layer reset.
	frm_ctrl->cmd_type = CMD_TYPE_INVALID;

	frm_ctrl->status = 0;		// used for concurrent process (cmd rx & sector write).
	frm_ctrl->read_idx = 0;		// used for concurrent process (cmd rx & sector write).
	frm_ctrl->write_idx = 0;	// should be reset for concurrent process ( indicates no param data received).

	frm_ctrl->cmd_len = 0;		// not necessary, will be set in state machine.

	return;
}

static void reset_rx_buff(rx_link_buf_t * link_buf)
{
	// discards all bytes in buffer.
	link_buf->read_idx = link_buf->write_idx;
}

static void rx_frm_bk_hci(rx_frm_ctrl_t * frm_ctrl, u8 rx_data)
{
	u8  * rx_cmd_buff = (u8 *)&frm_ctrl->cmd_param[0];
       
	if(frm_ctrl->hci_rx_state == HCI_RX_CMD_LEN)
	{
		frm_ctrl->cmd_len = rx_data;
		frm_ctrl->hci_rx_state = HCI_RX_CMD_ID;

		if(frm_ctrl->cmd_len == 0)  // frame fault.
		{
			reset_rx_ctrl(frm_ctrl);
			return;
		}
		/*
		else if(frm_ctrl->cmd_len >= RX_CMD_BUFF_SIZE)	// frame fault.
		{
			reset_rx_ctrl(frm_ctrl);
			return;
		}
		*/  // rx_data < 256, RX_CMD_BUFF_SIZE > 256.
	}
	else if(frm_ctrl->hci_rx_state == HCI_RX_CMD_ID)
	{
		frm_ctrl->cmd_id = rx_data;

		if(frm_ctrl->cmd_len == 0xFF)
		{
			if(frm_ctrl->cmd_id == 0xF4)	// EXT_CMD, it is a command of FLASH.
			{
				frm_ctrl->cmd_type = CMD_TYPE_FLASH;
				frm_ctrl->hci_rx_state = HCI_RX_FLASH_CMD_LEN_LOW;
			}
			else  // frame fault.
			{
				reset_rx_ctrl(frm_ctrl);
				return;
			}
		}
		else
		{
			frm_ctrl->cmd_type = CMD_TYPE_COMMON;
			frm_ctrl->hci_rx_state = HCI_RX_CMD_PARAM;
			frm_ctrl->write_idx = 0;
		}
	}
	else if(frm_ctrl->hci_rx_state == HCI_RX_CMD_PARAM)
	{
		rx_cmd_buff[frm_ctrl->write_idx] = rx_data;
		frm_ctrl->write_idx++;
	}
	else if(frm_ctrl->hci_rx_state == HCI_RX_FLASH_CMD_LEN_LOW)
	{
		frm_ctrl->cmd_len = rx_data;
		frm_ctrl->hci_rx_state = HCI_RX_FLASH_CMD_LEN_HIGH;
	}
	else if(frm_ctrl->hci_rx_state == HCI_RX_FLASH_CMD_LEN_HIGH)
	{
		frm_ctrl->cmd_len += ((u16)rx_data << 8);
		frm_ctrl->hci_rx_state = HCI_RX_FLASH_CMD_ID;

		if(frm_ctrl->cmd_len == 0)	// frame fault.
		{
			reset_rx_ctrl(frm_ctrl);
			return;
		}
		else if(frm_ctrl->cmd_len >= RX_CMD_BUFF_SIZE)	// frame fault.
		{
			reset_rx_ctrl(frm_ctrl);
			return;
		}
	}
	else if(frm_ctrl->hci_rx_state == HCI_RX_FLASH_CMD_ID)
	{
		frm_ctrl->cmd_id = rx_data;
		frm_ctrl->hci_rx_state = HCI_RX_CMD_PARAM;
		frm_ctrl->write_idx = 0;
	}
	else  // software fault.
	{
		reset_rx_ctrl(frm_ctrl);
		return;
	}

	// after HCI_RX_CMD_ID, HCI_RX_FLASH_CMD_ID, HCI_RX_CMD_PARAM process.
	if(frm_ctrl->hci_rx_state == HCI_RX_CMD_PARAM)
	{
		if((frm_ctrl->write_idx + 1) >= frm_ctrl->cmd_len)
		{
			frm_ctrl->frm_done = 1;
		}
	}

	return;
}

static void rx_frm_check_type(rx_frm_ctrl_t * frm_ctrl, u8 rx_data)
{
	if(frm_ctrl->hci_hdr_state == HCI_WAIT_0X01)
	{
		if(rx_data == 0x01)
			frm_ctrl->hci_hdr_state = HCI_WAIT_0XE0;
	}
	else if(frm_ctrl->hci_hdr_state == HCI_WAIT_0XE0)
	{
		if(rx_data == 0xE0)
			frm_ctrl->hci_hdr_state = HCI_WAIT_0XFC;
		else if(rx_data != 0x01)
			frm_ctrl->hci_hdr_state = HCI_WAIT_0X01;
	}
	else if(frm_ctrl->hci_hdr_state == HCI_WAIT_0XFC)
	{
		if(rx_data == 0xFC)
		{
			//----->   frame type check completed   ------->
			frm_ctrl->frm_type = FRM_TYPE_BK_HCI;
			frm_ctrl->frm_done = 0;

			frm_ctrl->hci_rx_state = HCI_RX_CMD_LEN;
			frm_ctrl->write_idx = 0;
		}
		else if(rx_data != 0x01)
			frm_ctrl->hci_hdr_state = HCI_WAIT_0X01;
		else
			frm_ctrl->hci_hdr_state = HCI_WAIT_0XE0;
	}
	else  // software fault.
	{
		reset_rx_ctrl(frm_ctrl);
		return;
	}

	return;
}

u32 boot_rx_frm_handler(void)
{
	rx_frm_ctrl_t * frm_ctrl = &rx_frm_ctrl;
	rx_link_buf_t * link_buf = &rx_link_buf;
	
	u8	* rx_temp_buff = (u8 *)&link_buf->rx_buf[0];
	u8    rx_data;
	u16   rd_idx = link_buf->read_idx;
	
	while(rd_idx != link_buf->write_idx)
	{
		rx_data = rx_temp_buff[rd_idx];

		if(frm_ctrl->frm_type == FRM_TYPE_BK_HCI)
		{
			rx_frm_bk_hci(frm_ctrl, rx_data);
		}
		/*
		else if(frm_ctrl->frm_type == FRM_TYPE_PPP_LIKE)
		{
		}
		*/
		else  // if (frm_ctrl->frm_type == FRM_TYPE_INVALID)
		{
			rx_frm_check_type(frm_ctrl, rx_data);
		}

		rd_idx++;
		if(rd_idx >= sizeof(link_buf->rx_buf))
			rd_idx = 0;
		link_buf->read_idx = rd_idx;

		if(frm_ctrl->frm_done)
			break;

	}

	// handle commands, 
	// do not care what type of link frame the command is carried by.
	// do not care what type of hardware interface the command is transferred by.
	if(frm_ctrl->frm_done)
	{
		if(frm_ctrl->cmd_type == CMD_TYPE_COMMON)
		{
			common_cmd_process(frm_ctrl);
		}
		else if(frm_ctrl->cmd_type == CMD_TYPE_FLASH)
		{
			if(frm_ctrl->cmd_id == FLASH_CMD_SECTOR_WRITE)
				flash_cmd_sector_write_done(frm_ctrl);
			else
				flash_cmd_process(frm_ctrl);
		}
		else  // !!!!  FAULT  !!!!
		{
			/* reset rx frame! */
		}

		/* command handle complete, reset state machine for next cmd process. */
		reset_rx_ctrl(frm_ctrl);

		// discards all bytes in buffer.
		reset_rx_buff(link_buf);
	}
	else
	{
		// speed up the flash write process.
		if(frm_ctrl->cmd_type == CMD_TYPE_FLASH)
		{
			if(frm_ctrl->cmd_id == FLASH_CMD_SECTOR_WRITE)
			{
				if(frm_ctrl->write_idx > 0)  // some param data received.
				{
					flash_cmd_sector_write(frm_ctrl);
				}
			}
		}
	}

	return 0;
}


/*==============================================================================================
 *
 *                  ================    Application Layer.  ================ 
 *                  ================     command handler    ================ 
 *
 *==============================================================================================*/

#define TBL_SIZE(tbl)		(sizeof(tbl) / sizeof(tbl[0]))
const u8 build_version[] = " " __DATE__ " " __TIME__;
static void tx_rsp_data(u8 * buf, u16 len)
{
	u16 	i;

	for(i = 0; i < len; i++)
	{
		while( !UART0_TX_WRITE_READY )
		{
		}

		UART0_WRITE_BYTE(buf[i]);
	}
}
extern void uart_send(unsigned char *buff, int len);

static void tx_rsp_for_common_cmd(u16 cmd_id, u8 * cmd_param, u16 param_len)
{
	bk_hci_rsp_frm_t	rsp_frm;

	rsp_frm.rsp_hdr_0x04 = 0x04;
	rsp_frm.rsp_hdr_0x0e = 0x0e;
	rsp_frm.rsp_len = param_len + 4;
	rsp_frm.cmd_hdr_0x01 = 0x01;
	rsp_frm.cmd_hdr_0xe0 = 0xe0;
	rsp_frm.cmd_hdr_0xfc = 0xfc;
	rsp_frm.cmd_id = (u8)cmd_id;

	tx_rsp_data((u8 *)&rsp_frm, sizeof(rsp_frm));
	tx_rsp_data(cmd_param, param_len);
}

void boot_tx_startup_indication(void)
{
	#if 0
	u8	 temp[2] = {0x95, 0x27};

	// 04 0e 06 01 e0 fc fe 95 27

	tx_rsp_for_common_cmd(COMMON_CMD_STARTUP, &temp[0], 2);

	#else

	// 04 0e nn 01 e0 fc fe 20 xx xx xx xx ......

	tx_rsp_for_common_cmd(COMMON_CMD_STARTUP, (u8 *)build_version, sizeof(build_version) - 1);
	#endif
}

u8 uart_link_check_flag = 0 ;

//COMMON_CMD_LINK_CHECK
static u32 cmd_link_check_handler(u8 * cmd_param, u16 param_len)
{
	// 01 e0 fc 01 00
	// 04 0e 05 01 e0 fc 01 00

	u8	temp = 0;

	uart_link_check_flag = 1;
	
	//printf("enter cmd_link_check_handler \r\n");
	
	tx_rsp_for_common_cmd(COMMON_RSP_LINK_CHECK, &temp, 1);
	// not use the COMMON_CMD_LINK_CHECK as the parameter, because a bug in old version of ROM code.

	return 0;
}

#define ANA_XVR_NUM		16
static u32 XVR_ANALOG_REG_BAK[ANA_XVR_NUM] ;

//COMMON_CMD_REG_WRITE
static u32 cmd_reg_write_handler(u8 * cmd_param, u16 param_len)
{
	// 01 e0 fc 09   01   00 28 80 00    68 00 00 00
	// 04 0E 0C   01 E0 FC  01   00 28 80 00   68 00 00 00
	
    u32  reg_addr = 0, reg_data = 0;

    if(param_len < 8)
    return 1;

    memcpy(&reg_addr, cmd_param, 4);
    memcpy(&reg_data, cmd_param + 4, 4);

    *((volatile u32 *)reg_addr) = reg_data;

    s32  reg_index = (reg_addr - REG_XVR_BASE_ADDR) / 4;

    if ((reg_index >= 0) && (reg_index < ANA_XVR_NUM))
    {
        XVR_ANALOG_REG_BAK[reg_index] = reg_data;
    }

    tx_rsp_for_common_cmd(COMMON_CMD_REG_WRITE, cmd_param, param_len);

	return 0;
}

//COMMON_CMD_REG_READ
static u32 cmd_reg_read_handler(u8 * cmd_param, u16 param_len)
{
	// 01 e0 fc 05 03 00 28 80 00
	// 04 0E 0C   01 E0 FC 03 00 28 80 00 00 00 00 00
	
	u32  reg_addr = 0, reg_data = 0;

	if(param_len < sizeof(u32))
		return 1;

	memcpy(&reg_addr, cmd_param, sizeof(u32));

	s32  reg_index = (reg_addr - REG_XVR_BASE_ADDR) / 4;

	if ((reg_index >= 0) && (reg_index < ANA_XVR_NUM))
	{
		reg_data = XVR_ANALOG_REG_BAK[reg_index];
	}
	else
	{
		reg_data = *((volatile u32 *)reg_addr);
	}

	memcpy(&cmd_param[4], &reg_data, 4);
	
	//printf("enter cmd_reg_read_handler reg_addr :0x%x, param_len :0x%x \r\n",reg_addr,param_len);
	
	tx_rsp_for_common_cmd(COMMON_CMD_REG_READ, cmd_param, param_len + 4);

	return 0;
}

#define XOR_MASK_REG_RW		0x37323536

//COMMON_CMD_EXT_REG_WRITE
static u32 cmd_ext_reg_write_handler(u8 * cmd_param, u16 param_len)
{
	// 01 e0 fc 09   11   00 28 80 00    68 00 00 00
	// 04 0E 0C   01 E0 FC  11   00 28 80 00   68 00 00 00
	
	u32  reg_addr = 0, reg_data = 0;

	if(param_len < 8)
		return 1;

	memcpy(&reg_addr, cmd_param, 4);
	memcpy(&reg_data, cmd_param + 4, 4);

	reg_data = reg_data ^ XOR_MASK_REG_RW;

	*((volatile u32 *)reg_addr) = reg_data;

	tx_rsp_for_common_cmd(COMMON_CMD_EXT_REG_WRITE, cmd_param, param_len);

	return 0;
}

//COMMON_CMD_EXT_REG_READ
static u32 cmd_ext_reg_read_handler(u8 * cmd_param, u16 param_len)
{
	// 01 e0 fc 05 13 00 28 80 00
	// 04 0E 0C   01 E0 FC 13 00 28 80 00 00 00 00 00
	
	u32  reg_addr = 0, reg_data = 0;

	if(param_len < sizeof(u32))
		return 1;

	memcpy(&reg_addr, cmd_param, sizeof(u32));

	reg_data = *((volatile u32 *)reg_addr);

	reg_data = reg_data ^ XOR_MASK_REG_RW;

	memcpy(&cmd_param[4], &reg_data, 4);

	tx_rsp_for_common_cmd(COMMON_CMD_EXT_REG_READ, cmd_param, param_len + 4);

	return 0;
}

// EXT_CMD_RAM_WRITE
static u32 cmd_ram_write_handler(u8 * cmd_param, u16 param_len)
{
	// 01 e0 fc 5+(n+1)   21   00 28 80 00  (D0 D1 ~ Dn)
	// 04 0E 0a   01 E0 FC  21   00 28 80 00   size0~size1
	
	u32  ram_addr = 0;

	if(param_len < 4)
		return 1;

	memcpy(&ram_addr, cmd_param, 4);
	memcpy((u8 *)ram_addr, cmd_param + 4, param_len - 4);

	cmd_param[4] = param_len - 4;
	cmd_param[5] = (param_len - 4) >> 8;

	tx_rsp_for_common_cmd(EXT_CMD_RAM_WRITE, cmd_param, 6);

	return 0;
}

// #include "mon_call.h"

// EXT_CMD_RAM_READ
static u32 cmd_ram_read_handler(u8 * cmd_param, u16 param_len)
{
	// 01 e0 fc 07 23 00 28 80 00 size0~size1
	// 04 0E 8+(n+1)   01 E0 FC 23 00 28 80 00 (D0 D1 ~ Dn)
	
	u32  ram_addr = 0;
	u16  size = 0;

	if(param_len < 6)
		return 1;

	memcpy(&ram_addr, cmd_param, 4);
	memcpy(&size, cmd_param + 4, 2);

	if(size > 0xF0)
	{
		#if 0  // these codes were used to debug monitor.
		if(size == 0xF8)
		{
			//u32 buf[8];
			//memset(buf, 0xFF, 32);
			
			u32 status = mon_write_otp(40, (u32 *)ram_addr, 32);
			bk_printf("\r\nResult:%x", status);
		}
		else if(size == 0xF2)
		{
			extern void test_flash(void);
			test_flash();
			// mon_task_yield();
		}
		else if(size == 0xF3)
		{
			__asm volatile( ".word 0x1234;" );
		}
		else if(size == 0xF4)
		{
			mon_app_started();
		}
		else if(size == 0xF5)
		{
			u32 status = mon_read_otp((u32 *)ram_addr, 40, 32);
			bk_printf("\r\nResult:%x", status);
		}
		#endif 
		return 1;
	}

	memcpy(cmd_param + 4, (u8 *)ram_addr, size);

	tx_rsp_for_common_cmd(EXT_CMD_RAM_READ, cmd_param, 4 + size);

	return 0;
}

//COMMON_CMD_REBOOT
static u32 cmd_reboot_handler(u8 * cmd_param, u16 param_len)
{
	// 01 E0 FC 02 0E A5
#if 0
	u8	 temp = 0xA5;

	if(param_len < 1)
		return 1;

	if(cmd_param[0] != 0xA5)
	{
		// 04 0e 05 01 e0 fc 0e a5

		tx_rsp_for_common_cmd(COMMON_CMD_REBOOT, &temp, 1);
	}
	else
	{
		uart0_disable();
		wdt_time_set(0x5);
		while(1)
		{
		}
	}
#endif
	return 0;
}

//COMMON_CMD_RESET  // 01 E0 FC 05 70 53 45 41 4E
static u32 cmd_reset_handler(u8 * cmd_param, u16 param_len)
{
	// 01 E0 FC 05 70 53 45 41 4E
	
	if(param_len < 4)
		return 1;

	if( (cmd_param[0] != 0x53) || 
		(cmd_param[1] != 0x45) || 
		(cmd_param[2] != 0x41) || 
		(cmd_param[3] != 0x4E) )
	{
		// 04 0e 08 01 e0 fc 70 p[0] p[1] p[2] p[3]

		tx_rsp_for_common_cmd(COMMON_CMD_RESET, cmd_param, param_len);
	}
	else
	{
		uart0_disable();
		wdt_time_set(0x5);
		while(1)
		{
		}
	}

	return 0;
}

// COMMON_CMD_STAY_ROM
static u32 cmd_stay_rom_handler(u8 * cmd_param, u16 param_len)
{
	// 01 e0 fc 02 AA 55
	// 04 0e 05 01 E0 FC AA 55
	
	if(param_len < 1)
		return 1;

	if(cmd_param[0] != 0x55)
		return 2;

	// discards all data in rx_link_buf ???

	uart_link_check_flag = 1;
      //  uart_tx_pin_cfg();
	
	tx_rsp_for_common_cmd(COMMON_CMD_STAY_ROM, cmd_param, param_len);

	return 0;
}

// COMMON_CMD_SET_BAUDRATE
extern void boot_uart_init(uint32_t baud_rate,uint8_t rx_fifo_thr);
static u32 cmd_set_br_handler(u8 * cmd_param, u16 param_len)
{
	// 01 e0 fc 06 0f 80 25 00 00 05
	// 04 0E 09   01 E0 FC 0f 80 25 00 00 05
	
	u32  rate;
	u8   delay_ms;

	if(param_len < 5)
	{
		return 1;
	}

	memcpy(&rate, cmd_param, 4);
	delay_ms = cmd_param[4];

	//uart0_init(rate);
	boot_uart_init(rate,85);
	
	//sys_delay_ms(delay_ms);
	delay_us(delay_ms);
	tx_rsp_for_common_cmd(COMMON_CMD_SET_BAUDRATE, cmd_param, param_len);

	return 0;
}

//COMMON_CMD_CHECK_CRC32
static u32 cmd_check_crc_handler(u8 * cmd_param, u16 param_len)
{
	// 01 e0 fc 09 10  00 00 00 00  00 10 00 00
	// 04 0e 08  01 e0 fc 10 crc0 crc1 crc2 crc3

	u32  start_addr = 0, end_addr = 0, data_len;
	u32  data_crc32 = 0xffffffff;

	if(param_len < (sizeof(u32) * 2))
		return 1;

	memcpy(&start_addr, cmd_param, 4);
	memcpy(&end_addr, cmd_param + 4, 4);

	while( start_addr <= end_addr )
	{
		wdt_time_set(0xA000);

		data_len = end_addr - start_addr + 1;

		if(data_len > 256)
		{
			data_len = 256;
		}

		// cmd_param is the rx_frm_ctrl.cmd_param
		//flash_read_data(cmd_param, start_addr, data_len);

		//data_crc32 = calc_crc32(data_crc32, cmd_param, data_len);

		start_addr += data_len;

	}

	tx_rsp_for_common_cmd(COMMON_CMD_CHECK_CRC32, (u8 *)&data_crc32, sizeof(data_crc32));

	return 0;
}

// EXT_CMD_JUMP
static u32 cmd_jump_handler(u8 * cmd_param, u16 param_len)
{
	// 01 E0 FC 05 25 a1 a2 a3 a4

	void (* jump_addr)(void);

	if(param_len < 4)
		return 1;

	memcpy(&jump_addr, cmd_param, 4);

	tx_rsp_for_common_cmd(EXT_CMD_JUMP, NULL, 0);

	//sys_delay_ms(3);
	delay_us(3000);
	uart0_disable();

	jump_addr();

	return 0;
}


#include "bootloader.h"

// SECURITY_AES_KEY
static u32 cmd_process_aes_key_handler(u8 * cmd_param, u16 param_len)
{
	//PC-Tx 01 e0 fc len 26 payload.
	//PC-Rx 04 0e len 01 e0 fc 26 payload.

	u8 rev_buf[256] = {0};

	if(param_len < sizeof(u32)&&(param_len > 255))
		return 1;

	printf("%s :enter \r\n",__FUNCTION__);

	memcpy(rev_buf, cmd_param , param_len);

	printf("param_len :0x%x \r\n", param_len);
	security_key_process_register_callback((u8*)(rev_buf),param_len,bl_aes_key_handler);

	tx_rsp_for_common_cmd(SECURE_AES_KEY, cmd_param, param_len);

	return 0;
}


extern void move_aes_bootloader_image(void);
//SECURITY_RANDOM_KEY
static u32 cmd_process_random_key_handler(u8 * cmd_param, u16 param_len)
{
	//PC-Tx 01 e0 fc len 27 payload
	//PC-Rx 04 0e len 01 e0 fc 27 payload.

	u8 rev_buf[256] = {0};
	u8 ret_vaule = 0;

	if(param_len < sizeof(u32)&&(param_len > 255))
		return 1;

	memcpy(rev_buf, cmd_param , param_len);
	printf("param_len :0x%x \r\n", param_len);
	ret_vaule = security_key_process_register_callback((u8*)(rev_buf),param_len,bl_random_key_handler);

	tx_rsp_for_common_cmd(SECURE_RANDOM_KEY, cmd_param, param_len);

	if(ret_vaule == 0)
	{
		hal_wdt_close();
		security_aes_image_process_register_callback(NULL); //need adapt.
		//move_aes_bootloader_image();
	}
	return 0;
}

static u32 cmd_process_receive_random_key_over(u8 * cmd_param, u16 param_len)
{
	//PC-Tx 01 e0 fc len 28 payload
	//PC-Rx 04 0e len 01 e0 fc 28 payload.

	u8 rev_buf[256] = {0};

	if(param_len < sizeof(u32)&&(param_len > 255))
		return 1;

	memcpy(rev_buf, cmd_param , param_len);

	security_key_process_register_callback((u8*)(rev_buf),param_len,bl_receive_random_key_over_handler);

	tx_rsp_for_common_cmd(SEC_RANDOM_KEY_REV_OVER, cmd_param, param_len);

	return 0;
}


static u8 secure_boot_enable_flag = 0;
static u32 cmd_process_secure_boot_enable(u8 * cmd_param, u16 param_len)
{
	//PC-Tx 01 e0 fc len 29 payload
	//PC-Rx 04 0e len 01 e0 fc 29 payload.

	if(param_len < sizeof(u32)&&(param_len > 255))
		return 1;

	memcpy(&secure_boot_enable_flag, cmd_param , param_len);

	security_key_process_register_callback((u8*)(&secure_boot_enable_flag),param_len,bl_secure_boot_enable_handler);

	tx_rsp_for_common_cmd(SECURE_BOOT_ENABLE, cmd_param, param_len);

	return 0;
}

static u32 cmd_unkown_handler(u16 cmd_id)
{
	u8	temp = UNKNOW_CMD;

	// to be compatible with old version of BOOTROM & PC tool,
	// return CMD_ID & status to PC.
	// it is not a good fault handling,
	// because some CMD use the same format for right ACK, (CMD_ID + result).
	// should use a dedicated flag for fault.
	tx_rsp_for_common_cmd(cmd_id, &temp, 1);

	return 0;
}


#define FLASH_PAGE_SIZE			 (0x100)

typedef u32 (*cmd_handler_t)(u8 * cmd_param, u16 param_len);

typedef struct
{
	u16				cmd_id;
	cmd_handler_t	cmd_handler;
} cmd_hdlr_tbl_t;

static const cmd_hdlr_tbl_t	common_cmd_hdlr_tbl[] = 
{
	{ COMMON_CMD_LINK_CHECK,	cmd_link_check_handler    },
	{ COMMON_CMD_REG_WRITE,		cmd_reg_write_handler     },
	{ COMMON_CMD_REG_READ,		cmd_reg_read_handler      },
	{ COMMON_CMD_EXT_REG_WRITE,	cmd_ext_reg_write_handler },
	{ COMMON_CMD_EXT_REG_READ,	cmd_ext_reg_read_handler  },
	{ EXT_CMD_RAM_WRITE,		cmd_ram_write_handler     },
	{ EXT_CMD_RAM_READ,			cmd_ram_read_handler      },
	{ COMMON_CMD_REBOOT,		cmd_reboot_handler	      },
	{ COMMON_CMD_SET_BAUDRATE,	cmd_set_br_handler        },
	{ COMMON_CMD_CHECK_CRC32,	cmd_check_crc_handler	  },
	{ COMMON_CMD_RESET,			cmd_reset_handler	      },
	{ COMMON_CMD_STAY_ROM,		cmd_stay_rom_handler      },
	{ EXT_CMD_JUMP,				cmd_jump_handler          },
	{ SECURE_AES_KEY,			cmd_process_aes_key_handler},
	{ SECURE_RANDOM_KEY,		cmd_process_random_key_handler},
	{ SEC_RANDOM_KEY_REV_OVER,	cmd_process_receive_random_key_over},
	{ SECURE_BOOT_ENABLE,		cmd_process_secure_boot_enable},
};

u32 common_cmd_process(rx_frm_ctrl_t * frm_ctrl)
{
	u16		i;

	for(i = 0; i < TBL_SIZE(common_cmd_hdlr_tbl); i++)
	{
		if(frm_ctrl->cmd_id == common_cmd_hdlr_tbl[i].cmd_id)
		{
			common_cmd_hdlr_tbl[i].cmd_handler((u8 *)&frm_ctrl->cmd_param[0], frm_ctrl->cmd_len - 1);
			return 0;
		}
	}

	cmd_unkown_handler(frm_ctrl->cmd_id);

	return 1;
}

static int security_key_process_register_callback(void*data, u16 len, key_process_callback do_key_process)
{
	if(!do_key_process)
	{
		return -1;
	}
	else
	{
		aes_key_data_process = do_key_process;
		(*aes_key_data_process)(data,len);
	}

	return 0;
}

static int security_aes_image_process_register_callback(key_process_callback do_key_process)
{
	if(!do_key_process)
	{
		return -1;
	}
	else
	{
		aes_key_data_process = do_key_process;
		(*aes_key_data_process)(NULL,0);
	}

	return 0;
}

static void tx_rsp_for_flash_cmd_hdr(u16 cmd_id, u8 status, u16 param_len)
{
	bk_flash_rsp_frm_t	rsp_frm;

	rsp_frm.rsp_hdr_0x04 = 0x04;
	rsp_frm.rsp_hdr_0x0e = 0x0e;
	rsp_frm.rsp_len = 0xff;
	rsp_frm.cmd_hdr_0x01 = 0x01;
	rsp_frm.cmd_hdr_0xe0 = 0xe0;
	rsp_frm.cmd_hdr_0xfc = 0xfc;
	rsp_frm.cmd_id = 0xf4;
	rsp_frm.flash_rsp_len_low  = (u8)(param_len + 2);
	rsp_frm.flash_rsp_len_high = (u8)((param_len + 2) >> 8);
	rsp_frm.flash_cmd_id = (u8)cmd_id;
	rsp_frm.flash_cmd_result = status;

	tx_rsp_data((u8 *)&rsp_frm, sizeof(rsp_frm));
//	tx_rsp_data(cmd_param, param_len);

}

static void tx_rsp_for_flash_cmd(u16 cmd_id, u8 status, u8 * cmd_param, u16 param_len)
{
	#if 0
	bk_flash_rsp_frm_t	rsp_frm;

	rsp_frm.rsp_hdr_0x04 = 0x04;
	rsp_frm.rsp_hdr_0x0e = 0x0e;
	rsp_frm.rsp_len = 0xff;
	rsp_frm.cmd_hdr_0x01 = 0x01;
	rsp_frm.cmd_hdr_0xe0 = 0xe0;
	rsp_frm.cmd_hdr_0xfc = 0xfc;
	rsp_frm.cmd_id = 0xf4;
	rsp_frm.flash_rsp_len_low  = (u8)(param_len + 2);
	rsp_frm.flash_rsp_len_high = (u8)((param_len + 2) >> 8);
	rsp_frm.flash_cmd_id = (u8)cmd_id;
	rsp_frm.flash_cmd_result = status;

	tx_rsp_data((u8 *)&rsp_frm, sizeof(rsp_frm));
	#else
	tx_rsp_for_flash_cmd_hdr(cmd_id, status, param_len);
	#endif
	tx_rsp_data(cmd_param, param_len);

}

// FLASH_CMD_WRITE
#define BOOT_DL_START_ADDR (0x11000)

static u32 flash_cmd_write_handler(u8 * cmd_param, u16 param_len)
{
	// (01 E0 FC FF  F4)  09 00 06   00 00 00 00 (D0 D1 ~ Dn)
	// 04 0E FF   01 E0 FC   F4 07 00 06 status 00 00 00 00 size

	u32  addr;
	u8   ret_val = 0;

	if(param_len < 4)
		return 1;

	memcpy(&addr, cmd_param, 4);

    if(addr < BOOT_DL_START_ADDR)
    {
		ret_val = PARAM_ERROR;
        // tx_rsp_for_flash_cmd(FLASH_CMD_WRITE, PARAM_ERROR, cmd_param, 4);
        // return 2;
    }
	else
	{
		//flash_write_data(&cmd_param[4], addr, param_len - 4);
	}

	cmd_param[4] = param_len - 4;

	tx_rsp_for_flash_cmd(FLASH_CMD_WRITE, ret_val, cmd_param, 5);

	return 0;
}

// FLASH_CMD_SECTOR_WRITE
u32 flash_cmd_sector_write(rx_frm_ctrl_t * frm_ctrl)
{
	// (01 E0 FC ff F4)  05 10 07   00 00 00 00 (D0 D1 D2 ...D4095)
	// 04 0e ff   01 e0 fc f4  06 00 07  AA 00 00 00 00

	u32  addr;
	u8   ret_val = 0;

	if(frm_ctrl->cmd_len != 0x1005)  // cmd, addr0~3, DATA0~4095.
		frm_ctrl->status = PACK_LEN_ERROR;

	if(frm_ctrl->status != 0)
	{
		frm_ctrl->read_idx = frm_ctrl->cmd_len - 1;  // to the end of param. so never write data again.
		return 1;
	}

	if(frm_ctrl->write_idx < 4)
		return 0;

	if(frm_ctrl->read_idx == 0)
		frm_ctrl->read_idx = 4;  // point to the write data.

	if(frm_ctrl->write_idx < (frm_ctrl->read_idx + FLASH_PAGE_SIZE))
	{
		return 0;
	}

	u8 * cmd_param = (u8 *)&frm_ctrl->cmd_param[0];

	memcpy(&addr, cmd_param, 4);   // get the write addr.

    if(addr < BOOT_DL_START_ADDR)
    {
		frm_ctrl->status = PARAM_ERROR;   // can't write bootloader.
        return 2;
    }
	if(addr & (FLASH_PAGE_SIZE - 1))
	{
		frm_ctrl->status = PARAM_ERROR;  // page write, must align on page boundary.
		return 2;
	}

	addr += frm_ctrl->read_idx - 4;
	cmd_param += frm_ctrl->read_idx;

	while(frm_ctrl->write_idx >= (frm_ctrl->read_idx + FLASH_PAGE_SIZE))
	{
		wdt_time_set(0xA000);

		//flash_write_data(cmd_param, addr, FLASH_PAGE_SIZE);

		addr += FLASH_PAGE_SIZE;
		cmd_param += FLASH_PAGE_SIZE;
		frm_ctrl->read_idx += FLASH_PAGE_SIZE;
	}

	if(ret_val != 0)
	{
		frm_ctrl->status = ret_val;
		return 3;
	}

	return 0;
}

// FLASH_CMD_SECTOR_WRITE
u32 flash_cmd_sector_write_done(rx_frm_ctrl_t * frm_ctrl)
{
	// (01 E0 FC ff F4)  05 10 07   00 00 00 00 (D0 D1 D2 ...D4095)
	// 04 0e ff   01 e0 fc f4  06 00 07  AA 00 00 00 00

	flash_cmd_sector_write(frm_ctrl);

	u8 * cmd_param = (u8 *)&frm_ctrl->cmd_param[0];

	tx_rsp_for_flash_cmd(FLASH_CMD_SECTOR_WRITE, frm_ctrl->status, cmd_param, 4);

	return 0;
}

// FLASH_CMD_READ
static u32 flash_cmd_read_handler(u8 * cmd_param, u16 param_len)
{
	// (01 E0 FC FF F4) 06 00 08 00 00 00 00 04
	// 04 0E FF   01 E0 FC F4   0A 00 08   status 00 00 00 00  D0 D1 D2 D3

	u32  addr;
	u16  size;
	u8   ret_val = STATUS_OK;

	// param check.
//	if(param_len < 5)
//		return 1;

	memcpy(&addr, cmd_param, 4);
	size = cmd_param[4];

	if(param_len == 5)
	{
		//flash_read_data( &cmd_param[4], addr, size);
	}
	else
	{
		ret_val = PACK_LEN_ERROR;
	}

	if(ret_val != 0)
		size = 0;

	tx_rsp_for_flash_cmd(FLASH_CMD_READ, ret_val, cmd_param, size + 4);

	return 0;
}

// FLASH_CMD_SECTOR_READ
static u32 flash_cmd_sector_read_handler(u8 * cmd_param, u16 param_len)
{
	// (01 E0 FC FF F4)   05 00 09   00 00 00 00
	// 04 0e ff   01 e0 fc f4  06 10 09 status 00 00 00 00  D0 D1 D2~ D4095

	u32  start_addr = 0, read_cnt = 0;
	u8   ret_val = STATUS_OK;

	// param check.
//	if(param_len < 4)
//		return 1;

	memcpy(&start_addr, cmd_param, 4);

	if(param_len == 4)
	{
		ret_val = STATUS_OK;
	}
	else
	{
		ret_val = PACK_LEN_ERROR;
	}

	tx_rsp_for_flash_cmd_hdr(FLASH_CMD_SECTOR_READ, ret_val, 0x1000 + 4); // 4 bytes address + 4096 bytes flash data.
	tx_rsp_data((u8 *)&start_addr, 4);  // 4 bytes address

	while(read_cnt < 0x1000)
	{
		wdt_time_set(0xA000);

		//flash_read_data(cmd_param, start_addr, 128);

		if(ret_val == STATUS_OK)
		{
			tx_rsp_data(cmd_param, 128);
			start_addr += 128;
			read_cnt += 128;
		}
		else
			break;
	}

	return 0;
}

// FLASH_CMD_SECTOR_ERASE
static u32 flash_cmd_sector_erase_handler(u8 * cmd_param, u16 param_len)
{
	// (01 E0 FC FF F4)  05 00 0B   00 00 00 00
	// 04 0e ff   01 e0 fc f4  06 00 0b AA 00 00 00 00

	u32  addr;
	u8   ret_val = 0;

	// param check.
//	if(param_len < 4)
//		return 1;

	memcpy(&addr, cmd_param, 4);

    if(addr < BOOT_DL_START_ADDR)
    {
		ret_val = PARAM_ERROR;   // can't erase bootloader.
    }
	else if(param_len == 4)
	{
		//flash_erase_cmd(addr, FLASH_OPCODE_SE);
	}
	else
	{
		ret_val = PACK_LEN_ERROR;
	}

	tx_rsp_for_flash_cmd((u16)FLASH_CMD_SECTOR_ERASE, ret_val, cmd_param, param_len);

	return 0;
}

// FLASH_CMD_SIZE_ERASE
static u32 flash_cmd_size_erase_handler(u8 * cmd_param, u16 param_len)
{
	// (01 E0 FC FF F4)  06 00 0f TYPE_CMD 00 00 00 00
	// 04 0e ff   01 e0 fc f4  07 00 0F 00 TYPE_CMD 00 00 00 00

	u32  addr;
	u8   ret_val = 0;
	u8   size_cmd;

	// param check.
//	if(param_len < 5)
//		return 1;

	size_cmd = cmd_param[0];

	memcpy(&addr, cmd_param + 1, 4);

    if(addr < BOOT_DL_START_ADDR)
    {
		ret_val = PARAM_ERROR;   // can't erase bootloader.
    }
	else if(param_len == 5)
	{
		if(size_cmd == 0x20)    //0x1000
        {
            //flash_erase_cmd(addr,FLASH_OPCODE_SE);
        }
        else if(size_cmd == 0xd8)  //0x10000   64KB block erase.
        {
           // flash_erase_cmd(addr,FLASH_OPCODE_BE2);
        }
        else if(size_cmd == 0x52)  //0x8000   32KB block erase.
        {
           // flash_erase_cmd(addr,FLASH_OPCODE_BE1);
        }          
	}
	else
	{
		ret_val = PACK_LEN_ERROR;
	}

	tx_rsp_for_flash_cmd(FLASH_CMD_SIZE_ERASE, ret_val, cmd_param, param_len);

	return 0;
}

// FLASH_CMD_REG_READ
static u32 flash_cmd_reg_read_handler(u8 * cmd_param, u16 param_len)
{
	// (01 E0 FC FF F4)   02 00 0c  05
	// 04 0e ff 01 e0 fc f4 04 00 0c aa reg_addr reg_value

	// u8   reg_addr;
	u8   ret_val = 0;

	// param check.
	if(param_len < 1)
		return 1;

	// reg_addr = cmd_param[0];

	cmd_param[1] = 0;//flash_read_sr(1); 

	tx_rsp_for_flash_cmd(FLASH_CMD_REG_READ, ret_val, cmd_param, 2);

	return 0;
}

// FLASH_CMD_REG_WRITE
static u32 flash_cmd_reg_write_handler(u8 * cmd_param, u16 param_len)
{
	// (01 E0 FC FF F4)   03 00 0D  reg_addr reg_value
	// 04 0e ff 01 e0 fc f4 04 00 0D 00 reg_addr reg_value

	u8   ret_val = 0;
	u16  reg_val;
	//u8   sr_bytes = 1;
	u8   cnt = 0;

	// param check.
	if(param_len < 2)
		return 1;

	reg_val  = cmd_param[1];

	if(param_len > 2)
	{
		//sr_bytes = 2;
		memcpy(&reg_val, cmd_param + 1, 2);
	}

	while(cnt < 3)
	{
	#if 0
		flash_write_sr(sr_bytes, reg_val);

		if(flash_read_sr(sr_bytes) == reg_val)
			break;
	 #endif
		cnt++;
	}

	if(cnt >= 3)
		ret_val = FLASH_OP_T_OUT;

	tx_rsp_for_flash_cmd(FLASH_CMD_REG_WRITE, ret_val, cmd_param, param_len);

	return 0;
}

// FLASH_CMD_SPI_OPERATE
static u32 flash_cmd_spi_op_handler(u8 * cmd_param, u16 param_len)
{
	// (01 E0 FC FF F4)   n + 2 00 0E  D0 D1 D2 D3 D4 Dn
	// 04 0e ff 01 e0 fc f4 n+3 00 0e status d0 d1 d2 d3 d4 dn

	u8   ret_val = 0;

	// param check.
	if(param_len > 2048)  // cmd_param points to a buffer with 4200 bytes in size.
		return 1;

	u8 * p_rx = cmd_param + 2100;

	if(cmd_param[0] != 0x9F)
		ret_val = PARAM_ERROR;

	// memcpy(p_rx, &flash_id, sizeof(flash_id));
#if 0
    p_rx[0] = (flash_id >> 24);
    p_rx[1] = (flash_id >> 16);
    p_rx[2] = (flash_id >> 8);
    p_rx[3] = (flash_id & 0xff);	
#endif
	tx_rsp_for_flash_cmd(FLASH_CMD_SPI_OPERATE, ret_val, p_rx, param_len);

	return 0;
}

// EXT_CMD_RAM_WRITE
static u32 flash_cmd_ram_write_handler(u8 * cmd_param, u16 param_len)
{
	// (01 E0 FC FF F4) 5+(n+1)   21   00 28 80 00  (D0 D1 ~ Dn)
	// 04 0E FF   01 E0 FC  F4  08 00 21 status 00 28 80 00   size0~size1
	
	u32  ram_addr = 0;

	if(param_len < 4)
		return 1;

	memcpy(&ram_addr, cmd_param, 4);
	memcpy((u8 *)ram_addr, cmd_param + 4, param_len - 4);

	cmd_param[4] = param_len - 4;
	cmd_param[5] = (param_len - 4) >> 8;

	tx_rsp_for_flash_cmd(EXT_CMD_RAM_WRITE, 0, cmd_param, 6);

	return 0;
}

// EXT_CMD_RAM_READ
static u32 flash_cmd_ram_read_handler(u8 * cmd_param, u16 param_len)
{
	// (01 E0 FC FF F4) 07 00 23 00 00 00 00 size0~size1
	// 04 0E FF   01 E0 FC F4   6+(n+1)  23 status  00 00 00 00  (D0 D1  ~ Dn)
	
	u32  ram_addr = 0;
	u16  size = 0;

	if(param_len < 6)
		return 1;

	memcpy(&ram_addr, cmd_param, 4);
	memcpy(&size, cmd_param + 4, 2);

	if(size > 0x1000)
		return 1;

	memcpy(cmd_param + 4, (u8 *)ram_addr, size);

	tx_rsp_for_flash_cmd(EXT_CMD_RAM_READ, 0, cmd_param, 4 + size);

	return 0;
}

static u32 flash_cmd_unkown_handler(u16 cmd_id)
{
	u8	temp = UNKNOW_CMD;

	// status(temp) indicates whether the cmd is OK.
	tx_rsp_for_flash_cmd(cmd_id, temp, NULL, 0);

	return 0;
}


static const cmd_hdlr_tbl_t	flash_cmd_hdlr_tbl[] = 
{
	{ FLASH_CMD_WRITE,				flash_cmd_write_handler            },
//	{ FLASH_CMD_SECTOR_WRITE,		flash_cmd_sector_write_handler     },
	{ FLASH_CMD_READ,				flash_cmd_read_handler             },
	{ FLASH_CMD_SECTOR_READ,		flash_cmd_sector_read_handler	   },
//	{ FLASH_CMD_CHIP_ERASE,			flash_cmd_chip_erase_handler	   },
	{ FLASH_CMD_SECTOR_ERASE,		flash_cmd_sector_erase_handler     },
	{ FLASH_CMD_REG_READ,			flash_cmd_reg_read_handler		   },
	{ FLASH_CMD_REG_WRITE,			flash_cmd_reg_write_handler		   },
	{ FLASH_CMD_SPI_OPERATE,		flash_cmd_spi_op_handler		   },
	{ FLASH_CMD_SIZE_ERASE,			flash_cmd_size_erase_handler	   },
	{ EXT_CMD_RAM_WRITE,			flash_cmd_ram_write_handler        },
	{ EXT_CMD_RAM_READ,				flash_cmd_ram_read_handler         },
};

u32 flash_cmd_process(rx_frm_ctrl_t * frm_ctrl)
{
	u16		i;

	for(i = 0; i < TBL_SIZE(flash_cmd_hdlr_tbl); i++)
	{
		if(frm_ctrl->cmd_id == flash_cmd_hdlr_tbl[i].cmd_id)
		{
			flash_cmd_hdlr_tbl[i].cmd_handler((u8 *)&frm_ctrl->cmd_param[0], frm_ctrl->cmd_len - 1);
			return 0;
		}
	}

	flash_cmd_unkown_handler(frm_ctrl->cmd_id);

	return 1;
}



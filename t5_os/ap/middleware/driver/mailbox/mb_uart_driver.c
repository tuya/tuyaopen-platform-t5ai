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
#include <string.h>

#include <os/os.h>
#include <driver/mailbox_channel.h>
#include <driver/mb_chnl_buff.h>
#include <driver/mb_uart_driver.h>

#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif

#define MOD_TAG		"mUart"

// #define MB_UART_CRC8_ENABLE

#if (CONFIG_CPU_CNT > 1)

#define UART_XCHG_DATA_MAX		MB_CHNL_BUFF_LEN

typedef union
{
	struct
	{
		mb_chnl_hdr_t	chnl_hdr;
		void *	cmd_buff;
		u16		cmd_data_len;
		
		u8		uart_rts     : 1;
		u8		resvd        : 7;

		u8  	crc8;
	};

	mb_chnl_cmd_t	mb_cmd;
} mb_uart_cmd_t;

typedef union
{
	struct
	{
		mb_chnl_hdr_t	chnl_hdr;
		
		u8		uart_rts     : 1;
		u8		uart_tx_fail : 1;
		u8		resvd        : 6;
	};

	mb_chnl_ack_t	mb_ack;
} mb_uart_rsp_t;

enum
{
	MB_UART_SEND_DATA = 0,
	MB_UART_SEND_STATE,

	MB_UART_CMD_MAX,
};

#define MB_UART_TX_FIFO_LEN		UART_XCHG_DATA_MAX
#define MB_UART_RX_FIFO_LEN		UART_XCHG_DATA_MAX

typedef struct
{
	/* chnl data */
	u8				chnl_inited;
	u8				chnl_id;

	/* uart CSR */
	u8				ctrl_rts        : 1;   /* low level Asserted. */
	u8				ctrl_resvd      : 7;

	u8				status_cts      : 1;   /* low level Asserted. */
	u8  			status_overflow : 1;   /* NOT used, will discard RX data when error. */
	u8  			status_parity   : 1;   /* NOT used, will discard RX data when error. */
	u8  			status_frm_err  : 1;   /* NOT used, will discard RX data when error. */
	u8  			status_resvd    : 4;

	/* uart data */
	mb_uart_isr_t	rx_isr_callback;
	mb_uart_isr_t	tx_isr_callback;
	void        *   rx_isr_param;
	void        *   tx_isr_param;

	/* tx channel */
	u8			*	tx_xchg_buff;        /* uart tx FIFO */
	u8				tx_buf[MB_UART_TX_FIFO_LEN + 1];
	u16				tx_rd_idx;
	u16				tx_wr_idx;
	volatile u8		tx_in_process;
	volatile u8		tx_state_req;
	u8				tx_cmd;

	/* rx channel */
	u8			*	rx_xchg_buff;        /* uart rx FIFO */
	u8				rx_buf[2 * MB_UART_RX_FIFO_LEN + 1];
	u16				rx_rd_idx;
	u16				rx_wr_idx;
} mb_uart_cb_t;

#ifdef MB_UART_CRC8_ENABLE
static const u8 crc8_table[] =
{
0x00,0x31,0x62,0x53,0xc4,0xf5,0xa6,0x97,0xb9,0x88,0xdb,0xea,0x7d,0x4c,0x1f,0x2e,
0x43,0x72,0x21,0x10,0x87,0xb6,0xe5,0xd4,0xfa,0xcb,0x98,0xa9,0x3e,0x0f,0x5c,0x6d,
0x86,0xb7,0xe4,0xd5,0x42,0x73,0x20,0x11,0x3f,0x0e,0x5d,0x6c,0xfb,0xca,0x99,0xa8,
0xc5,0xf4,0xa7,0x96,0x01,0x30,0x63,0x52,0x7c,0x4d,0x1e,0x2f,0xb8,0x89,0xda,0xeb,
0x3d,0x0c,0x5f,0x6e,0xf9,0xc8,0x9b,0xaa,0x84,0xb5,0xe6,0xd7,0x40,0x71,0x22,0x13,
0x7e,0x4f,0x1c,0x2d,0xba,0x8b,0xd8,0xe9,0xc7,0xf6,0xa5,0x94,0x03,0x32,0x61,0x50,
0xbb,0x8a,0xd9,0xe8,0x7f,0x4e,0x1d,0x2c,0x02,0x33,0x60,0x51,0xc6,0xf7,0xa4,0x95,
0xf8,0xc9,0x9a,0xab,0x3c,0x0d,0x5e,0x6f,0x41,0x70,0x23,0x12,0x85,0xb4,0xe7,0xd6,
0x7a,0x4b,0x18,0x29,0xbe,0x8f,0xdc,0xed,0xc3,0xf2,0xa1,0x90,0x07,0x36,0x65,0x54,
0x39,0x08,0x5b,0x6a,0xfd,0xcc,0x9f,0xae,0x80,0xb1,0xe2,0xd3,0x44,0x75,0x26,0x17,
0xfc,0xcd,0x9e,0xaf,0x38,0x09,0x5a,0x6b,0x45,0x74,0x27,0x16,0x81,0xb0,0xe3,0xd2,
0xbf,0x8e,0xdd,0xec,0x7b,0x4a,0x19,0x28,0x06,0x37,0x64,0x55,0xc2,0xf3,0xa0,0x91,
0x47,0x76,0x25,0x14,0x83,0xb2,0xe1,0xd0,0xfe,0xcf,0x9c,0xad,0x3a,0x0b,0x58,0x69,
0x04,0x35,0x66,0x57,0xc0,0xf1,0xa2,0x93,0xbd,0x8c,0xdf,0xee,0x79,0x48,0x1b,0x2a,
0xc1,0xf0,0xa3,0x92,0x05,0x34,0x67,0x56,0x78,0x49,0x1a,0x2b,0xbc,0x8d,0xde,0xef,
0x82,0xb3,0xe0,0xd1,0x46,0x77,0x24,0x15,0x3b,0x0a,0x59,0x68,0xff,0xce,0x9d,0xac
};
#endif

static u8 cal_crc8_0x31(u8 *data_buf, u16 len)
{
	u8    crc = 0x00;

#if MB_UART_CRC8_ENABLE
	for (u16 i = 0; i < len; i++)
	{
		crc = crc8_table[crc ^ data_buf[i]];
	}
#endif

	return (crc);
}

#if CONFIG_SOC_SMP
#include "spinlock.h"
static SPINLOCK_SECTION volatile spinlock_t mb_uart_spin_lock = SPIN_LOCK_INIT;
#endif // CONFIG_SOC_SMP
static inline uint32_t mb_uart_enter_critical()
{
	uint32_t flags = rtos_disable_int();

#if CONFIG_SOC_SMP
	spin_lock(&mb_uart_spin_lock);
#endif // CONFIG_SOC_SMP

	return flags;
}

static inline void mb_uart_exit_critical(uint32_t flags)
{
#if CONFIG_SOC_SMP
	spin_unlock(&mb_uart_spin_lock);
#endif // CONFIG_SOC_SMP

	rtos_enable_int(flags);
}

static bk_err_t mb_uart_send_data(mb_uart_cb_t *chnl_cb);
static bk_err_t mb_uart_send_state(mb_uart_cb_t *chnl_cb);
static bk_err_t mb_uart_send_data_trigger(mb_uart_cb_t *chnl_cb);

static void rx_isr_data_handler(mb_uart_cb_t *chnl_cb, mb_uart_cmd_t * uart_cmd)
{
	u8      tx_fail = 0;

	u16    rd_idx, wr_idx, free_cnt;

	rd_idx = chnl_cb->rx_rd_idx;
	wr_idx = chnl_cb->rx_wr_idx;

	if(rd_idx > wr_idx)
	{
		free_cnt = rd_idx - wr_idx - 1;  // always reserved one byte space.
	}
	else
	{
		free_cnt = sizeof(chnl_cb->rx_buf) - (wr_idx - rd_idx) - 1;
	}

	/* chnl_cb->rx_xchg_buff == uart_cmd->cmd_buff. MUST be true. */

	if((uart_cmd->cmd_buff == NULL) || (uart_cmd->cmd_data_len == 0))
	{
		goto rx_isr_data_xit;
	}

	if(uart_cmd->cmd_data_len > free_cnt)
	{
		chnl_cb->status_overflow = 1;   /* discards all rx-data.  */
		tx_fail = 1;
		
		goto rx_isr_data_xit;
	}

	#if CONFIG_CACHE_ENABLE
	flush_dcache(uart_cmd->cmd_buff, uart_cmd->cmd_data_len);
	#endif

	u16    rem_len, cpy_len;

	cpy_len = uart_cmd->cmd_data_len;

	if(uart_cmd->crc8 != cal_crc8_0x31((u8 *)uart_cmd->cmd_buff, cpy_len))
	{
		chnl_cb->status_parity = 1;   /* discards all rx-data.  */
		tx_fail = 1;
		
		goto rx_isr_data_xit;
	}

	if(wr_idx < rd_idx)
	{
		memcpy(chnl_cb->rx_buf + wr_idx, uart_cmd->cmd_buff, cpy_len);
		chnl_cb->rx_wr_idx += cpy_len;
	}
	else
	{
		rem_len = sizeof(chnl_cb->rx_buf) - wr_idx;

		if(rd_idx == 0)
			rem_len -= 1;  // if rx_idx == 0, reserved one byte at tail of the array.

		if(rem_len > cpy_len)
		{
			memcpy(chnl_cb->rx_buf + wr_idx, uart_cmd->cmd_buff, cpy_len);
			chnl_cb->rx_wr_idx += cpy_len;
		}
		else
		{
			memcpy(chnl_cb->rx_buf + wr_idx, uart_cmd->cmd_buff, rem_len);

			if(rem_len < cpy_len)
			{
				memcpy(chnl_cb->rx_buf, uart_cmd->cmd_buff + rem_len, cpy_len - rem_len);
			}

			chnl_cb->rx_wr_idx = cpy_len - rem_len;
		}
	}

	free_cnt -= cpy_len;

	if(chnl_cb->rx_isr_callback != NULL)
	{
		chnl_cb->rx_isr_callback(chnl_cb->rx_isr_param);
	}

rx_isr_data_xit:
	
	if(free_cnt <= MB_UART_RX_FIFO_LEN)
	{
		chnl_cb->ctrl_rts = 1;
	}

	u8      old_cts = chnl_cb->status_cts;

	chnl_cb->status_cts = uart_cmd->uart_rts;

	/* overwrite the uart_cmd after the ISR handle completed.
	 * return the rsp info to caller using the SAME buffer with cmd buffer.
	 *     !!!! [input as param / output as result ]  !!!!
	 */
	mb_uart_rsp_t * uart_rsp = (mb_uart_rsp_t *)uart_cmd;

	uart_rsp->uart_rts = chnl_cb->ctrl_rts;
	uart_rsp->uart_tx_fail = tx_fail;
	uart_rsp->resvd = 0;

	if(chnl_cb->status_cts != old_cts)
	{
		if(chnl_cb->status_cts == 0)
		{
			/* trigger send! */
			mb_uart_send_data_trigger(chnl_cb);
		}
	}
	
	return;
}

static void rx_isr_state_handler(mb_uart_cb_t *chnl_cb, mb_uart_cmd_t *uart_cmd)
{
	u8      old_cts = chnl_cb->status_cts;

	chnl_cb->status_cts = uart_cmd->uart_rts;

	/* overwrite the uart_cmd after the ISR handle completed.
	 * return the rsp info to caller using the SAME buffer with cmd buffer.
	 *     !!!! [input as param / output as result ]  !!!!
	 */
	mb_uart_rsp_t * uart_rsp = (mb_uart_rsp_t *)uart_cmd;

	uart_rsp->uart_rts = chnl_cb->ctrl_rts;
	uart_rsp->uart_tx_fail = 0;
	uart_rsp->resvd = 0;

	if(chnl_cb->status_cts != old_cts)
	{
		if(chnl_cb->status_cts == 0)
		{
			/* trigger send! */
			mb_uart_send_data_trigger(chnl_cb);
		}
	}

	return;
}

static void mb_uart_rx_isr(mb_uart_cb_t *chnl_cb, mb_chnl_cmd_t *cmd_buf)
{
	/* clear here, because rts state will be passed in response data! */
	chnl_cb->tx_state_req = 0;
	
	if(cmd_buf->hdr.cmd == MB_UART_SEND_DATA)
	{
		rx_isr_data_handler(chnl_cb, (mb_uart_cmd_t *)cmd_buf);
	}
	else if(cmd_buf->hdr.cmd == MB_UART_SEND_STATE)
	{
		rx_isr_state_handler(chnl_cb, (mb_uart_cmd_t *)cmd_buf);
	}
	else
	{
		/* overwrite the cmd_buf after the ISR handle completed.
		 * return the ack info to caller using the SAME buffer with cmd buffer.
		 *     !!!! [input as param / output as result ]  !!!!
		 */
		mb_uart_rsp_t * uart_rsp = (mb_uart_rsp_t *)cmd_buf;

		uart_rsp->uart_rts = chnl_cb->ctrl_rts;
		uart_rsp->uart_tx_fail = 1;
		uart_rsp->resvd = 0;
	}
	
	return;
}

static void mb_uart_tx_cmpl_isr(mb_uart_cb_t *chnl_cb, mb_chnl_ack_t *ack_buf)  /* tx_cmpl_isr */
{
	mb_uart_rsp_t * uart_rsp = (mb_uart_rsp_t *)ack_buf;

	u8      tx_fail = 0;

	/* chnl_cb->rx_xchg_buff == uart_cmd->cmd_buff. MUST be true. */

	if( (ack_buf->hdr.cmd != chnl_cb->tx_cmd) || 
		(chnl_cb->tx_in_process == 0) )
	{
		/*
		 *   !!!  FAULT  !!!
		 */
		BK_LOGE(MOD_TAG, "Fault tx_cmd:%x, rsp_cmd:%x\r\n", chnl_cb->tx_cmd, ack_buf->hdr.cmd);

		return;
	}

	chnl_cb->status_cts = uart_rsp->uart_rts;

	if(ack_buf->hdr.state & CHNL_STATE_COM_FAIL)
	{
		tx_fail = 0x01;
	}
	else
	{
		tx_fail = uart_rsp->uart_tx_fail;
	}

	/* try next Tx. */

	bk_err_t	ret_val = BK_FAIL;

	ret_val = mb_uart_send_data(chnl_cb);

	if(ret_val == BK_OK)
	{
		chnl_cb->tx_state_req = 0;
	}
	else
	{
		if(chnl_cb->tx_state_req)
		{
			ret_val = mb_uart_send_state(chnl_cb);
			
			if(ret_val == BK_OK)
			{
				chnl_cb->tx_state_req = 0;
			}
		}
	}

	if(ret_val != BK_OK)
	{
		chnl_cb->tx_in_process = 0;  /* clear here, because no tx_cmpl_isr callback! */
	}

	if(tx_fail)
	{
		// fail handle.
		BK_LOGE(MOD_TAG, "Tx Failed!\r\n");
	}

	if(chnl_cb->tx_isr_callback != NULL)
	{
		chnl_cb->tx_isr_callback(chnl_cb->tx_isr_param);
	}

	return;
}

static bk_err_t mb_uart_init(mb_uart_cb_t *chnl_cb, u8 chnl_id)
{
	bk_err_t		ret_code;

	if(chnl_cb->chnl_inited)
		return BK_OK;

	memset(chnl_cb, 0, sizeof(mb_uart_cb_t));
	chnl_cb->chnl_id = chnl_id;

	chnl_cb->tx_xchg_buff = mb_chnl_get_tx_buff(chnl_id);
	chnl_cb->rx_xchg_buff = mb_chnl_get_rx_buff(chnl_id);

	if( (chnl_cb->tx_xchg_buff == NULL) || 
		(chnl_cb->tx_xchg_buff == NULL) )
	{
		return BK_FAIL;
	}

	ret_code = mb_chnl_open(chnl_id, chnl_cb);
	if(ret_code != BK_OK)
	{
		return ret_code;
	}

	mb_chnl_ctrl(chnl_id, MB_CHNL_SET_RX_ISR, (void *)mb_uart_rx_isr);
	mb_chnl_ctrl(chnl_id, MB_CHNL_SET_TX_CMPL_ISR, (void *)mb_uart_tx_cmpl_isr);
	mb_chnl_ctrl(chnl_id, MB_CHNL_SET_TX_ISR, NULL);

	chnl_cb->chnl_inited = 1;

	return BK_OK;
}

static bk_err_t mb_uart_deinit(mb_uart_cb_t *chnl_cb, u8 chnl_id)
{
	if(!chnl_cb->chnl_inited)
		return BK_OK;

	mb_chnl_ctrl(chnl_id, MB_CHNL_SET_RX_ISR, NULL);
	mb_chnl_ctrl(chnl_id, MB_CHNL_SET_TX_CMPL_ISR, NULL);
	mb_chnl_ctrl(chnl_id, MB_CHNL_SET_TX_ISR, NULL);

	mb_chnl_close(chnl_id);

	chnl_cb->rx_isr_callback = NULL;
	chnl_cb->tx_isr_callback = NULL;

	chnl_cb->chnl_inited = 0;

	return BK_OK;
}

static bk_err_t mb_uart_send_data(mb_uart_cb_t *chnl_cb)
{
	bk_err_t	ret_val = BK_FAIL;

	u16    rem_len, cpy_len;
	u16    rd_idx, wr_idx;

	if(chnl_cb->status_cts != 0)
	{
		return BK_ERR_BUSY;
	}

	rd_idx = chnl_cb->tx_rd_idx;
	wr_idx = chnl_cb->tx_wr_idx;

	if(rd_idx <= wr_idx)
	{
		cpy_len = wr_idx - rd_idx;
	}
	else
	{
		cpy_len = sizeof(chnl_cb->tx_buf) - (rd_idx - wr_idx);
	}

	if(cpy_len == 0)
	{
		return BK_FAIL;
	}

	if(cpy_len > MB_UART_TX_FIFO_LEN)
	{
		cpy_len = MB_UART_TX_FIFO_LEN;
	}

	if(rd_idx < wr_idx)
	{
		memcpy(chnl_cb->tx_xchg_buff, chnl_cb->tx_buf + rd_idx, cpy_len);
	}
	else
	{
		rem_len = sizeof(chnl_cb->tx_buf) - rd_idx;

		if(rem_len >= cpy_len)
		{
			memcpy(chnl_cb->tx_xchg_buff, chnl_cb->tx_buf + rd_idx, cpy_len);
		}
		else
		{
			memcpy(chnl_cb->tx_xchg_buff, chnl_cb->tx_buf + rd_idx, rem_len);

			memcpy(chnl_cb->tx_xchg_buff + rem_len, chnl_cb->tx_buf, cpy_len - rem_len);
		}
	}

	chnl_cb->tx_cmd = MB_UART_SEND_DATA;

	mb_uart_cmd_t	uart_cmd;

	uart_cmd.chnl_hdr.cmd  = MB_UART_SEND_DATA;
	uart_cmd.cmd_buff      = (void *)(chnl_cb->tx_xchg_buff);
	uart_cmd.cmd_data_len  = cpy_len;
	uart_cmd.uart_rts      = chnl_cb->ctrl_rts;
	uart_cmd.resvd         = 0;
	uart_cmd.crc8          = cal_crc8_0x31(chnl_cb->tx_xchg_buff, cpy_len);

	ret_val = mb_chnl_write(chnl_cb->chnl_id, (mb_chnl_cmd_t *)&uart_cmd);

	if(ret_val == BK_OK)
	{
		chnl_cb->tx_rd_idx = (rd_idx + cpy_len) % sizeof(chnl_cb->tx_buf);
	}

	return ret_val;
}

static bk_err_t mb_uart_send_data_trigger(mb_uart_cb_t *chnl_cb)
{
	u32 flag = mb_uart_enter_critical();

	if(chnl_cb->tx_in_process != 0)
	{
		mb_uart_exit_critical(flag);
		return BK_OK;
	}

	chnl_cb->tx_in_process = 1;

	mb_uart_exit_critical(flag);

	bk_err_t	ret_val = BK_FAIL;

	ret_val = mb_uart_send_data(chnl_cb);

	if(ret_val != BK_OK)
	{
		chnl_cb->tx_in_process = 0;  /* clear here, because no tx_cmpl_isr callback! */
	}

	return ret_val;
}

static bk_err_t mb_uart_send_state(mb_uart_cb_t *chnl_cb)
{
	mb_uart_cmd_t	uart_cmd;

	chnl_cb->tx_cmd = MB_UART_SEND_STATE;

	uart_cmd.chnl_hdr.cmd  = MB_UART_SEND_STATE;
	uart_cmd.cmd_buff      = NULL;
	uart_cmd.cmd_data_len  = 0;
	uart_cmd.uart_rts      = chnl_cb->ctrl_rts;
	uart_cmd.resvd         = 0;
	uart_cmd.crc8          = 0;

	bk_err_t	ret_val = BK_FAIL;

	ret_val = mb_chnl_write(chnl_cb->chnl_id, (mb_chnl_cmd_t *)&uart_cmd);

	return ret_val;
}

static bk_err_t mb_uart_send_state_trigger(mb_uart_cb_t *chnl_cb)
{
	u32 flag = mb_uart_enter_critical();

	if(chnl_cb->tx_in_process != 0)
	{
		chnl_cb->tx_state_req = 1;
		mb_uart_exit_critical(flag);
		return BK_OK;
	}

	chnl_cb->tx_in_process = 1;

	mb_uart_exit_critical(flag);

	bk_err_t	ret_val = BK_FAIL;

	ret_val = mb_uart_send_state(chnl_cb);

	if(ret_val != BK_OK)
	{
		chnl_cb->tx_in_process = 0;  /* clear here, because no tx_cmpl_isr callback! */
	}

	return ret_val;
}

static u16 mb_uart_write_data(mb_uart_cb_t *chnl_cb, u8 * data_buf, u16 data_len)
{
	u16    rem_len, wr_len;
	u16    rd_idx, wr_idx;
	
	rd_idx = chnl_cb->tx_rd_idx;
	wr_idx = chnl_cb->tx_wr_idx;

	if(rd_idx > wr_idx)
	{
		rem_len = rd_idx - wr_idx - 1;  // always reserved one byte space.
	}
	else
	{
		rem_len = sizeof(chnl_cb->tx_buf) - (wr_idx - rd_idx) - 1;
	}

	wr_len = data_len;
	if(wr_len > rem_len)
		wr_len = rem_len;

	if(wr_idx < rd_idx)
	{
		memcpy(chnl_cb->tx_buf + wr_idx, data_buf, wr_len);
	}
	else
	{
		if(rd_idx != 0)
			rem_len = sizeof(chnl_cb->tx_buf) - wr_idx;  // if rx_idx == 0, reserved one byte at tail of the array.

		if(rem_len >= wr_len)
		{
			memcpy(chnl_cb->tx_buf + wr_idx, data_buf, wr_len);
		}
		else
		{
			memcpy(chnl_cb->tx_buf + wr_idx, data_buf, rem_len);

			memcpy(chnl_cb->tx_buf, data_buf + rem_len, wr_len - rem_len);
		}
	}

	chnl_cb->tx_wr_idx = (wr_idx + wr_len) % sizeof(chnl_cb->tx_buf);

	/* trigger send! */
	mb_uart_send_data_trigger(chnl_cb);

	return wr_len;
}

static u16 mb_uart_read_data(mb_uart_cb_t *chnl_cb, u8 * data_buf, u16 buf_len)
{
	u16    rem_len, rd_len;

	if(!chnl_cb->chnl_inited)
		return 0;

	u16    rd_idx, wr_idx, data_cnt;

	rd_idx = chnl_cb->rx_rd_idx;
	wr_idx = chnl_cb->rx_wr_idx;

	if(rd_idx <= wr_idx)
	{
		data_cnt = wr_idx - rd_idx;
	}
	else
	{
		data_cnt = sizeof(chnl_cb->rx_buf) - (rd_idx - wr_idx);
	}

	rd_len = data_cnt;
	if(rd_len > buf_len)
		rd_len = buf_len;

	if(rd_len == 0)
	{
		return 0;
	}

	if(rd_idx < wr_idx)
	{
		memcpy(data_buf, chnl_cb->rx_buf + rd_idx, rd_len);
		chnl_cb->rx_rd_idx += rd_len;
	}
	else
	{
		rem_len = sizeof(chnl_cb->rx_buf) - rd_idx;

		if(rem_len > rd_len)
		{
			memcpy(data_buf, chnl_cb->rx_buf + rd_idx, rd_len);
			chnl_cb->rx_rd_idx += rd_len;
		}
		else
		{
			memcpy(data_buf, chnl_cb->rx_buf + rd_idx, rem_len);
			if(rem_len < rd_len)
			{
				memcpy(data_buf + rem_len, chnl_cb->rx_buf, rd_len - rem_len);
			}

			chnl_cb->rx_rd_idx = rd_len - rem_len;
		}
	}

	data_cnt -= rd_len;

	/* rts assert...... */
	if(data_cnt < ((sizeof(chnl_cb->rx_buf) - 1 - MB_UART_RX_FIFO_LEN) * 8 / 10))
	{
		if( chnl_cb->ctrl_rts != 0)
		{
			chnl_cb->ctrl_rts = 0;
			mb_uart_send_state_trigger(chnl_cb);
		}
	}

	return rd_len;

}

/************************************************************************************
 *
 *             mb_uart_driver API
 *
 ************************************************************************************/

static mb_uart_cb_t		mb_uart_cb[MB_UART_MAX] = {0};
static const u8  		mb_uart_chnl_id[MB_UART_MAX] = {
	MB_CHNL_UART0,
	MB_CHNL_UART1
};

bk_err_t bk_mb_uart_dev_init(u8 id)
{
	bk_err_t    ret_val;

	if(id >= MB_UART_MAX)
		return BK_ERR_PARAM;

	ret_val = mb_uart_init(&mb_uart_cb[id], mb_uart_chnl_id[id]);

	mb_uart_send_state_trigger(&mb_uart_cb[id]);

	return ret_val;
}

bk_err_t bk_mb_uart_dev_deinit(u8 id)
{
	bk_err_t    ret_val;

	if(id >= MB_UART_MAX)
		return BK_ERR_PARAM;

	ret_val = mb_uart_deinit(&mb_uart_cb[id], mb_uart_chnl_id[id]);

	return ret_val;
}

bk_err_t bk_mb_uart_register_rx_isr(u8 id, mb_uart_isr_t isr, void *param)
{
	if(id >= MB_UART_MAX)
		return BK_ERR_PARAM;

	if(!mb_uart_cb[id].chnl_inited)
		return BK_ERR_NOT_INIT;

	mb_uart_cb[id].rx_isr_param = param;    /* save rx_isr_param firstly! */
	mb_uart_cb[id].rx_isr_callback = isr;

	return BK_OK;
}

bk_err_t bk_mb_uart_register_tx_isr(u8 id, mb_uart_isr_t isr, void *param)
{
	if(id >= MB_UART_MAX)
		return BK_ERR_PARAM;

	if(!mb_uart_cb[id].chnl_inited)
		return BK_ERR_NOT_INIT;

	mb_uart_cb[id].tx_isr_param = param;    /* save tx_isr_param firstly! */
	mb_uart_cb[id].tx_isr_callback = isr;

	return BK_OK;
}

u16 bk_mb_uart_write_ready(u8 id)
{
	if(id >= MB_UART_MAX)
		return 0;
	
	if(!mb_uart_cb[id].chnl_inited)
		return 0;

	u16    rem_len;
	u16    rd_idx, wr_idx;
	
	rd_idx = mb_uart_cb[id].tx_rd_idx;
	wr_idx = mb_uart_cb[id].tx_wr_idx;

	if(rd_idx > wr_idx)
	{
		rem_len = rd_idx - wr_idx - 1;  // always reserved one byte space.
	}
	else
	{
		rem_len = sizeof(mb_uart_cb[id].tx_buf) - (wr_idx - rd_idx) - 1;
	}

	return rem_len;
}

u16 bk_mb_uart_read_ready(u8 id)
{
	if(id >= MB_UART_MAX)
		return 0;
	
	if(!mb_uart_cb[id].chnl_inited)
		return 0;

	u16    rd_idx, wr_idx, data_cnt;

	rd_idx = mb_uart_cb[id].rx_rd_idx;
	wr_idx = mb_uart_cb[id].rx_wr_idx;

	if(rd_idx <= wr_idx)
	{
		data_cnt = wr_idx - rd_idx;
	}
	else
	{
		data_cnt = sizeof(mb_uart_cb[id].rx_buf) - (rd_idx - wr_idx);
	}

	return data_cnt;
}

u16 bk_mb_uart_write_byte(u8 id, u8 data)
{
	u16     len;
	
	if(id >= MB_UART_MAX)
		return 0;

	if(!mb_uart_cb[id].chnl_inited)
		return 0;

	len = mb_uart_write_data(&mb_uart_cb[id], &data, 1);

	return len;
}

u16 bk_mb_uart_write(u8 id, u8 *data_buf, u16 data_len)
{
	u16     len;

	if(id >= MB_UART_MAX)
		return 0;

	if(!mb_uart_cb[id].chnl_inited)
		return 0;

	if((data_buf == NULL) || (data_len == 0))
		return 0;

	len = mb_uart_write_data(&mb_uart_cb[id], data_buf, data_len);

	return len;
}

u16 bk_mb_uart_read_byte(u8 id, u8 * data)
{
	u16     len;
	
	if(id >= MB_UART_MAX)
		return 0;

	if(!mb_uart_cb[id].chnl_inited)
		return 0;

	if(data == NULL)
		return 0;

	len = mb_uart_read_data(&mb_uart_cb[id], data, 1);

	return len;
}

u16 bk_mb_uart_read(u8 id, u8 *data_buf, u16 buf_len)
{
	u16     len;

	if(id >= MB_UART_MAX)
		return 0;

	if(!mb_uart_cb[id].chnl_inited)
		return 0;

	if((data_buf == NULL) || (buf_len == 0))
		return 0;

	len = mb_uart_read_data(&mb_uart_cb[id], data_buf, buf_len);

	return len;
}

bool bk_mb_uart_is_tx_over(u8 id)
{
	if(id >= MB_UART_MAX)
		return 1;

	if(!mb_uart_cb[id].chnl_inited)
		return 1;

	if( (mb_uart_cb[id].tx_rd_idx == mb_uart_cb[id].tx_wr_idx) && 
		(mb_uart_cb[id].tx_in_process == 0) )
	{
		return 1;
	}

	return 0;
}

u16 bk_mb_uart_get_status(u8 id)
{
	u16    status = 0;
	
	if(id >= MB_UART_MAX)
		return 0;

	if(!mb_uart_cb[id].chnl_inited)
		return 0;

	if(mb_uart_cb[id].status_overflow)
	{
		status |= MB_UART_STATUS_OVF;
		mb_uart_cb[id].status_overflow = 0;
	}
	
	if(mb_uart_cb[id].status_parity)
	{
		status |= MB_UART_STATUS_PARITY_ERR;
		mb_uart_cb[id].status_parity = 0;
	}

	return status;
}

#endif


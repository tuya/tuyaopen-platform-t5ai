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

#include <common/bk_include.h>
#include <os/os.h>
#include <driver/flash.h>
#include "flash_driver.h"
// #include "mb_ipc_cmd.h"

static void (*s_flash_op_notify)(uint32_t param) = NULL;

bk_err_t mb_flash_register_op_notify(void * notify_cb)
{
    s_flash_op_notify = (void (*)(uint32_t))notify_cb;

	return BK_OK;
}

bk_err_t mb_flash_unregister_op_notify(void * notify_cb)
{
	if(s_flash_op_notify == notify_cb)
	{
		s_flash_op_notify = NULL;
		return BK_OK;
	}

	return BK_ERR_FLASH_WAIT_CB_NOT_REGISTER;
}

#if CONFIG_FLASH_MB

#include <driver/mailbox_channel.h>
#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif

enum
{
	IPC_FLASH_OP_COMPLETE = 0,
	IPC_FLASH_OP_REQ,
	IPC_FLASH_OP_ACK,
};

enum
{
	IPC_FLASH_OP_START = 0,
	IPC_FLASH_OP_END,
};


#include <driver/aon_rtc.h>

#define FLASH_WAIT_ACK_TIMEOUT 5000

static volatile uint32_t flash_erase_ipc_state = IPC_FLASH_OP_COMPLETE;

extern int mb_ipc_cpu_is_power_off(u32 cpu_id);

#if 1  // new design of flash operation notification.

static bk_err_t send_flash_op_state(uint8_t state)
{
	uint64_t us_start = 0;
	uint64_t us_end = 0;

	flash_erase_ipc_state = IPC_FLASH_OP_REQ;

	mb_chnl_cmd_t  cmd_buf;

	cmd_buf.hdr.data = 0; /* clear hdr. */
	cmd_buf.hdr.cmd  = state;
	cmd_buf.param1 = flash_erase_ipc_state;

	bk_err_t   ret_val = mb_chnl_write(MB_CHNL_FLASH, &cmd_buf);

	if(ret_val != BK_OK)
		return ret_val;

	us_start = bk_aon_rtc_get_us();

	for(int i = 0; i < 2000; i++)
	{
		if(flash_erase_ipc_state == IPC_FLASH_OP_ACK)
		{
			break;
		}

		us_end = bk_aon_rtc_get_us();
		// wait ack time should not be more than 5 ms
		if((us_end - us_start) > FLASH_WAIT_ACK_TIMEOUT)
		{
			return BK_FAIL;
		}
	}

	return BK_OK;
}

bk_err_t mb_flash_op_prepare(void)			// CPU0 notify CPU1 before flash operation
{
	bk_err_t   ret_val = BK_OK;
	
	#if (CONFIG_CPU_CNT > 1)
	if( mb_ipc_cpu_is_power_off(1) )  // cpu1 power off!
		return BK_OK;
	
	ret_val = send_flash_op_state(IPC_FLASH_OP_START);
	#endif

	return ret_val;
}

bk_err_t mb_flash_op_finish(void)			// CPU0 notify CPU1 after flash operation
{
	bk_err_t   ret_val = BK_OK;
	
	#if (CONFIG_CPU_CNT > 1)
	if( mb_ipc_cpu_is_power_off(1) )  // cpu1 power off!
		return BK_OK;
	
	ret_val = send_flash_op_state(IPC_FLASH_OP_END);
	#endif

	return ret_val;
}

#endif


static void mb_flash_ipc_tx_cmpl_isr(void *chn_param, mb_chnl_ack_t *ack_buf)
{
	if((ack_buf->hdr.state & CHNL_STATE_COM_FAIL) == 0)
	{
		flash_erase_ipc_state = ack_buf->ack_data1;
	}
	
	return;
}


static void mb_flash_ipc_rx_isr(void *chn_param, mb_chnl_cmd_t *cmd_buf)
{

	return;
}

bk_err_t mb_flash_ipc_init(void)
{
	bk_err_t ret_code = mb_chnl_open(MB_CHNL_FLASH, NULL);

	if(ret_code != BK_OK)
	{
		return ret_code;
	}

	// call chnl driver to register isr callback;
	mb_chnl_ctrl(MB_CHNL_FLASH, MB_CHNL_SET_TX_CMPL_ISR, (void *)mb_flash_ipc_tx_cmpl_isr);

	mb_chnl_ctrl(MB_CHNL_FLASH, MB_CHNL_SET_RX_ISR, (void *)mb_flash_ipc_rx_isr);

	return ret_code;
}

#else

bk_err_t mb_flash_ipc_init(void)
{
	return BK_OK;
}

bk_err_t mb_flash_op_prepare(void)
{
	// disable the LCD dev interrupt.
	if(s_flash_op_notify != NULL)
		s_flash_op_notify(0);

	return BK_OK;
}

bk_err_t mb_flash_op_finish(void)
{
	// enable the LCD dev interrupt.
	if(s_flash_op_notify != NULL)
		s_flash_op_notify(1);
	
	return BK_OK;
}

#endif

static volatile flash_op_status_t s_flash_op_status = 0;

__attribute__((section(".itcm_sec_code"))) bk_err_t bk_flash_set_operate_status(flash_op_status_t status)
{
	s_flash_op_status = status;
	return BK_OK;
}

__attribute__((section(".itcm_sec_code"))) flash_op_status_t bk_flash_get_operate_status(void)
{
	return s_flash_op_status;
}


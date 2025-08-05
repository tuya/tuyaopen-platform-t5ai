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
//#include "flash_driver.h"
// #include "mb_ipc_cmd.h"

static void (*s_flash_op_notify)(uint32_t param) = NULL;
static void (*s_flash_op_notify_dvp)(uint32_t param) = NULL;

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
bk_err_t mb_flash_register_op_dvp_notify(void * notify_cb)
{
	s_flash_op_notify_dvp = (void (*)(uint32_t))notify_cb;

	return BK_OK;
}

bk_err_t mb_flash_unregister_op_dvp_notify(void * notify_cb)
{
	if(s_flash_op_notify_dvp == notify_cb)
	{
		s_flash_op_notify_dvp = NULL;
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



#if 1

static void cpu1_pause_handle(mb_chnl_cmd_t *cmd_buf)
{
	if(cmd_buf->param1 != IPC_FLASH_OP_REQ)
	{
		return;
	}

	if(cmd_buf->hdr.cmd == IPC_FLASH_OP_START)
	{
		// disable the LCD dev interrupt.
		if(s_flash_op_notify != NULL)
			s_flash_op_notify(0);
        if(s_flash_op_notify_dvp != NULL)
        {
			s_flash_op_notify_dvp(0);
        }
	}
	else if(cmd_buf->hdr.cmd == IPC_FLASH_OP_END)
	{
		// enable the LCD dev interrupt.
		if(s_flash_op_notify != NULL)
			s_flash_op_notify(1);
	}

	cmd_buf->param1 = IPC_FLASH_OP_ACK;

	return;

}

#else

__attribute__((section(".iram"))) static bk_err_t cpu1_pause_handle(mb_chnl_cmd_t *cmd_buf)
{
	volatile uint32_t * stat_addr = (volatile uint32_t *)cmd_buf->param1;

#if CONFIG_CACHE_ENABLE
	flush_dcache((void *)stat_addr, 4);
#endif

	// only puase cpu1 when flash erasing
	if(*(stat_addr) == IPC_FLASH_OP_REQ)
	{
		uint32_t flags = rtos_disable_int();

		// disable the LCD dev interrupt.
		if(s_flash_op_notify != NULL)
			s_flash_op_notify(0);

		rtos_enable_int(flags);

		bk_flash_set_operate_status(FLASH_OP_BUSY);
		*(stat_addr) = IPC_FLASH_OP_ACK;
		while(*(stat_addr) != IPC_FLASH_OP_COMPLETE)
		{
#if CONFIG_CACHE_ENABLE
			flush_dcache((void *)stat_addr, 4);
#endif
		}
		bk_flash_set_operate_status(FLASH_OP_IDLE);

		// enable the LCD dev interrupt.
		if(s_flash_op_notify != NULL)
			s_flash_op_notify(1);
	}

	return BK_OK;
}
#endif


static void mb_flash_ipc_rx_isr(void *chn_param, mb_chnl_cmd_t *cmd_buf)
{
	cpu1_pause_handle(cmd_buf);
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
	if(s_flash_op_notify_dvp != NULL)
		s_flash_op_notify_dvp(0);

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


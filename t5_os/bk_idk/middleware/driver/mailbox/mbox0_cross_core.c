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

// Mailbox for CrossCore
#include "mbox0_drv.h"
#include "sys_driver.h"
#include "mbox0_fifo_cfg.h"
#include <components/log.h>
#include "driver/mailbox_types.h"
#include "sdkconfig.h"
#include "cpu_id.h"

#define MAILB_TAG "mailbox"
#define MAILB_LOGI(...) BK_LOGI(MAILB_TAG, ##__VA_ARGS__)
#define MAILB_LOGW(...) BK_LOGW(MAILB_TAG, ##__VA_ARGS__)
#define MAILB_LOGE(...) BK_LOGE(MAILB_TAG, ##__VA_ARGS__)
#define MAILB_LOGD(...) BK_LOGD(MAILB_TAG, ##__VA_ARGS__)

#define CHANNEL_COUNT    (CONFIG_CPU_CNT)

typedef struct
{
	mbox0_hal_t hal;
	const hal_chn_drv_t *chn_drv[CHANNEL_COUNT];

	mbox0_rx_callback_t rx_callback;
} mailbox_dev_t;

static mailbox_dev_t mailbox_dev = {
		.rx_callback = NULL
	};
static u8 mailbox_init_flag = 0;
static mbox0_fifo_cfg_t chn_fifo_settings[] = MBOX0_FIFO_CFG_TABLE;

extern void crosscore_mb_rx_isr(mailbox_data_t *data);

static void mailbox_cc_cfg_fifo(int core_id)
{
	mailbox_dev.chn_drv[core_id]->chn_cfg_fifo(&mailbox_dev.hal, chn_fifo_settings[core_id].start & 0xFF, chn_fifo_settings[core_id].len & 0xFF);
}

static void mailbox_cc_cfg_exceptional_intr(int core_id)
{
	mbox0_hal_t *hal = &mailbox_dev.hal;

	switch(core_id){
		case CPU0_CORE_ID:
			mbox0_hal_set_chn0_int_wrerr_en(hal, 1);
			mbox0_hal_set_chn0_int_rderr_en(hal, 1);
			mbox0_hal_set_chn0_int_wrfull_en(hal, 1);
			break;
		case CPU1_CORE_ID:
			mbox0_hal_set_chn1_int_wrerr_en(hal, 1);
			mbox0_hal_set_chn1_int_rderr_en(hal, 1);
			mbox0_hal_set_chn1_int_wrfull_en(hal, 1);
			break;
		case CPU2_CORE_ID:
			mbox0_hal_set_chn2_int_wrerr_en(hal, 1);
			mbox0_hal_set_chn2_int_rderr_en(hal, 1);
			mbox0_hal_set_chn2_int_wrfull_en(hal, 1);
			break;
		default:
			break;
	}
}

static void mailbox_cc_chn0_exception_handler(void)
{
	uint32_t val;
	int exception_flag = 0;
	mbox0_hal_t *hal = &mailbox_dev.hal;

	if(mbox0_hal_get_chn0_wrerr_int_sta(hal)){
		MAILB_LOGE("[exception:%d]chn0_wr_err\r\n", rtos_get_core_id());
		exception_flag ++;
	}

	if(mbox0_hal_get_chn0_rderr_int_sta(hal)){
		MAILB_LOGE("[exception:%d]chn0_rd_err\r\n", rtos_get_core_id());
		exception_flag ++;
	}

	if(mbox0_hal_get_chn0_wrfull_int_sta(hal)){
		MAILB_LOGE("[exception:%d]chn0_wr_full\r\n", rtos_get_core_id());
		exception_flag ++;
	}

	if(exception_flag){
		val = mbox0_hal_get_reg_0x10_value(hal);
		mbox0_hal_set_reg_0x10_value(hal, val);
	}
}

static void mailbox_cc_chn1_exception_handler(void)
{
	uint32_t val;
	int exception_flag = 0;
	mbox0_hal_t *hal = &mailbox_dev.hal;

	if(mbox0_hal_get_chn1_wrerr_int_sta(hal)){
		MAILB_LOGE("[exception:%d]chn1_wr_err\r\n", rtos_get_core_id());
		exception_flag ++;
	}

	if(mbox0_hal_get_chn1_rderr_int_sta(hal)){
		MAILB_LOGE("[exception:%d]chn1_rd_err\r\n", rtos_get_core_id());
		exception_flag ++;
	}

	if(mbox0_hal_get_chn1_wrfull_int_sta(hal)){
		MAILB_LOGE("[exception:%d]chn1_wr_full\r\n", rtos_get_core_id());
		exception_flag ++;
	}

	if(exception_flag){
		val = mbox0_hal_get_reg_0x20_value(hal);
		mbox0_hal_set_reg_0x20_value(hal, val);
	}
}

static void mailbox_cc_chn2_exception_handler(void)
{
	uint32_t val;
	int exception_flag = 0;
	mbox0_hal_t *hal = &mailbox_dev.hal;

	if(mbox0_hal_get_chn2_wrerr_int_sta(hal)){
		MAILB_LOGE("[exception:%d]chn2_wr_err\r\n", rtos_get_core_id());
		exception_flag ++;
	}

	if(mbox0_hal_get_chn2_rderr_int_sta(hal)){
		MAILB_LOGE("[exception:%d]chn2_rd_err\r\n", rtos_get_core_id());
		exception_flag ++;
	}

	if(mbox0_hal_get_chn2_wrfull_int_sta(hal)){
		MAILB_LOGE("[exception:%d]chn2_wr_full\r\n", rtos_get_core_id());
		exception_flag ++;
	}

	if(exception_flag){
		val = mbox0_hal_get_reg_0x30_value(hal);
		mbox0_hal_set_reg_0x30_value(hal, val);
	}
}

static void mailbox_cc_exception_intr_handler(int core_id)
{
	switch(core_id){
		case CPU0_CORE_ID:
			mailbox_cc_chn0_exception_handler();
			break;
		case CPU1_CORE_ID:
			mailbox_cc_chn1_exception_handler();
			break;
		case CPU2_CORE_ID:
			mailbox_cc_chn2_exception_handler();
			break;
		default:
			break;
	}
}

static int mailbox_cc_recieve_message(const hal_chn_drv_t *channel_drv_ptr, mbox0_message_t* message)
{
	mailbox_data_t data;

	crosscore_mb_rx_isr(&data);
	return channel_drv_ptr->chn_recv(&mailbox_dev.hal, message);
}

static void mailbox_cc_isr_handler(void)
{
	int id;
	uint32_t int_status;
	uint32_t fifo_status;
	mbox0_message_t message;
	const hal_chn_drv_t *channel_drv_ptr;

	id = rtos_get_core_id();
	channel_drv_ptr = mailbox_dev.chn_drv[id];

	int_status = channel_drv_ptr->chn_get_int_status(&mailbox_dev.hal);
	fifo_status = channel_drv_ptr->chn_get_rx_fifo_stat(&mailbox_dev.hal);

	if((int_status != 0) && (fifo_status & RX_FIFO_STAT_NOT_EMPTY))
	{
		mailbox_cc_recieve_message(channel_drv_ptr, &message);

		#if CONFIG_ENABLE_RX_CB
		if(mailbox_dev.rx_callback != NULL)
			mailbox_dev.rx_callback(&message);
		#endif
	}
	mailbox_cc_exception_intr_handler(id);
}

int mailbox_cc_get_send_stat(void)
{
	int id, ret = 0;
	uint32_t fifo_status;
	const hal_chn_drv_t *channel_drv_ptr;

	id = rtos_get_core_id();
	channel_drv_ptr = mailbox_dev.chn_drv[id];
	
	fifo_status = channel_drv_ptr->chn_clear_tx_fifo_stat(&mailbox_dev.hal);
	if(fifo_status & (TX_FIFO_STAT_WR_FULL | TX_FIFO_STAT_WR_ERR))
		ret = -1;

	return ret;
}

int mailbox_cc_send_message(const hal_chn_drv_t *channel_drv, mbox0_message_t *message)
{
	return channel_drv->chn_send(&mailbox_dev.hal, message);
}

int mailbox_cc_callback_register(mbox0_rx_callback_t callback)
{
	mailbox_dev.rx_callback = callback;

	return 0;
}

bk_err_t bk_mailbox_master_send(mailbox_data_t *data, uint8_t src, uint8_t dst)
{
	int id;
	mbox0_message_t msg = {0};
	const hal_chn_drv_t *channel_drv_ptr;

	id = rtos_get_core_id();
	channel_drv_ptr = mailbox_dev.chn_drv[id];

	msg.src_cpu = src;
	msg.dest_cpu = dst;
	return mailbox_cc_send_message(channel_drv_ptr, &msg);
}

bk_err_t bk_mailbox_slave_send(mailbox_data_t *data, uint8_t src, uint8_t dst)
{
	int id;
	mbox0_message_t msg = {0};
	const hal_chn_drv_t *channel_drv_ptr;

	id = rtos_get_core_id();
	channel_drv_ptr = mailbox_dev.chn_drv[id];

	msg.src_cpu = src;
	msg.dest_cpu = dst;
	return mailbox_cc_send_message(channel_drv_ptr, &msg);
}

void bk_core_mbox_enable_int(int core_id, uint8_t mbox)
{
	/*FIXME*/
}

bk_err_t bk_mailbox_cc_init_on_current_core(int id)
{
	int int_src = INT_SRC_MAILBOX;

	mbox0_hal_init(&mailbox_dev.hal);

	if(CPU0_CORE_ID == id){
		/* can only be initialized by CPU0. */
		mbox0_hal_dev_init(&mailbox_dev.hal);
	}

	/* cpu0 cannot initialize mailbox channel 1/2*/
	mailbox_cc_cfg_fifo(id);
	mailbox_cc_cfg_exceptional_intr(id);
	mailbox_dev.chn_drv[id]->chn_int_enable(&mailbox_dev.hal, 1);
	
	/*enable system interrupt, every channel is hard-bound to a processor*/
	sys_drv_core_intr_group2_enable(id, (1 << (int_src - 32)));

	return 0;
}

bk_err_t bk_mailbox_cc_init(void)
{
	mbox0_hal_t *hal;
	int int_src = INT_SRC_MAILBOX;

	if(mailbox_init_flag == 1)
		return 0;

	mailbox_dev.chn_drv[0] = &hal_chn0_drv;

	#if (CONFIG_CPU_CNT > 1)
	mailbox_dev.chn_drv[1] = &hal_chn1_drv;
	#endif

	#if (CONFIG_CPU_CNT > 2)
	mailbox_dev.chn_drv[2] = &hal_chn2_drv;
	#endif

	BK_ASSERT(CPU0_CORE_ID == rtos_get_core_id());
	bk_int_isr_register(int_src, mailbox_cc_isr_handler, NULL);

#if CONFIG_FORCE_PROTECT_CHANNEL
	bk_mailbox_cc_init_on_current_core(CPU0_CORE_ID);
#else
	hal = &mailbox_dev.hal;
	bk_mailbox_cc_init_on_current_core(CPU0_CORE_ID);

	mbox0_hal_set_reg_0x2_chn_pro_disable(hal, 1); /* channel unprotect*/
#if (CONFIG_CPU_CNT > 1)
	bk_mailbox_cc_init_on_current_core(CPU1_CORE_ID);
#endif

#if (CONFIG_CPU_CNT > 2)
	bk_mailbox_cc_init_on_current_core(CPU2_CORE_ID);
#endif
	mbox0_hal_set_reg_0x2_chn_pro_disable(hal, 0);/* channel protect*/
#endif

	mailbox_init_flag = 1;

	return 0;
}

bk_err_t bk_mailbox_cc_deinit(void)
{
	uint8_t int_src = INT_SRC_MAILBOX;

	if(mailbox_init_flag == 0)
		return -1;

	sys_drv_int_group2_disable(1 << (int_src - 32));
	
	mailbox_dev.chn_drv[0]->chn_int_enable(&mailbox_dev.hal, 0);
	mailbox_dev.chn_drv[1]->chn_int_enable(&mailbox_dev.hal, 0);
	mailbox_dev.chn_drv[2]->chn_int_enable(&mailbox_dev.hal, 0);

	bk_int_isr_unregister(int_src);

	mbox0_hal_dev_deinit(&mailbox_dev.hal);  /* can only be de-initialized by CPU0. */
	mbox0_hal_deinit(&mailbox_dev.hal);
	
	mailbox_dev.rx_callback = NULL;
	mailbox_init_flag = 0;

	return 0;
}

// eof


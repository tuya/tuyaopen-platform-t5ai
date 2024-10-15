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

#include <os/mem.h>
#include <os/os.h>
#include <os/str.h>
#include <modules/pm.h>
#include <stdio.h>
#include <stdlib.h>
#include "aud_intf.h"
#include "aud_intf_types.h"
#include <driver/audio_ring_buff.h>
#include <modules/pm.h>
#include <asr_mb.h>
#include <driver/mailbox_channel.h>

#include "BK7256_RegList.h"
#include "ff.h"
#include "diskio.h"


#define GPIO_DEBUG 0
#define FATFS_DEBUG 0

#define ASR_BUFF_SIZE 8000  //>960*2

#if FATFS_DEBUG
#define FATFS_WRITER  "1:/wanson_asr_save.pcm"

static beken_thread_t  asr_demo_thread_hdl = NULL;
static beken_queue_t asr_demo_msg_que = NULL;
#endif

static aud_intf_drv_setup_t aud_intf_drv_setup = DEFAULT_AUD_INTF_DRV_SETUP_CONFIG();
static aud_intf_mic_setup_t aud_intf_mic_setup = DEFAULT_AUD_INTF_MIC_SETUP_CONFIG();
static aud_intf_work_mode_t aud_work_mode = AUD_INTF_WORK_MODE_NULL;

static uint8_t *asr_ring_buff = NULL;
static RingBufferContext asr_rb;
static int8_t *asr_buff = NULL;
#if FATFS_DEBUG
static FATFS *pfs = NULL;
FIL file_out;


typedef enum {
	ASR_DEMO_INIT = 0,
	ASR_DEMO_PROCESS,
	ASR_DEMO_EXIT,
} asr_demo_op_t;

typedef struct {
	asr_demo_op_t op;
} asr_demo_msg_t;

bk_err_t aud_intf_asr_demo_stop(void);

static bk_err_t aud_asr_demo_send_msg(asr_demo_op_t op)
{
	bk_err_t ret;

	asr_demo_msg_t msg;
	msg.op = op;

	if (asr_demo_msg_que) {
		ret = rtos_push_to_queue(&asr_demo_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			os_printf("asr_demo_send_int_msg fail \r\n");
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}


static bk_err_t tf_mount(void)
{
	FRESULT fr;

	if (pfs != NULL)
	{
		os_free(pfs);
	}

	pfs = os_malloc(sizeof(FATFS));
	if(NULL == pfs)
	{
		os_printf("f_mount malloc failed!\r\n");
		return BK_FAIL;
	}

	fr = f_mount(pfs, "1:", 1);
	if (fr != FR_OK)
	{
		os_printf("f_mount failed:%d\r\n", fr);
		return BK_FAIL;
	}
	else
	{
		os_printf("f_mount OK!\r\n");
	}

	return BK_OK;
}

static bk_err_t tf_unmount(void)
{
    FRESULT fr;
    fr = f_unmount(DISK_NUMBER_SDIO_SD, "1:", 1);
    if (fr != FR_OK)
    {
        os_printf("f_unmount failed:%d\r\n", fr);
		return BK_FAIL;
    }
    else
    {
        os_printf("f_unmount OK!\r\n");
    }

    return BK_OK;
}

static bk_err_t tfcard_record_open(void)
{
	FRESULT fr;

	tf_mount();

	fr = f_open(&file_out, FATFS_WRITER, FA_CREATE_ALWAYS | FA_WRITE);
	if (fr != FR_OK) {
		os_printf("open %s fail.\r\n", FATFS_WRITER);
		return BK_FAIL;
	}

	return BK_OK;
}

static bk_err_t tfcard_record_close(void)
{
	FRESULT fr;

	fr = f_close(&file_out);
	if (fr != FR_OK) {
		os_printf("close mic file %s fail!\r\n", FATFS_WRITER);
		return BK_FAIL;
	}

	tf_unmount();

	return BK_OK;
}
#endif

static bk_err_t asr_send_mb(asr_mb_event_t cmd, uint32 arg)
{
    bk_err_t ret = BK_FAIL;
    mb_chnl_cmd_t mb_cmd;
    mb_cmd.hdr.cmd = cmd;
    mb_cmd.param1 = arg;
    mb_cmd.param2 = 0;
    mb_cmd.param3 = 0;
    ret = mb_chnl_write(MB_CHNL_AUD, &mb_cmd);
    return ret;
}

/* mic file format: signal channel, 16K sample rate, 16bit width */
static int aud_asr_handle(uint8_t *data, unsigned int len)
{
#if FATFS_DEBUG
	FRESULT fr;
	static uint8_t i = 1;
#endif
uint32 uiTemp = 0;
#if GPIO_DEBUG
	addAON_GPIO_Reg0x2 = 2;
	addAON_GPIO_Reg0x2 = 0;
#endif
	//os_printf("%s, write data fail, len: %d \n", __func__, len);

	/* write data to file */
	if (ring_buffer_get_free_size(&asr_rb) >= len) {
		uiTemp = ring_buffer_write(&asr_rb, data, len);
		if (uiTemp != len) {
			os_printf("%s, write data fail, uiTemp: %d \n", __func__, uiTemp);
		}
	}
	if (ring_buffer_get_fill_size(&asr_rb) >= 960) {
		uiTemp = ring_buffer_read(&asr_rb, (uint8_t *)asr_buff, 960);
		//os_memset(asr_buff, i, 960);
		extern void flush_dcache(void *va, long size);
		flush_dcache((void *)asr_buff, 960);
		//bk_mem_dump("asr_buff", (uint32_t)asr_buff, 10);
		asr_send_mb(EVENT_ASR_PROCESS, (uint32_t)asr_buff);
		//os_printf("asr_buff: %p \n", asr_buff);
#if GPIO_DEBUG
		addAON_GPIO_Reg0x3 = 2;
		addAON_GPIO_Reg0x3 = 0;
#endif
#if FATFS_DEBUG
		fr = f_write(&file_out, (void *)asr_buff, 960, &uiTemp);
		if (fr != FR_OK || uiTemp != 960) {
			os_printf("write output data %s fail.\r\n", FATFS_WRITER);
		}
		i++;
		if (i == 0xff)
			aud_asr_demo_send_msg(ASR_DEMO_EXIT);
#endif
	}

	return len;
}


bk_err_t asr_mailbox_chl_open(void)
{
	return mb_chnl_open(MB_CHNL_AUD, NULL);
}

#if FATFS_DEBUG
static void asr_demo_main(void)
{
	bk_err_t ret = BK_OK;
	asr_demo_msg_t msg;
	while(1) {
		ret = rtos_pop_from_queue(&asr_demo_msg_que, &msg, BEKEN_WAIT_FOREVER);
		//rtos_delay_milliseconds(1000);
		if (kNoErr == ret) {
			if (msg.op == ASR_DEMO_EXIT) {

				tfcard_record_close();

				aud_intf_asr_demo_stop();
			}
		}
	}
}
#endif

bk_err_t aud_intf_asr_demo_start(void)
{
	bk_err_t ret = BK_OK;

	os_printf("init wanson asr test \r\n");

	/* set cpu frequency to 320MHz */
	bk_pm_module_vote_cpu_freq(PM_DEV_ID_PSRAM, PM_CPU_FRQ_320M);

	asr_mailbox_chl_open();
#if FATFS_DEBUG
	tfcard_record_open();
#endif

	asr_ring_buff = os_malloc(ASR_BUFF_SIZE);
	if (asr_ring_buff ==  NULL) {
		os_printf("os_malloc asr_ring_buff fail \n");
		return BK_FAIL;
	}

	ring_buffer_init(&asr_rb, asr_ring_buff, ASR_BUFF_SIZE, DMA_ID_MAX, RB_DMA_TYPE_NULL);

	asr_buff = os_malloc(960);
	if (asr_buff ==  NULL) {
		os_printf("os_malloc asr_buff fail \n");
		return BK_FAIL;
	}

//	addAON_GPIO_Reg0x2 = 0;
//	addAON_GPIO_Reg0x3 = 0;
//	addAON_GPIO_Reg0x4 = 0;

	aud_intf_drv_setup.aud_intf_tx_mic_data = aud_asr_handle;
	ret = bk_aud_intf_drv_init(&aud_intf_drv_setup);
	if (ret != BK_ERR_AUD_INTF_OK) {
		os_printf("bk_aud_intf_drv_init fail, ret:%d \r\n", ret);
	} else {
		os_printf("bk_aud_intf_drv_init complete \r\n");
	}

	aud_work_mode = AUD_INTF_WORK_MODE_GENERAL;
	ret = bk_aud_intf_set_mode(aud_work_mode);
	if (ret != BK_ERR_AUD_INTF_OK) {
		os_printf("bk_aud_intf_set_mode fail, ret:%d \r\n", ret);
	} else {
		os_printf("bk_aud_intf_set_mode complete \r\n");
	}

	aud_intf_mic_setup.samp_rate = 16000;
	ret = bk_aud_intf_mic_init(&aud_intf_mic_setup);
	if (ret != BK_ERR_AUD_INTF_OK) {
		os_printf("bk_aud_intf_mic_init fail, ret:%d \r\n", ret);
	} else {
		os_printf("bk_aud_intf_mic_init complete \r\n");
	}
	os_printf("init mic complete \r\n");

//	rtos_delay_milliseconds(3000);

	ret = bk_aud_intf_mic_start();
	if (ret != BK_ERR_AUD_INTF_OK) {
		os_printf("bk_aud_intf_mic_start fail, ret:%d \r\n", ret);
	} else {
		os_printf("bk_aud_intf_mic_start complete \r\n");
	}
	os_printf("start asr test \r\n");

#if FATFS_DEBUG
	ret = rtos_init_queue(&asr_demo_msg_que,
						  "asr_demo_que",
						  sizeof(asr_demo_msg_t),
						  1);
	if (ret != kNoErr) {
		os_printf("ceate asr message queue fail \r\n");
		return BK_FAIL;
	}
	os_printf("ceate asr message queue complete \r\n");

	/* create task to asr */
	ret = rtos_create_thread(&asr_demo_thread_hdl,
						 5,
						 "asr_demo",
						 (beken_thread_function_t)asr_demo_main,
						 1536,
						 NULL);
	if (ret != kNoErr) {
		os_printf("create asr demo task fail \r\n");
		asr_demo_thread_hdl = NULL;
	}
	os_printf("create asr demo task complete \r\n");
#endif

	return ret;
}

bk_err_t aud_intf_asr_demo_stop(void)
{
	bk_err_t ret = BK_OK;

	ret = bk_aud_intf_mic_stop();
	if (ret != BK_ERR_AUD_INTF_OK) {
		os_printf("bk_aud_intf_mic_stop fail, ret:%d \r\n", ret);
	} else {
		os_printf("bk_aud_intf_mic_stop complete \r\n");
	}
	os_printf("stop mic \r\n");

	ret = bk_aud_intf_mic_deinit();
	if (ret != BK_ERR_AUD_INTF_OK) {
		os_printf("bk_aud_intf_mic_deinit fail, ret:%d \r\n", ret);
	} else {
		os_printf("bk_aud_intf_mic_deinit complete \r\n");
	}

	ret = bk_aud_intf_drv_deinit();
	if (ret != BK_ERR_AUD_INTF_OK) {
		os_printf("bk_aud_intf_drv_deinit fail, ret:%d \r\n", ret);
	} else {
		os_printf("bk_aud_intf_drv_deinit complete \r\n");
	}

	asr_send_mb(EVENT_ASR_CLOSE, 0);

	/* free buffer  */
	ring_buffer_clear(&asr_rb);
	if (asr_ring_buff) {
		os_free(asr_ring_buff);
		asr_ring_buff = NULL;
	}
	if (asr_buff) {
		os_free(asr_buff);
		asr_buff = NULL;
	}

	return ret;
}

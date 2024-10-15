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
#include <driver/dma.h>

#include <driver/media_types.h>
#include <driver/psram.h>
#include "h264_hal.h"
#include "jpeg_hal.h"
#include <driver/jpeg_enc.h>
#include <os/mem.h>
#include <driver/timer.h>
#include <components/video_transfer.h>
#include "bk_drv_model.h"

#define TAG "dvp_minor"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

static uint8_t dvp_video_dma = DMA_ID_MAX;
static uint32_t frame_total_len = 0;
static video_config_t *dvp_video_config = NULL;

void dvp_camera_reset_dma(void)
{
	bk_dma_stop(dvp_video_dma);
	dvp_video_config->rx_read_len = 0;
	bk_dma_start(dvp_video_dma);
}

static void dvp_camera_dma_finish_handler(dma_id_t dma_id)
{
	uint16_t already_len = dvp_video_config->rx_read_len;
	uint16_t copy_len = dvp_video_config->node_len;
	frame_total_len += copy_len;
	GLOBAL_INT_DECLARATION();
	// if (jpeg_drop_image == 0)
	// {
		if (dvp_video_config && dvp_video_config->node_full_handler != NULL)
			dvp_video_config->node_full_handler(dvp_video_config->rxbuf + already_len, copy_len, 0, 0);
	// }

	already_len += copy_len;
	if (already_len >= dvp_video_config->rxbuf_len)
		already_len = 0;
	GLOBAL_INT_DISABLE();
	dvp_video_config->rx_read_len = already_len;
	GLOBAL_INT_RESTORE();
}

bk_err_t dvp_camera_dma_config(video_config_t *dvp_trans_config)
{
	dma_config_t dma_config = {0};
	uint32_t encode_fifo_addr;
	int err = 0;
	dvp_video_config = dvp_trans_config;

	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 0;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
	dma_config.dst.start_addr = (uint32_t)dvp_video_config->rxbuf;
	dma_config.dst.end_addr = (uint32_t)(dvp_video_config->rxbuf + dvp_video_config->rxbuf_len);

	// dma config
	if (dvp_video_dma >= DMA_ID_MAX)
	{
		if (dvp_video_config->device->fmt == PIXEL_FMT_JPEG)
		{
			encode_fifo_addr = JPEG_R_RX_FIFO;
			dma_config.src.start_addr = encode_fifo_addr;
			dvp_video_dma = bk_dma_alloc(DMA_DEV_JPEG);
			dma_config.src.dev = DMA_DEV_JPEG;
		}
		else if (dvp_video_config->device->fmt == PIXEL_FMT_H264)
		{
			encode_fifo_addr = H264_R_RX_FIFO;
			dma_config.src.start_addr = encode_fifo_addr;
			dvp_video_dma = bk_dma_alloc(DMA_DEV_H264);
			dma_config.src.dev = DMA_DEV_H264;
		}
		if ((dvp_video_dma < DMA_ID_0) || (dvp_video_dma >= DMA_ID_MAX))
		{
			return err;
		}
	}
	LOGI("dvp dma:%d\r\n", dvp_video_dma);
	LOGI("rxbuf addr  %x \r\n", dma_config.dst.start_addr);
	LOGI("rxbuf len %x \r\n", dvp_video_config->rxbuf_len);

	BK_LOG_ON_ERR(bk_dma_init(dvp_video_dma, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(dvp_video_dma, dvp_video_config->node_len));
	BK_LOG_ON_ERR(bk_dma_register_isr(dvp_video_dma, NULL, dvp_camera_dma_finish_handler));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(dvp_video_dma));
#if (CONFIG_SPE)
	BK_LOG_ON_ERR(bk_dma_set_dest_sec_attr(dvp_video_dma, DMA_ATTR_SEC));
	BK_LOG_ON_ERR(bk_dma_set_src_sec_attr(dvp_video_dma, DMA_ATTR_SEC));
#endif
	BK_LOG_ON_ERR(bk_dma_start(dvp_video_dma));

	return BK_OK;
}

bk_err_t bk_dvp_dma_deinit(void)
{
	int ret = kNoErr;

	bk_dma_stop(dvp_video_dma);
	bk_dma_deinit(dvp_video_dma);
	if (dvp_video_config->device->fmt == PIXEL_FMT_H264) {
		bk_dma_free(DMA_DEV_H264, dvp_video_dma);
	} else if (dvp_video_config->device->fmt == PIXEL_FMT_JPEG) {
		bk_dma_free(DMA_DEV_JPEG, dvp_video_dma);
	}

	// jpeg_eof_flag = 0;
	// jpeg_drop_image = 0;
	frame_total_len = 0;
	dvp_video_dma = DMA_ID_MAX;

	dvp_video_config = NULL;

	LOGI("camera deinit finish\r\n");
	return ret;
}

void bk_dvp_camera_eof_handler()
{
	//os_printf("h264 eof \r\n");
	uint32_t left_len = bk_dma_get_remain_len(dvp_video_dma);
	uint32_t already_len = dvp_video_config->rx_read_len;
	uint32_t rec_len = dvp_video_config->node_len - left_len;
	GLOBAL_INT_DECLARATION();

	dvp_video_config->node_full_handler(dvp_video_config->rxbuf + already_len, rec_len, 1, frame_total_len);
	GLOBAL_INT_DISABLE();
	dvp_video_config->rx_read_len = already_len;
	GLOBAL_INT_RESTORE();
	if (already_len >= dvp_video_config->rxbuf_len) {
		already_len -= dvp_video_config->rxbuf_len;
	}

	dvp_camera_reset_dma();

	if (dvp_video_config && dvp_video_config->data_end_handler)
		dvp_video_config->data_end_handler();
}



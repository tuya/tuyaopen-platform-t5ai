// Copyright 2023-2024 Beken
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


#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include <driver/int.h>
#include <driver/h264.h>
#include <driver/yuv_buf.h>
#include <driver/video_common_driver.h>
#include <driver/dma.h>
#include <driver/psram.h>

#include "media_evt.h"
#include "frame_buffer.h"
#include "yuv_encode.h"
#include "uvc_pipeline_act.h"

#include "mux_pipeline.h"
#include "media_utils.h"
#include "avdk_crc.h"

#define TAG "h264_pipline"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#ifdef ENCODE_DIAG_DEBUG

#define H264_FRAME_START()		do { GPIO_UP(GPIO_DVP_D3); } while (0)
#define H264_FRAME_END()		do { GPIO_DOWN(GPIO_DVP_D3); } while (0)

#define H264_LINE_START()		do { GPIO_UP(GPIO_DVP_D2); } while (0)
#define H264_LINE_END()			do { GPIO_DOWN(GPIO_DVP_D2); } while (0)

#else
#define H264_FRAME_START()
#define H264_FRAME_END()

#define H264_LINE_START()
#define H264_LINE_END()
#endif

typedef enum
{
	H264_STATE_IDLE = 0,
	H264_STATE_ENCODING,
} h264_state_t;

#define H264_SPS_PPS_SIZE (41)
#define H264_SELF_DEFINE_SEI_SIZE (96)
#define H264_DMA_LEN (1024 * 10)

typedef struct {
	uint8_t task_state;
	uint8_t input_buf_type; // 1: input a complete yuv_frame, 0: input a 16line yuv_buffer
	uint8_t h264_init; // h264 release param have been init ok or not
	uint8_t frame_err;
	uint8_t sps_pps_flag;
	uint8_t i_frame;
	uint8_t regenerate_idr; // h264 module reset, will regenerate idr frame
	uint8_t line_done_cnt; // urrent frame resolution total line done number = width / 16;
	uint8_t line_done_index; // current encode line index
	uint8_t psram_overwrite_id;
	uint8_t dma_channel;

#ifdef CONFIG_H264_ADD_SELF_DEFINE_SEI
	uint8_t sei[H264_SELF_DEFINE_SEI_SIZE]; // save frame infomation
#endif

	uint8_t sequence;
	uint16_t width;
	uint16_t height;
	uint32_t encode_dma_length; // dma copy length
	uint32_t encode_node_length; // h264 encode 16 line size of yuv422
	uint32_t encode_offset;
	complex_buffer_t *decoder_buffer;
	uint8_t *yuv_buf;
	//frame_buffer_t *yuv_frame;
	void *stream;
	frame_buffer_t *h264_frame;
	beken_semaphore_t h264_sem;
	beken_queue_t h264_queue;
	beken_thread_t h264_thread;
	LIST_HEADER_T request_list;
	h264_state_t state;

	beken2_timer_t h264_timer;

	mux_callback_t decoder_free_cb;
	mux_callback_t reset_cb;

} h264_encode_config_t;

typedef struct {
	beken_mutex_t lock;
} h264_info_t;

extern media_debug_t *media_debug;
static h264_encode_config_t * h264_encode_config = NULL;
static h264_info_t *h264_info = NULL;

bk_err_t h264_encode_task_send_msg(uint8_t type, uint32_t param)
{
	bk_err_t ret = BK_FAIL;

	media_msg_t msg;

	if (h264_encode_config
		&& h264_encode_config->h264_queue
		&& h264_encode_config->task_state)
	{
		msg.event = type;
		msg.param = param;
		ret = rtos_push_to_queue(&h264_encode_config->h264_queue, &msg, BEKEN_NO_WAIT);
		if (BK_OK != ret)
		{
			LOGE("%s failed, type:%d\r\n", __func__, type);
		}
	}

	return ret;
}

bk_err_t bk_h264_reset_request(mux_callback_t cb)
{
	rtos_lock_mutex(&h264_info->lock);

	h264_encode_config->reset_cb = cb;

	if (BK_OK != h264_encode_task_send_msg(H264_ENCODE_RESET, 0))
	{
		LOGI("%s send failed\n", __func__);
		goto error;
	}

	rtos_unlock_mutex(&h264_info->lock);

	return BK_OK;

error:

	if (h264_encode_config
		&& h264_encode_config->reset_cb)
	{
		h264_encode_config->reset_cb = NULL;
	}

	rtos_unlock_mutex(&h264_info->lock);

	LOGE("%s failed\n", __func__);
	return BK_FAIL;
}

static void h264_encode_reset_handle(void)
{
	if (h264_encode_config && h264_encode_config->h264_init == false)
	{
		return;
	}

	if (rtos_is_oneshot_timer_running(&h264_encode_config->h264_timer))
	{
		rtos_stop_oneshot_timer(&h264_encode_config->h264_timer);
	}

	// step 1: stop dma
	bk_dma_stop(h264_encode_config->dma_channel);

	// step 2: reset h264
	bk_yuv_buf_soft_reset();
	bk_yuv_buf_stop(H264_MODE);
	bk_h264_encode_disable();
	bk_h264_deinit();
	bk_yuv_buf_deinit();

	// step 3: reset frame_buffer
	h264_encode_config->h264_frame->fmt = PIXEL_FMT_H264;
	h264_encode_config->h264_frame->h264_type = 0;
	h264_encode_config->h264_frame->length = 0;

	bk_dma_start(h264_encode_config->dma_channel);
	h264_encode_config->state = H264_STATE_IDLE;
	h264_encode_config->sequence = 0;
	h264_encode_config->h264_init = false;
	h264_encode_config->encode_offset = 0;
	h264_encode_config->encode_dma_length = 0;
	if (h264_encode_config->decoder_buffer)
	{
		h264_encode_config->decoder_free_cb(h264_encode_config->decoder_buffer);
		h264_encode_config->decoder_buffer = NULL;
	}

	LOGV("%s, %d-%d\r\n", __func__, h264_encode_config->line_done_index, h264_encode_config->line_done_cnt);

	LOGD("%s, complete\r\n", __func__);

	if(h264_encode_config->reset_cb)
		h264_encode_config->reset_cb(NULL);
}

static void h264_dump_head_eof(frame_buffer_t * frame)
{
	uint32_t length = frame->size;
	LOGD("%s, %d-%d, sof:%02x-%02x-%02x-%02x, eof:%02x-%02x-%02x-%02x\r\n", __func__, frame->length, length, frame->frame[0], frame->frame[1], frame->frame[2],
		frame->frame[3], frame->frame[length - 4], frame->frame[length -3], frame->frame[length - 2],
		frame->frame[length - 1]);
}

static void h264_encode_line_done_handler(h264_unit_t id, void *param)
{
	H264_LINE_END();

	if (h264_encode_config->task_state == false)
	{
		if (h264_encode_config->input_buf_type)
		{
			h264_encode_config->decoder_free_cb(NULL);
		}
		return;
	}

	if (h264_encode_config->input_buf_type)
	{
		if (h264_encode_config->decoder_buffer == NULL)
		{
			LOGE("%s %d, decoder buffer is NULL %d\n", __func__, __LINE__, h264_encode_config->line_done_index);
			return;
		}

		if (!check_jpeg_decode_task_is_open())
		{
			h264_encode_config->decoder_free_cb(h264_encode_config->decoder_buffer);
			h264_encode_config->decoder_buffer = NULL;
			LOGW("%s, %d\n", __func__, __LINE__);
			return;
		}
		h264_encode_task_send_msg(H264_ENCODE_LINE_CONTINUE, 0);
	}
	else
	{
		h264_encode_config->state = H264_STATE_IDLE;
		if (rtos_is_oneshot_timer_running(&h264_encode_config->h264_timer))
		{
			rtos_stop_oneshot_timer(&h264_encode_config->h264_timer);
		}

		if (!list_empty(&h264_encode_config->request_list))
		{
			LIST_HEADER_T *pos, *n, *list = &h264_encode_config->request_list;
			pipeline_encode_request_t *request = NULL;

			list_for_each_safe(pos, n, list)
			{
				request = list_entry(pos, pipeline_encode_request_t, list);
				if (request != NULL)
				{
					h264_encode_task_send_msg(H264_ENCODE_START, (uint32_t)request);
					list_del(pos);
				}
			}
		}

		h264_encode_task_send_msg(H264_ENCODE_LINE_DONE, 0);
	}
}

static void h264_encode_final_out_handler(h264_unit_t id, void *param)
{
	H264_FRAME_END();

	if (h264_encode_config->state == H264_STATE_ENCODING)
	{
		h264_encode_config->state = H264_STATE_IDLE;
	}

	h264_encode_config->sequence++;

	if (h264_encode_config->sequence > H264_GOP_FRAME_CNT)
	{
		h264_encode_config->sequence = 1;
	}

	if (h264_encode_config->sequence == 1)
	{
		h264_encode_config->i_frame = 1;
	}
	else
	{
		h264_encode_config->i_frame = 0;
	}


	if (h264_encode_config->task_state)
		h264_encode_task_send_msg(H264_ENCODE_FINISH, 0);
}

static void h264_encode_dma_finish_callback(dma_id_t id)
{
	h264_encode_config->encode_dma_length += H264_DMA_LEN;
}

static bk_err_t h264_encode_dma_config(void)
{
	dma_config_t dma_config = {0};
	uint32_t encode_fifo_addr;

	bk_h264_get_fifo_addr(&encode_fifo_addr);

	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_H264;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
	dma_config.src.start_addr = encode_fifo_addr;
	dma_config.src.end_addr = encode_fifo_addr + 4;

	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
	dma_config.dst.start_addr = (uint32_t)h264_encode_config->h264_frame->frame;
	dma_config.dst.end_addr = (uint32_t)(h264_encode_config->h264_frame->frame + h264_encode_config->h264_frame->size);

	BK_LOG_ON_ERR(bk_dma_init(h264_encode_config->dma_channel, &dma_config));
	BK_LOG_ON_ERR(bk_dma_register_isr(h264_encode_config->dma_channel, NULL, h264_encode_dma_finish_callback));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(h264_encode_config->dma_channel));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(h264_encode_config->dma_channel, H264_DMA_LEN));
#if (CONFIG_SPE)
	BK_LOG_ON_ERR(bk_dma_set_src_burst_len(h264_encode_config->dma_channel, BURST_LEN_SINGLE));
	BK_LOG_ON_ERR(bk_dma_set_dest_burst_len(h264_encode_config->dma_channel, BURST_LEN_INC16));
	BK_LOG_ON_ERR(bk_dma_set_dest_sec_attr(h264_encode_config->dma_channel, DMA_ATTR_SEC));
	BK_LOG_ON_ERR(bk_dma_set_src_sec_attr(h264_encode_config->dma_channel, DMA_ATTR_SEC));
#endif
	bk_dma_start(h264_encode_config->dma_channel);

	return BK_OK;
}

static void h264_encode_start_handle(uint32_t param)
{
	pipeline_encode_request_t *h264_notify = (pipeline_encode_request_t *)param;

	if (h264_encode_config->state == H264_STATE_ENCODING)
	{
		GLOBAL_INT_DECLARATION();
		GLOBAL_INT_DISABLE();
		list_add_tail(&h264_notify->list, &h264_encode_config->request_list);
		GLOBAL_INT_RESTORE();
		return;
	}

	if (h264_notify->jdec_type != h264_encode_config->input_buf_type
		|| h264_notify->width != h264_encode_config->width
		|| h264_notify->height != h264_encode_config->height)
	{
		LOGV("%s, %d\n", __func__, __LINE__);
		h264_encode_reset_handle();
	}

	if (h264_encode_config->decoder_buffer)
	{
		LOGE("%s %d decoder_buffer NOT NULL\n", __func__, __LINE__);
		goto out;
	}
	else
	{
		h264_encode_config->decoder_buffer = (complex_buffer_t*)os_malloc(sizeof(complex_buffer_t));

		if (h264_encode_config->decoder_buffer == NULL)
		{
			LOGE("%s decoder_buffer malloc failed\n", __func__);
			goto out;
		}
		os_memcpy(h264_encode_config->decoder_buffer, h264_notify->buffer, sizeof(complex_buffer_t));
	}

	h264_encode_config->state = H264_STATE_ENCODING;

	if (!h264_encode_config->h264_init)
	{
		if (h264_notify->buffer->index == 1)
		{
			h264_encode_config->h264_init = true;
			LOGD("%s, %d, %d-%d,%d, %p\r\n", __func__, __LINE__, h264_notify->width, h264_notify->height, h264_notify->buffer->index,  h264_notify->buffer->data);
			yuv_buf_config_t yuv_buf_config = {0};
			yuv_buf_config.x_pixel = h264_notify->width / 8;
			yuv_buf_config.y_pixel = h264_notify->height / 8;
			yuv_buf_config.work_mode = H264_MODE;
			yuv_buf_config.base_addr = NULL;
			yuv_buf_config.yuv_mode_cfg.yuv_format = YUV_FORMAT_YUYV;

			bk_yuv_buf_init(&yuv_buf_config);

			bk_yuv_buf_enable_nosensor_encode_mode();

			// there need attation
			bk_h264_init(h264_notify->width, h264_notify->height);

			h264_encode_config->input_buf_type = h264_notify->jdec_type;
			h264_encode_config->encode_node_length = h264_notify->width * 16 * 2;// 16line + 16line
			h264_encode_config->line_done_cnt = h264_notify->height / 16;
			h264_encode_config->width = h264_notify->width;
			h264_encode_config->height = h264_notify->height;

			if (h264_notify->jdec_type)
			{
				// for encode complete frame
				h264_encode_config->yuv_buf = get_mux_sram_buffer();
				bk_yuv_buf_set_em_base_addr((uint32_t)h264_encode_config->yuv_buf);
			}
			else
			{
				bk_yuv_buf_set_em_base_addr((uint32_t)h264_encode_config->decoder_buffer->data);
			}

			/* register h264 callback */
			bk_h264_register_isr(H264_LINE_DONE, h264_encode_line_done_handler, 0);
			bk_h264_register_isr(H264_FINAL_OUT, h264_encode_final_out_handler, 0);

			bk_yuv_buf_start(H264_MODE);
			bk_h264_encode_enable();
		}
		else
		{
			if (!h264_notify->jdec_type)
			{
				h264_encode_config->state = H264_STATE_IDLE;
				h264_encode_config->decoder_free_cb(h264_encode_config->decoder_buffer);
				h264_encode_config->decoder_buffer = NULL;
				LOGD("%s, %d, %d\r\n", __func__, h264_notify->buffer->index, h264_encode_config->line_done_index);
				goto out;
			}
		}
	}

	if (h264_notify->buffer->index == 1 && h264_encode_config->h264_frame)
	{
		h264_encode_config->h264_frame->width = h264_notify->width;
		h264_encode_config->h264_frame->height = h264_notify->height;
		h264_encode_config->h264_frame->sequence = h264_notify->sequence;
		h264_encode_config->h264_frame->length = 0;
		h264_encode_config->h264_frame->type = UVC_CAMERA;
		h264_encode_config->h264_frame->fmt = PIXEL_FMT_H264;
		h264_encode_config->h264_frame->h264_type = 0;
	}

	if (h264_encode_config->input_buf_type)
	{
		frame_buffer_t *yuv_frame = (frame_buffer_t *)h264_encode_config->decoder_buffer->data;
		os_memcpy(h264_encode_config->yuv_buf, yuv_frame->frame, h264_encode_config->encode_node_length * 2);
		h264_encode_config->encode_offset = h264_encode_config->encode_node_length;
	}
	else
	{
		if (!h264_notify->buffer->ok)
		{
			LOGV("%s, yuv frame error\r\n", __func__);
			h264_encode_config->frame_err = true;
		}
	}

	H264_LINE_START();
	H264_FRAME_START();

	if (!rtos_is_oneshot_timer_running(&h264_encode_config->h264_timer))
	{
		rtos_start_oneshot_timer(&h264_encode_config->h264_timer);
	}

	h264_encode_config->line_done_index = h264_notify->buffer->index;
	bk_yuv_buf_rencode_start();

out:
	os_free(h264_notify);
	h264_notify = NULL;
}

static void h264_encode_pingpang_buf_done_handle()
{
	if (rtos_is_oneshot_timer_running(&h264_encode_config->h264_timer))
	{
		rtos_stop_oneshot_timer(&h264_encode_config->h264_timer);
	}

	if (h264_encode_config->decoder_buffer == NULL)
	{
		LOGE("%s %d, coder buffer is NULL, %d\n", __func__, __LINE__, h264_encode_config->line_done_index);
		return;
	}

	// send message to tell jpegdec one buffer have been encode finish, please transfer another buf
	h264_encode_config->decoder_free_cb(h264_encode_config->decoder_buffer);
	h264_encode_config->decoder_buffer = NULL;
}

static void h264_encode_pingpang_buf_continue_handle()
{
	H264_LINE_START();
	if (h264_encode_config->line_done_index < h264_encode_config->line_done_cnt)
	{
		// this is for delay 10us, when start the next h264 encode;
		// h264 encode interval is too short maybe cause timeout
		for (volatile int i = 0 ; i < 200 ; i ++);
		bk_yuv_buf_rencode_start();
	}

	h264_encode_config->line_done_index ++;

	if (h264_encode_config->line_done_index == h264_encode_config->line_done_cnt)
	{
		return;
	}

	frame_buffer_t *frame_buffer = (frame_buffer_t *)h264_encode_config->decoder_buffer->data;
	h264_encode_config->encode_offset += h264_encode_config->encode_node_length;
	if (h264_encode_config->line_done_index % 2 == 1)
	{
		os_memcpy(h264_encode_config->yuv_buf + h264_encode_config->encode_node_length, frame_buffer->frame + h264_encode_config->encode_offset, h264_encode_config->encode_node_length);
	}
	else
	{
		os_memcpy(h264_encode_config->yuv_buf, frame_buffer->frame + h264_encode_config->encode_offset, h264_encode_config->encode_node_length);
	}
}

static bool h264_encode_check_head_handle(uint8_t *data)
{
	if (data[0] != 0x00 || data[1] != 0x00 || data[2] != 0x00 || data[3] != 0x01)
	{
		return false;
	}

	return true;
}

static void h264_encode_finish_handle(void)
{
	H264_LINE_END();
	frame_buffer_t *new_frame = NULL;
	uint32_t real_length = 0;

	if (!h264_encode_config->task_state)
	{
		if (h264_encode_config->input_buf_type)
		{
			h264_encode_config->decoder_free_cb(NULL);
		}
		return;
	}

	bk_dma_flush_src_buffer(h264_encode_config->dma_channel);

	bk_dma_stop(h264_encode_config->dma_channel);

	real_length = bk_h264_get_encode_count() << 2;

	h264_encode_config->h264_frame->length = real_length;

	h264_encode_config->encode_dma_length += (H264_DMA_LEN - bk_dma_get_remain_len(h264_encode_config->dma_channel));

	if (real_length > (CONFIG_H264_FRAME_SIZE - 128) || h264_encode_config->frame_err)
	{
		LOGW("%s, %d-%d, error:%d\r\n", __func__, real_length, CONFIG_H264_FRAME_SIZE, h264_encode_config->frame_err);

		h264_encode_config->frame_err = true;
		goto error;
	}

	if (h264_encode_config->encode_dma_length != real_length)
	{
		if ((real_length - h264_encode_config->encode_dma_length) != H264_DMA_LEN)
		{
			LOGW("%s, size not match:--%d-%d=%d-----------\r\n", __func__, h264_encode_config->encode_dma_length, real_length,
					real_length - h264_encode_config->encode_dma_length);

			h264_encode_config->frame_err = true;
			goto error;
		}
	}

	if (h264_encode_check_head_handle(h264_encode_config->h264_frame->frame) == false)
	{
		LOGW("%s, h264 head error\r\n", __func__);
		h264_encode_config->frame_err = true;
		goto error;
	}

	if (h264_encode_config->regenerate_idr == false)
	{
		new_frame = frame_buffer_fb_malloc(h264_encode_config->stream, CONFIG_H264_FRAME_SIZE);
		if (new_frame == NULL)
		{
			h264_encode_config->frame_err = true;
		}
	}

	media_debug->isr_h264++;
	media_debug->h264_length = h264_encode_config->h264_frame->length;

error:

	h264_encode_config->encode_dma_length = 0;
	if (h264_encode_config->frame_err || h264_encode_config->regenerate_idr)
	{
		h264_encode_config->regenerate_idr = true;
		h264_encode_config->frame_err = false;
		h264_encode_config->h264_frame->length = 0;
		goto out;
	}

	media_debug->h264_kbps += h264_encode_config->h264_frame->length;

	h264_encode_config->h264_frame->timestamp = get_current_timestamp();

#ifdef CONFIG_H264_ADD_SELF_DEFINE_SEI
	h264_encode_config->h264_frame->crc = hnd_crc8(h264_encode_config->h264_frame->frame, h264_encode_config->h264_frame->length, 0xFF);
	h264_encode_config->h264_frame->length += H264_SELF_DEFINE_SEI_SIZE;
	os_memcpy(&h264_encode_config->sei[23], (uint8_t *)h264_encode_config->h264_frame, sizeof(frame_buffer_t));
	os_memcpy(&h264_encode_config->h264_frame->frame[h264_encode_config->h264_frame->length - H264_SELF_DEFINE_SEI_SIZE], &h264_encode_config->sei[0], H264_SELF_DEFINE_SEI_SIZE);
#endif

	bk_psram_disable_write_through(h264_encode_config->psram_overwrite_id);

	if (h264_encode_config->i_frame)
	{
		h264_encode_config->h264_frame->h264_type |= 1 << H264_NAL_I_FRAME;

#if (CONFIG_H264_GOP_START_IDR_FRAME)
		h264_encode_config->h264_frame->h264_type |= (1 << H264_NAL_SPS) | (1 << H264_NAL_PPS) | (1 << H264_NAL_IDR_SLICE);
#endif
	}
	else
	{
		h264_encode_config->h264_frame->h264_type |= 1 << H264_NAL_P_FRAME;
	}

	LOGV("%s, I:%d, p:%d\r\n", __func__, (h264_encode_config->h264_frame->h264_type & 0x1000020) > 0 ? 1 : 0, (h264_encode_config->h264_frame->h264_type >> 23) & 0x1);

	frame_buffer_fb_push(h264_encode_config->stream, h264_encode_config->h264_frame);
	h264_encode_config->h264_frame = new_frame;

	bk_psram_enable_write_through(h264_encode_config->psram_overwrite_id, (uint32_t)h264_encode_config->h264_frame->frame,
			(uint32_t)(h264_encode_config->h264_frame->frame + h264_encode_config->h264_frame->size));

	bk_dma_set_dest_addr(h264_encode_config->dma_channel, (uint32_t)(h264_encode_config->h264_frame->frame),
		(uint32_t)(h264_encode_config->h264_frame->frame + h264_encode_config->h264_frame->size));

out:
	bk_yuv_buf_soft_reset();

#if (CONFIG_H264_GOP_START_IDR_FRAME)
	if (h264_encode_config->sequence == H264_GOP_FRAME_CNT)
	{
		bk_h264_soft_reset();
		h264_encode_config->sequence = 0;
	}
#endif

	if (h264_encode_config->regenerate_idr)
	{
		bk_h264_soft_reset();
		h264_encode_config->sequence = 0;
		h264_encode_config->regenerate_idr = false;
	}

	bk_dma_start(h264_encode_config->dma_channel);
	h264_encode_config->encode_offset = 0;

	// send message to jpeg dec task encode finish. please transfer next frame buf1
	if (h264_encode_config->input_buf_type)
		h264_encode_task_send_msg(H264_ENCODE_LINE_DONE, 0);
}

static void h264_encode_task_deinit(void)
{
	if (h264_encode_config)
	{
		// step 1: deinit h264 encode
		if (h264_encode_config->h264_init)
		{
			bk_yuv_buf_stop(H264_MODE);
			bk_yuv_buf_soft_reset();
			bk_h264_encode_disable();

			// step 2: deinit yuv_buf and h264 driver
			bk_h264_deinit();
			bk_yuv_buf_deinit();
		}

		// step 3: free h264_encode_config
		// step 3.1: free dma
		bk_dma_stop(h264_encode_config->dma_channel);
		bk_dma_deinit(h264_encode_config->dma_channel);
		bk_dma_free(DMA_DEV_H264, h264_encode_config->dma_channel);

		// step 3.2: free frame_buffer
		if (h264_encode_config->h264_frame)
		{
			LOGV("%s, frame free start\r\n", __func__);
			bk_psram_disable_write_through(h264_encode_config->psram_overwrite_id);
			bk_psram_free_write_through_channel(h264_encode_config->psram_overwrite_id);
			frame_buffer_fb_free(h264_encode_config->stream, h264_encode_config->h264_frame);
			h264_encode_config->h264_frame = NULL;
			LOGV("%s, frame free success\r\n", __func__);
		}

		if (!list_empty(&h264_encode_config->request_list))
		{
			LIST_HEADER_T *pos, *n, *list = &h264_encode_config->request_list;
			pipeline_encode_request_t *request = NULL;

			list_for_each_safe(pos, n, list)
			{
				request = list_entry(pos, pipeline_encode_request_t, list);
				if (request != NULL)
				{
					list_del(pos);
					if(h264_encode_config->input_buf_type)
					{
						LOGW("%s, %d\n", __func__, __LINE__);
					}
					else
					{
						complex_buffer_t *decoder_buffer = (complex_buffer_t*)os_malloc(sizeof(complex_buffer_t));
						os_memcpy(decoder_buffer, request->buffer, sizeof(complex_buffer_t));
						h264_encode_config->decoder_free_cb(decoder_buffer);
					}
					os_free(request);
					request = NULL;
				}
			}
			INIT_LIST_HEAD(list);
		}

		if (h264_encode_config->h264_sem)
		{
			rtos_deinit_semaphore(&h264_encode_config->h264_sem);
			h264_encode_config->h264_sem = NULL;
		}

		if (h264_encode_config->h264_queue)
		{
			rtos_deinit_queue(&h264_encode_config->h264_queue);
			h264_encode_config->h264_queue = NULL;
		}

		if(!h264_encode_config->input_buf_type)
		{
			if (h264_encode_config->decoder_buffer)
			{
				LOGD("clear decoder_buffer: %d\n", h264_encode_config->decoder_buffer->index);

				h264_encode_config->decoder_free_cb(h264_encode_config->decoder_buffer);
				h264_encode_config->decoder_buffer = NULL;
			}
		}
		else
		{
			if (h264_encode_config->decoder_buffer)
			{
				LOGD("clear decoder_buffer: %d\n", h264_encode_config->decoder_buffer->index);
				h264_encode_config->decoder_free_cb(NULL);
				os_free(h264_encode_config->decoder_buffer);
				h264_encode_config->decoder_buffer = NULL;
			}
		}

		frame_buffer_list_node_clear(h264_encode_config->stream);
		frame_buffer_list_node_deinit(h264_encode_config->stream);
		h264_encode_config->stream = NULL;

		os_free(h264_encode_config);
		h264_encode_config = NULL;
	}
}

static void h264_encode_main(beken_thread_arg_t data)
{
	int ret = BK_OK;
	h264_encode_config->task_state = true;
	rtos_set_semaphore(&h264_encode_config->h264_sem);

	while (1)
	{
		media_msg_t msg;
		ret = rtos_pop_from_queue(&h264_encode_config->h264_queue, &msg, BEKEN_WAIT_FOREVER);
		if (ret == BK_OK)
		{
			switch (msg.event)
			{
				case H264_ENCODE_START:
					h264_encode_start_handle(msg.param);
					break;

				case H264_ENCODE_LINE_DONE:
					h264_encode_pingpang_buf_done_handle();
					break;

				case H264_ENCODE_LINE_CONTINUE:
					h264_encode_pingpang_buf_continue_handle();
					break;

				case H264_ENCODE_FINISH:
					h264_encode_finish_handle();
					break;

				case H264_ENCODE_REGENERATE_IDR:
					h264_encode_config->regenerate_idr = true;
					break;

				case H264_ENCODE_RESET:
					h264_encode_reset_handle();
					break;

				case H264_ENCODE_STOP:
				{
					beken_semaphore_t *beken_semaphore = (beken_semaphore_t*)msg.param;

					LOGD("%s H264_ENCODE_STOP\n", __func__);

					if (rtos_is_oneshot_timer_running(&h264_encode_config->h264_timer))
					{
						rtos_stop_oneshot_timer(&h264_encode_config->h264_timer);
					}

					if (rtos_is_oneshot_timer_init(&h264_encode_config->h264_timer))
					{
						rtos_deinit_oneshot_timer(&h264_encode_config->h264_timer);
					}

					do {
						ret = rtos_pop_from_queue(&h264_encode_config->h264_queue, &msg, BEKEN_NO_WAIT);

						if (ret != BK_OK)
						{
							break;
						}

						if (msg.event == H264_ENCODE_START)
						{
							pipeline_encode_request_t *request = (pipeline_encode_request_t *)msg.param;
							complex_buffer_t *buffer = NULL;

							if (request == NULL)
							{
								LOGE("%s request is NULL\n", __func__);
								break;
							}

							buffer = (complex_buffer_t*)(complex_buffer_t*)os_malloc(sizeof(complex_buffer_t));
							os_memcpy(buffer, request->buffer, sizeof(complex_buffer_t));
							h264_encode_config->decoder_free_cb(buffer);

							os_free(request);
						}

					} while (true);

					if (h264_encode_config->input_buf_type)
					{
						h264_encode_config->decoder_free_cb(NULL);
					}

					rtos_deinit_queue(&h264_encode_config->h264_queue);
					h264_encode_config->h264_queue = NULL;
					h264_encode_config->h264_thread = NULL;

					rtos_set_semaphore(beken_semaphore);
				}
				goto exit;

				default:
					break;

			}
		}
	}

exit:

	LOGD("%s exit, %d\r\n", __func__, __LINE__);

	rtos_delete_thread(NULL);
}

bool check_h264_task_is_open(void)
{
	if (h264_encode_config == NULL)
	{
		return false;
	}
	else
	{
		return h264_encode_config->task_state;
	}
}

static void h264_timer_handle(void *arg1, void *arg2)
{
	LOGD("%s, timeout\n", __func__);

	h264_encode_reset_handle();

	if (!list_empty(&h264_encode_config->request_list))
	{
		LIST_HEADER_T *pos, *n, *list = &h264_encode_config->request_list;
		pipeline_encode_request_t *request = NULL;

		list_for_each_safe(pos, n, list)
		{
			request = list_entry(pos, pipeline_encode_request_t, list);
			if (request != NULL)
			{
				h264_encode_task_send_msg(H264_ENCODE_START, (uint32_t)request);
				list_del(pos);
			}
		}
	}

	if (h264_encode_config->decoder_buffer == NULL)
	{
		LOGE("%s decoder buffer is NULL\n", __func__);
		return;
	}

	// complete frame
	h264_encode_config->decoder_free_cb(h264_encode_config->decoder_buffer);
	h264_encode_config->decoder_buffer = NULL;
}

bk_err_t h264_encode_task_open(media_camera_device_t *device)
{
	int ret = BK_OK;
	// step 1: check h264_encode_task
	if (h264_encode_config)
	{
		LOGD("%s have been opened\r\n", __func__);
		return ret;
	}

	// step 2: init encode config
	if (h264_encode_config == NULL)
	{
		h264_encode_config = (h264_encode_config_t *)os_malloc(sizeof(h264_encode_config_t));
		if (h264_encode_config == NULL)
		{
			LOGE("%s, malloc h264_encode_config failed\r\n", __func__);
			ret = BK_FAIL;
			goto error;
		}

		os_memset(h264_encode_config, 0, sizeof(h264_encode_config_t));

		INIT_LIST_HEAD(&h264_encode_config->request_list);

		if (!rtos_is_oneshot_timer_init(&h264_encode_config->h264_timer))
		{
			ret = rtos_init_oneshot_timer(&h264_encode_config->h264_timer, 1000, h264_timer_handle, NULL, NULL);

			if (ret != BK_OK)
			{
				LOGE("create h264 timer failed\n");
			}
		}

		h264_encode_config->dma_channel = bk_fixed_dma_alloc(DMA_DEV_H264, DMA_ID_8);
		LOGD("dma for encode is %x \r\n", h264_encode_config->dma_channel);

		rtos_init_semaphore(&h264_encode_config->h264_sem, 1);
		if (h264_encode_config->h264_sem == NULL)
		{
			LOGE("%s, init h264_sem failed\r\n", __func__);
			ret = BK_FAIL;
			goto error;
		}
	}

	// step 3: init h264 frame buffer and malloc h264 frame
	h264_encode_config->stream = frame_buffer_list_node_init(0, UVC_CAMERA, IMAGE_H264);
	if (h264_encode_config->stream == NULL)
	{
		LOGE("%s, create h264 stream list failed\r\n", __func__);
		ret = BK_FAIL;
		goto error;
	}
	else
	{
		LOGD("%s, %d, %p\n", __func__, __LINE__, h264_encode_config->stream);
	}

	h264_encode_config->h264_frame = frame_buffer_fb_malloc(h264_encode_config->stream, CONFIG_H264_FRAME_SIZE);
	if (h264_encode_config->h264_frame == NULL)
	{
		LOGE("%s, malloc failed\r\n", __func__);
		ret = BK_FAIL;
		goto error;
	}

	h264_encode_config->psram_overwrite_id = bk_psram_alloc_write_through_channel();

	bk_psram_enable_write_through(h264_encode_config->psram_overwrite_id, (uint32_t)h264_encode_config->h264_frame->frame,
			(uint32_t)(h264_encode_config->h264_frame->frame + h264_encode_config->h264_frame->size));

	// step 4: init h264 encode hw
	if (device->width * device->height > 1280 * 720)
	{
		LOGE("%s, not support more than 1280X720 resolution\r\n", __func__);
		ret = BK_FAIL;
		goto error;
	}

	media_debug->isr_h264 = 0;
	media_debug->h264_length = 0;
	media_debug->h264_kbps = 0;

#ifdef CONFIG_H264_ADD_SELF_DEFINE_SEI
	os_memset(&h264_encode_config->sei[0], 0xFF, H264_SELF_DEFINE_SEI_SIZE);
	h264_encode_sei_init(&h264_encode_config->sei[0]);
#endif

	//step 4: init dma h264 fifo to h264 frame buffer
	h264_encode_dma_config();

	// step 5: init h264 process task
	ret = rtos_init_queue(&h264_encode_config->h264_queue,
							"h264_que",
							sizeof(media_msg_t),
							20);
	if (BK_OK != ret)
	{
		LOGE("%s h264_encode_config->h264_queue init failed\n", __func__);
		goto error;
	}

	ret = rtos_smp_create_thread(&h264_encode_config->h264_thread,
							BEKEN_DEFAULT_WORKER_PRIORITY - 1,
							"h264_task",
							(beken_thread_function_t)h264_encode_main,
							1024 * 2,
							NULL);

	if (BK_OK != ret)
	{
		LOGE("%s h264_task init failed\n", __func__);
		goto error;
	}

	rtos_get_semaphore(&h264_encode_config->h264_sem, BEKEN_WAIT_FOREVER);

	return ret;

error:

	LOGE("%s failed!\r\n", __func__);
	h264_encode_task_deinit();

	return ret;
}

void h264_encode_task_stop(void)
{
	beken_semaphore_t sem;
	media_msg_t msg;

	int ret = rtos_init_semaphore(&sem, 1);

	if (ret != BK_OK)
	{
		LOGE("%s, init sem faild, %d\n", __func__, ret);
		return;
	}

	msg.event = H264_ENCODE_STOP;
	msg.param = (uint32_t)&sem;

	ret = rtos_push_to_queue(&h264_encode_config->h264_queue, &msg, BEKEN_NO_WAIT);

	if (BK_OK != ret)
	{
		LOGE("%s rtos_push_to_queue failed:%d\r\n", __func__, ret);
	}

	rtos_get_semaphore(&sem, BEKEN_NEVER_TIMEOUT);

	rtos_deinit_semaphore(&sem);
}

bk_err_t h264_encode_task_close(void)
{
	if (h264_encode_config == NULL || !h264_encode_config->task_state)
		return BK_FAIL;

	rtos_lock_mutex(&h264_info->lock);
	h264_encode_config->task_state = false;
	rtos_unlock_mutex(&h264_info->lock);
	h264_encode_task_stop();

	h264_encode_task_deinit();

	LOGD("%s complete\n", __func__);
	return BK_OK;
}

bk_err_t bk_h264_encode_request(pipeline_encode_request_t *request, mux_callback_t cb)
{
	pipeline_encode_request_t *h264_request = NULL;

	if (h264_encode_config == NULL || h264_encode_config->task_state == false)
	{
		LOGD("%s not open\n", __func__);
		return BK_FAIL;
	}

	rtos_lock_mutex(&h264_info->lock);
	h264_request = (pipeline_encode_request_t *)os_malloc(sizeof(pipeline_encode_request_t));

	if (h264_request == NULL)
	{
		LOGD("%s malloc failed\n", __func__);
		goto error;
	}

	os_memcpy(h264_request, request, sizeof(pipeline_encode_request_t));

	h264_encode_config->decoder_free_cb = cb;
	if (BK_OK != h264_encode_task_send_msg(H264_ENCODE_START, (uint32_t)h264_request))
	{
		LOGD("%s send failed\n", __func__);
		goto error;
	}

	rtos_unlock_mutex(&h264_info->lock);

	return BK_OK;

error:
	if (h264_request)
	{
		os_free(h264_request);
		h264_request = NULL;
	}
	rtos_unlock_mutex(&h264_info->lock);

	return BK_FAIL;
}

bk_err_t h264_encode_regenerate_idr_frame(void)
{
	if (h264_encode_config == NULL || !h264_encode_config->task_state)
		return BK_FAIL;

	h264_encode_task_send_msg(H264_ENCODE_REGENERATE_IDR, 0);
	return BK_OK;
}

bk_err_t bk_h264_pipeline_init(void)
{
	bk_err_t ret = BK_FAIL;

	if(h264_info != NULL)
	{
		os_free(h264_info);
		h264_info = NULL;
	}

	h264_info = (h264_info_t*)os_malloc(sizeof(h264_info_t));

	if (h264_info == NULL)
	{
		LOGE("%s malloc h264_info failed\n", __func__);
		return BK_FAIL;
	}

	os_memset(h264_info, 0, sizeof(h264_info_t));

	ret = rtos_init_mutex(&h264_info->lock);

	if (ret != BK_OK)
	{
		LOGE("%s, init mutex failed\r\n", __func__);
		return BK_FAIL;
	}

	return ret;
}



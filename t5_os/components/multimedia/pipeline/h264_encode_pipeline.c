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
#include <driver/aon_rtc.h>

#include "media_evt.h"
#include "frame_buffer.h"
#include "yuv_encode.h"

#include "mux_pipeline.h"

#define TAG "h264_pipline"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#ifdef ENCODE_DIAG_DEBUG

#define H264_FRAME_START()		//do { GPIO_UP(GPIO_DVP_D2); } while (0)
#define H264_FRAME_END()		//do { GPIO_DOWN(GPIO_DVP_D2); } while (0)

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

static const uint8 crc8_table[256] =
{
	0x00, 0xF7, 0xB9, 0x4E, 0x25, 0xD2, 0x9C, 0x6B,
	0x4A, 0xBD, 0xF3, 0x04, 0x6F, 0x98, 0xD6, 0x21,
	0x94, 0x63, 0x2D, 0xDA, 0xB1, 0x46, 0x08, 0xFF,
	0xDE, 0x29, 0x67, 0x90, 0xFB, 0x0C, 0x42, 0xB5,
	0x7F, 0x88, 0xC6, 0x31, 0x5A, 0xAD, 0xE3, 0x14,
	0x35, 0xC2, 0x8C, 0x7B, 0x10, 0xE7, 0xA9, 0x5E,
	0xEB, 0x1C, 0x52, 0xA5, 0xCE, 0x39, 0x77, 0x80,
	0xA1, 0x56, 0x18, 0xEF, 0x84, 0x73, 0x3D, 0xCA,
	0xFE, 0x09, 0x47, 0xB0, 0xDB, 0x2C, 0x62, 0x95,
	0xB4, 0x43, 0x0D, 0xFA, 0x91, 0x66, 0x28, 0xDF,
	0x6A, 0x9D, 0xD3, 0x24, 0x4F, 0xB8, 0xF6, 0x01,
	0x20, 0xD7, 0x99, 0x6E, 0x05, 0xF2, 0xBC, 0x4B,
	0x81, 0x76, 0x38, 0xCF, 0xA4, 0x53, 0x1D, 0xEA,
	0xCB, 0x3C, 0x72, 0x85, 0xEE, 0x19, 0x57, 0xA0,
	0x15, 0xE2, 0xAC, 0x5B, 0x30, 0xC7, 0x89, 0x7E,
	0x5F, 0xA8, 0xE6, 0x11, 0x7A, 0x8D, 0xC3, 0x34,
	0xAB, 0x5C, 0x12, 0xE5, 0x8E, 0x79, 0x37, 0xC0,
	0xE1, 0x16, 0x58, 0xAF, 0xC4, 0x33, 0x7D, 0x8A,
	0x3F, 0xC8, 0x86, 0x71, 0x1A, 0xED, 0xA3, 0x54,
	0x75, 0x82, 0xCC, 0x3B, 0x50, 0xA7, 0xE9, 0x1E,
	0xD4, 0x23, 0x6D, 0x9A, 0xF1, 0x06, 0x48, 0xBF,
	0x9E, 0x69, 0x27, 0xD0, 0xBB, 0x4C, 0x02, 0xF5,
	0x40, 0xB7, 0xF9, 0x0E, 0x65, 0x92, 0xDC, 0x2B,
	0x0A, 0xFD, 0xB3, 0x44, 0x2F, 0xD8, 0x96, 0x61,
	0x55, 0xA2, 0xEC, 0x1B, 0x70, 0x87, 0xC9, 0x3E,
	0x1F, 0xE8, 0xA6, 0x51, 0x3A, 0xCD, 0x83, 0x74,
	0xC1, 0x36, 0x78, 0x8F, 0xE4, 0x13, 0x5D, 0xAA,
	0x8B, 0x7C, 0x32, 0xC5, 0xAE, 0x59, 0x17, 0xE0,
	0x2A, 0xDD, 0x93, 0x64, 0x0F, 0xF8, 0xB6, 0x41,
	0x60, 0x97, 0xD9, 0x2E, 0x45, 0xB2, 0xFC, 0x0B,
	0xBE, 0x49, 0x07, 0xF0, 0x9B, 0x6C, 0x22, 0xD5,
	0xF4, 0x03, 0x4D, 0xBA, 0xD1, 0x26, 0x68, 0x9F
};

static uint8 hnd_crc8(
    uint8 *pdata,   /* pointer to array of data to process */
    uint32  nbytes,   /* number of input data bytes to process */
    uint8 crc   /* either CRC8_INIT_VALUE or previous return value */
)
{
	/* hard code the crc loop instead of using CRC_INNER_LOOP macro
	 * to avoid the undefined and unnecessary (uint8 >> 8) operation.
	 */
	while (nbytes-- > 0)
	{
		crc = crc8_table[(crc ^ *pdata++) & 0xff];
	}

	return crc;
}


typedef struct {
	uint8_t task_state;
	uint8_t input_buf_type : 1; // 1: input a complete yuv_frame, 0: input a 16line yuv_buffer
	uint8_t h264_init : 1; // h264 release param have been init ok or not
	uint8_t frame_err : 1;
	uint8_t sps_pps_flag : 1;
	uint8_t i_frame : 1;
	uint8_t regenerate_idr : 1; // h264 module reset, will regenerate idr frame
	uint8_t line_done_cnt; // urrent frame resolution total line done number = width / 16;
	uint8_t line_done_index; // current encode line index
	uint8_t psram_overwrite_id;
	uint8_t dma_channel;
#ifndef CONFIG_H264_GOP_START_IDR_FRAME
	uint8_t sps_pps[64]; //for psram write through
#endif

#ifdef CONFIG_H264_ADD_SELF_DEFINE_SEI
	uint8_t sei[H264_SELF_DEFINE_SEI_SIZE]; // save frame infomation
#endif

	uint8_t sequence;
	uint32_t encode_dma_length; // dma copy length
	uint32_t encode_node_length; // h264 encode 16 line size of yuv422
	uint32_t encode_offset;
	complex_buffer_t *decoder_buffer;
	uint8_t *yuv_buf;
	//frame_buffer_t *yuv_frame;
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


static uint32_t h264_encode_get_milliseconds(void)
{
	uint32_t time = 0;

#if CONFIG_ARCH_RISCV
	extern u64 riscv_get_mtimer(void);

	time = (riscv_get_mtimer() / 26000) & 0xFFFFFFFF;
#elif CONFIG_ARCH_CM33

#if CONFIG_AON_RTC
	time = (bk_aon_rtc_get_us() / 1000) & 0xFFFFFFFF;
#endif

#endif

	return time;
}


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
	if (rtos_is_oneshot_timer_running(&h264_encode_config->h264_timer))
	{
		rtos_stop_oneshot_timer(&h264_encode_config->h264_timer);
	}

	// step 1: stop dma
	bk_dma_stop(h264_encode_config->dma_channel);

	// step 2: reset h264
	bk_yuv_buf_soft_reset();
	bk_video_encode_stop(H264_MODE);
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
	LOGD("%s, %d-%d\r\n", __func__, h264_encode_config->line_done_index, h264_encode_config->line_done_cnt);

	LOGI("%s, complete\r\n", __func__);

    if(h264_encode_config->reset_cb)
        h264_encode_config->reset_cb(NULL);
}

static void h264_dump_head_eof(frame_buffer_t * frame)
{
	uint32_t length = frame->size;
	LOGI("%s, %d-%d, sof:%02x-%02x-%02x-%02x, eof:%02x-%02x-%02x-%02x\r\n", __func__, frame->length, length, frame->frame[0], frame->frame[1], frame->frame[2],
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
			jpeg_decode_task_send_msg(JPEGDEC_H264_FRAME_NOTIFY, 0);
		}
		return;
	}

	if (h264_encode_config->input_buf_type)
	{
		if (h264_encode_config->decoder_buffer == NULL)
		{
			LOGE("%s decoder buffer is NULL\n", __func__);
			return;
		}

		if (!check_jpeg_decode_task_is_open())
		{
			jpeg_decode_task_send_msg(JPEGDEC_H264_FRAME_NOTIFY, (uint32_t)h264_encode_config->decoder_buffer);
			h264_encode_config->decoder_buffer = NULL;
			return;
		}
		for (volatile int i = 0; i < 100; i ++); //this is for h264 timeout
		if ((h264_encode_config->line_done_index % 2) == 0)
		{
			frame_buffer_t *frame_buffer = (frame_buffer_t*)h264_encode_config->decoder_buffer->data;
			h264_encode_config->encode_offset += h264_encode_config->encode_node_length;
			os_memcpy(h264_encode_config->yuv_buf, frame_buffer->frame + h264_encode_config->encode_offset, h264_encode_config->encode_node_length);
		}

		H264_LINE_START();
		if (h264_encode_config->line_done_index < h264_encode_config->line_done_cnt)
			bk_yuv_buf_rencode_start();
		h264_encode_config->line_done_index ++;
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

	//if (h264_encode_config->task_state)
	{
		if (h264_encode_config->state == H264_STATE_ENCODING)
		{
			GLOBAL_INT_DECLARATION();
			GLOBAL_INT_DISABLE();
			list_add_tail(&h264_notify->list, &h264_encode_config->request_list);
			GLOBAL_INT_RESTORE();
			return;
		}


		if (h264_encode_config->decoder_buffer)
		{
			LOGE("%s decoder_buffer NOT NULL\n", __func__);
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
				LOGI("%s, %d, %d-%d,%d, %p\r\n", __func__, __LINE__, h264_notify->width, h264_notify->height, h264_notify->buffer->index,  h264_notify->buffer->data);
				yuv_buf_config_t yuv_buf_config = {0};
				yuv_buf_config.x_pixel = h264_notify->width / 8;
				yuv_buf_config.y_pixel = h264_notify->height / 8;
				yuv_buf_config.work_mode = H264_MODE;
				yuv_buf_config.base_addr = NULL;
				yuv_buf_config.yuv_mode_cfg.yuv_format = YUV_FORMAT_YUYV;

				bk_yuv_buf_init(&yuv_buf_config);

				bk_yuv_buf_enable_nosensor_encode_mode();

				// there need attation
				bk_h264_init((h264_notify->width << 16) | h264_notify->height);

				h264_encode_config->input_buf_type = h264_notify->jdec_type;
				h264_encode_config->encode_node_length = h264_notify->width * 32 * 2;// 16line + 16line
				h264_encode_config->line_done_cnt = h264_notify->height / 16;
				h264_encode_config->h264_frame->width = h264_notify->width;
				h264_encode_config->h264_frame->height = h264_notify->height;
				//bk_yuv_buf_set_frame_resolution(h264_notify->width, h264_notify->height);
				if (h264_notify->jdec_type)
				{
					// for encode complete frame
					h264_encode_config->yuv_buf = jdec_decode_get_yuv_buffer();
					bk_yuv_buf_set_em_base_addr((uint32_t)h264_encode_config->yuv_buf);
				}
				else
				{
					bk_yuv_buf_set_em_base_addr((uint32_t)h264_encode_config->decoder_buffer->data);
				}

				/* register h264 callback */
				bk_h264_register_isr(H264_LINE_DONE, h264_encode_line_done_handler, 0);
				bk_h264_register_isr(H264_FINAL_OUT, h264_encode_final_out_handler, 0);

				bk_video_encode_start(H264_MODE);
			}
			else
			{
				if (!h264_notify->jdec_type)
				{
					h264_encode_config->state = H264_STATE_IDLE;
					h264_encode_config->decoder_free_cb(h264_encode_config->decoder_buffer);
					h264_encode_config->decoder_buffer = NULL;
					LOGI("%s, %d, %d\r\n", __func__, h264_notify->buffer->index, h264_encode_config->line_done_index);
					goto out;
				}
			}
		}

		if (h264_encode_config->input_buf_type)
		{
			frame_buffer_t *yuv_frame = (frame_buffer_t *)h264_encode_config->decoder_buffer->data;
			os_memcpy(h264_encode_config->yuv_buf, yuv_frame->frame, h264_encode_config->encode_node_length);
		}
		else
		{
			if (!h264_notify->buffer->ok)
			{
				LOGD("%s, yuv frame error\r\n", __func__);
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
	}

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
		LOGE("%s decoder buffer is NULL\n", __func__);
		return;
	}

	//if (h264_encode_config->task_state)
	{
		// send message to tell jpegdec one buffer have been encode finish, please transfer another buf
		if (h264_encode_config->input_buf_type)
		{
			jpeg_decode_task_send_msg(JPEGDEC_H264_FRAME_NOTIFY, (uint32_t)h264_encode_config->decoder_buffer);
			h264_encode_config->decoder_buffer = NULL;
		}
		else
		{
            h264_encode_config->decoder_free_cb(h264_encode_config->decoder_buffer);
			h264_encode_config->decoder_buffer = NULL;
		}
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
	uint32_t width = 0, height = 0, sequence = 0, offset = 0;
#if (!CONFIG_H264_GOP_START_IDR_FRAME)
	uint8_t sps_pps_fill = 0;
#endif
	if (!h264_encode_config->task_state)
	{
		if (h264_encode_config->input_buf_type)
		{
			jpeg_decode_task_send_msg(JPEGDEC_H264_FRAME_NOTIFY, 0);
		}
		return;
	}

	width = h264_encode_config->h264_frame->width;
	height = h264_encode_config->h264_frame->height;
	sequence = h264_encode_config->h264_frame->sequence;

	bk_dma_flush_src_buffer(h264_encode_config->dma_channel);

	bk_dma_stop(h264_encode_config->dma_channel);

	real_length = bk_h264_get_encode_count() << 2;

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

	h264_encode_config->h264_frame->fmt = PIXEL_FMT_H264;
	h264_encode_config->h264_frame->length += real_length;

	if (h264_encode_check_head_handle(h264_encode_config->h264_frame->frame) == false)
	{
		LOGW("%s, h264 head error\r\n", __func__);
		h264_encode_config->frame_err = true;
		goto error;
	}

	new_frame = frame_buffer_fb_malloc(FB_INDEX_H264, CONFIG_H264_FRAME_SIZE);
	if (new_frame == NULL)
	{
		h264_encode_config->frame_err = true;
	}

	media_debug->isr_h264++;
	media_debug->h264_length = h264_encode_config->h264_frame->length;

error:

	h264_encode_config->encode_dma_length = 0;
	if (h264_encode_config->frame_err)
	{
		h264_encode_config->regenerate_idr = true;
		h264_encode_config->frame_err = false;
		h264_encode_config->h264_frame->width = width;
		h264_encode_config->h264_frame->height = height;
		h264_encode_config->h264_frame->sequence = sequence + 1;
		h264_encode_config->h264_frame->fmt = PIXEL_FMT_H264;
		h264_encode_config->h264_frame->length = 0;
		goto out;
	}

	media_debug->h264_kbps += h264_encode_config->h264_frame->length;

	h264_encode_config->h264_frame->timestamp = h264_encode_get_milliseconds();

#ifdef CONFIG_H264_ADD_SELF_DEFINE_SEI
	h264_encode_config->h264_frame->crc = hnd_crc8(h264_encode_config->h264_frame->frame, h264_encode_config->h264_frame->length, 0xFF);
	h264_encode_config->h264_frame->length += H264_SELF_DEFINE_SEI_SIZE;
	os_memcpy(&h264_encode_config->sei[23], (uint8_t *)h264_encode_config->h264_frame, sizeof(frame_buffer_t));
	os_memcpy(&h264_encode_config->h264_frame->frame[h264_encode_config->h264_frame->length - H264_SELF_DEFINE_SEI_SIZE], &h264_encode_config->sei[0], H264_SELF_DEFINE_SEI_SIZE);
#endif

	bk_psram_disable_write_through(h264_encode_config->psram_overwrite_id);

#if (!CONFIG_H264_GOP_START_IDR_FRAME)
	if (!h264_encode_config->sps_pps_flag)
	{
		if (h264_encode_config->h264_frame->frame[4] == 0x67)
		{
			const uint8_t sei_fill_data[] = {
				0x00, 0x00, 0x00, 0x01, 0x06, 0x05, 0x0F, 0x01,
				0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01,
				0x02, 0x03, 0x04, 0x01, 0x05, 0x00, 0x80
			};

			os_memcpy(h264_encode_config->sps_pps, h264_encode_config->h264_frame->frame, H264_SPS_PPS_SIZE);
			os_memcpy(&h264_encode_config->sps_pps[H264_SPS_PPS_SIZE], sei_fill_data, sizeof(h264_encode_config->sps_pps) - H264_SPS_PPS_SIZE);
			h264_encode_config->sps_pps_flag = 1;
			h264_encode_config->h264_frame->h264_type |= (1 << H264_NAL_SPS) | (1 << H264_NAL_PPS);
		}
		else
		{
			LOGE("%s sps pps head miss\n", __func__);
		}
	}
#endif

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

#if (!CONFIG_H264_GOP_START_IDR_FRAME)
		if (h264_encode_config->sequence == (CONFIG_H264_P_FRAME_CNT + 1)
			&& h264_encode_config->sps_pps_flag)
		{
			sps_pps_fill = true;
		}
#endif

	}

	LOGD("%s, I:%d, p:%d\r\n", __func__, (h264_encode_config->h264_frame->h264_type & 0x1000020) > 0 ? 1 : 0, (h264_encode_config->h264_frame->h264_type >> 23) & 0x1);

	frame_buffer_fb_push(h264_encode_config->h264_frame);

	h264_encode_config->h264_frame = new_frame;
	h264_encode_config->h264_frame->width = width;
	h264_encode_config->h264_frame->height = height;
	h264_encode_config->h264_frame->sequence = sequence + 1;
	h264_encode_config->h264_frame->fmt = PIXEL_FMT_H264;
	h264_encode_config->h264_frame->length = 0;
	h264_encode_config->h264_frame->h264_type = 0;

#if (!CONFIG_H264_GOP_START_IDR_FRAME)
	if (sps_pps_fill)
	{
		offset = sizeof(h264_encode_config->sps_pps);
		os_memcpy(h264_encode_config->h264_frame->frame, h264_encode_config->sps_pps, offset);
		h264_encode_config->h264_frame->length += offset;
		h264_encode_config->h264_frame->h264_type |= (1 << H264_NAL_SPS) | (1 << H264_NAL_PPS);
	}
#endif

	bk_psram_enable_write_through(h264_encode_config->psram_overwrite_id, (uint32_t)h264_encode_config->h264_frame->frame,
			(uint32_t)(h264_encode_config->h264_frame->frame + h264_encode_config->h264_frame->size));

	bk_dma_set_dest_addr(h264_encode_config->dma_channel, (uint32_t)(h264_encode_config->h264_frame->frame + offset),
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
			bk_video_encode_stop(H264_MODE);
			bk_yuv_buf_soft_reset();

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
			LOGD("%s, frame free start\r\n", __func__);
			bk_psram_disable_write_through(h264_encode_config->psram_overwrite_id);
			bk_psram_free_write_through_channel(h264_encode_config->psram_overwrite_id);
			frame_buffer_fb_direct_free(h264_encode_config->h264_frame);
			h264_encode_config->h264_frame = NULL;
			LOGD("%s, frame free success\r\n", __func__);
		}

		//GLOBAL_INT_DECLARATION();
		//GLOBAL_INT_DISABLE();
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
						//
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
		//GLOBAL_INT_RESTORE();

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
				LOGI("clear decoder_buffer: %d\n", h264_encode_config->decoder_buffer->index);

				h264_encode_config->decoder_free_cb(h264_encode_config->decoder_buffer);
				h264_encode_config->decoder_buffer = NULL;
			}
		}
		else
		{
			if (h264_encode_config->decoder_buffer)
			{
				LOGI("clear decoder_buffer: %d\n", h264_encode_config->decoder_buffer->index);
				jpeg_decode_task_send_msg(JPEGDEC_H264_FRAME_NOTIFY, 0);
				os_free(h264_encode_config->decoder_buffer);
				h264_encode_config->decoder_buffer = NULL;
			}
		}
		frame_buffer_fb_clear(FB_INDEX_H264);

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
				{
					h264_encode_start_handle(msg.param);
					break;
				}

				case H264_ENCODE_LINE_DONE:
					h264_encode_pingpang_buf_done_handle();
					break;

				case H264_ENCODE_FINISH:
					h264_encode_finish_handle();
					break;

				case H264_ENCODE_RESET:
					h264_encode_reset_handle();
					break;

				case H264_ENCODE_STOP:
				{
					beken_semaphore_t *beken_semaphore = (beken_semaphore_t*)msg.param;

					LOGI("%s H264_ENCODE_STOP\n", __func__);

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
							if (h264_encode_config->input_buf_type)
							{
								jpeg_decode_task_send_msg(JPEGDEC_H264_FRAME_NOTIFY, (uint32_t)buffer);
							}
							else
							{
                                h264_encode_config->decoder_free_cb(buffer);
							}

							os_free(request);
						}

					} while (true);

					if (h264_encode_config->input_buf_type)
					{
						jpeg_decode_task_send_msg(JPEGDEC_H264_FRAME_NOTIFY, 0);
					}
					else
					{
						//jpeg_decode_task_send_msg(JPEGDEC_H264_CLEAR_NOTIFY, 0);
					}

					h264_encode_config->task_state = false;
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

	LOGI("%s exit, %d\r\n", __func__, __LINE__);

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
	LOGI("%s, timeout\n", __func__);

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

	if (h264_encode_config->input_buf_type)
	{
		// complete frame
		jpeg_decode_task_send_msg(JPEGDEC_H264_FRAME_NOTIFY, (uint32_t)h264_encode_config->decoder_buffer);
		h264_encode_config->decoder_buffer = NULL;
	}
	else
	{
        h264_encode_config->decoder_free_cb(h264_encode_config->decoder_buffer);
		h264_encode_config->decoder_buffer = NULL;
	}
}

bk_err_t h264_encode_task_open(media_camera_device_t *device)
{
	int ret = BK_OK;
	// step 1: check h264_encode_task
	if (h264_encode_config)
	{
		LOGI("%s have been opened\r\n", __func__);
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
		LOGI("dma for encode is %x \r\n", h264_encode_config->dma_channel);

		rtos_init_semaphore(&h264_encode_config->h264_sem, 1);
		if (h264_encode_config->h264_sem == NULL)
		{
			LOGE("%s, init h264_sem failed\r\n", __func__);
			ret = BK_FAIL;
			goto error;
		}
	}

	// step 3: init h264 frame buffer and malloc h264 frame
	frame_buffer_fb_init(FB_INDEX_H264);
	h264_encode_config->h264_frame = frame_buffer_fb_malloc(FB_INDEX_H264, CONFIG_H264_FRAME_SIZE);
	if (h264_encode_config->h264_frame == NULL)
	{
		LOGE("%s, malloc failed\r\n", __func__);
		ret = BK_FAIL;
		goto error;
	}

	h264_encode_config->psram_overwrite_id = bk_psram_alloc_write_through_channel();

	bk_psram_enable_write_through(h264_encode_config->psram_overwrite_id, (uint32_t)h264_encode_config->h264_frame->frame,
			(uint32_t)(h264_encode_config->h264_frame->frame + h264_encode_config->h264_frame->size));

	h264_encode_config->h264_frame->width = device->info.resolution.width;
	h264_encode_config->h264_frame->height = device->info.resolution.height;
	h264_encode_config->h264_frame->sequence = 0;
	h264_encode_config->h264_frame->fmt = PIXEL_FMT_H264;
	//h264_dump_head_eof(h264_encode_config->h264_frame);

	// step 4: init h264 encode hw
	if (device->info.resolution.width * device->info.resolution.height > 1280 * 720)
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

	ret = rtos_create_thread(&h264_encode_config->h264_thread,
							BEKEN_DEFAULT_WORKER_PRIORITY,
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

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	h264_encode_config->task_state = false;
	GLOBAL_INT_RESTORE();

	h264_encode_task_stop();

	h264_encode_task_deinit();

	LOGI("%s complete\n", __func__);
	return BK_OK;
}


bk_err_t bk_h264_encode_request(pipeline_encode_request_t *request, mux_callback_t cb)
{
	pipeline_encode_request_t *h264_request = NULL;
	rtos_lock_mutex(&h264_info->lock);

	if (h264_encode_config == NULL || h264_encode_config->task_state == false)
	{
		LOGI("%s not open\n", __func__);
		goto error;
	}

	h264_request = (pipeline_encode_request_t *)os_malloc(sizeof(pipeline_encode_request_t));

	if (h264_request == NULL)
	{
		LOGI("%s malloc failed\n", __func__);
		goto error;
	}

	os_memcpy(h264_request, request, sizeof(pipeline_encode_request_t));

	h264_encode_config->decoder_free_cb = cb;
	if (BK_OK != h264_encode_task_send_msg(H264_ENCODE_START, (uint32_t)h264_request))
	{
		LOGI("%s send failed\n", __func__);
		goto error;
	}

	rtos_unlock_mutex(&h264_info->lock);

	return BK_OK;

error:

	if (h264_encode_config
		&& h264_encode_config->decoder_free_cb)
	{
		h264_encode_config->decoder_free_cb = NULL;
	}

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

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	h264_encode_config->regenerate_idr = true;
	GLOBAL_INT_RESTORE();
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



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
#include <driver/rott_driver.h>

#include "frame_buffer.h"
#include "yuv_encode.h"
#include <driver/gpio.h>
#include "media_evt.h"
#include "yuv_encode.h"

#if CONFIG_HW_ROTATE_PFC
#include <driver/rott_driver.h>
#endif
#include <driver/media_types.h>
#include <lcd_rotate.h>
#include "modules/image_scale.h"
#include <driver/dma2d.h>
#include <driver/dma2d_types.h>
#include <driver/psram.h>
#include "modules/image_scale.h"
#include "dma2d_ll_macro_def.h"

#include "bk_list.h"

#include "mux_pipeline.h"

#define TAG "rot_pipline"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#if CONFIG_LVGL
uint8_t lvgl_disp_enable = 0;
#endif

#ifdef ROTATE_DIAG_DEBUG
#define ROTATE_LINE_START()			do { GPIO_UP(GPIO_DVP_D3); } while (0)
#define ROTATE_LINE_END()			do { GPIO_DOWN(GPIO_DVP_D3); } while (0)

#define DMA2D_LINE_START()			do { GPIO_UP(GPIO_DVP_D4); } while (0)
#define DMA2D_LINE_END()			do { GPIO_DOWN(GPIO_DVP_D4); } while (0)

#else
#define ROTATE_LINE_START()
#define ROTATE_LINE_END()
#define DMA2D_LINE_START()
#define DMA2D_LINE_END()
#endif

typedef enum {
	BUF_IDLE = 0,
	BUF_READY = 1,
	BUF_ROTATING = 2,
	BUF_ROTATED = 3,
	BUF_COPYING = 4,
	BUF_COPY_READY,
}buf_status_t;


typedef enum
{
	ROTATE_STATE_IDLE = 0,
	ROTATE_STATE_ENCODING,
} rotate_state_t;

typedef struct {
	complex_buffer_t *rotate_buf;
	LIST_HEADER_T list;
} rotate_copy_request_t;

typedef struct {
	uint8_t enable;
	uint8_t task_running : 1;
	uint8_t err_frame : 1;
	uint8_t reset_status : 1;
	uint8_t psram_overwrite_id;
	frame_buffer_t *rotate_frame;
	beken_semaphore_t rot_sem;
	beken_queue_t rotate_queue;
	beken_thread_t rotate_thread;

	uint16_t jpeg_width;        /**< memcpy or pfc src image width */
	uint16_t jpeg_height;       /**< imemcpy or pfc src image height  */
	media_rotate_t rot_angle;
	media_rotate_mode_t rot_mode;
	pixel_format_t fmt;
	complex_buffer_t *decoder_buffer;

	complex_buffer_t buf[2];
	LIST_HEADER_T rotate_pedding_list;
	LIST_HEADER_T copy_pedding_list;
	rotate_state_t state;
	complex_buffer_t * rotate_buffer;
	complex_buffer_t  direct_copy_buffer;

	uint16_t dma2d_isr_cnt;
	uint8_t dma2d_copy;

	uint8_t rotate_ena;

	mux_callback_t decoder_free_cb;
    mux_callback_t reset_cb;
} rotate_config_t;

typedef struct {
	beken_mutex_t lock;
} rotate_info_t;


static rotate_config_t *rotate_config = NULL;
static rotate_info_t *rotate_info = NULL;

beken2_timer_t rotate_timer;

bk_err_t rotate_task_send_msg(uint8_t type, uint32_t param)
{
	bk_err_t ret = BK_FAIL;
	media_msg_t msg;

	if (rotate_config && rotate_config->enable)
	{
		msg.event = type;
		msg.param = param;
		ret = rtos_push_to_queue(&rotate_config->rotate_queue, &msg, BEKEN_WAIT_FOREVER);
	}

	if (BK_OK != ret)
	{
		LOGE("%s failed, type:%d\r\n", __func__, type);
	}

	return ret;
}
static void rotate_watermark_cb(void)
{
	LOGD("rotate_watermark_cb\r\n");
}
static void rotate_cfg_err_cb(void)
{
	LOGI("rotate_cfg_err_cb\r\n");
}
static void rotate_complete_cb(void)
{
	if (rotate_config)
	{
		rotate_task_send_msg(ROTATE_FINISH, (uint32_t)rotate_config->rotate_buffer);
	}
}
static void dma2d_config_error(void)
{
	LOGE("%s \n", __func__);
}

static void dma2d_transfer_error(void)
{
	LOGE("%s %d %p\n", __func__, rotate_config->rotate_buffer->index, rotate_config->rotate_frame->frame);
}

static complex_buffer_t *rotate_get_idle_buf(void)
{
	if (rotate_config->buf[0].state == BUF_IDLE)
		return &rotate_config->buf[0];
	else if (rotate_config->buf[1].state == BUF_IDLE)
		return &rotate_config->buf[1];
	else
		return NULL;
}


static void dma2d_transfer_complete(void)
{
	int ret = BK_OK;
	DMA2D_LINE_END();

	uint32_t rot_buf = dma2d_ll_get_dma2d_fg_address_value();
	//rotate_config->dma2d_isr_cnt++;

	if (rotate_config->dma2d_isr_cnt == (rotate_config->jpeg_height / PIPELINE_DECODE_LINE))
	{
		rotate_config->dma2d_isr_cnt = 0;
		rotate_config->rotate_frame->fmt = rotate_config->fmt;

	    if ((rotate_config->rot_angle == ROTATE_90) || (rotate_config->rot_angle == ROTATE_270))
        {
    		rotate_config->rotate_frame->width = rotate_config->jpeg_height;
    		rotate_config->rotate_frame->height = rotate_config->jpeg_width;
        }
        else
        {
    		rotate_config->rotate_frame->width = rotate_config->jpeg_width;
    		rotate_config->rotate_frame->height = rotate_config->jpeg_height;
        }

        bk_psram_disable_write_through(rotate_config->psram_overwrite_id);
        if (rotate_config->err_frame || rotate_config->reset_status)
        {
            frame_buffer_display_free(rotate_config->rotate_frame);
            rotate_config->rotate_frame = NULL;
            rotate_config->err_frame = false;
        }
        else
        {
            #if CONFIG_LVGL
            if (lvgl_disp_enable) {
                frame_buffer_display_free(rotate_config->rotate_frame);
            }
            else 
            #endif
            {
                if (lcd_display_frame_request(rotate_config->rotate_frame) != BK_OK)
                {
                    frame_buffer_display_free(rotate_config->rotate_frame);
                }
            }
            rotate_config->rotate_frame = NULL;
        }
        ROTATE_LINE_END();

#if (PIPELINE_ROTATE_CONTINUE == 0)
		rotate_config->buf[0].state = BUF_IDLE;
		rotate_config->buf[1].state = BUF_IDLE;
		rotate_config->decoder_free_cb(rotate_config->decoder_buffer);
		rotate_config->decoder_buffer = NULL;
#endif
	}

	if (rot_buf == (uint32_t)rotate_config->buf[0].data)
	{
		rotate_config->buf[0].state = BUF_IDLE;
	}
	else if(rot_buf == (uint32_t)rotate_config->buf[1].data)
	{
		rotate_config->buf[1].state = BUF_IDLE;
	}

	if (!list_empty(&rotate_config->copy_pedding_list))
	{
		LIST_HEADER_T *pos, *n, *list = &rotate_config->copy_pedding_list;
		rotate_copy_request_t *request = NULL;

		list_for_each_safe(pos, n, list)
		{
			request = list_entry(pos, rotate_copy_request_t, list);
			if (request != NULL)
			{
				ret = rotate_task_send_msg(ROTATE_LINE_COPY_START, (uint32_t)request);
				if (ret == BK_OK)
				{
					list_del(pos);
				}
				break;
			}
		}
	}

	if ((rotate_config->state == ROTATE_STATE_IDLE)
		&& (!list_empty(&rotate_config->rotate_pedding_list)))
	{
		LIST_HEADER_T *pos, *n, *list = &rotate_config->rotate_pedding_list;
		pipeline_encode_request_t *request = NULL;

		list_for_each_safe(pos, n, list)
		{
			request = list_entry(pos, pipeline_encode_request_t, list);
			if (request != NULL)
			{
				ret = rotate_task_send_msg(ROTATE_DEC_LINE_NOTIFY, (uint32_t)request);
				if (ret == BK_OK)
				{
					list_del(pos);
				}
				break;
			}
		}
	}

    if ((rotate_config->rot_mode == SW_ROTATE) && (rotate_config->rot_angle == ROTATE_NONE))
    {
		rotate_config->decoder_free_cb(rotate_config->decoder_buffer);
		rotate_config->decoder_buffer = NULL;
        rotate_config->state = ROTATE_STATE_IDLE;
    }

	rotate_config->dma2d_copy = false;
}

#if CONFIG_SOFTWARE_DECODE_SRAM_MAPPING
void rotate_set_dma2d_cb(void)
{
	bk_dma2d_register_int_callback_isr(DMA2D_CFG_ERROR_ISR, dma2d_config_error);
	bk_dma2d_register_int_callback_isr(DMA2D_TRANS_ERROR_ISR, dma2d_transfer_error);
	bk_dma2d_register_int_callback_isr(DMA2D_TRANS_COMPLETE_ISR, dma2d_transfer_complete);
}
#endif

static void rotate_finish_handler(uint32_t param)
{
	ROTATE_LINE_END();
	int ret = BK_OK;

	complex_buffer_t *rotate_buf = (complex_buffer_t*)param;

	if (rtos_is_oneshot_timer_running(&rotate_timer))
	{
		rtos_stop_oneshot_timer(&rotate_timer);
	}

	if (!list_empty(&rotate_config->rotate_pedding_list))
	{
		LIST_HEADER_T *pos, *n, *list = &rotate_config->rotate_pedding_list;
		pipeline_encode_request_t *request = NULL;

		list_for_each_safe(pos, n, list)
		{
			request = list_entry(pos, pipeline_encode_request_t, list);
			if (request != NULL)
			{
				ret = rotate_task_send_msg(ROTATE_DEC_LINE_NOTIFY, (uint32_t)request);
				list_del(pos);
				if (ret != BK_OK)
				{
					os_free(request);
					request = NULL;
				}
			}
		}
	}

	rotate_copy_request_t *rotate_copy_request = (rotate_copy_request_t*)os_malloc(sizeof(rotate_copy_request_t));
	rotate_copy_request->rotate_buf = rotate_buf;

#if (PIPELINE_ROTATE_CONTINUE == 0)
	if (rotate_buf->index < (rotate_config->jpeg_height / PIPELINE_DECODE_LINE))
#endif
	{
		rotate_config->decoder_free_cb(rotate_config->decoder_buffer);
		rotate_config->decoder_buffer = NULL;
	}
	if (BK_OK != rotate_task_send_msg(ROTATE_LINE_COPY_START, (uint32_t)rotate_copy_request))
	{
		os_free(rotate_copy_request);
	}

	rotate_config->rotate_ena = 0;
	rotate_config->state = ROTATE_STATE_IDLE;
}

static bk_err_t rotate_memcopy_handler(uint32_t param)
{
	bk_err_t ret = BK_OK;

	rotate_copy_request_t *rotate_copy_request = (rotate_copy_request_t*)param;
	complex_buffer_t *rotate_buf = rotate_copy_request->rotate_buf;

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

	if (rotate_config->dma2d_copy)
	{
        LOGD("===add copy_pedding_list===>>>>%s %d\n", __func__, __LINE__);
		list_add_tail(&rotate_copy_request->list, &rotate_config->copy_pedding_list);
		GLOBAL_INT_RESTORE();
		return ret;
	}
	else
	{
		rotate_config->dma2d_copy = true;
	}

	GLOBAL_INT_RESTORE();

    if ((rotate_config->dma2d_isr_cnt != (rotate_buf->index - 1)) && (rotate_config->dma2d_isr_cnt != (rotate_config->jpeg_height / PIPELINE_DECODE_LINE)))
    {
        LOGD("%s %d %d %d %d \n", __func__, __LINE__, rotate_config->dma2d_isr_cnt, rotate_buf->index, rotate_config->rotate_buffer->index);
    }

    rotate_config->dma2d_isr_cnt = rotate_buf->index;
	if (rotate_buf->index == 1)
	{
		if (rotate_config->rotate_frame)
		{
			frame_buffer_display_free(rotate_config->rotate_frame);
			rotate_config->rotate_frame = NULL;
		}
		if (rotate_config->fmt == PIXEL_FMT_RGB888)
			rotate_config->rotate_frame = frame_buffer_display_malloc(rotate_config->jpeg_width * rotate_config->jpeg_height * 3);
		else  //RGB565 YUYV
			rotate_config->rotate_frame = frame_buffer_display_malloc(rotate_config->jpeg_width * rotate_config->jpeg_height * 2);

		if (rotate_config->rotate_frame != NULL)
		{
			rotate_config->rotate_frame->fmt = rotate_config->fmt;
			bk_psram_enable_write_through(rotate_config->psram_overwrite_id, (uint32_t)rotate_config->rotate_frame->frame,
				(uint32_t)(rotate_config->rotate_frame->frame + rotate_config->rotate_frame->size));
		}
        else
        {
            LOGE("%s, malloc rotate psram buffer failed\r\n", __func__);
        }
    }

    if (rotate_config->rotate_frame == NULL)
    {
        goto error;
    }

	DMA2D_LINE_START();
	dma2d_memcpy_pfc_t dma2d_memcpy_pfc = {0};

	rotate_buf->state = BUF_COPYING;

	dma2d_memcpy_pfc.input_addr = (char *)rotate_buf->data;
	dma2d_memcpy_pfc.output_addr = (char *)rotate_config->rotate_frame->frame;

	if (rotate_config->fmt == PIXEL_FMT_RGB888)
	{
		dma2d_memcpy_pfc.mode = DMA2D_M2M_PFC;
		dma2d_memcpy_pfc.input_color_mode = DMA2D_INPUT_YUYV;
		dma2d_memcpy_pfc.src_pixel_byte = TWO_BYTES;
		dma2d_memcpy_pfc.output_color_mode = DMA2D_OUTPUT_RGB888;
		dma2d_memcpy_pfc.dst_pixel_byte = THREE_BYTES;
	}
	else if(rotate_config->fmt == PIXEL_FMT_YUYV) //RGB565 / RGB565_LE
	{
		dma2d_memcpy_pfc.mode = DMA2D_M2M;
		dma2d_memcpy_pfc.input_color_mode = DMA2D_INPUT_YUYV;
		dma2d_memcpy_pfc.src_pixel_byte = TWO_BYTES;
		dma2d_memcpy_pfc.output_color_mode = DMA2D_OUTPUT_YUYV;
		dma2d_memcpy_pfc.dst_pixel_byte = TWO_BYTES;
	}
    else  //RGB565
    {
        dma2d_memcpy_pfc.mode = DMA2D_M2M;
        dma2d_memcpy_pfc.input_color_mode = DMA2D_INPUT_RGB565;
        dma2d_memcpy_pfc.src_pixel_byte = TWO_BYTES;
        dma2d_memcpy_pfc.output_color_mode = DMA2D_OUTPUT_RGB565;
        dma2d_memcpy_pfc.dst_pixel_byte = TWO_BYTES;
    }

	dma2d_memcpy_pfc.src_frame_xpos = 0;
    dma2d_memcpy_pfc.src_frame_ypos = 0;
    if (rotate_config->rot_angle == ROTATE_90)
    {
    	dma2d_memcpy_pfc.dma2d_width = PIPELINE_DECODE_LINE;                     //rotate_config->dma2d_width;
    	dma2d_memcpy_pfc.dma2d_height = rotate_config->jpeg_width  ; // 800
    	dma2d_memcpy_pfc.src_frame_width = PIPELINE_DECODE_LINE;
    	dma2d_memcpy_pfc.src_frame_height = rotate_config->jpeg_width;
    	dma2d_memcpy_pfc.dst_frame_width = rotate_config->jpeg_height; //480
    	dma2d_memcpy_pfc.dst_frame_height = rotate_config->jpeg_width; //800
	    dma2d_memcpy_pfc.dst_frame_xpos = rotate_config->jpeg_height - (PIPELINE_DECODE_LINE * (rotate_config->dma2d_isr_cnt));
        dma2d_memcpy_pfc.dst_frame_ypos = 0;
    }
    else if (rotate_config->rot_angle == ROTATE_270)
    {
        dma2d_memcpy_pfc.dma2d_width = PIPELINE_DECODE_LINE;                     //rotate_config->dma2d_width;
    	dma2d_memcpy_pfc.dma2d_height = rotate_config->jpeg_width  ; // 800
    	dma2d_memcpy_pfc.src_frame_width = PIPELINE_DECODE_LINE;
    	dma2d_memcpy_pfc.src_frame_height = rotate_config->jpeg_width;
    	dma2d_memcpy_pfc.dst_frame_width = rotate_config->jpeg_height; //480
    	dma2d_memcpy_pfc.dst_frame_height = rotate_config->jpeg_width; //800
        dma2d_memcpy_pfc.dst_frame_xpos = PIPELINE_DECODE_LINE * (rotate_config->dma2d_isr_cnt - 1);
    	dma2d_memcpy_pfc.dst_frame_ypos = 0;
    }
    else if (rotate_config->rot_angle == ROTATE_NONE)
    {
        dma2d_memcpy_pfc.dma2d_width = rotate_config->jpeg_width;
    	dma2d_memcpy_pfc.dma2d_height = PIPELINE_DECODE_LINE;
    	dma2d_memcpy_pfc.src_frame_width = rotate_config->jpeg_width;
    	dma2d_memcpy_pfc.src_frame_height = PIPELINE_DECODE_LINE;
    	dma2d_memcpy_pfc.dst_frame_width = rotate_config->jpeg_width;
    	dma2d_memcpy_pfc.dst_frame_height =  rotate_config->jpeg_height;
        dma2d_memcpy_pfc.dst_frame_xpos = 0;
    	dma2d_memcpy_pfc.dst_frame_ypos = PIPELINE_DECODE_LINE * (rotate_config->dma2d_isr_cnt - 1);
    }
    else //180
    {
        dma2d_memcpy_pfc.dma2d_width = rotate_config->jpeg_width;
    	dma2d_memcpy_pfc.dma2d_height = PIPELINE_DECODE_LINE;
    	dma2d_memcpy_pfc.src_frame_width = rotate_config->jpeg_width;
    	dma2d_memcpy_pfc.src_frame_height = PIPELINE_DECODE_LINE;
    	dma2d_memcpy_pfc.dst_frame_width = rotate_config->jpeg_width;
    	dma2d_memcpy_pfc.dst_frame_height = rotate_config->jpeg_height;
        dma2d_memcpy_pfc.dst_frame_xpos = 0;
    	dma2d_memcpy_pfc.dst_frame_ypos = (rotate_config->jpeg_height - (rotate_config->dma2d_isr_cnt* PIPELINE_DECODE_LINE));
    }
	bk_dma2d_memcpy_or_pixel_convert(&dma2d_memcpy_pfc);
	bk_dma2d_start_transfer();

	os_free(rotate_copy_request);
    return ret;

error:
    if ((rotate_config->rot_mode == SW_ROTATE) && (rotate_config->rot_angle == ROTATE_NONE))
    {
		rotate_config->decoder_free_cb(rotate_config->decoder_buffer);
		rotate_config->decoder_buffer = NULL;
        rotate_config->state = ROTATE_STATE_IDLE;
    }
    else
    {
        rotate_buf->state = BUF_IDLE;
    }

    rotate_config->dma2d_copy = false;
    os_free(rotate_copy_request);
    return BK_FAIL;
}

bk_err_t rotate_clear_status(void)
{
	bk_err_t ret= BK_FAIL;
	LOGI("%s, set reset\n", __func__);
	rotate_config->reset_status = true;
    if(rotate_config->reset_cb)
        rotate_config->reset_cb(NULL);
    return ret;
}

bk_err_t bk_rotate_reset_request(mux_callback_t cb)
{
    rtos_lock_mutex(&rotate_info->lock);
 
    rotate_config->reset_cb = cb;

    if (BK_OK != rotate_task_send_msg(ROTATE_RESET, 0))
    {
        LOGI("%s send failed\n", __func__);
        goto error;
    }

    rtos_unlock_mutex(&rotate_info->lock);

    return BK_OK;

error:

    if (rotate_config
        && rotate_config->reset_cb)
    {
        rotate_config->reset_cb = NULL;
    }

    rtos_unlock_mutex(&rotate_info->lock);

    LOGE("%s failed\n", __func__);

    return BK_FAIL;
}

static void rotate_timer_handle(void *arg1, void *arg2)
{
	LOGI("%s, timeout, rotate: %d, %p, %p\n", __func__, rotate_config->rotate_ena,
	rotate_config->decoder_buffer, rotate_config->decoder_buffer->data);
	//decoder_mux_dump();
	rotate_config->reset_status = true;
    if(rotate_config->rot_mode == HW_ROTATE)
    {
		int ret = bk_rott_enable();
		if (ret != BK_OK)
		{
			LOGE("rotate enable fail again\n");
		}
		else
		{
			rotate_config->rotate_ena = 1;
		}

		if (!rtos_is_oneshot_timer_running(&rotate_timer))
		{
			rtos_start_oneshot_timer(&rotate_timer);
		}

		if (rotate_config->rotate_buffer != &rotate_config->buf[0])
		{
			rotate_config->buf[0].state = BUF_IDLE;
		}
		if (rotate_config->rotate_buffer != &rotate_config->buf[1])
		{
			rotate_config->buf[1].state = BUF_IDLE;
		}
    }
    else
    {
        LOGE("SW rotate Timeout\n");
    }
}

static bk_err_t rotate_no_rotate_direct_copy_handler(uint32_t param)
{
	if (rtos_is_oneshot_timer_running(&rotate_timer))
	{
		rtos_stop_oneshot_timer(&rotate_timer);
	}

    //1:dma2d memcopy, rotate_buf = decode buffer
    complex_buffer_t *dec_buf = (complex_buffer_t *)param;
    rotate_copy_request_t *rotate_copy_request = (rotate_copy_request_t*)os_malloc(sizeof(rotate_copy_request_t));
    complex_buffer_t *rotate_buf = &rotate_config->direct_copy_buffer;

    if(rotate_copy_request == NULL)
    {
        LOGE("%s, malloc fail \n", __func__);
        return BK_FAIL;
    }
    rotate_copy_request->rotate_buf = rotate_buf;
    rotate_copy_request->rotate_buf->data = dec_buf->data;
    rotate_copy_request->rotate_buf->index = dec_buf->index;

    if (BK_OK != rotate_task_send_msg(ROTATE_LINE_COPY_START, (uint32_t)rotate_copy_request))
    {
        LOGE("%s, malloc fail \n", __func__);
        os_free(rotate_copy_request);
    }
    return BK_OK;

    //2:dma2d isr finish send to decode notify

    //3:get dma2d pendding list to memcopy
}

//decode complete, start to rotate
static bk_err_t rotate_dec_line_complete_handler(uint32_t param)
{
	bk_err_t ret= BK_FAIL;

	pipeline_encode_request_t *rotate_notify = (pipeline_encode_request_t*)param;
	complex_buffer_t *temp_buf = rotate_get_idle_buf();
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	if (rotate_config->reset_status == true)
	{
		if (rotate_notify->buffer->index != 1)
		{
			LOGI("%s %d %d %d rot_state:%d\n", __func__, __LINE__, rotate_notify->buffer->index, rotate_config->rotate_buffer?rotate_config->rotate_buffer->index:0, rotate_config->state);
		}
	}
	if (rotate_config->state != ROTATE_STATE_IDLE
		|| temp_buf == NULL)
	{
	    LOGD("===add rotate_pedding_list===>>>>>%s %d %d %p \n", __func__, __LINE__, rotate_config->state, temp_buf);
		list_add_tail(&rotate_notify->list, &rotate_config->rotate_pedding_list);
		GLOBAL_INT_RESTORE();
		return ret;
	}
	else
	{
		rotate_config->state = ROTATE_STATE_ENCODING;
	}
	GLOBAL_INT_RESTORE();

	if(rotate_notify->buffer->index == 1)
	{
		rotate_config->jpeg_width = rotate_notify->width;
		rotate_config->jpeg_height = rotate_notify->height;
		rotate_config->reset_status = false;
	}

	if (rotate_notify->buffer->index == (rotate_config->jpeg_height / PIPELINE_DECODE_LINE))
	{
		rotate_config->err_frame = !(rotate_notify->buffer->ok);
	}

	rotate_config->rotate_buffer = temp_buf;
    rotate_config->rotate_buffer->index = rotate_notify->buffer->index;

	rotate_config->rotate_buffer->state = BUF_ROTATING;

	rotate_config->rotate_buffer->index = rotate_notify->buffer->index;

	if (rotate_config->decoder_buffer)
	{
		LOGE("%s decoder_buffer error NOT NULL\n", __func__);
		goto out;
	}
	else
	{
		rotate_config->decoder_buffer = (complex_buffer_t*)os_malloc(sizeof(complex_buffer_t));

		if (rotate_config->decoder_buffer == NULL)
		{
			LOGE("%s decoder_buffer malloc failed\n", __func__);
			goto out;
		}

		os_memcpy(rotate_config->decoder_buffer, rotate_notify->buffer, sizeof(complex_buffer_t));
	}

	ROTATE_LINE_START();

	if (!rtos_is_oneshot_timer_running(&rotate_timer))
	{
		rtos_start_oneshot_timer(&rotate_timer);
	}

	int (*func)(unsigned char *vuyy, unsigned char *rotatedVuyy, int width, int height);
    switch (rotate_config->rot_angle)
    {
        case ROTATE_90:
            default:
            func = yuyv_rotate_degree90_to_yuyv;
            break;
        case ROTATE_270:
            func = yuyv_rotate_degree270_to_yuyv;
            break;
        case ROTATE_180:
            yuyv_rotate_degree180_to_yuyv(rotate_notify->buffer->data, rotate_config->rotate_buffer->data, rotate_config->jpeg_width, PIPELINE_DECODE_LINE);
            rotate_task_send_msg(ROTATE_FINISH, (uint32_t)rotate_config->rotate_buffer);
            goto out;
    }

	if (rotate_config->rot_mode == SW_ROTATE)   //sw rotate
	{
	    if (rotate_config->rot_angle == ROTATE_NONE)
        {
            rotate_config->rotate_buffer->state = BUF_IDLE;

            //yuv-->DMA2D-->rgb888 or yuv-->copy to psram-->yuv
            rotate_task_send_msg(ROTATE_NO_ROTATE_DIRECT_COPY, (uint32_t)rotate_config->decoder_buffer);
        }
        else
        {
            func(rotate_notify->buffer->data, rotate_config->rotate_buffer->data, rotate_config->jpeg_width, PIPELINE_DECODE_LINE);
            rotate_task_send_msg(ROTATE_FINISH, (uint32_t)rotate_config->rotate_buffer);
        }
    }
    else
	{
		rott_config_t rott_cfg = {0};
		rott_cfg.input_addr = rotate_notify->buffer->data;
		rott_cfg.output_addr = rotate_config->rotate_buffer->data;
		rott_cfg.rot_mode = rotate_config->rot_angle;
		rott_cfg.input_fmt = PIXEL_FMT_YUYV;
		rott_cfg.input_flow = ROTT_INPUT_NORMAL;
		rott_cfg.output_flow = ROTT_OUTPUT_NORMAL;
		rott_cfg.picture_xpixel = rotate_notify->width;
		rott_cfg.picture_ypixel = PIPELINE_DECODE_LINE;

		/* config twice for register sync */
		ret = rott_config(&rott_cfg);

		if (ret != BK_OK)
		{
			LOGE(" rott_config ERR\n");
		}

		ret = bk_rott_enable();

		rotate_config->rotate_ena = 1;

		if (ret != BK_OK)
		{
			LOGE("rotate enable failed\n");
		}
    }
out:
	os_free(rotate_notify);
	rotate_notify = NULL;
	return ret;
}

bk_err_t rotate_task_deinit(void)
{
	int ret =BK_OK;

	LOGI("%s\n", __func__);
	bk_dma2d_driver_deinit();
	bk_rott_driver_deinit();

	LOGI("%s complete\n", __func__);
	return ret;
}

static void rotate_main(beken_thread_arg_t data)
{
	int ret = BK_OK;
	rotate_config->task_running = true;
	LOGI("%s %d\n", __func__, __LINE__);

	rtos_set_semaphore(&rotate_config->rot_sem);

	while (rotate_config->task_running)
	{
		media_msg_t msg;
		ret = rtos_pop_from_queue(&rotate_config->rotate_queue, &msg, BEKEN_WAIT_FOREVER);

		if (ret == BK_OK)
		{
			switch (msg.event)
			{
				case ROTATE_DEC_LINE_NOTIFY:
				{
					rotate_dec_line_complete_handler(msg.param);
					break;
				}
				case ROTATE_FINISH:
					rotate_finish_handler(msg.param);  //send to dma2d/DECODE
					break;
                case ROTATE_NO_ROTATE_DIRECT_COPY:
                    rotate_no_rotate_direct_copy_handler(msg.param);
                    break;

				case ROTATE_LINE_COPY_START:
					rotate_memcopy_handler(msg.param);
					break;

				case ROTATE_RESET:
					rotate_clear_status();
					break;

				case ROTATE_STOP:
				{
					LOGI("%s exit\n", __func__);
					rotate_config->task_running = 0;

					if (rtos_is_oneshot_timer_running(&rotate_timer))
					{
						rtos_stop_oneshot_timer(&rotate_timer);
					}

					if (rtos_is_oneshot_timer_init(&rotate_timer))
					{
						rtos_deinit_oneshot_timer(&rotate_timer);
					}

					beken_semaphore_t *beken_semaphore = (beken_semaphore_t*)msg.param;
					rtos_deinit_queue(&rotate_config->rotate_queue);
					rotate_config->rotate_queue = NULL;
					rotate_config->rotate_thread = NULL;
					rtos_set_semaphore(beken_semaphore);
					rtos_delete_thread(NULL);
				}
				break;

				default:
					break;
			}
		}
	}
}
 
static bk_err_t rotate_init(media_rotate_mode_t mode)
{
	LOGI("%s %d\n", __func__, __LINE__);
	if (mode == HW_ROTATE)
	{
		bk_rott_driver_init();
		bk_rott_int_enable(ROTATE_COMPLETE_INT | ROTATE_CFG_ERR_INT | ROTATE_WARTERMARK_INT, 1);
		bk_rott_isr_register(ROTATE_COMPLETE_INT, rotate_complete_cb);
		bk_rott_isr_register(ROTATE_WARTERMARK_INT, rotate_watermark_cb);
		bk_rott_isr_register(ROTATE_CFG_ERR_INT, rotate_cfg_err_cb);
	}
	bk_dma2d_driver_init();
//	dma2d_driver_transfes_ability(TRANS_128BYTES);
	bk_dma2d_int_enable(DMA2D_CFG_ERROR | DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE,1);
	bk_dma2d_register_int_callback_isr(DMA2D_CFG_ERROR_ISR, dma2d_config_error);
	bk_dma2d_register_int_callback_isr(DMA2D_TRANS_ERROR_ISR, dma2d_transfer_error);
	bk_dma2d_register_int_callback_isr(DMA2D_TRANS_COMPLETE_ISR, dma2d_transfer_complete);
	return BK_OK;
}

bool check_rotate_task_is_open(void)
{
	if (rotate_config == NULL)
	{
		return false;
	}
	else
	{
		return rotate_config->task_running;
	}
}



/**
 * @brief rotate select 
 * params: rotate mode
 *         rotate angle
 *         rotate_fmt (d:default)
 *       __________________________________
 *      | Lcd Input |    SW     |    HW    |
 *      |———————————|———————————|——————————|
 *      |           |  RGB888   |          |
 *      |  rot 0    |  YUYV(d)  |  RGB565  |
 *      |———————————|———————————|——————————|
 *      |  rot 90   |  RGB888   |          |
 *      |  rot 270  |  YUYV(d)  |  RGB565  |
 *      |___________|___________|__________|
 */

bk_err_t rotate_task_open(rot_open_t *rot_open)
{
	int ret = BK_OK;

	rtos_lock_mutex(&rotate_info->lock);

	LOGI("%s %d\n", __func__, __LINE__);

	if (rotate_config != NULL && rotate_config->task_running)
	{
		LOGE("%s, rotate task have been opened!\r\n", __func__);
		rtos_unlock_mutex(&rotate_info->lock);
		return ret;
	}

	rotate_config = (rotate_config_t *)os_malloc(sizeof(rotate_config_t));
	if (rotate_config == NULL)
	{
		LOGE("%s, malloc rotate_config failed\r\n", __func__);
		rtos_unlock_mutex(&rotate_info->lock);
		return BK_FAIL;
	}

	os_memset(rotate_config, 0, sizeof(rotate_config_t));

	INIT_LIST_HEAD(&rotate_config->rotate_pedding_list);
	INIT_LIST_HEAD(&rotate_config->copy_pedding_list);

	rotate_config->psram_overwrite_id = bk_psram_alloc_write_through_channel();

	if (!rtos_is_oneshot_timer_init(&rotate_timer))
	{
		ret = rtos_init_oneshot_timer(&rotate_timer, 5 * 1000, rotate_timer_handle, NULL, NULL);

		if (ret != BK_OK)
		{
			LOGE("create rotate timer failed\n");
		}
	}

	rotate_config->buf[0].data = mux_sram_buffer->rotate;
	rotate_config->buf[1].data = mux_sram_buffer->rotate + ROTATE_MAX_PIPELINE_LINE_SIZE;
	LOGI("%s rot_sram (%p, %p, %p)\n", __func__, mux_sram_buffer->rotate, rotate_config->buf[0].data, rotate_config->buf[1].data);

	ROTATE_LINE_END();
	DMA2D_LINE_END();

	ret = rotate_init(rot_open->mode);

	if(ret != BK_OK)
	{
		LOGE("%s, rotate pipeline init fail\r\n", __func__);
		return ret;
	}

    ///APK select LCD out format is RGB888
    if (rot_open->angle == ROTATE_180)
    {
        rotate_config->fmt = PIXEL_FMT_YUYV;
    }
    else if (rot_open->mode == SW_ROTATE)
    {
        rotate_config->rot_mode = SW_ROTATE;
        if (rot_open->angle == ROTATE_NONE)
        {
            ///yuyv-->dma2d-->yuyv
            rotate_config->fmt = PIXEL_FMT_YUYV;
            ///yuyv-->dma2d-->rgb888
//            rotate_config->fmt = PIXEL_FMT_RGB888;
        }
        else
        {
#if CONFIG_SW_ROTATE_TO_YUYV_AND_DMA2D_TO_YUYV_NOT_RGB888
            ///yuyv -->sw rotate-->yuyv-->DMA2D -->yuyv-->DISPLAY==>rgb888
            rotate_config->fmt = PIXEL_FMT_YUYV;  //5
#else
            ///(dafault) yuyv -->sw rotate-->yuyv-->DMA2D -->RGB888-->DISPLAY==>rgb888
            rotate_config->fmt = PIXEL_FMT_RGB888; //25
#endif
        }
    }
    else
    {
        rotate_config->rot_mode = HW_ROTATE;
        ///yuyv-->hw rotate-->RGB565
        rotate_config->fmt = rot_open->fmt; //RGB656_LE 22
    }
    LOGI("%s, mode %d(1:sw, 2:hw) angle(0:0, 1:90,2:180,3:270) %d, fmt:%d(5:yuv, 22:rgb565_LE, 25:rgb888)\r\n", __func__, rotate_config->rot_mode, rot_open->angle, rotate_config->fmt);

	rotate_config->rot_angle = rot_open->angle;

	ret = rtos_init_semaphore(&rotate_config->rot_sem, 1);

	if (ret != BK_OK)
	{
		LOGE("%s, init jdec_config->rot_sem failed\r\n", __func__);
		goto error;
	}

	if (rotate_config->rotate_queue != NULL)
	{
		LOGE("%s, rotate_config->rotate_queue already init, exit!\n", __func__);
		goto error;
	}

	if (rotate_config->rotate_thread != NULL)
	{
		LOGE("%s, rotate_config->rotate_thread already init, exit!\n", __func__);
		goto error;
	}

	ret = rtos_init_queue(&rotate_config->rotate_queue,
							"rotate_queue",
							sizeof(media_msg_t),
							10);

	if (ret != BK_OK)
	{
		LOGE("%s, init rot_queue failed\r\n", __func__);
		goto error;
	}

	ret = rtos_create_thread(&rotate_config->rotate_thread,
						BEKEN_DEFAULT_WORKER_PRIORITY,
						"rotate_thread",
						(beken_thread_function_t)rotate_main,
						1024 * 2,
						NULL);

	if (ret != BK_OK)
	{
		LOGE("%s, create rotate thread failed\r\n", __func__);
		goto error;
	}
	rtos_get_semaphore(&rotate_config->rot_sem, BEKEN_NEVER_TIMEOUT);

	rotate_config->enable = true;

	LOGI("%s complete\n", __func__);

	rtos_unlock_mutex(&rotate_info->lock);

	return ret;
error:

	LOGE("%s error\n", __func__);
	if (rotate_config->rotate_queue)
	{
		rtos_deinit_queue(&rotate_config->rotate_queue);
		rotate_config->rotate_queue = NULL;
	}

    if (rotate_config)
    {
        os_free(rotate_config);
        rotate_config = NULL;
    }

	rtos_unlock_mutex(&rotate_info->lock);

	return BK_FAIL;
}


void rotate_task_stop(void)
{
	beken_semaphore_t sem;
	media_msg_t msg;
	msg.event = ROTATE_STOP;
	msg.param = (uint32_t)&sem;

	int ret = rtos_init_semaphore(&sem, 1);

	if (ret != BK_OK)
	{
		LOGE("%s, init sem faild, %d\n", __func__, ret);
		return;
	}

	ret = rtos_push_to_queue(&rotate_config->rotate_queue, &msg, BEKEN_WAIT_FOREVER);

	rtos_get_semaphore(&sem, BEKEN_NEVER_TIMEOUT);
	rtos_deinit_semaphore(&sem);
}

bk_err_t rotate_task_close(void)
{
	LOGI("%s \n", __func__);

	rtos_lock_mutex(&rotate_info->lock);

	if (rotate_config == NULL || !rotate_config->task_running)
	{
		rtos_unlock_mutex(&rotate_info->lock);
		return BK_FAIL;
	}

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	rotate_config->enable = false;
	GLOBAL_INT_RESTORE();

	rotate_task_stop();
	rtos_deinit_semaphore(&rotate_config->rot_sem);
	rotate_config->rot_sem = NULL;

	rotate_task_deinit();

	if (!list_empty(&rotate_config->rotate_pedding_list))
	{
		LOGI("%s, clear rotate_pedding_list\n", __func__);

		LIST_HEADER_T *pos, *n, *list = &rotate_config->rotate_pedding_list;
		pipeline_encode_request_t *request = NULL;

		list_for_each_safe(pos, n, list)
		{
			request = list_entry(pos, pipeline_encode_request_t, list);
			if (request != NULL)
			{
				complex_buffer_t *decoder_buffer = (complex_buffer_t*)os_malloc(sizeof(complex_buffer_t));
				os_memcpy(decoder_buffer, request->buffer, sizeof(complex_buffer_t));

				rotate_config->decoder_free_cb(decoder_buffer);
				list_del(pos);
				os_free(request);
			}
		}
	}

	if (rotate_config->decoder_buffer)
	{
		rotate_config->decoder_free_cb(rotate_config->decoder_buffer);
		rotate_config->decoder_buffer = NULL;
	}

	if (!list_empty(&rotate_config->copy_pedding_list))
	{
		LOGI("%s, clear copy_pedding_list\n", __func__);

		LIST_HEADER_T *pos, *n, *list = &rotate_config->copy_pedding_list;
		rotate_copy_request_t *request = NULL;

		list_for_each_safe(pos, n, list)
		{
			request = list_entry(pos, rotate_copy_request_t, list);
			if (request != NULL)
			{
				list_del(pos);
				os_free(request);
			}
		}
	}


	if (rotate_config->rotate_frame)
	{

		frame_buffer_display_free(rotate_config->rotate_frame);
		rotate_config->rotate_frame = NULL;
		LOGI("%s free rotate_frame\n", __func__);
	}
	bk_psram_disable_write_through(rotate_config->psram_overwrite_id);
	bk_psram_free_write_through_channel(rotate_config->psram_overwrite_id);

	os_free(rotate_config);
	rotate_config = NULL;

	LOGI("%s complete\n", __func__);

	rtos_unlock_mutex(&rotate_info->lock);

	return BK_OK;
}


bk_err_t bk_rotate_encode_request(pipeline_encode_request_t *request, mux_callback_t cb)
{
	pipeline_encode_request_t *rotate_request = NULL;

	rtos_lock_mutex(&rotate_info->lock);

	if (rotate_config == NULL || rotate_config->enable == false)
	{
		LOGI("%s not open\n", __func__);
		goto error;
	}

	rotate_request = (pipeline_encode_request_t *)os_malloc(sizeof(pipeline_encode_request_t));

	if (rotate_request == NULL)
	{
		LOGI("%s malloc failed\n", __func__);
		goto error;
	}

	os_memcpy(rotate_request, request, sizeof(pipeline_encode_request_t));

	rotate_config->decoder_free_cb = cb;

	if (BK_OK != rotate_task_send_msg(ROTATE_DEC_LINE_NOTIFY, (uint32_t)rotate_request))
	{
		LOGI("%s send failed\n", __func__);
		goto error;
	}

	rtos_unlock_mutex(&rotate_info->lock);

	return BK_OK;

error:

	if (rotate_config
		&& rotate_config->decoder_free_cb)
	{
		rotate_config->decoder_free_cb = NULL;
	}

	if (rotate_request)
	{
		os_free(rotate_request);
		rotate_request = NULL;
	}

	rtos_unlock_mutex(&rotate_info->lock);
	return BK_FAIL;
}

bk_err_t bk_rotate_pipeline_init(void)
{
	bk_err_t ret = BK_FAIL;

    if(rotate_info != NULL)
    {
        os_free(rotate_info);
        rotate_info = NULL;
    }
	rotate_info = (rotate_info_t*)os_malloc(sizeof(rotate_info_t));

	if (rotate_info == NULL)
	{
		LOGE("%s malloc rotates_info failed\n", __func__);
		return BK_FAIL;
	}

	os_memset(rotate_info, 0, sizeof(rotate_info_t));

	ret = rtos_init_mutex(&rotate_info->lock);

	if (ret != BK_OK)
	{
		LOGE("%s, init mutex failed\r\n", __func__);
		return BK_FAIL;
	}

	return ret;
}

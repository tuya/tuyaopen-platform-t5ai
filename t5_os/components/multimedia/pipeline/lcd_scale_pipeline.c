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
#include "frame_buffer.h"
#include "yuv_encode.h"
#include <driver/gpio.h>
#include "media_mailbox_list_util.h"
#include "media_evt.h"
#include "yuv_encode.h"

#include <driver/media_types.h>
#include <driver/psram.h>

#include <driver/hw_scale_types.h>
#include "driver/hw_scale.h"

#include "bk_list.h"
#include "bk_list_edge.h"

#include "mux_pipeline.h"

#define TAG "scale_pipline"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


#ifdef SCALE_DIAG_DEBUG
#define HW_SCALE_FRAME_START()			do { GPIO_UP(GPIO_DVP_D5); } while (0)
#define HW_SCALE_FRAME_END()				do { GPIO_DOWN(GPIO_DVP_D5); } while (0)

#define HW_SCALE_SRC_START()			do { GPIO_UP(GPIO_DVP_D6); } while (0)
#define HW_SCALE_SRC_END()				do { GPIO_DOWN(GPIO_DVP_D6); } while (0)

#define HW_SCALE_DST_START()			do { GPIO_UP(GPIO_DVP_D7); } while (0)
#define HW_SCALE_DST_END()				do { GPIO_DOWN(GPIO_DVP_D7); } while (0)

#define ROTATE_SCALE_NOTIFY() \
do{ 			   \
	GPIO_DOWN(GPIO_DVP_VSYNC);  \
	GPIO_UP(GPIO_DVP_VSYNC);    \
	GPIO_DOWN(GPIO_DVP_VSYNC);  \
				   \
}while(0)

#else
#define HW_SCALE_FRAME_START()
#define HW_SCALE_FRAME_END()

#define HW_SCALE_SRC_START()
#define HW_SCALE_SRC_END()
#define HW_SCALE_DST_START()
#define HW_SCALE_DST_END()

#define ROTATE_SCALE_NOTIFY()
#endif


#define SCALE_DEBUG_LOG		(0)

#define HW_SCALE  HW_SCALE0

typedef struct {
	frame_buffer_t *scale_src_frame;
	LIST_HEADER_T list;
} scale_request_t;

typedef enum
{
	SCALE_STATE_IDLE = 0,
	SCALE_STATE_SCALING,
	SCALE_STATE_FRAME_COMPLETE,
	SCALE_STATE_DEST_COMPLETE,
	SCALE_STATE_SOURCE_COMPLETE,
} scale_state_t;

typedef enum {
	BUF_IDLE = 0,
	BUF_SCALING = 1,
	BUF_ROTATEING = 2,
}buf_status_t;


typedef struct{

	scale_drv_config_t drv_config;
	scale_block_t src_block;
	scale_block_t dst_block;
	scale_result_t src_result;
	scale_result_t dst_result;
} scale_param_t;


typedef struct {
	uint8_t task_running;
	uint8_t enable;
	scale_state_t state;
	uint16_t src_width;
	uint16_t dst_width;
	uint16_t src_height;
	uint16_t dst_height;
	uint8_t psram_overwrite_id;

	frame_buffer_t *scale_frame;
	frame_buffer_t *scale_src_frame;

	beken_semaphore_t scale_sem;
	beken_queue_t scale_queue;
	beken_thread_t scale_thread;

	LIST_HEADER_T scale_pedding_list;
	beken2_timer_t scale_timer;

	complex_buffer_t scale_buffer[2];

	mux_callback_t decoder_free_cb;

	complex_buffer_t *decoder_buffer;

	scale_param_t scale_param;

	LIST_HEADER_T request_list;

	uint16_t line_count;
    mux_callback_t reset_cb;

} scale_config_t;

typedef struct {
	beken_mutex_t lock;
} scale_info_t;


static scale_config_t *scale_config = NULL;
static scale_info_t *scale_info = NULL;

static complex_buffer_t *scale_get_idle_buf(void)
{
	if (scale_config->scale_buffer[0].state == BUF_IDLE)
		return &scale_config->scale_buffer[0];
	else if (scale_config->scale_buffer[1].state == BUF_IDLE)
		return &scale_config->scale_buffer[1];
	else
		return NULL;
}

bk_err_t scale_task_send_msg(uint8_t type, uint32_t param)
{
	bk_err_t ret = BK_FAIL;
	media_msg_t msg;

	if (scale_config && scale_config->enable)
	{
		msg.event = type;
		msg.param = param;
		ret = rtos_push_to_queue(&scale_config->scale_queue, &msg, BEKEN_WAIT_FOREVER);
	}

	if (BK_OK != ret)
	{
		LOGE("%s failed, type:%d\r\n", __func__, type);
	}

	return ret;
}

static void scale0_complete_cb(void *param)
{
    HW_SCALE_FRAME_END();
//    scale_task_send_msg(SCALE_FINISH, (uint32_t)scale_config->scale_frame);
}

static void scale_timer_handle(void *arg1, void *arg2)
{
    LOGW("%s %d  timeout %d [%d %d] [%d %d]\n", __func__, __LINE__,scale_config->line_count, scale_config->scale_buffer[0].index,scale_config->scale_buffer[0].state, scale_config->scale_buffer[1].index,scale_config->scale_buffer[1].state);
    scale_task_send_msg(SCALE_RESET, 1);
}


uint32_t get_scale_list_num(LIST_HEADER_T *list)
{
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

    uint32_t cnt = 0;
	LIST_HEADER_T *pos, *n;
    scale_request_t *tmp = NULL;
    pos = NULL, n = NULL;
	list_for_each_safe(pos, n, &scale_config->scale_pedding_list)
	{
		tmp = list_entry(pos, scale_request_t, list);
		if (tmp != NULL)
		{
			if (tmp->scale_src_frame != NULL)
			{
			    cnt ++;
			}
		}
	}

	GLOBAL_INT_RESTORE();
    return cnt;
}

bk_err_t scale_request_list_push(frame_buffer_t *scale_src_frame, LIST_HEADER_T *list)
{
	bk_err_t ret = BK_OK;
	scale_request_t *scale_list = os_malloc(sizeof(scale_request_t));
	if (scale_list == NULL)
	{
		return BK_ERR_NO_MEM;
	}
	scale_list->scale_src_frame = scale_src_frame;
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	list_add_tail(&scale_list->list, list);
	GLOBAL_INT_RESTORE();
	return ret;
}

frame_buffer_t *scale_request_list_pop(LIST_HEADER_T *list)
{
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	LIST_HEADER_T *pos, *n;
	frame_buffer_t *frame = NULL;
	scale_request_t *tmp = NULL;
	list_for_each_safe(pos, n, list)
	{
		tmp = list_entry(pos, scale_request_t, list);
		if (tmp != NULL)
		{
			frame = tmp->scale_src_frame;
			list_del(pos);
			os_free(tmp);
			break;
		}
	}
	GLOBAL_INT_RESTORE();
	return frame;
}

bk_err_t lcd_scale_finish(uint32_t param)
{
	if (rtos_is_oneshot_timer_running(&scale_config->scale_timer))
	{
		rtos_stop_oneshot_timer(&scale_config->scale_timer);
	}

    LOGD("%s free %p, push %p \n", __func__, scale_config->scale_src_frame->frame, scale_config->scale_frame->frame);
    frame_buffer_display_free(scale_config->scale_src_frame);
    scale_config->scale_src_frame = NULL;

    bk_psram_disable_write_through(scale_config->psram_overwrite_id);
    if (lcd_display_frame_request(scale_config->scale_frame) != BK_OK)
    {
        frame_buffer_display_free(scale_config->scale_frame);
    }
    scale_config->scale_frame = NULL;

    frame_buffer_t *request = NULL;
    request = scale_request_list_pop(&scale_config->scale_pedding_list);
    if (request != NULL)
        scale_task_send_msg(SCALE_START ,(uint32_t)request);

    scale_config->state = SCALE_STATE_IDLE;
    return BK_OK;
}


bk_err_t lcd_scale_start(uint32_t param)
{
    bk_err_t ret = BK_OK;

    frame_buffer_t *scale_src_frame = (frame_buffer_t * )param;
    if (scale_src_frame == NULL)
       return BK_FAIL;
    int num = get_scale_list_num(&scale_config->scale_pedding_list);
    if(num >= 1 || scale_config->state != SCALE_STATE_IDLE)
    {
        LOGD("%s free rotate frame \n", __func__);
        frame_buffer_display_free(scale_src_frame);
        scale_src_frame = NULL;
        return BK_FAIL;
    }
	if (scale_config->state != SCALE_STATE_IDLE)
	{
        ret = scale_request_list_push(scale_src_frame, &scale_config->scale_pedding_list);
        if(ret !=BK_OK)
            LOGE("%s push error\n", __func__);
        return ret;
	}
	else
	{
		scale_config->state = SCALE_STATE_SCALING;
	}

	if (!rtos_is_oneshot_timer_running(&scale_config->scale_timer))
	{
		rtos_start_oneshot_timer(&scale_config->scale_timer);
	}
    //all frame size is fixed, free and re-malloc not change real size, so malloc scale size is equal to rotate size
    scale_config->scale_frame = frame_buffer_display_malloc(scale_src_frame->width * scale_src_frame->height * 2);
    if(scale_config->scale_frame == NULL)
    {
        LOGE("scale frame malloc fail\n");
        goto error;
    }
    HW_SCALE_FRAME_START();

    LOGD("%s, src %p, dst %p\n", __func__, scale_src_frame->frame, scale_config->scale_frame->frame);
    scale_config->scale_src_frame = scale_src_frame;
    scale_config->scale_frame->width = scale_config->dst_width;
    scale_config->scale_frame->height = scale_config->dst_height;
    scale_config->scale_frame->fmt = scale_src_frame->fmt;
    scale_config->scale_frame->size = scale_config->dst_width * scale_config->dst_height * 2;
    bk_psram_enable_write_through(scale_config->psram_overwrite_id, (uint32_t)scale_config->scale_frame->frame,
        (uint32_t)(scale_config->scale_frame->frame + scale_config->scale_frame->size));

	scale_drv_config_t scale_drv_config;
	scale_drv_config.src_width = scale_src_frame->width;
	scale_drv_config.dst_width = scale_config->dst_width;
	scale_drv_config.src_height = scale_src_frame->height;
	scale_drv_config.dst_height = scale_config->dst_height;
	scale_drv_config.src_addr = scale_src_frame->frame;
	scale_drv_config.dst_addr = scale_config->scale_frame->frame;
	scale_drv_config.pixel_fmt = scale_src_frame->fmt;
	scale_drv_config.scale_mode = FRAME_SCALE;

	ret = hw_scale_frame(HW_SCALE, &scale_drv_config);
    if(ret != BK_OK)
        goto error;
    return ret;

error:
    LOGE("%s error.\n", __func__);

    if(scale_config->scale_frame)
    {
        bk_psram_disable_write_through(scale_config->psram_overwrite_id);
        frame_buffer_display_free(scale_config->scale_frame);
        scale_config->scale_frame = NULL;
        LOGE("%s free scale_frame\n", __func__);
    }

    return BK_FAIL;
}


void scale_frame_complete(scale_result_t *src, scale_result_t *dst, scale_block_t *src_block, scale_block_t *dst_block)
{
    HW_SCALE_FRAME_END();
    HW_SCALE_SRC_END();
    HW_SCALE_DST_END();
    if (rtos_is_oneshot_timer_running(&scale_config->scale_timer))
    {
        rtos_stop_oneshot_timer(&scale_config->scale_timer);
    }
	scale_param_t *scale_param = &scale_config->scale_param;

#if SCALE_DEBUG_LOG
	LOGI("%s, %d, [%d, %d] [%d, %d]\n",
		__func__,
		__LINE__,
		src->current_frame_line, src->current_block_line,
		dst->current_frame_line, dst->current_block_line);
#endif

	os_memcpy(&scale_param->src_result, src, sizeof(scale_result_t));
	os_memcpy(&scale_param->dst_result, dst, sizeof(scale_result_t));

	scale_task_send_msg(SCALE_LINE_SOURCE_FREE, (uint32_t)src_block->args);
	scale_task_send_msg(SCALE_LINE_DEST_FREE, (uint32_t)dst_block->args);

	scale_config->state = SCALE_STATE_FRAME_COMPLETE;
	scale_task_send_msg(SCALE_LINE_START_LOOP, 0);

}

void scale_frame_source_block_complete(scale_result_t *result, scale_block_t *block)
{
    HW_SCALE_SRC_END();

	scale_param_t *scale_param = &scale_config->scale_param;

#if SCALE_DEBUG_LOG
	LOGI("%s, [%d %d %d %d]\n", __func__,
		result->current_frame_line,
		result->current_frame_line,
		result->current_block_line,
		result->next_block_line);
#endif

	os_memcpy(&scale_param->src_result, result, sizeof(scale_result_t));
	scale_task_send_msg(SCALE_LINE_SOURCE_FREE, (uint32_t)block->args);
}

void scale_frame_dest_block_complete(scale_result_t *result, scale_block_t *block)
{
    HW_SCALE_DST_END();
    if (rtos_is_oneshot_timer_running(&scale_config->scale_timer))
    {
        rtos_stop_oneshot_timer(&scale_config->scale_timer);
    }
	scale_param_t *scale_param = &scale_config->scale_param;

#if SCALE_DEBUG_LOG
	LOGI("%s, [%d %d %d %d]\n", __func__,
		result->current_frame_line,
		result->current_frame_line,
		result->current_block_line,
		result->next_block_line);
#endif
	os_memcpy(&scale_param->dst_result, result, sizeof(scale_result_t));
	scale_task_send_msg(SCALE_LINE_DEST_FREE, (uint32_t)block->args);
}

void scale_block_result( scale_block_t *src_block, scale_block_t *dst_block)
{
    complex_buffer_t * src_block_args = (complex_buffer_t *)src_block->args;
    complex_buffer_t * dst_block_args = (complex_buffer_t *)dst_block->args;
    dst_block_args->ok =  src_block_args->ok;
}

bk_err_t scale_rotate_line_request_callback(void *param)
{
    ROTATE_SCALE_NOTIFY();

	scale_task_send_msg(SCALE_LINE_SCALE_COMPLETE, (uint32_t)param);
	return BK_OK;
}


bk_err_t lcd_scale_line_fill_start(complex_buffer_t *decoder_buffer, complex_buffer_t *scale_buffer)
{
	bk_err_t ret = BK_FAIL;

	scale_buffer->state = BUF_SCALING;

    HW_SCALE_FRAME_START();
    HW_SCALE_SRC_START();
    HW_SCALE_DST_START();
	if (!rtos_is_oneshot_timer_running(&scale_config->scale_timer))
	{
		rtos_start_oneshot_timer(&scale_config->scale_timer);
	}
	if (decoder_buffer->index == 1)
	{
		scale_param_t *scale_param = &scale_config->scale_param;

		os_memset(scale_param, 0 , sizeof(scale_param_t));

		scale_param->drv_config.src_width = scale_config->src_width;
		scale_param->drv_config.dst_width = scale_config->dst_width;
		scale_param->drv_config.src_height = scale_config->src_height;
		scale_param->drv_config.dst_height = scale_config->dst_height;
		scale_param->drv_config.src_addr = (uint8_t *)decoder_buffer->data;
		scale_param->drv_config.dst_addr = (uint8_t *)scale_buffer->data;
		scale_param->drv_config.pixel_fmt = PIXEL_FMT_YUYV;
		scale_param->drv_config.scale_mode = BLOCK_SCALE;
		scale_param->drv_config.frame_complete = scale_frame_complete;
		scale_param->drv_config.source_block_complete = scale_frame_source_block_complete;
		scale_param->drv_config.dest_block_complete = scale_frame_dest_block_complete;
		scale_param->drv_config.scale_block_result = scale_block_result;
		scale_param->drv_config.line_cycle = 16;
		scale_param->drv_config.line_mask = 0x1F;
		scale_param->src_block.line_index = 0;
		scale_param->src_block.line_count = 16;
		scale_param->src_block.data = scale_param->drv_config.src_addr;
		scale_param->src_block.args = decoder_buffer;

		scale_param->dst_block.line_index = 0;
		scale_param->dst_block.line_count = 16;
		scale_param->dst_block.data = scale_param->drv_config.dst_addr;
		scale_param->dst_block.args = scale_buffer;

#if SCALE_DEBUG_LOG
		LOGI("%s %dX%d -> %dX%d\n", __func__,
			scale_param->drv_config.src_width,
			scale_param->drv_config.src_height,
			scale_param->drv_config.dst_width,
			scale_param->drv_config.dst_height);
#endif

		ret = hw_scale_block_config(HW_SCALE, &scale_param->drv_config);

		if (ret != BK_OK)
		{
			LOGE("%s scale config err: %d\n", __func__, ret);
			return ret;
		}

		ret = hw_scale_block_start(HW_SCALE, &scale_param->src_block, &scale_param->dst_block);

		if (ret != BK_OK)
		{
			LOGE("%s scale start err: %d\n", __func__, ret);
			return ret;
		}
	}
	else
	{
		LOGD("%s should not be here. decoder_buffer->index=%d\n", __func__, decoder_buffer->index);
	}

	return ret;
}

bk_err_t lcd_scale_line_state_machine(scale_state_t state, void *args)
{
	bk_err_t ret = BK_FAIL;

	switch (state)
	{
		case SCALE_STATE_IDLE:
		case SCALE_STATE_FRAME_COMPLETE:
		{

			complex_buffer_t *scale_buffer = NULL;
			pipeline_encode_request_t *scale_request = NULL;

			scale_buffer = scale_get_idle_buf();

			if (scale_buffer == NULL)
			{
				LOGD("%s scale_buffer NULL: %d\n", __func__, __LINE__);
				break;
			}

			GLOBAL_INT_DECLARATION();
			GLOBAL_INT_DISABLE();
			scale_request = list_pop_edge(&scale_config->request_list, pipeline_encode_request_t, list);
			GLOBAL_INT_RESTORE();

			if (scale_request == NULL)
			{
#if SCALE_DEBUG_LOG
				LOGE("%s scale_request NULL: %d\n", __func__, __LINE__);
#endif
				scale_config->state = SCALE_STATE_IDLE;
				break;
			}

			if (scale_config->decoder_buffer)
			{
				LOGE("%s decoder_buffer not NULL\n", __func__);
				break;
			}

			scale_config->decoder_buffer = (complex_buffer_t*)os_malloc(sizeof(complex_buffer_t));
			os_memcpy(scale_config->decoder_buffer, scale_request->buffer, sizeof(complex_buffer_t));

			scale_config->state = SCALE_STATE_SCALING;
			scale_config->line_count = 1;
			scale_buffer->state = BUF_SCALING;
			scale_buffer->index = scale_config->line_count;
            scale_config->src_width = scale_request->width;
            scale_config->src_height = scale_request->height;

#if SCALE_DEBUG_LOG
			LOGI("%s %d, fill src: %d\n",  __func__, __LINE__, scale_config->decoder_buffer->index);
#endif
			ret = lcd_scale_line_fill_start(scale_config->decoder_buffer, scale_buffer);

			os_free(scale_request);
            if (ret != BK_OK)
            {
                scale_buffer->state = BUF_IDLE;
                scale_config->state = SCALE_STATE_IDLE;
				LOGI("%s state %x invalid index: %d\n", __func__,state, scale_config->decoder_buffer->index);
                scale_task_send_msg(SCALE_RESET, 1);
            }
		}
		break;

		case SCALE_STATE_SOURCE_COMPLETE:
		{
			if (scale_config->decoder_buffer)
			{
				LOGD("%s %d decoder decoder_buffer not NULL %p %p %d  %d\n", __func__, __LINE__, scale_config->decoder_buffer, scale_config->decoder_buffer->data, scale_config->decoder_buffer->index, scale_config->decoder_buffer->state);
				break;
			}

			GLOBAL_INT_DECLARATION();
			GLOBAL_INT_DISABLE();
			pipeline_encode_request_t *scale_request = list_pop_edge(&scale_config->request_list, pipeline_encode_request_t, list);
			GLOBAL_INT_RESTORE();

			if (scale_request == NULL)
			{
				scale_config->state = SCALE_STATE_SOURCE_COMPLETE;
				LOGD("%s %d SCALE_STATE_SOURCE_COMPLETE scale_request == NULL\n", __func__, __LINE__);
				break;
			}
            HW_SCALE_SRC_START();

			scale_config->decoder_buffer = (complex_buffer_t*)os_malloc(sizeof(complex_buffer_t));
			os_memcpy(scale_config->decoder_buffer, scale_request->buffer, sizeof(complex_buffer_t));

			scale_block_t scale_block;
			scale_block.data = scale_config->decoder_buffer->data;
			scale_block.line_index = 0;
			scale_block.line_count = 16;
			scale_block.args = scale_config->decoder_buffer;

			scale_config->state = SCALE_STATE_SCALING;

#if SCALE_DEBUG_LOG
            if(scale_config->decoder_buffer->index == (scale_config->decoder_buffer->frame_buffer->height / PIPELINE_DECODE_LINE))
    		    LOGI("fill src: %d, id: %d result:%d \n", scale_config->decoder_buffer->index, scale_config->decoder_buffer->id, scale_config->decoder_buffer->ok);
#endif

            ret = hw_scale_source_block_fill(HW_SCALE, &scale_block);

			if (ret != BK_OK)
			{
				LOGD("%s scale err: %d\n", __func__, ret);
                os_free(scale_request);
				break;
			}

			os_free(scale_request);
		}
		break;

		case SCALE_STATE_DEST_COMPLETE:
		{
			complex_buffer_t *scale_buffer = NULL;

			scale_buffer = scale_get_idle_buf();

			if (scale_buffer == NULL)
			{
				scale_config->state = SCALE_STATE_DEST_COMPLETE;
    			LOGD("%s %d SCALE_STATE_DEST_COMPLETE idle buffer NULL[%d %d] [%d %d]\n", __func__, __LINE__, scale_config->scale_buffer[0].index,scale_config->scale_buffer[0].state, scale_config->scale_buffer[1].index,scale_config->scale_buffer[1].state);
				break;
			}


			scale_config->line_count++;
			scale_buffer->state = BUF_SCALING;
			scale_buffer->index = scale_config->line_count;
			scale_block_t scale_block;
			scale_block.data = scale_buffer->data;
			scale_block.line_index = 0;
			scale_block.line_count = 16;
			scale_block.args = scale_buffer;
			scale_config->state = SCALE_STATE_SCALING;
			ret = hw_scale_dest_block_fill(HW_SCALE, &scale_block);

			if (ret != BK_OK)
			{
				LOGE("%s scale err: %d\n", __func__, ret);
				break;
			}
            HW_SCALE_DST_START();
            
            if (!rtos_is_oneshot_timer_running(&scale_config->scale_timer))
            {
                rtos_start_oneshot_timer(&scale_config->scale_timer);
            }
            
		}
		break;

		case SCALE_STATE_SCALING:
		{
#if SCALE_DEBUG_LOG
			LOGI("%s scale busy\n", __func__);
#endif
		}
		break;

	}

	return BK_OK;
}

bk_err_t lcd_scale_line_start_request_handle(pipeline_encode_request_t *scale_request)
{
	if (scale_request == NULL)
	{
		LOGE("%s scale_request NULL\n", __func__);
		return BK_FAIL;
	}

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	list_add_tail(&scale_request->list, &scale_config->request_list);
	GLOBAL_INT_RESTORE();

	if (BK_OK != scale_task_send_msg(SCALE_LINE_START_LOOP, 0))
	{
		LOGI("%s send failed\n", __func__);
		return BK_FAIL;
	}
	return BK_OK;
}

static void scale_main_entry(beken_thread_arg_t data)
{
	int ret = BK_OK;
	scale_config->task_running = true;
	LOGI("%s %d\n", __func__, __LINE__);

	rtos_set_semaphore(&scale_config->scale_sem);

	while (scale_config->task_running)
	{
		media_msg_t msg;
		ret = rtos_pop_from_queue(&scale_config->scale_queue, &msg, BEKEN_WAIT_FOREVER);

		if (ret == BK_OK)
		{
			switch (msg.event)
			{
				case SCALE_START:
					lcd_scale_start(msg.param);
					break;
				case SCALE_FINISH:
					lcd_scale_finish(msg.param);  //send to dma2d/DECODE
					break;

				case SCALE_LINE_START_LOOP:
				{
#if SCALE_DEBUG_LOG
					LOGI("%s, state: %d, %d \n", __func__, scale_config->state, scale_config->line_count);
#endif
					lcd_scale_line_state_machine(scale_config->state, (void*)msg.param);
				}
				break;

				case SCALE_LINE_SOURCE_FREE:
				{
					complex_buffer_t *decoder_buffer = (complex_buffer_t*)msg.param;
                    
#if SCALE_DEBUG_LOG
                    if(decoder_buffer->index == decoder_buffer->frame_buffer->height / PIPELINE_DECODE_LINE)
                    {
                        LOGI("decode free: %d, %d\n", decoder_buffer->index, decoder_buffer->ok);
                    }
#endif


                    if (scale_config->decoder_buffer != decoder_buffer)
                    {
                      LOGE("%s, %d, source complete buffer not match %p %p %d\n", __func__, __LINE__, decoder_buffer, scale_config->decoder_buffer, decoder_buffer->index);
                    }
					if (decoder_buffer)
					{
                        if (decoder_buffer->index != scale_config->src_height / PIPELINE_DECODE_LINE)
                        {
                            scale_config->state = SCALE_STATE_SOURCE_COMPLETE;
                            scale_task_send_msg(SCALE_LINE_START_LOOP, 0);
                        }
    				    scale_config->decoder_free_cb(decoder_buffer);
                        scale_config->decoder_buffer = NULL;
					}
					else
					{
						LOGE("%s,%d, source complete buffer NULL\n", __func__, __LINE__);
					}
				}
				break;

				case SCALE_LINE_DEST_FREE:
				{
					complex_buffer_t *scale_buffer = (complex_buffer_t*)msg.param;

					if (scale_buffer)
					{
						pipeline_encode_request_t request;
						request.jdec_type = 0;
						request.width = scale_config->dst_width;
						request.height = scale_config->dst_height;
						request.buffer = scale_buffer;
                        scale_buffer->state = BUF_ROTATEING;
#if SCALE_DEBUG_LOG
                        if(scale_buffer->index == request.height / PIPELINE_DECODE_LINE)
                        {
						    LOGI("rotate request: %d, %p, %p %d\n", scale_buffer->index, scale_buffer, scale_buffer->data, request.buffer->ok);
                        }
#endif
						bk_rotate_encode_request(&request, scale_rotate_line_request_callback);
                        if (scale_buffer->index != scale_config->dst_height / PIPELINE_DECODE_LINE)
                        {
                            scale_config->state = SCALE_STATE_DEST_COMPLETE;
                            scale_task_send_msg(SCALE_LINE_START_LOOP, 0);
                        }
					}
					else
					{
						LOGE("%s error unknow request NULL index=%d\n", __func__, scale_buffer->index);
					}

				}
				break;

				case SCALE_LINE_SCALE_COMPLETE:
				{
					complex_buffer_t *buffer = (complex_buffer_t*)msg.param;
					complex_buffer_t *scale_buffer = NULL;

					for (int i = 0; i < 2; i++)
					{
						if (scale_config->scale_buffer[i].data == buffer->data)
						{
							scale_buffer = &scale_config->scale_buffer[i];
							break;
						}
					}

					if (scale_buffer == NULL)
					{
						LOGE("%s scale_buffer NULL\n", __func__);
						break;
					}
					scale_buffer->state = BUF_IDLE;

#if SCALE_DEBUG_LOG
					LOGI("%s %d rotate_complete_cb[%d %p %p]\n", __func__, __LINE__, scale_buffer->index, scale_buffer, scale_buffer->data);
#endif

					os_free(buffer);

					scale_task_send_msg(SCALE_LINE_START_LOOP, 0);
				}
				break;

				case SCALE_RESET:
					if (rtos_is_oneshot_timer_running(&scale_config->scale_timer))
					{
						rtos_stop_oneshot_timer(&scale_config->scale_timer);
					}
                    bk_hw_scale_int_enable(HW_SCALE, 0);
                    bk_hw_scale_stop(HW_SCALE);
                    
                    GLOBAL_INT_DECLARATION();
                    GLOBAL_INT_DISABLE();
                    while (!list_empty(&scale_config->request_list))
                    {
                        pipeline_encode_request_t *scale_request = list_pop_edge(&scale_config->request_list, pipeline_encode_request_t, list);
                    
                        if (scale_request != NULL)
                        {
                            os_free(scale_request);
                            LOGI("%s %d SCALE_RESET\n", __func__, __LINE__);
                        }
                        else
                        {
                            break;
                        }
                    }
                    GLOBAL_INT_RESTORE();

                    HW_SCALE_FRAME_END();
                    HW_SCALE_SRC_END();
                    HW_SCALE_DST_END();
                    scale_config->state = SCALE_STATE_IDLE;
                    scale_config->scale_buffer[0].state = BUF_IDLE;
                    scale_config->scale_buffer[1].state = BUF_IDLE;
					LOGE("%s SCALE_RESET line_count%d  scale_config->state %x\n", __func__, scale_config->line_count, scale_config->state);
                    if(scale_config->reset_cb && (msg.param == 0))
                        scale_config->reset_cb(NULL);
                   break;

				case SCALE_STOP:
				{
					LOGI("%s exit\n", __func__);
					scale_config->task_running = 0;
					beken_semaphore_t *beken_semaphore = (beken_semaphore_t*)msg.param;

					bk_hw_scale_driver_deinit(HW_SCALE);
                    //bk_hw_scale_mem_free();
					if (rtos_is_oneshot_timer_running(&scale_config->scale_timer))
					{
						rtos_stop_oneshot_timer(&scale_config->scale_timer);
					}

					if (rtos_is_oneshot_timer_init(&scale_config->scale_timer))
					{
						rtos_deinit_oneshot_timer(&scale_config->scale_timer);
					}

					do {
						ret = rtos_pop_from_queue(&scale_config->scale_queue, &msg, BEKEN_NO_WAIT);

						if (ret != BK_OK)
						{
							break;
						}

						if (msg.event == SCALE_LINE_SCALE_COMPLETE)
						{
							complex_buffer_t *buffer = (complex_buffer_t*)msg.param;
							os_free(buffer);
						}

					} while (true);

					rtos_deinit_queue(&scale_config->scale_queue);
					scale_config->scale_queue = NULL;
					scale_config->scale_thread = NULL;
					rtos_set_semaphore(beken_semaphore);
					rtos_delete_thread(NULL);
				}
				return;

				default:
					break;
			}
		}
	}
}

bk_err_t scale_task_open(lcd_scale_t *lcd_scale)
{
	int ret =BK_OK;

	rtos_lock_mutex(&scale_info->lock);

	if (scale_config != NULL && scale_config->task_running)
	{
		LOGE("%s, scale task have been opened!\r\n", __func__);
		rtos_unlock_mutex(&scale_info->lock);
		return ret;
	}

	scale_config = (scale_config_t *)os_malloc(sizeof(scale_config_t));

	if (scale_config == NULL)
	{
		LOGE("%s, malloc scale_config failed\r\n", __func__);
		rtos_unlock_mutex(&scale_info->lock);
		return BK_FAIL;
	}

	os_memset(scale_config, 0, sizeof(scale_config_t));

	INIT_LIST_HEAD(&scale_config->scale_pedding_list);

	if (!rtos_is_oneshot_timer_init(&scale_config->scale_timer))
	{
		ret = rtos_init_oneshot_timer(&scale_config->scale_timer, 300, scale_timer_handle, NULL, NULL);

		if (ret != BK_OK)
		{
			LOGE("create scale timer failed\n");
		}
	}
	HW_SCALE_FRAME_END();
    HW_SCALE_SRC_END();
    HW_SCALE_DST_END();

	ret = bk_hw_scale_driver_init(HW_SCALE);
	if(ret != BK_OK)
	{
		LOGE("%s, scale pipeline init fail\r\n", __func__);
		goto error;
	}
    ///frame scale
	bk_hw_scale_isr_register(HW_SCALE, scale0_complete_cb, NULL);

	scale_config->src_width = lcd_scale->src_ppi >> 16;
	scale_config->src_height = lcd_scale->src_ppi & 0xFFFF;
	scale_config->dst_width = lcd_scale->dst_ppi >> 16;
	scale_config->dst_height = lcd_scale->dst_ppi & 0xFFFF;

    if(scale_config->dst_width == 0 || scale_config->dst_height == 0)
    {
        scale_config->dst_width = PIXEL_480;
        scale_config->dst_height = PIXEL_854;
        LOGI("%s, HW scale dst: (%d, %d)\r\n", __func__,  scale_config->dst_width, scale_config->dst_height);
    }
    else
        LOGI("%s, HW scale dst: (%d, %d)\r\n", __func__,  scale_config->dst_width, scale_config->dst_height);


#if SUPPORTED_IMAGE_MAX_720P
	scale_config->scale_buffer[0].data = mux_sram_buffer->scale;
	scale_config->scale_buffer[0].id = 0;

    scale_config->scale_buffer[1].data = mux_sram_buffer->scale + scale_config->dst_width * IMAGE_MAX_PIPELINE_LINE * 2;
	scale_config->scale_buffer[1].id = 1;
#else
	//TODO
#endif

	LOGI("%s, [%p, %p] [%p %p]\n", __func__,
		&scale_config->scale_buffer[0],
		scale_config->scale_buffer[0].data,
		&scale_config->scale_buffer[1],
		scale_config->scale_buffer[1].data);

	INIT_LIST_HEAD(&scale_config->request_list);


	ret = rtos_init_semaphore(&scale_config->scale_sem, 1);

	if (ret != BK_OK)
	{
		LOGE("%s, init scale sem failed\r\n", __func__);
		goto error;
	}

	if (scale_config->scale_queue != NULL)
	{
		LOGE("%s, scale_config->scale_queue already init, exit!\n", __func__);
		goto error;
	}

	if (scale_config->scale_thread != NULL)
	{
		LOGE("%s, scale_config->scale_thread already init, exit!\n", __func__);
		goto error;
	}

	ret = rtos_init_queue(&scale_config->scale_queue,
							"scale_queue",
							sizeof(media_msg_t),
							15);

	if (ret != BK_OK)
	{
		LOGE("%s, init rot_queue failed\r\n", __func__);
		goto error;
	}

	ret = rtos_create_thread(&scale_config->scale_thread,
						BEKEN_DEFAULT_WORKER_PRIORITY,
						"scale_thread",
						(beken_thread_function_t)scale_main_entry,
						1024 * 2,
						NULL);

	if (ret != BK_OK)
	{
		LOGE("%s, create scale thread failed\r\n", __func__);
		goto error;
	}
	rtos_get_semaphore(&scale_config->scale_sem, BEKEN_NEVER_TIMEOUT);

	scale_config->enable = true;

	LOGI("%s complete\n", __func__);

	rtos_unlock_mutex(&scale_info->lock);

	return ret;
error:

	LOGE("%s error\n", __func__);

	if (scale_config->scale_queue)
	{
		rtos_deinit_queue(&scale_config->scale_queue);
		scale_config->scale_queue = NULL;
	}

	if (scale_config)
	{
		os_free(scale_config);
		scale_config = NULL;
	}

	rtos_unlock_mutex(&scale_info->lock);

	return BK_FAIL;
}


void scale_task_stop(void)
{
	beken_semaphore_t sem;
	media_msg_t msg;
	msg.event = SCALE_STOP;
	msg.param = (uint32_t)&sem;

	int ret = rtos_init_semaphore(&sem, 1);

	if (ret != BK_OK)
	{
		LOGE("%s, init sem faild, %d\n", __func__, ret);
		return;
	}

	ret = rtos_push_to_queue(&scale_config->scale_queue, &msg, BEKEN_WAIT_FOREVER);

	rtos_get_semaphore(&sem, BEKEN_NEVER_TIMEOUT);
	rtos_deinit_semaphore(&sem);
}

bk_err_t scale_task_close(void)
{
	LOGI("%s\n", __func__);

	rtos_lock_mutex(&scale_info->lock);

	if (scale_config == NULL || !scale_config->task_running)
	{
		LOGI("%s already close\n", __func__);
		rtos_unlock_mutex(&scale_info->lock);
		return BK_OK;
	}

	scale_config->enable = false;

	scale_task_stop();

	rtos_deinit_semaphore(&scale_config->scale_sem);
	scale_config->scale_sem = NULL;

	if (!list_empty(&scale_config->scale_pedding_list))
	{
		LOGI("%s clear scale_pedding_list \n", __func__);

		LIST_HEADER_T *pos, *n, *list = &scale_config->scale_pedding_list;
		scale_request_t *request = NULL;

		list_for_each_safe(pos, n, list)
		{
			request = list_entry(pos, scale_request_t, list);
			if (request != NULL)
			{
				list_del(pos);
				os_free(request);
			}
		}
	}

	while (!list_empty(&scale_config->request_list))
	{
		pipeline_encode_request_t *scale_request = list_pop_edge(&scale_config->request_list, pipeline_encode_request_t, list);

		if (scale_request != NULL)
		{
			complex_buffer_t *decoder_buffer = (complex_buffer_t*)os_malloc(sizeof(complex_buffer_t));
			os_memcpy(decoder_buffer, scale_request->buffer, sizeof(complex_buffer_t));

			LOGI("%s free pending list\n", __func__);

			if (scale_config->decoder_free_cb)
			{
				scale_config->decoder_free_cb(decoder_buffer);
			}
			else
			{
				LOGE("%s decoder_free_cb should not be NULL\n", __func__);
			}

			os_free(scale_request);
		}
		else
		{
			break;
		}
	}

	if (scale_config->decoder_buffer)
	{

		LOGD("%s free working buffer\n", __func__);

		if (scale_config->decoder_free_cb)
		{
			scale_config->decoder_free_cb(scale_config->decoder_buffer);
		}
		else
		{
			LOGE("%s decoder_free_cb should not be NULL\n", __func__);
		}
		scale_config->decoder_buffer = NULL;
	}

	if (scale_config->scale_frame)
	{
		frame_buffer_fb_direct_free(scale_config->scale_frame);
		scale_config->scale_frame = NULL;
		LOGI("%s free scale_frame\n", __func__);
	}

	bk_psram_disable_write_through(scale_config->psram_overwrite_id);
	bk_psram_free_write_through_channel(scale_config->psram_overwrite_id);

	if(scale_config->scale_src_frame)
	{
		frame_buffer_fb_direct_free(scale_config->scale_src_frame);
		scale_config->scale_src_frame = NULL;
	}

	os_free(scale_config);
	scale_config = NULL;

	LOGI("%s complete\n", __func__);

	rtos_unlock_mutex(&scale_info->lock);

	return BK_OK;
}

bk_err_t bk_scale_reset_request(mux_callback_t cb)
{
    rtos_lock_mutex(&scale_info->lock);
 
    scale_config->reset_cb = cb;

    if (BK_OK != scale_task_send_msg(SCALE_RESET, 0))
    {
        LOGI("%s send failed\n", __func__);
        goto error;
    }

    rtos_unlock_mutex(&scale_info->lock);

    return BK_OK;

error:

    if (scale_config
        && scale_config->reset_cb)
    {
        scale_config->reset_cb = NULL;
    }

    rtos_unlock_mutex(&scale_info->lock);

    LOGE("%s failed\n", __func__);

    return BK_FAIL;

}

bk_err_t bk_scale_encode_request(pipeline_encode_request_t *request, mux_callback_t cb)
{
	pipeline_encode_request_t *scale_request = NULL;

	rtos_lock_mutex(&scale_info->lock);

	if (scale_config == NULL || scale_config->enable == false)
	{
		LOGI("%s not open\n", __func__);
		goto error;
	}

	scale_request = (pipeline_encode_request_t *)os_malloc(sizeof(pipeline_encode_request_t));

	if (scale_request == NULL)
	{
		LOGI("%s malloc failed\n", __func__);
		goto error;
	}

	os_memcpy(scale_request, request, sizeof(pipeline_encode_request_t));

	scale_config->decoder_free_cb = cb;

	if (BK_OK != lcd_scale_line_start_request_handle(scale_request))
	{
		LOGI("%s send failed\n", __func__);
		goto error;
	}

	rtos_unlock_mutex(&scale_info->lock);

	return BK_OK;

error:

	if (scale_config
		&& scale_config->decoder_free_cb)
	{
		scale_config->decoder_free_cb = NULL;
	}

	if (scale_request)
	{
		os_free(scale_request);
		scale_request = NULL;
	}

	rtos_unlock_mutex(&scale_info->lock);

	LOGE("%s failed\n", __func__);

	return BK_FAIL;
}

bk_err_t bk_scale_pipeline_init(void)
{
	bk_err_t ret = BK_FAIL;

    if(scale_info != NULL)
    {
        os_free(scale_info);
        scale_info = NULL;
    }
	scale_info = (scale_info_t*)os_malloc(sizeof(scale_info_t));

	if (scale_info == NULL)
	{
		LOGE("%s malloc scale_info failed\n", __func__);
		return BK_FAIL;
	}

	os_memset(scale_info, 0, sizeof(scale_info_t));

	ret = rtos_init_mutex(&scale_info->lock);

	if (ret != BK_OK)
	{
		LOGE("%s, init mutex failed\r\n", __func__);
		return BK_FAIL;
	}

	return ret;
}



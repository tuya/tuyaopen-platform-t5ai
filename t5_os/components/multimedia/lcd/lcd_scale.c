#include <stdlib.h>
#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>

#include "FreeRTOS.h"
#include "event_groups.h"

#include <driver/media_types.h>
#include <driver/hw_scale_types.h>

#include "modules/image_scale.h"
//#include "display_array.h"

#include "driver/hw_scale.h"
#include "lcd_scale.h"

#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif


#define TAG "lcd_scale"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define BLOCK_LINE_COUNT	(16)

#define SCALE_EVT_FRAME_COMPLETE  (1 << 0)
#define SCALE_EVT_SOURCE_COMPLETE (1 << 1)
#define SCALE_EVT_DEST_COMPLETE   (1 << 2)

#define SCALE_WAIT_EVENT	(SCALE_EVT_FRAME_COMPLETE | SCALE_EVT_SOURCE_COMPLETE | SCALE_EVT_DEST_COMPLETE)

typedef struct
{
	beken_semaphore_t scale_sem;
}scale_t;
static scale_t  s_scale = {0};

typedef struct{

	scale_drv_config_t drv_config;
	scale_block_t src_block;
	scale_block_t dst_block;
	scale_result_t src_result;
	scale_result_t dst_result;
} scale_param_t;

static scale_param_t g_scale_param = {0};


static uint8_t task_running = 0;
static beken_semaphore_t scale_sem;
static beken_queue_t scale_queue;
static beken_thread_t scale_thread;
static beken_semaphore_t block_sem;

static EventGroupHandle_t scale_event_handle;

typedef struct
{
	uint32_t event;
	uint32_t param;
} scale_msg_t;


typedef enum {
	SCALE_ONE = 0,
	SCALE_LINE_FRAME,
	SCALE_START_FRAME,
	SCALE_SOURCE_LINE_FILL,
	SCALE_DEST_LINE_FILL,
	SCALE_END_FRAME,
} scale_block_type_t;

uint16_t src_width_size = 0;
uint16_t dst_width_size = 0;

static uint8_t *src_data = NULL;
uint8_t src_offset = 0;
frame_buffer_t *src_frame = NULL;

static uint8_t *dst_data = NULL;
uint8_t dst_offset = 0;
frame_buffer_t *dst_frame = NULL;

static bk_err_t scale_send_msg(uint8_t type, uint32_t param)
{
	bk_err_t ret = BK_FAIL;
	scale_msg_t msg;

	msg.event = type;
	msg.param = param;
	ret = rtos_push_to_queue(&scale_queue, &msg, BEKEN_WAIT_FOREVER);

	if (BK_OK != ret)
	{
		LOGE("%s failed, type:%d\r\n", __func__, type);
	}

	return ret;
}


void scale0_complete_cb(void *param)
{
	scale_send_msg(SCALE_END_FRAME, 0);
}

void scale_frame_cb(scale_result_t *src, scale_result_t *dst, scale_block_t *src_block, scale_block_t *dst_block)
{
	os_memcpy(&g_scale_param.src_result, src, sizeof(scale_result_t));
	os_memcpy(&g_scale_param.dst_result, dst, sizeof(scale_result_t));

	os_memcpy(dst_frame->frame + (dst_offset++ * dst_width_size), dst_data, dst_width_size);

	scale_send_msg(SCALE_END_FRAME, 0);
}


bk_err_t lcd_scale_block_fill(uint8_t type, scale_block_t *block)
{
	//scale_block_t *scale_block = (scale_block_t*)os_malloc(sizeof(scale_block_t));
	//os_memcpy(scale_block, block, sizeof(scale_block_t));
	scale_send_msg(type, (uint32_t)block);
	return 0;
}


static void inline print_reuslt(void)
{
#if 0
	LOGI("[I %d, B %d, C %d-%d, N %d-%d], [I %d, B %d, C %d-%d, N %d-%d]\n",
		g_scale_param.src_block.line_index,
		g_scale_param.src_result.complete_block_count,
		g_scale_param.src_result.current_frame_line,
		g_scale_param.src_result.current_block_line,
		g_scale_param.src_result.next_frame_line,
		g_scale_param.src_result.next_block_line,
		g_scale_param.dst_block.line_index,
		g_scale_param.dst_result.complete_block_count,
		g_scale_param.dst_result.current_frame_line,
		g_scale_param.dst_result.current_block_line,
		g_scale_param.dst_result.next_frame_line,
		g_scale_param.dst_result.next_block_line);
#endif
}

static void scale_main_entry(beken_thread_arg_t data)
{
	int ret = BK_OK;
	task_running = true;
	LOGI("%s %d\n", __func__, __LINE__);

	rtos_set_semaphore(&scale_sem);

	while (task_running)
	{
		scale_msg_t msg;
		ret = rtos_pop_from_queue(&scale_queue, &msg, BEKEN_WAIT_FOREVER);

		if (ret == BK_OK)
		{
			switch (msg.event)
			{
				case SCALE_ONE:
				{
				}
				break;

				case SCALE_LINE_FRAME:
				{
					scale_param_t *param = (scale_param_t*)msg.param;

					print_reuslt();

					ret = hw_scale_block_start(HW_SCALE0, &param->src_block, &param->dst_block);
				
					if (ret != BK_OK)
					{
						LOGE("%s scale err: %d\n", __func__, ret);
						break;
					}

					//rtos_delay_milliseconds(50);
				}
				break;

				case SCALE_DEST_LINE_FILL:
				{
					scale_block_t *scale_block = (scale_block_t*)msg.param;
				

					print_reuslt();

					//rtos_delay_milliseconds(50);
					ret = hw_scale_dest_block_fill(HW_SCALE0, scale_block);
				
					if (ret != BK_OK)
					{
						LOGE("%s scale err: %d\n", __func__, ret);
						break;
					}

					//os_free(scale_block);
					//scale_block = NULL;
				}
				break;

				case SCALE_SOURCE_LINE_FILL:
				{
					scale_block_t *scale_block = (scale_block_t*)msg.param;
				

					print_reuslt();
					//rtos_delay_milliseconds(50);
					ret = hw_scale_source_block_fill(HW_SCALE0, scale_block);
				
					if (ret != BK_OK)
					{
						LOGE("%s scale err: %d\n", __func__, ret);
						break;
					}

					//os_free(scale_block);
					//scale_block = NULL;

				}
				break;

				case SCALE_START_FRAME:
				{
					//scale_param_t *param = (scale_param_t*)msg.param;
				


				}
				break;

				case SCALE_END_FRAME:
				{
					print_reuslt();

#if 0
					rtos_set_semaphore(&s_scale.scale_sem);
#else
					xEventGroupSetBits(scale_event_handle, SCALE_EVT_FRAME_COMPLETE);
#endif
				}
				break;

				default:
					break;
			}
		}
	}
}

void scale_block_cb(scale_result_t *src, scale_result_t *dst)
{
	os_memcpy(&g_scale_param.src_result, src, sizeof(scale_result_t));
	os_memcpy(&g_scale_param.dst_result, dst, sizeof(scale_result_t));

	g_scale_param.src_block.line_index += g_scale_param.src_result.complete_block_count;
	g_scale_param.src_block.line_count = BLOCK_LINE_COUNT;
	g_scale_param.src_block.data = g_scale_param.drv_config.src_addr;
	
	g_scale_param.dst_block.line_index+= g_scale_param.dst_result.complete_block_count;
	g_scale_param.dst_block.line_count = BLOCK_LINE_COUNT;
	g_scale_param.dst_block.data += g_scale_param.dst_result.complete_block_count *  g_scale_param.drv_config.dst_width * 2;

	scale_send_msg(SCALE_DEST_LINE_FILL, (uint32_t)&g_scale_param);
}

void source_block_complete(scale_result_t *result, scale_block_t *block)
{
	os_memcpy(&g_scale_param.src_result, result, sizeof(scale_result_t));

	g_scale_param.src_block.line_index += g_scale_param.src_result.complete_block_count;
	g_scale_param.src_block.line_count = BLOCK_LINE_COUNT;
	g_scale_param.src_block.data += g_scale_param.src_result.complete_block_count * g_scale_param.drv_config.src_width * 2;

	{	
		os_memcpy(src_data, src_frame->frame + (src_offset++ * (src_width_size)), src_width_size);
		g_scale_param.src_block.data = src_data;
	}

	lcd_scale_block_fill(SCALE_SOURCE_LINE_FILL, &g_scale_param.src_block);

}

void dest_block_complete(scale_result_t *result, scale_block_t *block)
{
	os_memcpy(&g_scale_param.dst_result, result, sizeof(scale_result_t));
	
	g_scale_param.dst_block.line_index += g_scale_param.dst_result.complete_block_count;
	g_scale_param.dst_block.line_count = BLOCK_LINE_COUNT;
	g_scale_param.dst_block.data += g_scale_param.dst_result.complete_block_count * g_scale_param.drv_config.dst_width * 2;

	{
		g_scale_param.dst_block.data = dst_data;
		os_memcpy(dst_frame->frame + (dst_offset++ * (sizeof(dst_data))), dst_data, sizeof(dst_data));
	}

	lcd_scale_block_fill(SCALE_DEST_LINE_FILL, &g_scale_param.dst_block);
}


bk_err_t lcd_scale(scale_param_t *scale_param, frame_buffer_t *src, frame_buffer_t *dst)
{
	bk_err_t ret;

	os_memset(scale_param, 0 , sizeof(scale_param_t));

	scale_param->drv_config.src_width = src->width;
	scale_param->drv_config.dst_width = dst->width;
	scale_param->drv_config.src_height = src->height;
	scale_param->drv_config.dst_height = dst->height;
	//scale_param->drv_config.src_addr = (uint8_t *)src->frame;
	//scale_param->drv_config.dst_addr = (uint8_t *)dst->frame;
	scale_param->drv_config.pixel_fmt = src->fmt;

//	scale_param->drv_config.scale_mode = BLOCK_SCALE;
	scale_param->drv_config.scale_mode = FRAME_SCALE;

	scale_param->drv_config.frame_complete = scale_frame_cb;
	scale_param->drv_config.source_block_complete = source_block_complete;
	scale_param->drv_config.dest_block_complete = dest_block_complete;

	scale_param->drv_config.line_cycle = BLOCK_LINE_COUNT;
	scale_param->drv_config.line_mask = 0x1F;

	scale_param->src_block.line_index = 0;
	scale_param->src_block.line_count = BLOCK_LINE_COUNT;
	scale_param->src_block.data = scale_param->drv_config.src_addr;
	scale_param->src_block.args = src;
	
	scale_param->dst_block.line_index = 0;
	scale_param->dst_block.line_count = BLOCK_LINE_COUNT;
	scale_param->dst_block.data = scale_param->drv_config.dst_addr;
	scale_param->dst_block.args = dst;

    if (scale_param->drv_config.scale_mode == BLOCK_SCALE)
	{
		if (src_width_size == 0)
		{
			src_width_size = src->width * 16 * 2;
			src_data = (uint8_t*)os_malloc(src_width_size);
		}

		if (dst_width_size == 0)
		{
			dst_width_size = dst->width * 16 * 2;
			dst_data = (uint8_t*)os_malloc(dst_width_size);
		}

		src_offset = 0;
		src_frame = src;
		os_memcpy(src_data, src_frame->frame + (src_offset++ * src_width_size), sizeof(src_data));
		scale_param->src_block.data = src_data;

		dst_offset = 0;
		dst_frame = dst;
		scale_param->dst_block.data = dst_data;

	}



	if (scale_param->drv_config.scale_mode == BLOCK_SCALE)
	{
		ret = hw_scale_block_config(HW_SCALE0, &scale_param->drv_config);
		
		if (ret != BK_OK)
		{
			LOGE("%s scale config err: %d\n", __func__, ret);
			return ret;
		}

		ret = hw_scale_block_start(HW_SCALE0, &scale_param->src_block, &scale_param->dst_block);
	}
	else
	{
        scale_param->drv_config.src_addr = src->frame;
        scale_param->drv_config.dst_addr = dst->frame;
		ret = hw_scale_frame(HW_SCALE0, &scale_param->drv_config);
	}

	
	if (ret != BK_OK)
	{
		LOGE("%s scale start err: %d\n", __func__, ret);
		return ret;
	}

	//rtos_delay_milliseconds(50);


	return 0;
}


bk_err_t lcd_hw_scale(frame_buffer_t *src, frame_buffer_t *dst)
{
	bk_err_t ret = BK_FAIL;

	lcd_scale(&g_scale_param, src, dst);

	do {
		int event = xEventGroupWaitBits(scale_event_handle,
				SCALE_WAIT_EVENT, 
				true, false, portMAX_DELAY);

		if (event & SCALE_EVT_FRAME_COMPLETE)
		{
			LOGD("%s, frame complete\n", __func__);
			ret = BK_OK;
			break;
		}

	} while (true);

	return ret;
}



bk_err_t lcd_scale_deinit(void)
{
	bk_err_t ret = BK_OK;

	ret = rtos_deinit_semaphore(&s_scale.scale_sem);

	if (ret != BK_OK)
	{
		LOGE("%s s_scale.scale_semdeinit failed: %d\n", __func__, ret);
		return ret;
	}
	bk_hw_scale_driver_deinit(HW_SCALE0);
	return ret;
}

bk_err_t lcd_scale_init(void)
{
	bk_err_t ret = BK_OK;

	LOGI("%s \n", __func__);
	ret = rtos_init_semaphore_ex(&s_scale.scale_sem, 1, 0);

	if (ret != BK_OK)
	{
		LOGE("%s s_scale.scale_sem init failed: %d\n", __func__, ret);
		return ret;
	}

	bk_hw_scale_driver_init(HW_SCALE0);
	bk_hw_scale_isr_register(HW_SCALE0, scale0_complete_cb, NULL);

	ret = rtos_init_queue(&scale_queue,
							"scale_queue",
							sizeof(scale_msg_t),
							10);

	if (ret != BK_OK)
	{
		LOGE("%s, init rot_queue failed\r\n", __func__);
		goto error;
	}

	scale_event_handle = xEventGroupCreate();

	if (scale_event_handle == NULL)
	{
		LOGE("%s scale_event_handle null!\n", __func__);
		goto error;
	}

	ret = rtos_init_semaphore(&scale_sem, 1);
	ret = rtos_init_semaphore(&block_sem, 1);
	ret = rtos_create_thread(&scale_thread,
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

	rtos_get_semaphore(&scale_sem, BEKEN_NEVER_TIMEOUT);

	return ret;

error:
	return ret;
}


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

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include <driver/jpeg_dec.h>
#include <modules/jpeg_decode_sw.h>
#include <modules/tjpgd.h>

#include "media_evt.h"
#include "frame_buffer.h"
#include "yuv_encode.h"

#include "mux_pipeline.h"

#define TAG "sw_dec_pip"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


//#define DECODE_DIAG_DEBUG
#ifdef DECODE_DIAG_DEBUG
#define DECODER_FRAME_START() do { GPIO_UP(GPIO_DVP_D0); } while (0)
#define DECODER_FRAME_END() do { GPIO_DOWN(GPIO_DVP_D0); } while (0)
#else
#define DECODER_FRAME_START()
#define DECODER_FRAME_END()
#endif

typedef struct {
	uint8_t task_state : 1;
	beken_semaphore_t sw_dec_sem;
	media_rotate_t rotate_angle;
	uint8_t *rotate_buffer;

	beken_queue_t sw_dec_queue;
	beken_thread_t sw_dec_thread;
} sw_dec_config_t;

static sw_dec_config_t *sw_dec_config = NULL;

#if (CONFIG_CACHE_ENABLE)
#include "cache.h"
#endif

#if CONFIG_SOFTWARE_DECODE_SRAM_MAPPING
#include "FreeRTOS.h"
#include "task.h"
#include <driver/dma2d.h>
#define DOUBLE_BUFFER_TO_COPY 0

__attribute__((section(".dtcm_sec_data "))) StaticTask_t xSWDecTaskTCB = {0};
__attribute__((section(".dtcm_sec_data "))) StackType_t uxSWDecTaskStack[ 512 ] = {0};

#define BUFFER_SIZE (864 * 8 * 2 * 2 * 2)
uint8_t *sw_dma2d_buffer_1 = NULL;
__attribute__((section(".dtcm_sec_data "))) uint8_t rotate_buffer_sw[16*16*2] = {0};
#if DOUBLE_BUFFER_TO_COPY
uint8_t *sw_dma2d_buffer_2 = NULL;
static beken_semaphore_t sw_sem;
static beken_mutex_t sw_dma2d_lock;
#endif
__attribute__((section(".dtcm_sec_data "))) uint8_t jpeg_decode_buffer[0xB0] = {0};
#endif

bk_err_t software_decode_task_send_msg(uint8_t type, uint32_t param)
{
	int ret = BK_OK;
	media_msg_t msg;

	if (sw_dec_config && sw_dec_config->sw_dec_queue)
	{
		msg.event = type;
		msg.param = param;

		ret = rtos_push_to_queue(&sw_dec_config->sw_dec_queue, &msg, BEKEN_NO_WAIT);

		if (ret != BK_OK)
		{
			LOGE("%s push failed\n", __func__);
		}
	}

	return ret;
}

static void software_decode_task_deinit(void)
{
	LOGI("%s\r\n", __func__);
	if (sw_dec_config)
	{
		if (sw_dec_config->sw_dec_queue)
		{
			rtos_deinit_queue(&sw_dec_config->sw_dec_queue);
			sw_dec_config->sw_dec_queue = NULL;
		}
        if(sw_dec_config->sw_dec_sem)
        {
            rtos_deinit_semaphore(&sw_dec_config->sw_dec_sem);
        }
#if CONFIG_SOFTWARE_DECODE_SRAM_MAPPING
		sw_dec_config->rotate_buffer = NULL;
#if DOUBLE_BUFFER_TO_COPY
		if(sw_sem)
		{
			rtos_deinit_semaphore(&sw_sem);
		}

		if(sw_dma2d_lock)
		{
			rtos_deinit_mutex(&sw_dma2d_lock);
		}
		bk_dma2d_driver_deinit();
#endif
#else
		if (sw_dec_config->rotate_buffer)
		{
			os_free(sw_dec_config->rotate_buffer);
			sw_dec_config->rotate_buffer = NULL;
		}
#endif
		sw_dec_config->sw_dec_thread = NULL;

		os_free(sw_dec_config);
		sw_dec_config = NULL;
	}
	bk_jpeg_dec_sw_deinit();
	frame_buffer_fb_deregister(MODULE_DECODER_CP1, FB_INDEX_JPEG);
	LOGI("%s complete\r\n", __func__);
}

#if CONFIG_SOFTWARE_DECODE_SRAM_MAPPING
#if DOUBLE_BUFFER_TO_COPY
static void sw_dma2d_config_error(void)
{
	LOGE("%s\n", __func__);
	rtos_set_semaphore(&sw_sem);
}

static void sw_dma2d_transfer_error(void)
{
	LOGE("%s\n", __func__);
	rtos_set_semaphore(&sw_sem);
}

static void sw_dma2d_transfer_complete(void)
{
	rtos_set_semaphore(&sw_sem);
}

void sw_dec_set_dma2d_cb(void)
{
	bk_dma2d_register_int_callback_isr(DMA2D_CFG_ERROR_ISR, sw_dma2d_config_error);
	bk_dma2d_register_int_callback_isr(DMA2D_TRANS_ERROR_ISR, sw_dma2d_transfer_error);
	bk_dma2d_register_int_callback_isr(DMA2D_TRANS_COMPLETE_ISR, sw_dma2d_transfer_complete);
}

bk_err_t sw_dma2d_copy(uint8_t *out, uint8_t *in,
			uint32_t in_w, uint32_t in_h,
			uint32_t out_w, uint32_t out_h,
			uint32_t x_pos, uint32_t y_pos)
{
	rtos_lock_mutex(&sw_dma2d_lock);
#if CONFIG_SOFTWARE_DECODE_SRAM_MAPPING
#if (CONFIG_CACHE_ENABLE)
	flush_all_dcache();
#endif
#endif
	bk_err_t ret = rtos_get_semaphore(&sw_sem, 5);
	if (ret != BK_OK)
	{
		LOGE("%s %d get sem timeout %x out:%p int:%p\n", __func__, __LINE__, ret, out, in);
		bk_dma2d_soft_reset();
		rtos_unlock_mutex(&sw_dma2d_lock);
		return ret;
	}
	dma2d_memcpy_pfc_t dma2d_memcpy_pfc = {0};

	dma2d_memcpy_pfc.input_addr = (char *)in;
	dma2d_memcpy_pfc.output_addr = (char *)out;

	dma2d_memcpy_pfc.mode = DMA2D_M2M;
	dma2d_memcpy_pfc.input_color_mode = DMA2D_INPUT_YUYV;
	dma2d_memcpy_pfc.src_pixel_byte = TWO_BYTES;
	dma2d_memcpy_pfc.output_color_mode = DMA2D_OUTPUT_YUYV;
	dma2d_memcpy_pfc.dst_pixel_byte = TWO_BYTES;

	dma2d_memcpy_pfc.src_frame_xpos = 0;
	dma2d_memcpy_pfc.src_frame_ypos = 0;

	dma2d_memcpy_pfc.dma2d_width = in_w;
	dma2d_memcpy_pfc.dma2d_height = in_h;
	dma2d_memcpy_pfc.src_frame_width = in_w;
	dma2d_memcpy_pfc.src_frame_height = in_h;
	dma2d_memcpy_pfc.dst_frame_width = out_w;
	dma2d_memcpy_pfc.dst_frame_height = out_h;
	dma2d_memcpy_pfc.dst_frame_xpos = x_pos;
	dma2d_memcpy_pfc.dst_frame_ypos = y_pos;
	bk_dma2d_memcpy_or_pixel_convert(&dma2d_memcpy_pfc);
	bk_dma2d_start_transfer();
	rtos_unlock_mutex(&sw_dma2d_lock);
	return BK_OK;
}
#endif

__attribute__((section(".itcm_sec_code "))) bk_err_t sw_cpu_copy(uint8_t *out, uint8_t *in,
			uint32_t in_w, uint32_t in_h,
			uint32_t out_w, uint32_t out_h,
			uint32_t x_pos, uint32_t y_pos)
{
	bk_err_t ret = BK_OK;

#if CONFIG_SOFTWARE_DECODE_SRAM_MAPPING
#if (CONFIG_CACHE_ENABLE)
	flush_all_dcache();
#endif
#endif
	uint32_t *src_w = NULL, *dst_w = NULL;
	uint16_t tmp_data = 0;
	src_w = (uint32_t *)in;   /* RGB bitmap to be output */
	dst_w = (uint32_t *)(out + ((x_pos) << 1) +
		((y_pos * out_w) << 1));
	tmp_data = ((out_w - in_w) << 1) >> 2;
	if (in_w == 8)
	{
		for (int j = in_h ; j > 0; j --)
		{
			*dst_w++ = *src_w++;
			*dst_w++ = *src_w++;
			*dst_w++ = *src_w++;
			*dst_w++ = *src_w++;
			dst_w += tmp_data;
		}
	}
	else if (in_w == 16)
	{
		for (int j = in_h ; j > 0; j --)
		{
			*dst_w++ = *src_w++;
			*dst_w++ = *src_w++;
			*dst_w++ = *src_w++;
			*dst_w++ = *src_w++;
			*dst_w++ = *src_w++;
			*dst_w++ = *src_w++;
			*dst_w++ = *src_w++;
			*dst_w++ = *src_w++;
			dst_w += tmp_data;
		}
	}
#if CONFIG_SOFTWARE_DECODE_SRAM_MAPPING
#if (CONFIG_CACHE_ENABLE)
	flush_all_dcache();
#endif
#endif

	return ret;
}

#endif

bk_err_t jpeg_decode_single_frame(frame_buffer_t *in_frame,
										frame_buffer_t *out_frame,
										uint8_t scale,
										JD_FORMAT_OUTPUT format,
										media_rotate_t rotate_angle,
										uint8_t *work_buf,
										uint8_t *rotate_buf)
{
	bk_err_t ret = BK_OK;
	sw_jpeg_dec_res_t result;
	if ((in_frame == NULL || in_frame->frame == NULL) || (out_frame == NULL || out_frame->frame == NULL))
	{
		return BK_ERR_PARAM;
	}
	ret = bk_jpeg_dec_sw_start_one_time(JPEGDEC_BY_FRAME, in_frame->frame, out_frame->frame,
				in_frame->length, out_frame->size, &result, scale, format, rotate_angle, work_buf, rotate_buf);
	if (ret != BK_OK)
	{
		LOGE("%s sw decoder error %x\n", __func__, ret);
	}
	return ret;
}

static bk_err_t jpeg_decode_frame(frame_buffer_t *in_frame, frame_buffer_t *out_frame)
{
	bk_err_t ret = BK_OK;
	sw_jpeg_dec_res_t result;
	if ((in_frame == NULL || in_frame->frame == NULL) || (out_frame == NULL || out_frame->frame == NULL))
	{
		return BK_ERR_PARAM;
	}
#if CONFIG_SOFTWARE_DECODE_SRAM_MAPPING
	if (sw_dec_config->rotate_buffer == NULL)
	{
		sw_dec_config->rotate_buffer = rotate_buffer_sw;
	}
#if DOUBLE_BUFFER_TO_COPY
	rtos_set_semaphore(&sw_sem);
#endif
#else
	if (sw_dec_config->rotate_buffer == NULL)
	{
		sw_dec_config->rotate_buffer = os_malloc(16 * 16 * 2);
		if (sw_dec_config->rotate_buffer == NULL)
		{
			return BK_ERR_NO_MEM;
		}
	}
#endif

	jd_set_rotate(sw_dec_config->rotate_angle, sw_dec_config->rotate_buffer);
	if (sw_dec_config->rotate_angle == ROTATE_90 || sw_dec_config->rotate_angle == ROTATE_270)
	{
		out_frame->width = in_frame->height;
		out_frame->height = in_frame->width;
	}
#if (CONFIG_CACHE_ENABLE)
	flush_all_dcache();
#endif
	ret = bk_jpeg_dec_sw_start(JPEGDEC_BY_FRAME, in_frame->frame, out_frame->frame,
				in_frame->length, out_frame->size, &result);
	if (ret != BK_OK)
	{
		LOGE("%s sw decoder error %x\n", __func__, ret);
	}
	return ret;
}


static void software_decode_main(beken_thread_arg_t data)
{
	int ret = BK_OK;
	sw_dec_config->task_state = true;
    rtos_set_semaphore(&sw_dec_config->sw_dec_sem);

	while(1)
	{
		media_msg_t msg;
		ret = rtos_pop_from_queue(&sw_dec_config->sw_dec_queue, &msg, BEKEN_WAIT_FOREVER);
		if (ret == BK_OK)
		{
			switch (msg.event)
			{
				case JPEGDEC_START:
				{
					media_software_decode_info_t *sw_dec_info = (media_software_decode_info_t *)msg.param;
					if (sw_dec_info != NULL)
					{
#if CONFIG_SOFTWARE_DECODE_SRAM_MAPPING
#if DOUBLE_BUFFER_TO_COPY
						if (sw_dma2d_buffer_1)
						{
							sw_dec_set_dma2d_cb();
						}
#endif
#endif
						DECODER_FRAME_START();
						ret = jpeg_decode_frame(sw_dec_info->in_frame, sw_dec_info->out_frame);
						if (ret != BK_OK)
						{
							LOGE("%s sw decoder error\n", __func__);
						}
						jpeg_decode_task_send_msg(JPEGDEC_FRAME_CP1_FINISH, ret);
						DECODER_FRAME_END();
					}
				}
				break;

				case JPEGDEC_SET_ROTATE_ANGLE:
				{
					uint32_t rotate_angle = msg.param;
					if (rotate_angle <= ROTATE_270)
					{
						sw_dec_config->rotate_angle = rotate_angle;
					}
				}
				break;

				case JPEGDEC_STOP:
					goto exit;

				default:
					break;
			}
		}
	}

exit:
	LOGI("%s, exit\r\n", __func__);
    rtos_set_semaphore(&sw_dec_config->sw_dec_sem);
	rtos_delete_thread(NULL);
}

bool check_software_decode_task_is_open(void)
{
	if (sw_dec_config == NULL)
	{
		return false;
	}
	else
	{
		return sw_dec_config->task_state;
	}
}

bk_err_t software_decode_task_open()
{
	int ret = BK_OK;
	LOGI("%s\r\n", __func__);

	if (sw_dec_config != NULL && sw_dec_config->task_state)
	{
		LOGE("%s have been opened!\r\n", __func__);
		return ret;
	}

	sw_dec_config = (sw_dec_config_t *)os_malloc(sizeof(sw_dec_config_t));
	if (sw_dec_config == NULL)
	{
		LOGE("%s, malloc sw_dec_config failed\r\n", __func__);
		return BK_FAIL;
	}

	os_memset(sw_dec_config, 0, sizeof(sw_dec_config_t));
	frame_buffer_fb_register(MODULE_DECODER_CP1, FB_INDEX_JPEG);

#if CONFIG_SOFTWARE_DECODE_SRAM_MAPPING
	bk_jpeg_dec_sw_init(jpeg_decode_buffer, sizeof(jpeg_decode_buffer));
#else
	bk_jpeg_dec_sw_init(NULL, 0);
#endif

	ret = rtos_init_semaphore(&sw_dec_config->sw_dec_sem, 1);
	if (ret != BK_OK)
	{
		LOGE("%s, init sw_dec_config->sw_dec_sem failed\r\n", __func__);
		goto error;
	}

	ret = rtos_init_queue(&sw_dec_config->sw_dec_queue,
							"sw_dec_que",
							sizeof(media_msg_t),
							15);

	if (ret != BK_OK)
	{
		LOGE("%s, init sw_dec_que failed\r\n", __func__);
		goto error;
	}

#if CONFIG_SOFTWARE_DECODE_SRAM_MAPPING
#if (CONFIG_CACHE_ENABLE)
	flush_all_dcache();
#endif

	sw_dma2d_buffer_1 = mux_sram_buffer->rotate;
#if DOUBLE_BUFFER_TO_COPY
	ret = rtos_init_semaphore(&sw_sem, 1);
	if (ret != BK_OK)
	{
		goto error;
	}
	rtos_init_mutex(&sw_dma2d_lock);

	sw_dma2d_buffer_2 = sw_dma2d_buffer_1 + (BUFFER_SIZE >> 2);

	bk_dma2d_driver_init();

	bk_dma2d_int_enable(DMA2D_CFG_ERROR | DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE,1);
	bk_dma2d_register_int_callback_isr(DMA2D_CFG_ERROR_ISR, sw_dma2d_config_error);
	bk_dma2d_register_int_callback_isr(DMA2D_TRANS_ERROR_ISR, sw_dma2d_transfer_error);
	bk_dma2d_register_int_callback_isr(DMA2D_TRANS_COMPLETE_ISR, sw_dma2d_transfer_complete);
	jd_set_jpg_copy_func(sw_dma2d_buffer_1), ADDR_TO_CACHE(sw_dma2d_buffer_2), (BUFFER_SIZE >> 2), sw_dma2d_copy, JD_DOUBLE_BUFFER_COPY);
#else
	jd_set_jpg_copy_func(sw_dma2d_buffer_1, NULL, (BUFFER_SIZE >> 1), sw_cpu_copy, JD_SINGLE_BUFFER_COPY);
#endif

	sw_dec_config->sw_dec_thread = xTaskCreateStatic( (TaskFunction_t)software_decode_main,
										 "sw_dec_task",
										 512,
										 ( void * ) NULL,
										 9 - BEKEN_DEFAULT_WORKER_PRIORITY,
										 uxSWDecTaskStack,
										 &xSWDecTaskTCB );
#else
	ret = rtos_create_thread(&sw_dec_config->sw_dec_thread,
							BEKEN_DEFAULT_WORKER_PRIORITY,
							"sw_dec_task",
							(beken_thread_function_t)software_decode_main,
							1024 * 2,
							NULL);

	if (ret != BK_OK)
	{
		LOGE("%s, init jdec_task failed\r\n", __func__);
		goto error;
	}
#endif

	rtos_get_semaphore(&sw_dec_config->sw_dec_sem, BEKEN_NEVER_TIMEOUT);
	LOGI("%s complete\r\n", __func__);

	return ret;

error:

	LOGE("%s, open failed\r\n", __func__);

	software_decode_task_deinit();

	return ret;
}

bk_err_t software_decode_task_close()
{
	LOGI("%s  %d\n", __func__, __LINE__);

	if (sw_dec_config == NULL || !sw_dec_config->task_state)
	{
		return BK_FAIL;
	}

	sw_dec_config->task_state = false;

	software_decode_task_send_msg(JPEGDEC_STOP, 0);

	rtos_get_semaphore(&sw_dec_config->sw_dec_sem, BEKEN_NEVER_TIMEOUT);

	software_decode_task_deinit();

	LOGI("%s complete, %d\n", __func__, __LINE__);

	return BK_OK;
}


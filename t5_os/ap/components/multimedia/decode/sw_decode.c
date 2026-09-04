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
#include "sw_decode.h"

#include "mux_pipeline.h"

#define TAG "sw_dec"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#if (CONFIG_CACHE_ENABLE)
#include "cache.h"
#endif

#if CONFIG_SOFTWARE_DECODE_SRAM_MAPPING
#if (CONFIG_CACHE_ENABLE)
#define FLUSH_ALL_DCACHE() flush_all_dcache()
#else
#define FLUSH_ALL_DCACHE()
#endif
#else
#define FLUSH_ALL_DCACHE()
#endif

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
	beken_queue_t sw_dec_queue;
	beken_thread_t sw_dec_thread;
	uint8_t *sw_dec_buffer;
	media_rotate_t rotate_angle;
	uint8_t *rotate_buffer;
} sw_dec_config_t;

static sw_dec_config_t *sw_dec_config = NULL;

#if CONFIG_SOFTWARE_DECODE_SRAM_MAPPING
#define BUFFER_SIZE (864 * 8 * 2 * 2)

__attribute__((section(".dtcm_cpu1"), aligned(0x10))) uint8_t rotate_buffer_sw_cp1[16*16*2] = {0};
__attribute__((section(".dtcm_cpu1"), aligned(0x10))) jd_workbuf_t jpeg_decode_workbuf_cp1 = {0};
__attribute__((aligned(0x10))) StaticTask_t xSWDecTaskTCB_cp1 = {0};
__attribute__((section(".dtcm_cpu1"), aligned(0x10))) StackType_t uxSWDecTaskStack_cp1[ 512 ] = {0};
#endif

jpeg_dec_handle_t jpeg_decode_handle;

bk_err_t software_decode_task_send_msg(uint32_t type, uint32_t param)
{
	int ret = BK_FAIL;
	media_msg_t msg;

	if (sw_dec_config && sw_dec_config->sw_dec_queue)
	{
		msg.event = type;
		msg.param = param;

		ret = rtos_push_to_queue(&sw_dec_config->sw_dec_queue, &msg, BEKEN_WAIT_FOREVER);

		if (ret != BK_OK)
		{
			LOGE("%s push failed\n", __func__);
		}
	}

	return ret;
}

static void software_decode_task_deinit(void)
{
	LOGD("%s\r\n", __func__);
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

	LOGD("%s complete\r\n", __func__);
}

#if CONFIG_SOFTWARE_DECODE_SRAM_MAPPING
__attribute__((section(".itcm_sec_code "))) bk_err_t software_cpu_copy(uint8_t *out, uint8_t *in,
			uint32_t in_w, uint32_t in_h,
			uint32_t out_w, uint32_t out_h,
			uint32_t x_pos, uint32_t y_pos)
{
	bk_err_t ret = BK_OK;
	uint32_t *src_w = NULL, *dst_w = NULL;
	uint16_t tmp_data = 0;
	FLUSH_ALL_DCACHE();

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
	FLUSH_ALL_DCACHE();

	return ret;
}
#endif

bk_err_t software_decode_single_frame(frame_buffer_t *in_frame,
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

static bk_err_t software_decode_frame(frame_buffer_t *in_frame, frame_buffer_t *out_frame)
{
	bk_err_t ret = BK_OK;
	sw_jpeg_dec_res_t result;
	if ((in_frame == NULL || in_frame->frame == NULL) || (out_frame == NULL || out_frame->frame == NULL))
	{
		return BK_ERR_PARAM;
	}
	if (sw_dec_config->rotate_angle != ROTATE_NONE)
	{
#if CONFIG_SOFTWARE_DECODE_SRAM_MAPPING
		if (sw_dec_config->rotate_buffer == NULL)
		{
			sw_dec_config->rotate_buffer = rotate_buffer_sw_cp1;
		}
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
	}

	jd_set_rotate_by_handle(jpeg_decode_handle, sw_dec_config->rotate_angle, sw_dec_config->rotate_buffer);
	if (sw_dec_config->rotate_angle == ROTATE_90 || sw_dec_config->rotate_angle == ROTATE_270)
	{
		out_frame->width = in_frame->height;
		out_frame->height = in_frame->width;
	}
	FLUSH_ALL_DCACHE();
	ret = bk_jpeg_dec_sw_start_by_handle(jpeg_decode_handle, JPEGDEC_BY_FRAME, in_frame->frame, out_frame->frame,
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

#if CONFIG_SOFTWARE_DECODE_SRAM_MAPPING
	ret = bk_jpeg_dec_sw_init_by_handle(&jpeg_decode_handle,
						(uint8_t *)&jpeg_decode_workbuf_cp1, sizeof(jd_workbuf_t));
#else
	ret = bk_jpeg_dec_sw_init_by_handle(&jpeg_decode_handle,
						NULL, 0);
#endif
	if (ret != BK_OK) {
		LOGE("%s, bk_jpeg_dec_sw_init failed\r\n", __func__);
	}

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
						DECODER_FRAME_START();
						ret = software_decode_frame(sw_dec_info->in_frame, sw_dec_info->out_frame);
						if (ret != BK_OK)
						{
							LOGE("%s sw decoder error\n", __func__);
						}
						if (sw_dec_info->cb)
						{
							sw_dec_info->cb(ret);
						}
						else
						{
							LOGE("%s dec complete ret is %d but no callback\n", __func__, ret);
						}
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
	LOGD("%s, exit\r\n", __func__);
	bk_jpeg_dec_sw_deinit_by_handle(jpeg_decode_handle);
    jpeg_decode_handle = NULL;
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

void software_decode_set_rotate(uint8_t rotate_angle)
{
	software_decode_task_send_msg(JPEGDEC_SET_ROTATE_ANGLE, rotate_angle);
}

static void create_thread_on_cp1(beken_thread_arg_t data)
{
#if CONFIG_SOFTWARE_DECODE_SRAM_MAPPING
	sw_dec_config->sw_dec_thread = xTaskCreateStaticPinnedToCore( (TaskFunction_t)software_decode_main,
										"sw_dec_task",
										512,
										( void * ) NULL,
										9 - BEKEN_DEFAULT_WORKER_PRIORITY,
										uxSWDecTaskStack_cp1,
										&xSWDecTaskTCB_cp1,
										0);
	if (sw_dec_config->sw_dec_thread == NULL)
	{
		LOGE("%s %d create thread sw_dec_task failed\r\n", __func__, __LINE__);
		rtos_set_semaphore(&sw_dec_config->sw_dec_sem);
	}
#else
	int ret = BK_OK;
	/* create a thread on core 1 */
	ret = rtos_core0_create_thread(&sw_dec_config->sw_dec_thread,
							BEKEN_DEFAULT_WORKER_PRIORITY,
							"sw_dec_task",
							(beken_thread_function_t)software_decode_main,
							1024 * 2,
							NULL);
	if (ret != BK_OK)
	{
		LOGE("Error: Failed to create sw_dec_task: %d\r\n",ret);
		rtos_set_semaphore(&sw_dec_config->sw_dec_sem);
	}
#endif
	rtos_delete_thread(NULL);
}

bk_err_t software_decode_task_open(uint32_t rotate_buffer)
{
	int ret = BK_OK;
	LOGD("%s\r\n", __func__);

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

	beken_thread_t sw_dec_thread_temp;
	/* create a thread on core 1 */
	ret = rtos_core0_create_thread(&sw_dec_thread_temp,
							BEKEN_DEFAULT_WORKER_PRIORITY - 1,
							"sw_dec_thread_temp",
							(beken_thread_function_t)create_thread_on_cp1,
							1024,
							NULL);

	if (ret != BK_OK)
	{
		LOGE("%s, init sw_dec_thread_temp failed\r\n", __func__);
		goto error;
	}

	rtos_get_semaphore(&sw_dec_config->sw_dec_sem, BEKEN_NEVER_TIMEOUT);

	if (sw_dec_config->sw_dec_thread == NULL)
	{
		goto error;
	}
	LOGD("%s complete\r\n", __func__);

	return ret;

error:

	LOGE("%s, open failed\r\n", __func__);

	software_decode_task_deinit();

	return ret;
}

bk_err_t software_decode_task_close()
{
	LOGD("%s  %d\n", __func__, __LINE__);

	if (sw_dec_config == NULL || !sw_dec_config->task_state)
	{
		return BK_FAIL;
	}

	sw_dec_config->task_state = false;

	software_decode_task_send_msg(JPEGDEC_STOP, 0);

	rtos_get_semaphore(&sw_dec_config->sw_dec_sem, BEKEN_NEVER_TIMEOUT);

	software_decode_task_deinit();

	LOGD("%s complete, %d\n", __func__, __LINE__);

	return BK_OK;
}

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
#include <string.h>
#include <stdlib.h>

#include <os/mem.h>
#include <os/os.h>
#include <components/log.h>

#include "media_core.h"
#include "media_evt.h"
#include "storage_act.h"
#include "lcd_act.h"
#include "camera_act.h"
#include "frame_buffer.h"

#include <driver/int.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>

#include <driver/dma.h>

#include <driver/jpeg_enc.h>
#include <driver/jpeg_enc_types.h>

#include <soc/mapping.h>

#include <driver/lcd.h>
#include <driver/dma.h>
#include <driver/gpio.h>
#include <driver/jpeg_dec.h>
#include <driver/dma2d.h>
#include <driver/jpeg_dec_types.h>
#include "modules/image_scale.h"
#include <driver/uvc_camera_types.h>
#include <driver/uvc_camera.h>

#include <driver/timer.h>
#include <driver/psram.h>

#include "driver/flash.h"
#include "driver/flash_partition.h"
#include "beken_image.h"
#include <modules/jpeg_decode_sw.h>
#include <os/str.h>
#include <blend_logo.h>
#include "modules/image_scale.h"
#include <driver/dma2d.h>
#include "modules/lcd_font.h"
#include <driver/media_types.h>

#include <lcd_decode.h>
#include <lcd_rotate.h>
#include <lcd_blend.h>
#include <camera_act.h>

#include <driver/uvc_camera_types.h>
#include <driver/uvc_camera.h>
#include <driver/dvp_camera.h>
#include <driver/dvp_camera_types.h>

#if CONFIG_LCD_QSPI
#include <lcd_qspi_display_service.h>
#endif

#ifdef CONFIG_LVGL
#include "lvgl.h"
#endif

#if CONFIG_ARCH_RISCV && CONFIG_CACHE_ENABLE
#include "cache.h"
#endif


#define TAG "lcd_act"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

//#define LCD_DIAG_DEBUG
#ifdef LCD_DIAG_DEBUG

#define LCD_DEBUG_IO0    GPIO_0

#define LCD_DIAG_DEBUG_INIT()                   \
	do {                                        \
		gpio_dev_unmap(GPIO_2);                 \
		bk_gpio_disable_pull(GPIO_2);           \
		bk_gpio_enable_output(GPIO_2);          \
		bk_gpio_set_output_low(GPIO_2);         \
		\
		gpio_dev_unmap(GPIO_3);                 \
		bk_gpio_disable_pull(GPIO_3);           \
		bk_gpio_enable_output(GPIO_3);          \
		bk_gpio_set_output_low(GPIO_3);         \
		\
		gpio_dev_unmap(GPIO_4);                 \
		bk_gpio_disable_pull(GPIO_4);           \
		bk_gpio_enable_output(GPIO_4);          \
		bk_gpio_set_output_low(GPIO_4);         \
		\
		gpio_dev_unmap(LCD_DEBUG_IO0);                 \
		bk_gpio_disable_pull(LCD_DEBUG_IO0);           \
		bk_gpio_enable_output(LCD_DEBUG_IO0);          \
		bk_gpio_set_output_low(LCD_DEBUG_IO0);         \
		\
	} while (0)

#define LCD_DECODER_START()                 bk_gpio_set_output_high(GPIO_2)
#define LCD_DECODER_END()                   bk_gpio_set_output_low(GPIO_2)

#define LCD_ROTATE_START()                  bk_gpio_set_output_high(LCD_DEBUG_IO0)
#define LCD_ROTATE_END()                    bk_gpio_set_output_low(LCD_DEBUG_IO0)

#define LCD_DISPLAY_START()                 bk_gpio_set_output_high(GPIO_4)
#define LCD_DISPLAY_END()                   bk_gpio_set_output_low(GPIO_4)

#define LCD_DISPLAY_ISR_ENTRY()             bk_gpio_set_output_high(GPIO_3)
#define LCD_DISPLAY_ISR_OUT()               bk_gpio_set_output_low(GPIO_3)
#else

#define LCD_DIAG_DEBUG_INIT()

#define LCD_DECODER_START()
#define LCD_DECODER_END()

#define LCD_ROTATE_START()
#define LCD_ROTATE_END()

#define LCD_DISPLAY_START()
#define LCD_DISPLAY_END()

#define LCD_DISPLAY_ISR_ENTRY()
#define LCD_DISPLAY_ISR_OUT()

#endif


#define LCD_DRIVER_FRAME_FREE(frame)			\
	do {										\
		if (frame								\
			&& lcd_info.lvgl_frame != frame)		\
		{										\
			lcd_info.fb_free(frame);		\
			frame = NULL;						\
		}										\
	} while (0)

extern media_debug_t *media_debug;
extern void flush_all_dcache(void);



beken_semaphore_t step_sem;

/**< camera jpeg display */
beken_semaphore_t camera_display_sem;
bool camera_display_task_running = false;
static beken_thread_t camera_display_task = NULL;

/**< lcd jpeg display */
#define DISPLAY_PIPELINE_TASK
#ifdef DISPLAY_PIPELINE_TASK
beken_semaphore_t jpeg_display_sem;
bool jpeg_display_task_running = false;
static beken_thread_t jpeg_display_task = NULL;
#endif

static frame_buffer_t *dbg_jpeg_frame = NULL;
static frame_buffer_t *dbg_display_frame = NULL;

lcd_info_t lcd_info = {0};
static lcd_config_t lcd_config = DEFAULT_LCD_CONFIG();

#if (CONFIG_LCD_QSPI && CONFIG_LVGL)

#else
static u8 g_gui_need_to_wait = BK_FALSE;
#endif
static u8 g_dma2d_use_flag = 0;

///char *g_blend_name = NULL;
#if (CONFIG_LCD_DMA2D_BLEND || CONFIG_LCD_FONT_BLEND)
static lcd_blend_data_t g_blend_data = {0};
#endif
extern u64 riscv_get_mtimer(void);


static char *frame_suffix(pixel_format_t fmt)
{
	switch (fmt)
	{
		case PIXEL_FMT_UNKNOW:
			return ".unknow";
		case PIXEL_FMT_JPEG:
			return "_dvp.jpg";
		case PIXEL_FMT_H264:
			return ".h264";
		case PIXEL_FMT_RGB565_LE:
			return ".rgb565le";
		case PIXEL_FMT_YUYV:
			return ".yuyv";
		case PIXEL_FMT_UYVY:
			return ".uyvy";
		case PIXEL_FMT_YYUV:
			return ".yyuv";
		case PIXEL_FMT_UVYY:
			return ".uvyy";
		case PIXEL_FMT_VUYY:
			return ".vuyy";
		default:
			break;
	}

	return ".unknow";
}

#ifdef DISPLAY_PIPELINE_TASK
static void jpeg_display_task_entry(beken_thread_arg_t data)
{
	frame_buffer_t *frame = NULL;
	media_rotate_t rotate = (media_rotate_t)data;

	LOGI("%s, rotate: %d\n", __func__, rotate);

	rtos_set_semaphore(&jpeg_display_sem);

	while (jpeg_display_task_running)
	{
		rotate = lcd_info.rotate;

		frame = frame_buffer_fb_display_pop_wait();

		if (frame == NULL)
		{
			LOGD("read display frame NULL\n");
			continue;
		}

//		if (lcd_info.resize)
//		{
//			LOGD("%d\n", lcd_info.resize_ppi);
//			frame = lcd_driver_resize_frame(frame, lcd_info.resize_ppi);
//
//			if (frame == NULL)
//			{
//				LOGD("resize frame NULL\n");
//				continue;
//			}
//		}
		if (lcd_info.rotate_en)
		{
			frame = lcd_driver_rotate_frame(frame, rotate);

			if (frame == NULL)
			{
				LOGD("rotate frame NULL\n");
				continue;
			}
		}
#if CONFIG_LCD_FONT_BLEND
			if(lcd_info.font_draw)
				lcd_font_handle(frame);
#endif
#if CONFIG_LCD_DMA2D_BLEND
			if(lcd_info.dma2d_blend)
				//lcd_dma2d_handle(frame);
#endif
		lcd_driver_display_frame(frame);
	}

	LOGI("jpeg display task exit\n");

	jpeg_display_task = NULL;
	rtos_set_semaphore(&jpeg_display_sem);
	rtos_delete_thread(NULL);
}


void jpeg_display_task_start(media_rotate_t rotate)
{
	bk_err_t ret;

	if (jpeg_display_task != NULL)
	{
		LOGE("%s jpeg_display_thread already running\n", __func__);
		return;
	}

	frame_buffer_fb_register(MODULE_LCD, FB_INDEX_DISPLAY);

	ret = rtos_init_semaphore_ex(&jpeg_display_sem, 1, 0);

	if (BK_OK != ret)
	{
		LOGE("%s semaphore init failed\n", __func__);
		return;
	}

	jpeg_display_task_running = true;

	ret = rtos_create_thread(&jpeg_display_task,
	                         4,
	                         "jpeg_display_thread",
	                         (beken_thread_function_t)jpeg_display_task_entry,
	                         2560,
	                         (beken_thread_arg_t)rotate);

	if (BK_OK != ret)
	{
		LOGE("%s jpeg_display_thread init failed\n");
		return;
	}

	ret = rtos_get_semaphore(&jpeg_display_sem, BEKEN_NEVER_TIMEOUT);

	if (BK_OK != ret)
	{
		LOGE("%s decoder_sem get failed\n", __func__);
	}
}

void jpeg_display_task_stop(void)
{
	bk_err_t ret;

	if (jpeg_display_task_running == false)
	{
		LOGI("%s already stop\n", __func__);
		return;
	}

	jpeg_display_task_running = false;

	frame_buffer_fb_deregister(MODULE_LCD);

	ret = rtos_get_semaphore(&jpeg_display_sem, BEKEN_NEVER_TIMEOUT);

	if (BK_OK != ret)
	{
		LOGE("%s jpeg_display_sem get failed\n");
	}

	LOGI("%s complete\n", __func__);

	ret = rtos_deinit_semaphore(&jpeg_display_sem);

	if (BK_OK != ret)
	{
		LOGE("%s jpeg_display_sem deinit failed\n");
	}
}

#endif

#ifdef CONFIG_FACE_DETECTION
#include "facedetectcnn.h"
int w = 128;
int h = 96;
int n = 3;
//int step = 128 * 3;
#define DETECT_BUFFER_SIZE 0x2000
extern u64 riscv_get_mtimer(void);
#endif
static void camera_display_task_entry(beken_thread_arg_t data)
{
	frame_buffer_t *jpeg_frame = NULL;
	frame_buffer_t *dec_frame = NULL;
	media_rotate_t rotate = (media_rotate_t)data;

	rtos_set_semaphore(&camera_display_sem);
#ifdef CONFIG_FACE_DETECTION
	int *pResults = NULL;
    unsigned char *pBuffer = (unsigned char *)os_malloc(0X2000);
	unsigned char *data_in = (unsigned char *)os_malloc(w * h * 3);
#endif

	while (camera_display_task_running)
	{
		if (lcd_info.step_mode == false)
		{
			/* Normal display workflow */

			jpeg_frame = frame_buffer_fb_read(MODULE_DECODER);

			if (jpeg_frame == NULL)
			{
				LOGD("%s read jpeg frame NULL\n", __func__);
				continue;
			}

			// if dvp work jpeg_yuv mode, use yuv frame display direct and jpeg data not decoder
			if (jpeg_frame->mix)
			{
				frame_buffer_fb_free(jpeg_frame, MODULE_DECODER);
				continue;
			}

			dec_frame = lcd_driver_decoder_frame(jpeg_frame, lcd_info.decode_mode);

			frame_buffer_fb_free(jpeg_frame, MODULE_DECODER);

			if (dec_frame == NULL)
			{
				LOGD("jpeg decoder frame NULL\n");
				continue;
			}

//			yuyv_to_yyuv(dec_frame);


#ifdef CONFIG_FACE_DETECTION
			flush_all_dcache();

			yuv422packed_to_rgb24(dec_frame->frame, data_in, dec_frame->width, dec_frame->height, w, h);
			float scx = (float)dec_frame->width / w;
			float scy = (float)dec_frame->height / h;
			//int scx = 10;
			//int scy = 10;
			//os_printf("nn in\n");
			uint64_t before = (uint64_t)riscv_get_mtimer();
			pResults = facedetect_cnn(pBuffer, data_in, w, h, w * 3);
			uint64_t after = (uint64_t)riscv_get_mtimer();
			//os_printf("nn out\n");
			int num = pResults ? *pResults : 0;
			//os_printf("face %d\n", num);
			os_printf("detection time: %d ms\n", (uint32_t)(after - before) / 26000);
			for (int i = 0; i < num; i++)
			{
				int* p = ((int*)(pBuffer + 4)) + 20 * i;
				int confidence = p[0];
				int x1 = (int)(p[1] * scx);
				if((x1 % 4) != 0){
					x1 = x1 + 4 - (x1 % 4);
				}
				int y1 = (int)(p[2] * scy);
				int w1 = (int)(p[3] * scx);
				if((w1 % 4) != 0){
					w1 = w1 + 4 - (w1 % 4);
				}
				int h1 = (int)(p[4] * scy);
				os_printf("%d: %d,%d,%d,%d\n", confidence, x1, y1, w1, h1);
				draw_box_yuv(dec_frame->frame, x1/2, y1/2, x1/2 + w1/2, y1/2 + h1/2, 16, 128, 128, dec_frame->width, dec_frame->height);
			}
			flush_all_dcache();

#endif

#ifdef  DISPLAY_PIPELINE_TASK
			frame_buffer_fb_push(dec_frame);
#else
			rotate = lcd_info.rotate;
			if (lcd_info.resize)
			{
				dec_frame = lcd_driver_resize_frame(dec_frame, lcd_info.resize_ppi);

				if (dec_frame == NULL)
				{
					LOGD("resize frame NULL\n");
					continue;
				}
			}

			if (rotate != ROTATE_NONE)
			{
				dec_frame = lcd_driver_rotate_frame(dec_frame, rotate);

				if (dec_frame == NULL)
				{
					LOGD("rotate frame NULL\n");
					continue;
				}
			}

#if CONFIG_LCD_FONT_BLEND
			if(lcd_info.font_draw)
				lcd_font_handle(dec_frame);
#endif
#if CONFIG_LCD_DMA2D_BLEND
			if(lcd_info.dma2d_blend)
				lcd_dma2d_handle(dec_frame);
#endif

			lcd_driver_display_frame(dec_frame);
#endif
		}
		else
		{
			/* Debug display workflow */
			if (rtos_get_semaphore(&step_sem, BEKEN_NEVER_TIMEOUT))
			{
				LOGE("%s step_sem get failed\n", __func__);
			}

			if (jpeg_frame)
			{
				LOGI("free frame: %u\n", jpeg_frame->sequence);
				frame_buffer_fb_free(jpeg_frame, MODULE_DECODER);
				jpeg_frame = NULL;
			}

			jpeg_frame = frame_buffer_fb_read(MODULE_DECODER);

			if (jpeg_frame == NULL)
			{
				LOGD("read jpeg frame NULL\n");
				continue;
			}

			dbg_jpeg_frame = jpeg_frame;

			LOGI("Got jpeg frame seq: %u, ptr: %p, length: %u, fmt: %u\n",
			     dbg_jpeg_frame->sequence, dbg_jpeg_frame->frame, dbg_jpeg_frame->length, dbg_jpeg_frame->fmt);

			dec_frame = lcd_driver_decoder_frame(jpeg_frame, lcd_info.decode_mode);

			if (dec_frame == NULL)
			{
				LOGD("jpeg decoder frame NULL\n");
				continue;
			}

			if (lcd_info.resize == true)
			{
				//dec_frame = lcd_driver_resize_frame(dec_frame, lcd_info.resize_ppi);

				if (dec_frame == NULL)
				{
					LOGD("resize frame NULL\n");
					continue;
				}
			}

			if (rotate == true)
			{
				dec_frame = lcd_driver_rotate_frame(dec_frame, lcd_info.rotate);

				if (dec_frame == NULL)
				{
					LOGD("rotate frame NULL\n");
					continue;
				}
			}

			dbg_display_frame = dec_frame;

			LOGI("Got display frame seq: %u, ptr: %p, length: %u, fmt: %u\n",
			     dbg_display_frame->sequence, dbg_display_frame->frame, dbg_display_frame->length, dbg_display_frame->fmt);

			lcd_driver_display_frame(dec_frame);

		}
	}
	LOGI("camera display task exit\n");
#ifdef CONFIG_FACE_DETECTION
	os_free(pBuffer);
	os_free(data_in);
#endif
	camera_display_task = NULL;
	rtos_set_semaphore(&camera_display_sem);

	rtos_delete_thread(NULL);
}

void camera_display_task_start(media_rotate_t rotate)
{
	bk_err_t ret;

	if (camera_display_task != NULL)
	{
		LOGE("%s camera_display_thread already running\n", __func__);
		return;
	}

	frame_buffer_fb_register(MODULE_DECODER, FB_INDEX_JPEG);

	ret = rtos_init_semaphore_ex(&camera_display_sem, 1, 0);

	if (BK_OK != ret)
	{
		LOGE("%s semaphore init failed\n", __func__);
		return;
	}

	camera_display_task_running = true;

	ret = rtos_create_thread(&camera_display_task,
	                         4,
	                         "camera_display_thread",
	                         (beken_thread_function_t)camera_display_task_entry,
	                         1536,
	                         (beken_thread_arg_t)rotate);

	if (BK_OK != ret)
	{
		LOGE("%s camera_display_task init failed\n");
		return;
	}

	ret = rtos_get_semaphore(&camera_display_sem, BEKEN_NEVER_TIMEOUT);

	if (BK_OK != ret)
	{
		LOGE("%s camera_display_sem get failed\n", __func__);
	}
}

void camera_display_task_stop(void)
{
	bk_err_t ret;
	if (camera_display_task_running == false)
	{
		LOGI("%s already stop\n", __func__);
		return;
	}
	camera_display_task_running = false;

	frame_buffer_fb_deregister(MODULE_DECODER);
	ret = rtos_get_semaphore(&camera_display_sem, BEKEN_NEVER_TIMEOUT);

	if (BK_OK != ret)
	{
		LOGE("%s camera_display_sem get failed\n");
	}

	ret = rtos_deinit_semaphore(&camera_display_sem);

	if (BK_OK != ret)
	{
		LOGE("%s decoder_sem deinit failed\n");
	}

	LOGI("%s complete\n", __func__);
}

__attribute__((section(".itcm_sec_code"))) static void lcd_driver_display_rgb_isr(void)
{
	LCD_DISPLAY_ISR_ENTRY();

	media_debug->isr_lcd++;

	if (lcd_info.enable == false)
	{
		return;
	}

	if (lcd_info.pingpong_frame != NULL)
	{
//		lcd_driver_ppi_set(lcd_info.pingpong_frame->width, lcd_info.pingpong_frame->height);
//		bk_lcd_set_yuv_mode(lcd_info.pingpong_frame->fmt);

		if (lcd_info.display_frame != NULL)
		{
			media_debug->fps_lcd++;

			LCD_DRIVER_FRAME_FREE(lcd_info.display_frame);

			lcd_info.display_frame = lcd_info.pingpong_frame;
			lcd_info.pingpong_frame = NULL;
			lcd_driver_set_display_base_addr((uint32_t)lcd_info.display_frame->frame);
			rtos_set_semaphore(&lcd_info.disp_sem);
		}
		else
		{
			lcd_info.display_frame = lcd_info.pingpong_frame;
			lcd_info.pingpong_frame = NULL;
			rtos_set_semaphore(&lcd_info.disp_sem);
		}
	}

	LCD_DISPLAY_ISR_OUT();
}

static void lcd_driver_display_mcu_isr(void)
{
	LCD_DISPLAY_ISR_ENTRY();

	media_debug->isr_lcd++;

	if (lcd_info.enable == false)
	{
		return;
	}

	if (lcd_info.pingpong_frame != NULL)
	{
		media_debug->fps_lcd++;
		LCD_DRIVER_FRAME_FREE(lcd_info.display_frame);

		lcd_info.display_frame = lcd_info.pingpong_frame;
		lcd_info.pingpong_frame = NULL;
		bk_lcd_8080_start_transfer(0);

		rtos_set_semaphore(&lcd_info.disp_sem);
	}

	LCD_DISPLAY_ISR_OUT();
}

frame_buffer_t *lcd_driver_decoder_frame(frame_buffer_t *frame, media_decode_mode_t decode_mode)
{
	bk_err_t ret = BK_FAIL;
	frame_buffer_t *dec_frame = NULL;
	uint64_t before, after;

	if (lcd_info.enable == false)
	{
		return dec_frame;
	}

	rtos_lock_mutex(&lcd_info.dec_lock);

	if (lcd_info.decoder_en == false)
	{
		rtos_unlock_mutex(&lcd_info.dec_lock);
		return dec_frame;
	}

#if CONFIG_ARCH_RISCV
	before = riscv_get_mtimer();
#else
	before = 0;
#endif
	lcd_info.decoder_frame = lcd_info.fb_malloc(frame->width * frame->height * 2);

	if (lcd_info.decoder_frame == NULL)
	{
		LOGE("malloc decoder NULL\n");
		goto out;
	}
	LCD_DECODER_START();

	lcd_info.decoder_frame->sequence = frame->sequence;

	if (lcd_info.decode_mode == HARDWARE_DECODING)
	{
		lcd_info.decoder_frame->fmt = PIXEL_FMT_VUYY;
		ret = lcd_hw_decode_start(frame, lcd_info.decoder_frame);
		if (ret != BK_OK)
		{
			LOGE("%s hw decoder error\n", __func__);
			LCD_DRIVER_FRAME_FREE(lcd_info.decoder_frame);
			goto out;
		}
	}
	else
	{
		lcd_info.decoder_frame->fmt = PIXEL_FMT_YUYV;
#if CONFIG_LCD_SW_DECODE
		if (lcd_info.decode_mode == SOFTWARE_DECODING_MAJOR)
		{
			ret = lcd_sw_jpegdec_start(frame, lcd_info.decoder_frame);
#if (CONFIG_TASK_WDT)
			extern void bk_task_wdt_feed(void);
			bk_task_wdt_feed();
#endif
			if (ret != BK_OK)
			{
				LOGE("%s sw decoder error\n", __func__);
				LCD_DRIVER_FRAME_FREE(lcd_info.decoder_frame);
				goto out;
			}
		}
		else
		{
			ret = lcd_sw_minor_jpegdec_start(frame, lcd_info.decoder_frame);
			if (ret != BK_OK)
			{
				LOGE("%s sw decoder error\n", __func__);
				LCD_DRIVER_FRAME_FREE(lcd_info.decoder_frame);
				goto out;
			}
		}
#endif
	}
	LCD_DECODER_END();

out:

    if (lcd_info.decoder_frame == NULL)
    {
        LOGI("%s decoder failed\n", __func__);
        ret = BK_FAIL;
    }
    else
    {
        dec_frame = lcd_info.decoder_frame;
        lcd_info.decoder_frame = NULL;
    }

	rtos_unlock_mutex(&lcd_info.dec_lock);
#if CONFIG_ARCH_RISCV
	after = riscv_get_mtimer();
#else
	after = 0;
#endif
	LOGD("decoder time: %lu\n", (after - before) / 26000);

	return dec_frame;
}



frame_buffer_t *lcd_driver_rotate_frame(frame_buffer_t *frame, media_rotate_t rotate)
{
	frame_buffer_t *rot_frame = NULL;
	uint64_t before, after;
	bk_err_t ret = BK_FAIL;

	if (rotate == ROTATE_NONE)
	{
		return frame;
	}
	LCD_ROTATE_START();

	if (lcd_info.enable == false)
	{
		LOGE("%s lcd_info.enable == false\r\n", __func__);
		LCD_DRIVER_FRAME_FREE(frame);
		return rot_frame;
	}

	rtos_lock_mutex(&lcd_info.rot_lock);

	if (lcd_info.rotate_en == false)
	{
		rtos_unlock_mutex(&lcd_info.rot_lock);
		return frame;
	}
#if CONFIG_ARCH_RISCV
	before = riscv_get_mtimer();
#else
	before = 0;
#endif
	if (lcd_info.rotate_frame == NULL)
	{
		lcd_info.rotate_frame = lcd_info.fb_malloc(frame->width * frame->height * 2);
	}
	if (lcd_info.rotate_frame == NULL)
	{
		LOGE("malloc rotate_frame NULL\n");
		goto out;
	}

	if ((rotate == ROTATE_180) || lcd_info.hw_yuv2rgb)
	{
		lcd_info.rotate_frame->height = frame->height;
		lcd_info.rotate_frame->width = frame->width;
	}
	else
	{
		lcd_info.rotate_frame->height = frame->width;
		lcd_info.rotate_frame->width = frame->height;
	}

	lcd_info.rotate_frame->fmt = frame->fmt;
	lcd_info.rotate_frame->sequence = frame->sequence;
	lcd_info.rotate_frame->length = frame->width * frame->height * 2;

	if ((lcd_info.rotate_mode == HW_ROTATE))
	{
#if CONFIG_HW_ROTATE_PFC
		ret = lcd_hw_rotate_yuv2rgb565(frame, lcd_info.rotate_frame, rotate);
#endif
	}
	else
	{
		ret = lcd_sw_rotate(frame, lcd_info.rotate_frame, rotate);
	}
	if (ret != BK_OK)
	{
		LOGE("%s rotate error: %d\n", __func__, ret);
		LCD_DRIVER_FRAME_FREE(lcd_info.rotate_frame);
		goto out;
	}

#if CONFIG_ARCH_RISCV
	after = riscv_get_mtimer();
#else
	after = 0;
#endif
	LOGD("rotate time: %lu\n", (after - before) / 26000);

	LCD_ROTATE_END();

out:
	if(lcd_info.rotate_frame == NULL)
	{
		media_debug->err_rot++;
	}
	else
	{
		rot_frame = lcd_info.rotate_frame;
		lcd_info.rotate_frame = NULL;
	}
	LCD_DRIVER_FRAME_FREE(frame);
	rtos_unlock_mutex(&lcd_info.rot_lock);

	return rot_frame;
}


bk_err_t lcd_driver_display_frame_wait(uint32_t timeout_ms)
{
	bk_err_t ret = rtos_get_semaphore(&lcd_info.disp_sem, timeout_ms);

	if (ret != BK_OK)
	{
		LOGE("%s semaphore get failed: %d\n", __func__, ret);
	}

	return ret;
}

bk_err_t lcd_driver_display_frame_sync(frame_buffer_t *frame, bool wait)
{
	bk_err_t ret = BK_FAIL;
	uint64_t before, after;

	if (lcd_info.enable == false)
	{
		LCD_DRIVER_FRAME_FREE(frame);
		return ret;
	}

	LCD_DISPLAY_START();
#if CONFIG_ARCH_RISCV
	before = riscv_get_mtimer();
#else
	before = 0;
#endif
	rtos_lock_mutex(&lcd_info.disp_lock);

	if (lcd_info.display_en == false)
	{
		LCD_DRIVER_FRAME_FREE(frame);
		rtos_unlock_mutex(&lcd_info.disp_lock);
		return ret;
	}

	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();

    if (lcd_info.display_frame == NULL)
    {
        lcd_info.pingpong_frame = frame;

        if (lcd_info.lcd_device->type == LCD_TYPE_QSPI) {
#if CONFIG_LCD_QSPI
            bk_lcd_qspi_display((uint32_t)frame->frame);
#endif
        } else {
            lcd_driver_ppi_set(frame->width, frame->height);
            bk_lcd_set_yuv_mode(frame->fmt);

            lcd_driver_set_display_base_addr((uint32_t)frame->frame);
            lcd_driver_display_enable();
            LOGI("display start,width, height %d, %d\n", frame->width, frame->height);
        }
    }
    else
    {
        if (lcd_info.pingpong_frame != NULL)
        {
            LCD_DRIVER_FRAME_FREE(lcd_info.pingpong_frame);
        }

        lcd_info.pingpong_frame = frame;

        if (lcd_info.lcd_device->type == LCD_TYPE_MCU8080)
        {
            lcd_driver_ppi_set(frame->width, frame->height);
            bk_lcd_set_yuv_mode(frame->fmt);
            lcd_driver_set_display_base_addr((uint32_t)frame->frame);

            lcd_driver_display_continue();
        }
#if CONFIG_LCD_QSPI
        else if (lcd_info.lcd_device->type == LCD_TYPE_QSPI) {
            bk_lcd_qspi_display((uint32_t)frame->frame);
        }
#endif
    }

	GLOBAL_INT_RESTORE();

	if(wait == BK_TRUE)
	{
		ret = lcd_driver_display_frame_wait(BEKEN_NEVER_TIMEOUT);
	}

	rtos_unlock_mutex(&lcd_info.disp_lock);
#if CONFIG_ARCH_RISCV
	after = riscv_get_mtimer();
#else
	after = 0;
#endif
	LOGD("display time: %lu\n", (after - before) / 26000);

	LCD_DISPLAY_END();

	return ret;
}

void dma2d_memcpy_psram_wait_last_transform_is_finish(void)
{
	if(1 == g_dma2d_use_flag)
	{
		while (bk_dma2d_is_transfer_busy()) {}
		g_dma2d_use_flag = 0;
	}
}

void dma2d_memcpy_psram_wait_last_transform(void *Psrc, void *Pdst, uint32_t xsize, uint32_t ysize, uint32_t src_offline, uint32_t dest_offline)
{
	dma2d_memcpy_psram_wait_last_transform_is_finish();
	dma2d_memcpy_psram(Psrc, Pdst, xsize, ysize, src_offline, dest_offline);
	g_dma2d_use_flag = 1;
}


void lcd_driver_display_frame_with_gui(void *buffer, int width, int height)
{
#if (CONFIG_LCD_QSPI && CONFIG_LVGL)
    if(lcd_info.lvgl_frame) {
        lcd_info.lvgl_frame->frame = buffer;
        lcd_driver_display_frame_sync(lcd_info.lvgl_frame, BK_FALSE);
    }
#else

    if(g_gui_need_to_wait)
	{
		lcd_driver_display_frame_wait(2000);

		g_gui_need_to_wait = BK_FALSE;
	}

    if(lcd_info.lvgl_frame){
        lcd_info.lvgl_frame->fmt = PIXEL_FMT_RGB565;
        lcd_info.lvgl_frame->frame = buffer;
        lcd_info.lvgl_frame->width = width;
        lcd_info.lvgl_frame->height = height;

        if (lcd_info.lcd_device->type == LCD_TYPE_RGB565)
        {
            lcd_driver_display_frame_sync(lcd_info.lvgl_frame, BK_TRUE);
            g_gui_need_to_wait = BK_FALSE;
        }
        else
        {
            lcd_driver_display_frame_sync(lcd_info.lvgl_frame, BK_TRUE);
            g_gui_need_to_wait = BK_FALSE;
        }
    }
    else{
        LOGI("[%s][%d] fb malloc fail\r\n", __FUNCTION__, __LINE__);
    }
#endif
}

bk_err_t lcd_driver_display_frame(frame_buffer_t *frame)
{
	return lcd_driver_display_frame_sync(frame, BK_TRUE);
}


void lcd_blend_font_handle(param_pak_t *param)
{
	int ret = BK_OK;
#if (CONFIG_LCD_DMA2D_BLEND || CONFIG_LCD_FONT_BLEND)
	lcd_blend_msg_t  *blend_data = (lcd_blend_msg_t *)param->param;
	if(blend_data != NULL)
	{
		LOGD("lcd_EVENT_LCD_BLEND_IND type=%d on=%d\n", blend_data->lcd_blend_type, blend_data->blend_on);

		if ((blend_data->lcd_blend_type & LCD_BLEND_WIFI) == LCD_BLEND_WIFI)
		{
			if(blend_data->blend_on == 0)
			{
				g_blend_data.lcd_blend_type &= (~LCD_BLEND_WIFI);
			}
			else
			{
				g_blend_data.lcd_blend_type |= LCD_BLEND_WIFI;
				g_blend_data.wifi_data = blend_data->data[0];

				LOGD("g_blend_data.wifi_data =%d\n", g_blend_data.wifi_data );
			}
		}
		if ((blend_data->lcd_blend_type & LCD_BLEND_TIME) == LCD_BLEND_TIME)
		{
			if(blend_data->blend_on == 0)
			{
				g_blend_data.lcd_blend_type &= (~LCD_BLEND_TIME);
			}
			else
			{
				g_blend_data.lcd_blend_type |= LCD_BLEND_TIME;
				os_memcpy(g_blend_data.time_data, blend_data->data, MAX_BLEND_NAME_LEN);
				LOGD("g_blend_data.time_data =%s\n", g_blend_data.time_data );
			}
		}
		if ((blend_data->lcd_blend_type & LCD_BLEND_DATA) == LCD_BLEND_DATA)
		{
			if(blend_data->blend_on == 0)
			{
				g_blend_data.lcd_blend_type &= (~LCD_BLEND_DATA);
			}
			else
			{
				g_blend_data.lcd_blend_type |= LCD_BLEND_DATA;
				os_memcpy(g_blend_data.year_to_data, blend_data->data, MAX_BLEND_NAME_LEN);
				LOGD("g_blend_data.chs =%s\n", g_blend_data.year_to_data);
			}
		}
		if ((blend_data->lcd_blend_type & LCD_BLEND_VERSION) == LCD_BLEND_VERSION)
		{
			if(blend_data->blend_on == 0)
			{
				g_blend_data.lcd_blend_type &= (~LCD_BLEND_VERSION);
			}
			else
			{
				g_blend_data.lcd_blend_type |= LCD_BLEND_VERSION;
				os_memcpy(g_blend_data.ver_data, blend_data->data, MAX_BLEND_NAME_LEN);
				LOGE("g_blend_data.ver_data =%s\n", g_blend_data.ver_data );
			}
		}
	}
#endif
	MEDIA_EVT_RETURN(param, ret);
}

bk_err_t lcd_font_handle(frame_buffer_t *frame)
{
#if CONFIG_LCD_FONT_BLEND
	lcd_font_config_t lcd_font_config = {0};

	uint16_t start_x = 0;
	uint16_t start_y = 0;
	uint32_t frame_addr_offset = 0;

	if(g_blend_data.lcd_blend_type == 0)
	{
		return BK_OK;
	}

	if ((lcd_info.lcd_width < frame->width) || (lcd_info.lcd_height < frame->height)) //for lcd size is small then frame image size
	{
		if (lcd_info.lcd_width < frame->width)
			start_x = (frame->width - lcd_info.lcd_width) / 2;
		if (lcd_info.lcd_height < frame->height)
			start_y = (frame->height - lcd_info.lcd_height) / 2;
	}

	if ((g_blend_data.lcd_blend_type & LCD_BLEND_TIME) != 0)         /// start display lcd (0,0)
	{
		frame_addr_offset = (start_y * frame->width + start_x) * 2;

		lcd_font_config.pbg_addr = (uint8_t *)(frame->frame + frame_addr_offset);
		lcd_font_config.bg_offline = frame->width - CLOCK_LOGO_W;
		lcd_font_config.xsize = CLOCK_LOGO_W;
		lcd_font_config.ysize = CLOCK_LOGO_H;
		lcd_font_config.str_num = 1;
		if (frame->fmt == PIXEL_FMT_VUYY)
			lcd_font_config.font_format = FONT_VUYY;
		else if (frame->fmt == PIXEL_FMT_YUYV)
			lcd_font_config.font_format = FONT_YUYV;
		else
			lcd_font_config.font_format = FONT_RGB565;

		lcd_font_config.str[0] = (font_str_t){(const char *)g_blend_data.time_data, FONT_WHITE, font_digit_Roboto53, 0,0};
		lcd_font_config.bg_data_format = frame->fmt;
		lcd_font_config.bg_width = frame->width;
		lcd_font_config.bg_height = frame->height;
		lcd_driver_font_blend(&lcd_font_config);
	}

	if ((g_blend_data.lcd_blend_type & LCD_BLEND_WIFI) != 0)      /// start display lcd (lcd_width,0)
	{
		lcd_blend_t lcd_blend = {0};
		LOGD("lcd wifi blend level =%d \n", g_blend_data.wifi_data);
		frame_addr_offset = (start_y * frame->width + start_x + (lcd_info.lcd_width - WIFI_LOGO_W)) * 2;

		lcd_blend.pfg_addr = (uint8_t *)wifi_logo[g_blend_data.wifi_data];
		lcd_blend.pbg_addr = (uint8_t *)(frame->frame + frame_addr_offset);
		lcd_blend.fg_offline = 0;
		lcd_blend.bg_offline = frame->width - WIFI_LOGO_W;
		lcd_blend.xsize = WIFI_LOGO_W;
		lcd_blend.ysize = WIFI_LOGO_H;
		lcd_blend.fg_alpha_value = FG_ALPHA;
		lcd_blend.fg_data_format = ARGB8888;
		lcd_blend.bg_data_format = frame->fmt;
		lcd_blend.bg_width = frame->width;
		lcd_blend.bg_height = frame->height;
		lcd_driver_blend(&lcd_blend);
	}
	if ((g_blend_data.lcd_blend_type & LCD_BLEND_DATA) != 0)   /// tart display lcd (DATA_POSTION_X,DATA_POSTION_Y)
	{
		if ((DATA_POSTION_X + DATA_LOGO_W) > lcd_info.lcd_width)
			frame_addr_offset = ((start_y + DATA_POSTION_Y + lcd_info.lcd_height - DATA_LOGO_H) * frame->width + start_x) * 2;
		else
			frame_addr_offset = ((start_y + DATA_POSTION_Y) * frame->width + start_x + DATA_POSTION_X) * 2;
		lcd_font_config.pbg_addr = (uint8_t *)(frame->frame + frame_addr_offset);
		lcd_font_config.bg_offline = frame->width - DATA_LOGO_W;
		lcd_font_config.xsize = DATA_LOGO_W;
		lcd_font_config.ysize = DVP_LOGO_H;
		lcd_font_config.str_num = 1;
		if (frame->fmt == PIXEL_FMT_VUYY)
			lcd_font_config.font_format = FONT_VUYY;
		else if (frame->fmt == PIXEL_FMT_YUYV)
			lcd_font_config.font_format = FONT_YUYV;
		else
			lcd_font_config.font_format = FONT_RGB565;

		lcd_font_config.str[0] = (font_str_t){(const char *)("晴转多云, 27℃"), FONT_WHITE, font_digit_black24, 0, 2};
		lcd_font_config.bg_data_format = frame->fmt;
		lcd_font_config.bg_width = frame->width;
		lcd_font_config.bg_height = frame->height;
		lcd_driver_font_blend(&lcd_font_config);

		lcd_font_config.pbg_addr += DVP_LOGO_H * frame->width * 2;
		lcd_font_config.str[0] = (font_str_t){(const char *)("2022-12-12 星期三"), FONT_WHITE, font_digit_black24, 0, 0};
		lcd_driver_font_blend(&lcd_font_config);
	}

	if ((g_blend_data.lcd_blend_type & LCD_BLEND_VERSION) != 0) /// start display lcd (VERSION_POSTION_X,VERSION_POSTION_Y)
	{
		frame_addr_offset = ((start_y + lcd_info.lcd_height - 50) * frame->width + start_x) * 2 + lcd_info.lcd_width - VERSION_LOGO_W;
		lcd_font_config.pbg_addr = (uint8_t *)(frame->frame + frame_addr_offset);
		lcd_font_config.bg_offline = frame->width - VERSION_LOGO_W;
		lcd_font_config.xsize = VERSION_LOGO_W;
		lcd_font_config.ysize = VERSION_LOGO_H;
		lcd_font_config.str_num = 1;
		if (frame->fmt == PIXEL_FMT_VUYY)
			lcd_font_config.font_format = FONT_VUYY;
		else if (frame->fmt == PIXEL_FMT_YUYV)
			lcd_font_config.font_format = FONT_YUYV;
		else
			lcd_font_config.font_format = FONT_RGB565;

		lcd_font_config.str[0] = (font_str_t){(const char *)g_blend_data.ver_data, FONT_WHITE, font_digit_black24, 0, 0};
		lcd_font_config.bg_data_format = frame->fmt;
		lcd_font_config.bg_width = frame->width;
		lcd_font_config.bg_height = frame->height;
		lcd_driver_font_blend(&lcd_font_config);
	}
#endif  //CONFIG_LCD_FONT_BLEND
	return BK_OK;
}

bk_err_t lcd_dma2d_handle(frame_buffer_t *frame)
{
#if CONFIG_LCD_DMA2D_BLEND
	lcd_blend_t lcd_blend = {0};

	if(g_blend_data.lcd_blend_type == 0)
	{
		return BK_OK;
	}
	if ((g_blend_data.lcd_blend_type & LCD_BLEND_WIFI) != 0)      /// start display lcd (xpos,ypos)
	{
		if(g_blend_data.wifi_data > WIFI_LEVEL_MAX - 1)
			return BK_FAIL;
		lcd_blend.pfg_addr = (uint8_t *)wifi_logo[g_blend_data.wifi_data];
		lcd_blend.pbg_addr = (uint8_t *)(frame->frame);
		lcd_blend.xsize = WIFI_LOGO_W;
		lcd_blend.ysize = WIFI_LOGO_H;
		lcd_blend.xpos = lcd_info.lcd_width - WIFI_LOGO_W;
		lcd_blend.ypos = WIFI_LOGO_YPOS;
		lcd_blend.fg_alpha_value = FG_ALPHA;
		lcd_blend.fg_data_format = ARGB8888;
		lcd_blend.bg_data_format = frame->fmt;
		lcd_blend.bg_width = frame->width;
		lcd_blend.bg_height = frame->height;
		lcd_blend.lcd_width = lcd_info.lcd_width;
		lcd_blend.lcd_height = lcd_info.lcd_height;
		lcd_dma2d_driver_blend(&lcd_blend);
	}
#endif
return BK_OK;
}


void lcd_blend_open_handler(param_pak_t *param)
{
	int ret = BK_OK;
	if (param->param == 0)
	{
#if (CONFIG_LCD_DMA2D_BLEND || CONFIG_LCD_FONT_BLEND)
		if (lcd_info.dma2d_blend == true)
		{
			lcd_dma2d_blend_deinit();
			lcd_info.font_draw = false;
			lcd_info.dma2d_blend = false;
			LOGI("%s lcd dma2d blend deinit \r\n", __func__);
		}
#endif
	}
	else
	{
#if CONFIG_LCD_DMA2D_BLEND || ONFIG_LCD_FONT_BLEND
		if ((lcd_info.dma2d_blend != true) && (lcd_info.font_draw != true))
		{
			lcd_info.dma2d_blend = true;
			lcd_info.font_draw = true;

			ret = lcd_blend_malloc_buffer();
			if (ret != BK_OK) {
				LOGE("lcd blend malloc error, lcd blend open fail\r\n");
				lcd_info.dma2d_blend = false;
				lcd_info.font_draw = false;
			}

			if(lcd_info.dma2d_blend)
			{
				ret= lcd_dma2d_blend_init();
					if (ret != BK_OK) {
					LOGE("lcd blend malloc error, lcd blend open fail\r\n");
					lcd_info.dma2d_blend = false;
					lcd_info.font_draw = false;
				}
			}
		}
		else
		{
			LOGI("%s, lcd blend already open, dma2d_blend or font_draw is true\r\n", __func__);
		}
 #endif
	}
	MEDIA_EVT_RETURN(param, ret);
}

static void lcd_act_deinit_display_frame(void)
{
#if (CONFIG_USB_UVC || CONFIG_DVP_CAMERA)
	if (CAMERA_STATE_DISABLED == get_camera_state())
	{
		frame_buffer_fb_deinit(FB_INDEX_DISPLAY);
	}
	else
	{
#if (CONFIG_USB_UVC)
		if (bk_uvc_camera_get_device())
		{
			// uvc is working, but uvc not use display frame buffer, you can deinit
			frame_buffer_fb_deinit(FB_INDEX_DISPLAY);
		}
#endif

#if (CONFIG_DVP_CAMERA)
		media_camera_device_t *dvp_device = bk_dvp_camera_get_device();
		if (dvp_device != NULL && dvp_device->mode == JPEG_YUV_MODE)
		{
			// while dvp sensor work in mix mode, you cannot deinit display frame buffer
			frame_buffer_fb_deinit(FB_INDEX_DISPLAY);
		}
#endif
	}
#else
	frame_buffer_fb_deinit(FB_INDEX_DISPLAY);
#endif
}

void lcd_open_handle(param_pak_t *param)
{
	int ret = BK_OK;

	if (LCD_STATE_DISPLAY == get_lcd_state())
	{
		camera_display_task_start(lcd_info.rotate);
		set_lcd_state(LCD_STATE_ENABLED);
		LOGI("%s: open camera_display_task \n", __func__);
	}

	if (LCD_STATE_ENABLED == get_lcd_state())
	{
		LOGW("%s already open\n", __func__);
		goto out;
	}

	lcd_open_t *lcd_open = (lcd_open_t *)param->param;
	os_memcpy(&lcd_config.lcd_open, lcd_open, sizeof(lcd_open_t));

	if (lcd_config.lcd_open.device_name != NULL)
			lcd_info.lcd_device = get_lcd_device_by_name(lcd_config.lcd_open.device_name);

	if (lcd_info.lcd_device == NULL)
	{
		lcd_info.lcd_device = get_lcd_device_by_ppi(lcd_config.lcd_open.device_ppi);
	}

	if (lcd_info.lcd_device == NULL)
	{
		LOGE("%s lcd device not found\n", __func__);
		goto out;
	}

	LOGI("%s, lcd ppi: %dX%d %s\n", __func__, lcd_info.lcd_device->ppi >> 16, lcd_info.lcd_device->ppi & 0xFFFF, lcd_info.lcd_device->name);

	lcd_info.lcd_width = ppi_to_pixel_x(lcd_info.lcd_device->ppi);
	lcd_info.lcd_height = ppi_to_pixel_y(lcd_info.lcd_device->ppi);
	lcd_info.fb_free  = frame_buffer_fb_direct_free;
	lcd_info.fb_malloc = frame_buffer_fb_display_malloc_wait;

	frame_buffer_fb_init(FB_INDEX_DISPLAY);

    lcd_driver_init(lcd_info.lcd_device);

    if (lcd_info.lcd_device->type == LCD_TYPE_MCU8080) {
        bk_lcd_isr_register(I8080_OUTPUT_EOF, lcd_driver_display_mcu_isr);
    } else {
        bk_lcd_isr_register(RGB_OUTPUT_EOF, lcd_driver_display_rgb_isr);
    }

#if (CONFIG_LCD_FONT_BLEND || CONFIG_LCD_DMA2D_BLEND)
	lcd_info.dma2d_blend = true;
	lcd_info.font_draw = true;
	ret = lcd_blend_malloc_buffer();
	if (ret != BK_OK) {
		LOGE("lcd blend malloc error, lcd blend open fail\r\n");
		lcd_info.dma2d_blend = false;
		lcd_info.font_draw = false;
	}

	if (lcd_info.dma2d_blend)
	{
		ret = lcd_dma2d_blend_init();
			if (ret != BK_OK)
			{
				LOGE("lcd blend malloc error, lcd blend open fail\r\n");
				lcd_info.dma2d_blend = false;
		}
	}
#endif

#if CONFIG_LCD_ROTATE
	lcd_info.hw_yuv2rgb = lcd_config.hw_yuv2rgb;
	if((lcd_info.rotate_en == 1) || (lcd_config.rotate_open == 1) || (lcd_info.hw_yuv2rgb == 1))
	{
		if(!lcd_info.rotate)
			lcd_info.rotate = lcd_config.rotate_angle;
		lcd_info.rotate_en = true;
		lcd_info.rotate_mode = lcd_config.rotate_mode;//in lcd_config set default hw rotate
		ret = lcd_rotate_init(lcd_info.rotate_mode);  //hw no rotate
		if (ret != BK_OK)
			LOGE("%s, lcd_rotate_init fail\r\n", __func__);

		LOGI("rotate mode(1-sw, 2-hw) = %d, rotate angle(0/1/2/3 -->0/90/180/270) = %d\r\n", lcd_info.rotate_mode, lcd_info.rotate);
	}
	else
	{
		LOGI("%s, lcd rotate not open\r\n", __func__);
		lcd_info.rotate_en = false;
	}
#endif
	if (lcd_info.decode_mode == NONE_DECODE)
		lcd_info.decode_mode = lcd_config.decode_mode;
	if (lcd_info.decode_mode == HARDWARE_DECODING)
	{
		lcd_info.decoder_en = true;
#if CONFIG_LCD_HW_DECODE
		lcd_hw_decode_init();
		LOGI("%s, hw decode init ok\r\n", __func__);
#endif
	}
	else
	{
		lcd_info.decoder_en = true;
#if CONFIG_LCD_SW_DECODE
		lcd_sw_decode_init(lcd_info.decode_mode);
		LOGI("%s, lcd SW decode init ok\r\n", __func__);
#endif
	}

	media_debug->fps_lcd= 0;
	media_debug->isr_lcd = 0;

	rtos_init_mutex(&lcd_info.dec_lock);
	rtos_init_mutex(&lcd_info.rot_lock);
	rtos_init_mutex(&lcd_info.resize_lock);
	rtos_init_mutex(&lcd_info.disp_lock);

	lcd_info.resize_en = true;
	lcd_info.display_en = true;

	lcd_info.lvgl_frame = os_malloc(sizeof(frame_buffer_t));
	os_memset(lcd_info.lvgl_frame, 0, sizeof(frame_buffer_t));

	ret = rtos_init_semaphore_ex(&lcd_info.disp_sem, 1, 0);

	if (ret != BK_OK)
	{
		LOGE("%s disp_sem init failed: %d\n", __func__, ret);
		goto out;
	}

	camera_display_task_start(lcd_info.rotate);

#if CONFIG_LCD_QSPI
    bk_lcd_qspi_disp_task_start(lcd_info.lcd_device);
#else
#ifdef DISPLAY_PIPELINE_TASK
	jpeg_display_task_start(lcd_info.rotate);
#endif
#endif

	set_lcd_state(LCD_STATE_ENABLED);
	lcd_info.enable = true;

	lcd_driver_backlight_open();

	LOGI("%s complete\n", __func__);

out:
	MEDIA_EVT_RETURN(param, ret);
}

void lcd_close_handle(param_pak_t *param)
{
	int ret = BK_OK;

	LOGI("%s\n", __func__);


	if (step_sem != NULL)
	{
		if (rtos_set_semaphore(&step_sem))
		{
			LOGE("%s step_sem set failed\n", __func__);
		}

		if (rtos_deinit_semaphore(&step_sem))
		{
			LOGE("%s step_sem deinit failed\n");
		}
		step_sem = NULL;
	}

	if (LCD_STATE_DISABLED == get_lcd_state())
	{
		LOGW("%s already close\n", __func__);
		goto out;
	}

	lcd_driver_backlight_close();

	camera_display_task_stop();

#if CONFIG_LCD_QSPI
    bk_lcd_qspi_disp_task_stop();
#else
#ifdef DISPLAY_PIPELINE_TASK
	jpeg_display_task_stop();
#endif
#endif

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	lcd_info.enable = false;
	GLOBAL_INT_RESTORE();

	rtos_lock_mutex(&lcd_info.dec_lock);
	lcd_info.decoder_en = false;
	rtos_unlock_mutex(&lcd_info.dec_lock);

	rtos_lock_mutex(&lcd_info.rot_lock);
	lcd_info.rotate_en = false;
	rtos_unlock_mutex(&lcd_info.rot_lock);

	rtos_lock_mutex(&lcd_info.resize_lock);
	lcd_info.resize_en = false;
	rtos_unlock_mutex(&lcd_info.resize_lock);

	rtos_lock_mutex(&lcd_info.disp_lock);
	lcd_info.display_en = false;
	rtos_unlock_mutex(&lcd_info.disp_lock);

    lcd_driver_deinit();

	if(lcd_info.decode_mode == HARDWARE_DECODING)
	{
#if CONFIG_LCD_HW_DECODE
		lcd_hw_decode_deinit();
		LOGW("%s already close\n", __func__);
#endif
	}
	else
	{
#if CONFIG_LCD_SW_DECODE
		if(lcd_info.decode_mode != NONE_DECODE)
			lcd_sw_decode_deinit(lcd_info.decode_mode);
#endif
	}
#if CONFIG_LCD_ROTATE
	lcd_rotate_deinit();
#endif

#if (CONFIG_LCD_DMA2D_BLEND || CONFIG_LCD_FONT_BLEND)
	if (lcd_info.dma2d_blend || lcd_info.font_draw)
		lcd_blend_free_buffer();
	if (lcd_info.dma2d_blend)
		lcd_dma2d_blend_deinit();
	lcd_info.dma2d_blend = false;
	lcd_info.font_draw = false;
#endif

	if (lcd_info.rotate_frame)
	{
		lcd_info.fb_free(lcd_info.rotate_frame);
		lcd_info.rotate_frame = NULL;
	}

	if (lcd_info.decoder_frame)
	{
		lcd_info.fb_free(lcd_info.decoder_frame);
		lcd_info.decoder_frame = NULL;
	}

	if (lcd_info.pingpong_frame)
	{
		lcd_info.fb_free(lcd_info.pingpong_frame);
		lcd_info.pingpong_frame = NULL;
	}

	if (lcd_info.display_frame)
	{
		if (lcd_info.display_frame != lcd_info.lvgl_frame)
		{
			lcd_info.fb_free(lcd_info.display_frame);
		}
		lcd_info.display_frame = NULL;
	}

	if (lcd_info.lvgl_frame)
	{
		os_free(lcd_info.lvgl_frame);
		lcd_info.lvgl_frame = NULL;
	}

	ret = rtos_deinit_semaphore(&lcd_info.disp_sem);

	if (ret != BK_OK)
	{
		LOGE("%s disp_sem deinit failed: %d\n", __func__, ret);
	}

	rtos_deinit_mutex(&lcd_info.dec_lock);
	rtos_deinit_mutex(&lcd_info.rot_lock);
	rtos_deinit_mutex(&lcd_info.resize_lock);
	rtos_deinit_mutex(&lcd_info.disp_lock);

	lcd_info.rotate = ROTATE_NONE;
	lcd_info.resize = false;
	lcd_info.resize_ppi = PPI_800X480;
	lcd_info.decoder_en = false;
	lcd_info.rotate_en = false;
	lcd_info.display_en = false;
	lcd_info.enable = false;

	set_lcd_state(LCD_STATE_DISABLED);

	LOGI("%s complete\n", __func__);

out:

	MEDIA_EVT_RETURN(param, ret);
}


void lcd_set_backligth_handle(param_pak_t *param)
{
	int ret = BK_OK;

	LOGI("%s, levle: %d\n", __func__, param->param);
	lcd_driver_backlight_set(param->param);

	MEDIA_EVT_RETURN(param, ret);
}

void lcd_act_dump_decoder_frame(void)
{
#if (CONFIG_IMAGE_STORAGE)
	//storage_frame_buffer_dump(lcd_info.decoder_frame, "decoder_vuyy.yuv");
#endif
}

void lcd_act_dump_jpeg_frame(void)
{
	if (dbg_jpeg_frame == NULL)
	{
		LOGE("dbg_jpeg_frame was NULL\n");
		return;
	}

	LOGI("Dump jpeg frame seq: %u, ptr: %p, length: %u, fmt: %u\n",
	     dbg_jpeg_frame->sequence, dbg_jpeg_frame->frame, dbg_jpeg_frame->length, dbg_jpeg_frame->fmt);

#if (CONFIG_IMAGE_STORAGE)
	storage_frame_buffer_dump(dbg_jpeg_frame, frame_suffix(dbg_jpeg_frame->fmt));
#endif
}

void lcd_act_dump_display_frame(void)
{
	if (dbg_display_frame == NULL)
	{
		LOGE("dbg_display_frame was NULL\n");
		return;
	}

	LOGI("Dump display frame seq: %u, ptr: %p, length: %u, fmt: %u\n",
	     dbg_display_frame->sequence, dbg_display_frame->frame, dbg_display_frame->length, dbg_display_frame->fmt);

#if (CONFIG_IMAGE_STORAGE)
	storage_frame_buffer_dump(dbg_display_frame, frame_suffix(dbg_display_frame->fmt));
#endif
}

bk_err_t lcd_rotate_handle(param_pak_t *param)
{
	int ret = BK_OK;
	lcd_info.rotate_en = true;
	lcd_info.rotate = param->param;
	return ret;
}
void lcd_event_handle(uint32_t event, uint32_t param)
{
	param_pak_t *param_pak = NULL;

	switch (event)
	{
		case EVENT_LCD_OPEN_IND:
			lcd_open_handle((param_pak_t *)param);
			break;

		case EVENT_LCD_ROTATE_ENABLE_IND:
		{
			LOGI("EVENT_LCD_ROTATE_ENABLE_IND\n");

			param_pak = (param_pak_t *)param;
			lcd_info.rotate = param_pak->param;
			MEDIA_EVT_RETURN(param_pak, BK_OK);
		}
		break;

		case EVENT_LCD_RESIZE_IND:
		{
			param_pak = (param_pak_t *)param;
			if (lcd_info.resize)
			{
				lcd_info.resize = false;
			}
			else
			{
				lcd_info.resize = true;
				lcd_info.resize_ppi = (media_ppi_t)(param_pak->param);
			}

			MEDIA_EVT_RETURN(param_pak, BK_OK);
		}
		break;

		case EVENT_LCD_FRAME_COMPLETE_IND:
			break;

		case EVENT_LCD_FRAME_LOCAL_ROTATE_IND:
#if 0
			lcd_act_rotate_degree90(param);
			lcd_act_rotate_complete(lcd_info.rotate_frame);
#endif
			break;

		case EVENT_LCD_CLOSE_IND:
			lcd_close_handle((param_pak_t *)param);
			break;

		case EVENT_LCD_SET_BACKLIGHT_IND:
			lcd_set_backligth_handle((param_pak_t *)param);
			break;

		case EVENT_LCD_DUMP_DECODER_IND:
			lcd_act_dump_decoder_frame();
			param_pak = (param_pak_t *)param;
			MEDIA_EVT_RETURN(param_pak, BK_OK);
			break;

		case EVENT_LCD_DUMP_JPEG_IND:
			lcd_act_dump_jpeg_frame();
			param_pak = (param_pak_t *)param;
			MEDIA_EVT_RETURN(param_pak, BK_OK);
			break;

		case EVENT_LCD_DUMP_DISPLAY_IND:
			lcd_act_dump_display_frame();
			param_pak = (param_pak_t *)param;
			MEDIA_EVT_RETURN(param_pak, BK_OK);
			break;

		case EVENT_LCD_STEP_MODE_IND:
			param_pak = (param_pak_t *)param;

			if (param_pak->param)
			{
				LOGI("step mode enable");
				lcd_info.step_mode = true;
				bk_timer_stop(TIMER_ID3);
				rtos_init_semaphore_ex(&step_sem, 1, 0);
			}
			else
			{
				LOGI("step mode disable");
				lcd_info.step_mode = false;

				if (rtos_set_semaphore(&step_sem))
				{
					LOGE("%s step_sem set failed\n", __func__);
				}

				if (rtos_deinit_semaphore(&step_sem))
				{
					LOGE("%s step_sem deinit failed\n");
				}

			}
			MEDIA_EVT_RETURN(param_pak, BK_OK);
			break;

		case EVENT_LCD_STEP_TRIGGER_IND:
			param_pak = (param_pak_t *)param;

			LOGI("step trigger start");
			lcd_info.step_trigger = true;

			if (rtos_set_semaphore(&step_sem))
			{
				LOGE("%s step_sem set failed\n", __func__);
			}

			MEDIA_EVT_RETURN(param_pak, BK_OK);
			break;

		case EVENT_LCD_DISPLAY_FILE_IND:
			//lcd_display_file_handle((param_pak_t *)param);
			break;

		case EVENT_LCD_BEKEN_LOGO_DISPLAY:
			//lcd_display_beken_logo_handle((param_pak_t *)param);
			break;

		case EVENT_LCD_DISPLAY_IND:
			//lcd_display_handle((param_pak_t *)param);
			break;

		case EVENT_LCD_BLEND_IND:
			lcd_blend_font_handle((param_pak_t *)param);
		break;
		case EVENT_LCD_BLEND_OPEN_IND:
		{
			lcd_blend_open_handler((param_pak_t *)param);
		}
		break;
	}
}



media_lcd_state_t get_lcd_state(void)
{
	return lcd_info.state;
}

void set_lcd_state(media_lcd_state_t state)
{
	lcd_info.state = state;
}


void lcd_init(void)
{
	os_memset(&lcd_info, 0, sizeof(lcd_info_t));
	lcd_info.state = LCD_STATE_DISABLED;
	lcd_info.debug = false;
	lcd_info.rotate = ROTATE_NONE;
	lcd_info.resize = false;
	lcd_info.resize_ppi = PPI_800X480;
}

uint8_t get_decode_mode(void)
{
	return lcd_info.decode_mode;
}

void set_decode_mode(uint8_t mode)
{
	lcd_info.decode_mode = mode;
}





#include <stdlib.h>
#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>

#include <driver/jpeg_dec.h>
#include <driver/jpeg_dec_types.h>
#include <driver/timer.h>

#include <modules/jpeg_decode_sw.h>
#include <modules/tjpgd.h>

#include "lcd_act.h"

#include <driver/media_types.h>
#include <lcd_decode.h>
#include <driver/dma.h>
#include "bk_general_dma.h"
#include <soc/mapping.h>
#if CONFIG_SYS_CPU0 && CONFIG_MAILBOX
#include <driver/mailbox_channel.h>
#endif

#define TAG "lcd_dec"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


extern media_debug_t *media_debug;
beken_semaphore_t dec_sem;

#if CONFIG_SOC_BK7256XX
uint8_t jpeg_dec_dma = 0;
#endif

static lcd_decode_t s_lcd_decode = {0};
#if CONFIG_SYS_CPU0 && CONFIG_MAILBOX
static void decode_mailbox_rx_isr(void *param, mb_chnl_cmd_t *cmd_buf)
{
	bk_err_t ret;

	if (cmd_buf->hdr.cmd == EVENT_LCD_DEC_SW_MBRSP)
	{
		ret = rtos_set_semaphore(&s_lcd_decode.dec_sem);

		if (ret != BK_OK)
		{
			LOGE("%s semaphore get failed: %d\n", __func__, ret);
		}
		s_lcd_decode.result = (uint8_t)cmd_buf->param3;
	}
	
	if (cmd_buf->hdr.cmd == EVENT_LCD_DEC_SW_OPEN_MBRSP)
	{
		ret = rtos_set_semaphore(&s_lcd_decode.dec_sem);

		if (ret != BK_OK)
		{
			LOGE("%s semaphore get failed: %d\n", __func__, ret);
		}
		s_lcd_decode.result = (uint8_t)cmd_buf->param3;
	}
}
#endif
static void lcd_driver_decoder_timeout(timer_id_t timer_id)
{
	bk_err_t ret = BK_FAIL;

	bk_timer_stop(TIMER_ID3);

	bk_jpeg_dec_stop();
	media_debug->isr_decoder--;

	s_lcd_decode.decode_timeout = true;
	LOGE("%s \n", __func__);

	ret = rtos_set_semaphore(&s_lcd_decode.dec_sem);
	if (ret != BK_OK)
	{
		LOGE("%s semaphore set failed: %d\n", __func__, ret);
	}
}


static void jpeg_dec_eof_cb(jpeg_dec_res_t *result)
{
	bk_err_t ret = BK_FAIL;
	s_lcd_decode.decode_err = false;

	media_debug->isr_decoder++;

	bk_timer_stop(TIMER_ID3);

	if (s_lcd_decode.decoder_frame)
	{
		s_lcd_decode.decoder_frame->height = result->pixel_y;
		s_lcd_decode.decoder_frame->width = result->pixel_x ;
		s_lcd_decode.decoder_frame->length = result->pixel_y * result->pixel_x *2;
	}

	if (result->ok == false)
	{
		s_lcd_decode.decode_err = true;
		media_debug->isr_decoder--;
		LOGD("%s decoder error\n", __func__);
	}

	ret = rtos_set_semaphore(&s_lcd_decode.dec_sem);

	if (ret != BK_OK)
	{
		LOGE("%s semaphore set failed: %d\n", __func__, ret);
	}
}

bk_err_t lcd_hw_decode_start(frame_buffer_t *src_frame, frame_buffer_t *dst_frame)
{
	int ret = BK_OK;
	s_lcd_decode.decoder_frame = dst_frame;
	s_lcd_decode.decode_timeout = false;
#if CONFIG_SOC_BK7236XX
	ret = bk_jpeg_dec_hw_start(src_frame->length, src_frame->frame, dst_frame->frame);
#else  // CONFIG_SOC_BK7256XX
	uint32_t length = src_frame->length;
	uint32_t left_len = 0;
	if (src_frame->fmt == PIXEL_FMT_JPEG && src_frame->type == UVC_CAMERA)
	{
		if (src_frame->length <= 128 * 1024)
		{
			if (src_frame->length <= 0xFFFF)
			{
				dma_memcpy_by_chnl((void*)JPEG_SRAM_ADDRESS, src_frame->frame,
				(length % 4) ? ((length  / 4 + 1) * 4) : length, jpeg_dec_dma);
			}
			else
			{
				dma_memcpy_by_chnl((void*)JPEG_SRAM_ADDRESS, src_frame->frame, 0xFFFF, jpeg_dec_dma);
				left_len = length - 0xFFFF;
				dma_memcpy_by_chnl((void*)JPEG_SRAM_ADDRESS + 0xFFFF, src_frame + 0xFFFF,
									(left_len % 4) ? ((left_len  / 4 + 1) * 4) : left_len, jpeg_dec_dma);
			}
			ret = bk_jpeg_dec_hw_start(src_frame->length, (void*)JPEG_SRAM_ADDRESS, dst_frame->frame);
		}
		else
		{
			LOGE("uvc output image too large!\n");
			ret = BK_FAIL;
		}
	}
	else
	{
		ret = bk_jpeg_dec_hw_start(src_frame->length, src_frame->frame, dst_frame->frame);
	}
#endif

	if (ret != BK_OK)
	{
		LOGE("%s, length:%d\r\n", __func__, src_frame->length);
		return ret;
	}

	bk_timer_start(TIMER_ID3, 200, lcd_driver_decoder_timeout);

	ret = rtos_get_semaphore(&s_lcd_decode.dec_sem, BEKEN_NEVER_TIMEOUT);
	if (ret != BK_OK)
	{
		LOGE("%s semaphore get failed: %d\n", __func__, ret);
	}
	if(s_lcd_decode.decode_timeout == true)
		ret = BK_LCD_DECODE_TIMEOUT;
	if(s_lcd_decode.decode_err == true)
		ret = BK_LCD_DECODE_ERR;

	return ret;
}

bk_err_t lcd_sw_jpegdec_start(frame_buffer_t *frame, frame_buffer_t *dst_frame)
{
	bk_err_t ret =  BK_OK;
#if CONFIG_LCD_SW_DECODE
	jd_output_format *format = NULL;
	sw_jpeg_dec_res_t result;
	format = os_malloc(sizeof(jd_output_format));
	if (format == NULL)
	{
		LOGE("%s no buffer\n", __func__);
		ret = BK_FAIL;
		goto out;
	}
	media_debug->isr_decoder++;
	switch (dst_frame->fmt)
	{
		case PIXEL_FMT_RGB565:
			format->format = JD_FORMAT_RGB565;
			format->scale = 1;
			format->byte_order = JD_BIG_ENDIAN;
			break;
		case PIXEL_FMT_YUYV:
			format->format = JD_FORMAT_YUYV;
			format->scale = 0;
			format->byte_order = JD_LITTLE_ENDIAN;
			break;
		case PIXEL_FMT_VUYY:
			format->format = JD_FORMAT_VUYY;
			format->scale = 0;
			format->byte_order = JD_LITTLE_ENDIAN;
			break;
		default:
			format->format = JD_FORMAT_VYUY;
			format->scale = 0;
			format->byte_order = JD_LITTLE_ENDIAN;
			break;
	}

	jd_set_output_format(format);
	ret = bk_jpeg_dec_sw_start(JPEGDEC_BY_FRAME, frame->frame, dst_frame->frame, frame->length, 0, &result);
	if (ret != BK_OK)
	{
		LOGE("%s sw decoder error\n", __func__);
		media_debug->isr_decoder--;
		goto out;
	}
	else
	{
		dst_frame->height = result.pixel_y;
		dst_frame->width = result.pixel_x ;
	}
out:
	if (format)
	{
		os_free(format);
	}
#endif
	return ret;
}

bk_err_t lcd_sw_minor_jpegdec_start(frame_buffer_t *frame, frame_buffer_t *dst_frame)
{
	bk_err_t ret =  BK_OK;

#if CONFIG_LCD_SW_DECODE
	jd_output_format *format = NULL;
	format = os_malloc(sizeof(jd_output_format));
	if (format == NULL)
	{
		LOGE("%s no buffer\n", __func__);
		ret = BK_FAIL;
		goto out;
	}
	media_debug->isr_decoder++;

	switch (dst_frame->fmt)
	{
		case PIXEL_FMT_RGB565:
			format->format = JD_FORMAT_RGB565;
			format->scale = 1;
			format->byte_order = JD_BIG_ENDIAN;
			break;
		case PIXEL_FMT_YUYV:
			format->format = JD_FORMAT_YUYV;
			format->scale = 0;
			format->byte_order = JD_LITTLE_ENDIAN;
			break;
		case PIXEL_FMT_VUYY:
			format->format = JD_FORMAT_VUYY;
			format->scale = 0;
			format->byte_order = JD_LITTLE_ENDIAN;
			break;
		default:
			format->format = JD_FORMAT_VYUY;
			format->scale = 0;
			format->byte_order = JD_LITTLE_ENDIAN;
			break;
	}
#if CONFIG_SYS_CPU0 && CONFIG_MAILBOX
	mb_chnl_cmd_t mb_cmd;
	mb_cmd.hdr.cmd = EVENT_LCD_DEC_SW_MBCMD;
	mb_cmd.param1 = (uint32_t)frame;
	mb_cmd.param2 = (uint32_t)dst_frame;
	mb_cmd.param3 = (uint32_t)format;
	s_lcd_decode.result = BK_FAIL;
	ret = mb_chnl_write(MB_CHNL_DEC, &mb_cmd);

	if (ret != BK_OK)
	{
		LOGE("%s mb_chnl_write failed: %d\n", __func__, ret);
		goto out;
	}

	//LOGI("%s wait rotate\n", __func__);
	ret = rtos_get_semaphore(&s_lcd_decode.dec_sem, BEKEN_NEVER_TIMEOUT);

	if (ret != BK_OK)
	{
		LOGE("%s semaphore get failed: %d\n", __func__, ret);
		goto out;
	}

	if (s_lcd_decode.result != BK_OK)
	{
	
		media_debug->isr_decoder--;
		LOGE("%s sw decoder error\n", __func__);
		goto out;
	}
#endif
out:
	if (format)
	{
		os_free(format);
	}
#endif
	return ret;
}

bk_err_t lcd_hw_decode_init(void)
{
	bk_err_t ret = BK_OK;
	media_debug->isr_decoder = 0;
	media_debug->err_dec = 0;

	ret = rtos_init_semaphore_ex(&s_lcd_decode.dec_sem, 1, 0);

	if (ret != BK_OK)
	{
		LOGE("%s hw  dec_sem init failed: %d\n", __func__, ret);
		return ret;
	}

		ret = bk_jpeg_dec_driver_init();
#if(1)  //enable jpeg complete int isr
			bk_jpeg_dec_isr_register(DEC_END_OF_FRAME, jpeg_dec_eof_cb);
#else   //enable uvc ppi 640X480 jpeg 24 line decode complete int isr
			bk_jpeg_dec_isr_register(DEC_END_OF_LINE_NUM, jpeg_dec_line_cb);
#endif

#if CONFIG_SOC_BK7256XX
	jpeg_dec_dma = bk_dma_alloc(DMA_DEV_JPEG);
	if ((jpeg_dec_dma < DMA_ID_0) || (jpeg_dec_dma >= DMA_ID_MAX))
	{
		LOGE("%s, jpeg dec malloc dma fail \r\n", __func__);
		return BK_FAIL;
	}
	
	LOGI("%s, malloc dma channal %d\n", __func__, jpeg_dec_dma);
#endif
	return ret;
}

bk_err_t lcd_hw_decode_deinit(void)
{
	bk_err_t ret = BK_OK;

	bk_jpeg_dec_driver_deinit();

#if CONFIG_SOC_BK7256XX
	bk_dma_stop(jpeg_dec_dma);
	bk_dma_deinit(jpeg_dec_dma);
	bk_dma_free(DMA_DEV_JPEG, jpeg_dec_dma);
#endif

	ret = rtos_deinit_semaphore(&s_lcd_decode.dec_sem);

	if (ret != BK_OK)
	{
		LOGE("%s dec_sem deinit failed: %d\n", __func__, ret);
		return ret;
	}
	return ret;
}

bk_err_t lcd_sw_decode_init(media_decode_mode_t sw_dec_mode)
{
	bk_err_t ret = BK_OK;
	LOGI("%s \n", __func__);
	ret = rtos_init_semaphore_ex(&s_lcd_decode.dec_sem, 1, 0);

	if (ret != BK_OK)
	{
		LOGE("%s sw dec_sem init failed: %d\n", __func__, ret);
		goto out;
	}
#if CONFIG_LCD_SW_DECODE
	//jd_set_format(JD_FORMAT_YUYV);
	if (sw_dec_mode == SOFTWARE_DECODING_MINOR)
	{
#if CONFIG_SYS_CPU0 && CONFIG_MAILBOX
		/* Mailbox */
		mb_chnl_open(MB_CHNL_DEC, NULL);
		mb_chnl_ctrl(MB_CHNL_DEC, MB_CHNL_SET_RX_ISR, decode_mailbox_rx_isr);

		mb_chnl_cmd_t mb_cmd;
		mb_cmd.hdr.cmd = EVENT_LCD_DEC_SW_OPEN_MBCMD;
		mb_cmd.param1 = true;
		s_lcd_decode.result = BK_FAIL;
		ret = mb_chnl_write(MB_CHNL_DEC, &mb_cmd);

		if (ret != BK_OK)
		{
			LOGE("%s mb_chnl_write failed: %d\n", __func__, ret);
			goto out;
		}

		ret = rtos_get_semaphore(&s_lcd_decode.dec_sem, BEKEN_NEVER_TIMEOUT);
		
		if (ret != BK_OK)
		{
			LOGE("%s semaphore get failed: %d\n", __func__, ret);
			goto out;
		}
		
		if (s_lcd_decode.result != BK_OK)
		{
			LOGE("%s error\n", __func__);
			goto out;
		}
#endif
	}
	else
	{
		ret = bk_jpeg_dec_sw_init(NULL, 0);
		if (ret != BK_OK)
		{
			LOGE("%s dec_sem init failed: %d\n", __func__, ret);
			return ret;
		}
	}
#endif
out:
	return ret;
}

bk_err_t lcd_sw_decode_deinit(media_decode_mode_t sw_dec_mode)
{
	bk_err_t ret = BK_OK;
	
	LOGI("%s sw_dec_mode = %d\n", __func__, sw_dec_mode);
	if (sw_dec_mode == SOFTWARE_DECODING_MINOR)
	{
#if CONFIG_SYS_CPU0 && CONFIG_MAILBOX
		/* Mailbox */
		mb_chnl_cmd_t mb_cmd;
		mb_cmd.hdr.cmd = EVENT_LCD_DEC_SW_OPEN_MBCMD;
		mb_cmd.param1 = false;
		s_lcd_decode.result = BK_FAIL;
		ret = mb_chnl_write(MB_CHNL_DEC, &mb_cmd);

		if (ret != BK_OK)
		{
			LOGE("%s mb_chnl_write failed: %d\n", __func__, ret);
			goto out;
		}

		ret = rtos_get_semaphore(&s_lcd_decode.dec_sem, BEKEN_NEVER_TIMEOUT);
		
		if (ret != BK_OK)
		{
			LOGE("%s semaphore get failed: %d\n", __func__, ret);
			goto out;
		}
		
		if (s_lcd_decode.result != BK_OK)
		{
			LOGE("%s sw decoder error\n", __func__);
			goto out;
		}
#endif
	}
	else
	{
		bk_jpeg_dec_sw_deinit();
		if (ret != BK_OK)
		{
			LOGE("%s dec_sem init failed: %d\n", __func__, ret);
			goto out;
		}
	}

	ret = rtos_deinit_semaphore(&s_lcd_decode.dec_sem);

	if (ret != BK_OK)
	{
		LOGE("%s dec_sem deinit failed: %d\n", __func__, ret);
		return ret;
	}

out:
	return ret;
}


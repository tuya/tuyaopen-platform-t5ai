#include <stdlib.h>
#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>

#if CONFIG_HW_ROTATE_PFC
#include <driver/rott_driver.h>
#endif
#include <driver/media_types.h>
#include <lcd_rotate.h>
#include "modules/image_scale.h"
#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif



#define TAG "lcd_rotate"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)



typedef struct
{
	media_rotate_t rotate_angle;
	beken_semaphore_t rot_sem;
	uint8_t rotate_en;
}rotate_t;
static rotate_t  s_rot = {0};



static void rotate_complete_cb(void)
{
	LOGD("rotate_complete_cb\r\n");
	rtos_set_semaphore(&s_rot.rot_sem);
}

static void rotate_watermark_cb(void)
{
	LOGD("rotate_watermark_cb\r\n");
	
}
static void rotate_cfg_err_cb(void)
{
	LOGI("rotate_cfg_err_cb\r\n");
}




static void rott_pfc_complete_cb(void)
{
	LOGD("rott_pfc_complete_cb\r\n");
	rtos_set_semaphore(&s_rot.rot_sem);
}


bk_err_t lcd_rotate_deinit(void)
{
	bk_err_t ret = BK_OK;
#ifdef CONFIG_HW_ROTATE_PFC
	bk_rott_driver_deinit();
#endif
	ret = rtos_deinit_semaphore(&s_rot.rot_sem);

	if (ret != BK_OK)
	{
		LOGE("%s rot_sem deinit failed: %d\n", __func__, ret);
		return ret;
	}
	return ret;
}

bk_err_t lcd_rotate_init(media_rotate_mode_t	rotate_mode)
{
	bk_err_t ret = BK_OK;
	ret = rtos_init_semaphore_ex(&s_rot.rot_sem, 1, 0);

	if (ret != BK_OK)
	{
		LOGE("%s rot_sem init failed: %d\n", __func__, ret);
		return ret;
	}

	if(rotate_mode == HW_ROTATE)
	{
		#ifdef CONFIG_HW_ROTATE_PFC
		bk_rott_driver_init();
		bk_rott_int_enable(ROTATE_COMPLETE_INT | ROTATE_CFG_ERR_INT | ROTATE_WARTERMARK_INT, 1);
		bk_rott_isr_register(ROTATE_COMPLETE_INT, rotate_complete_cb);
		bk_rott_isr_register(ROTATE_WARTERMARK_INT, rotate_watermark_cb);
		bk_rott_isr_register(ROTATE_CFG_ERR_INT, rotate_cfg_err_cb);
		#endif
	}
	return ret;
}

bk_err_t lcd_hw_rotate_yuv2rgb565(frame_buffer_t *src, frame_buffer_t *dst, media_rotate_t rotate)
{
	bk_err_t ret = BK_OK;

#ifdef CONFIG_HW_ROTATE_PFC
	rott_config_t rott_cfg = {0};
	
	dst->fmt = PIXEL_FMT_RGB565_LE;
	rott_cfg.input_addr = src->frame;
	rott_cfg.output_addr = dst->frame;
	rott_cfg.rot_mode = rotate;

	switch (src->fmt)
	{
		case PIXEL_FMT_YUYV:
			rott_cfg.input_fmt = src->fmt;
			rott_cfg.input_flow = ROTT_INPUT_NORMAL;
			rott_cfg.output_flow = ROTT_OUTPUT_NORMAL;
			break;
		case PIXEL_FMT_VUYY:
			rott_cfg.input_fmt = src->fmt;
			rott_cfg.input_flow = ROTT_INPUT_NORMAL;
			rott_cfg.output_flow = ROTT_OUTPUT_NORMAL;
			break;
		case PIXEL_FMT_RGB565_LE:
			rott_cfg.input_fmt = src->fmt;
			rott_cfg.input_flow = ROTT_INPUT_REVESE_HALFWORD_BY_HALFWORD;
			rott_cfg.output_flow = ROTT_OUTPUT_NORMAL;
			break;
		case PIXEL_FMT_RGB565:
		default:
			rott_cfg.input_fmt = src->fmt;
			rott_cfg.input_flow = ROTT_INPUT_REVESE_HALFWORD_BY_HALFWORD;
			rott_cfg.output_flow = ROTT_OUTPUT_NORMAL;
			break;
	}
	rott_cfg.picture_xpixel = src->width;
	rott_cfg.picture_ypixel = src->height;
//	rott_cfg.block_xpixel = ROTT_XBLOCK;
//	rott_cfg.block_ypixel = ROTT_YBLOCK
//	rott_cfg.block_cnt = ROTT_BLOCK_NUM;
	ret = rott_config(&rott_cfg);
	if (ret != BK_OK)
		LOGE(" rott_config ERR\n");
	bk_rott_enable();

	ret = rtos_get_semaphore(&s_rot.rot_sem, BEKEN_NEVER_TIMEOUT);

	if (ret != BK_OK)
	{
		LOGE("%s semaphore get failed: %d\n", __func__, ret);
	}
#endif
	return ret;
}

bk_err_t lcd_sw_rotate(frame_buffer_t *src, frame_buffer_t *dst, uint8_t rotate)
{
	bk_err_t ret = BK_OK;

	register uint16_t src_width, src_height;

	frame_buffer_t *decoder_frame = (frame_buffer_t *)src;
	frame_buffer_t *rotate_frame = (frame_buffer_t *)dst;

	int (*func)(unsigned char *vuyy, unsigned char *rotatedVuyy, int width, int height);
	src_width = decoder_frame->width;
	src_height = decoder_frame->height;

#if CONFIG_CACHE_ENABLE
#if CONFIG_SOC_BK7256XX
	register uint8_t *dst_frame_temp = rotate_frame->frame + 0x4000000;
	register uint8_t *src_frame_temp = decoder_frame->frame + 0x4000000;
#else
	register uint8_t *dst_frame_temp = rotate_frame->frame ;
	register uint8_t *src_frame_temp = decoder_frame->frame;
#endif
#else
	register uint8_t *dst_frame_temp = rotate_frame->frame ;
	register uint8_t *src_frame_temp = decoder_frame->frame;
#endif
#if  CONFIG_CACHE_ENABLE
		flush_dcache(src_frame_temp, decoder_frame->length);
		flush_dcache(dst_frame_temp, rotate_frame->length);
#endif

	switch (rotate_frame->fmt)
	{
		case PIXEL_FMT_VUYY:

			if (rotate == ROTATE_90)
			{
				func = vuyy_rotate_degree90_to_yuyv;
			}
			else
			{
				func = vuyy_rotate_degree270_to_yuyv;
			}

			rotate_frame->fmt = PIXEL_FMT_YUYV;
			break;
		case PIXEL_FMT_YUYV:
		default:
			if (rotate == ROTATE_90)
			{
				func = yuyv_rotate_degree90_to_yuyv;
			}
			else
			{
				func = yuyv_rotate_degree270_to_yuyv;
			}
			rotate_frame->fmt = PIXEL_FMT_YUYV;
			break;
	}

#if  CONFIG_CACHE_ENABLE
	flush_dcache(src_frame_temp, decoder_frame->length);
	flush_dcache(dst_frame_temp, rotate_frame->length);
#endif

	func(src_frame_temp, dst_frame_temp, src_width, src_height);
	return ret;
}


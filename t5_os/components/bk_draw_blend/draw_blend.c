#include <stdlib.h>
#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>

#include <driver/dma2d.h>
#include <driver/dma2d_types.h>
#include <driver/media_types.h>
#include <driver/lcd_types.h>
#include <soc/mapping.h>

#include "modules/image_scale.h"

#if  CONFIG_LCD_FONT_BLEND
#include "modules/lcd_font.h"
#endif

#include "draw_blend.h"

#define TAG "lcd_blend"
#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


#if CONFIG_LCD_DMA2D_BLEND || CONFIG_LCD_FONT_BLEND
static uint8_t *blend_addr1 = NULL;
static uint8_t *blend_addr2 = NULL;
#endif


typedef struct
{
	beken_semaphore_t dma2d_complete_sem;
	beken_semaphore_t dma2d_err_sem;

}blend_t;

static blend_t s_blend = {0};


#if (USE_DMA2D_BLEND_ISR_CALLBACKS == 1)
static void dma2d_config_error(void)
{
	LOGE("%s \n", __func__);
	rtos_set_semaphore(&s_blend.dma2d_err_sem);
}

static void dma2d_transfer_error(void)
{
	LOGE("%s \n", __func__);
	rtos_set_semaphore(&s_blend.dma2d_err_sem);
}

static void dma2d_transfer_complete(void)
{
	LOGD("%s \n", __func__);
	rtos_set_semaphore(&s_blend.dma2d_complete_sem);
}
#endif


/**
 * @brief This API used to direct draw fg argb8888 png raw data to bg yuv data
 */
bk_err_t lcd_driver_blend(lcd_blend_t *lcd_blend)
{
#if CONFIG_LCD_FONT_BLEND 
	int	i = 0;
	//uint8_t * p_logo_addr = (uint8_t *)lcd_blend->pfg_addr;
	uint8_t * p_yuv_src = (uint8_t *)lcd_blend->pbg_addr;
	uint8_t * p_yuv_dst = (uint8_t *)blend_addr1;
	uint8_t * p_yuv_temp = (uint8_t *)blend_addr2;
	pixel_format_t bg_fmt  = lcd_blend->bg_data_format;
    uint8_t pixel_bit;
	//STEP 1 COPY BG YUV DATA

    if (bg_fmt == PIXEL_FMT_RGB888)
    {
        pixel_bit = 3;
    }
    else
    {
        pixel_bit = 2;
    }

	for(i = 0; i < lcd_blend->ysize; i++)
	{
		os_memcpy((uint32_t *)p_yuv_dst,(uint32_t *) p_yuv_src, lcd_blend->xsize * pixel_bit);
		p_yuv_dst += (lcd_blend->xsize * pixel_bit);
		p_yuv_src += (lcd_blend->bg_width * pixel_bit);
	}
	p_yuv_dst = (uint8_t *)blend_addr1;
	
	if (lcd_blend->blend_rotate == ROTATE_270)
	{
		p_yuv_dst = (uint8_t *)blend_addr1;
		if (PIXEL_FMT_VUYY == bg_fmt)
		{
			//step2 rotate area 
			vuyy_rotate_degree90_to_yuyv((unsigned char *)p_yuv_dst, (unsigned char *)p_yuv_temp, lcd_blend->xsize, lcd_blend->ysize);
		}
		if (PIXEL_FMT_YUYV == bg_fmt)
		{
			yuyv_rotate_degree90_to_yuyv((unsigned char *)p_yuv_dst, (unsigned char *)p_yuv_temp, lcd_blend->xsize, lcd_blend->ysize);
		}

		i = lcd_blend->xsize;
		lcd_blend->xsize = lcd_blend->ysize;
		lcd_blend->ysize = i;
		bg_fmt = PIXEL_FMT_YUYV;
		p_yuv_dst = blend_addr2;
	}
	
	//STEP 2 check alpha=0 logo pixel,and copy alpha!=0 pixel to bg yuv data
//	if (lcd_blend->flag == 1)
//	{
//		p_yuv_temp = blend_addr1;
//		os_memcpy((uint32_t *)(p_yuv_temp), (uint32_t *)lcd_blend->pfg_addr, lcd_blend->xsize * lcd_blend->ysize *4);
//	}

	if (PIXEL_FMT_VUYY == bg_fmt)
	{
		argb8888_to_vuyy_blend((uint8_t *)lcd_blend->pfg_addr, p_yuv_dst, lcd_blend->xsize, lcd_blend->ysize);
	}
    else if (PIXEL_FMT_YUYV == bg_fmt)
    {
        argb8888_to_yuyv_blend((uint8_t *)lcd_blend->pfg_addr, p_yuv_dst, lcd_blend->xsize, lcd_blend->ysize);
    }
    else if (PIXEL_FMT_RGB888 == bg_fmt)
    {
        argb8888_to_rgb888_blend((uint8_t *)lcd_blend->pfg_addr, p_yuv_dst, lcd_blend->xsize, lcd_blend->ysize);
    }
    else
    {
        argb8888_to_rgb565_blend((uint8_t *)lcd_blend->pfg_addr, p_yuv_dst, lcd_blend->xsize, lcd_blend->ysize);
    }

	if (lcd_blend->blend_rotate == ROTATE_270)
	{
		p_yuv_temp = blend_addr1;
		
		if (PIXEL_FMT_VUYY == lcd_blend->bg_data_format)
		{
			// rotate back
			yuyv_rotate_degree270_to_vuyy((unsigned char *)p_yuv_dst, (unsigned char *)p_yuv_temp, lcd_blend->xsize, lcd_blend->ysize);
		}
		else
		{
			yuyv_rotate_degree270_to_yuyv((unsigned char *)p_yuv_dst, (unsigned char *)p_yuv_temp, lcd_blend->xsize, lcd_blend->ysize);
		}
		i = lcd_blend->xsize;
		lcd_blend->xsize = lcd_blend->ysize;
		lcd_blend->ysize = i;
		p_yuv_dst = blend_addr1;
	}
		
	//STEP 3 copy return bg image
	p_yuv_src = (uint8_t *)lcd_blend->pbg_addr;

	for(i = 0; i < lcd_blend->ysize; i++)
	{
		os_memcpy((uint32_t *)p_yuv_src, (uint32_t *)p_yuv_dst, lcd_blend->xsize * pixel_bit);
		p_yuv_dst += (lcd_blend->xsize * pixel_bit);
		p_yuv_src += (lcd_blend->bg_width * pixel_bit);
	}
#endif
	return BK_OK;
}


/**
 * @brief This API use dma2d to blend fg argb8888 png raw data to bg rgb565 or yuv data
 *
 * 1:in BK7258 V4 version dma2d not spport yuv to rgb convert, so 
 *                if bg is yuv we should use dma2d memcpy yuv and then use software convert to rgb565. 
 * 2:but in BK7258 V5, dma2d support yuv_2_rgb convert, So we direct make fg argb8888 blend with fg yuv data.
 *   users just concern V5
 */
bk_err_t lcd_dma2d_driver_blend(lcd_blend_t *lcd_blend)
{
#if CONFIG_LCD_DMA2D_BLEND
	uint16_t lcd_start_x = 0;
	uint16_t lcd_start_y = 0;
	if ((lcd_blend->lcd_width < lcd_blend->bg_width)  || (lcd_blend->lcd_height < lcd_blend->bg_height)) //for lcd size is small then frame image size
	{
		if (lcd_blend->lcd_width < lcd_blend->bg_width)
			lcd_start_x = (lcd_blend->bg_width - lcd_blend->lcd_width) / 2;
		if (lcd_blend->lcd_height < lcd_blend->bg_height)
			lcd_start_y = (lcd_blend->bg_height - lcd_blend->lcd_height) / 2;
	}
		//if bg data is rgb565(after hw rotate)
		dma2d_offset_blend_t dma2d_config;

		dma2d_config.pfg_addr = (char *)lcd_blend->pfg_addr;
		dma2d_config.pbg_addr = (char *)lcd_blend->pbg_addr;
		dma2d_config.pdst_addr = (char *)lcd_blend->pbg_addr;
		dma2d_config.fg_color_mode = DMA2D_INPUT_ARGB8888; 
		switch (lcd_blend->bg_data_format)
		{
			case PIXEL_FMT_YUYV:
				dma2d_config.bg_color_mode = DMA2D_INPUT_YUYV;
				dma2d_config.dst_color_mode = DMA2D_OUTPUT_YUYV;
				break;
			case PIXEL_FMT_VUYY:
				dma2d_config.bg_color_mode = DMA2D_INPUT_VUYY;
				dma2d_config.dst_color_mode = DMA2D_OUTPUT_YUYV;
				break;
			case PIXEL_FMT_RGB888:
				dma2d_config.bg_color_mode = DMA2D_INPUT_RGB888;
				dma2d_config.dst_color_mode = DMA2D_OUTPUT_RGB888;
				break;
			case PIXEL_FMT_RGB565:
				default:
				dma2d_config.bg_color_mode = DMA2D_INPUT_RGB565;
				dma2d_config.dst_color_mode = DMA2D_OUTPUT_RGB565;
				break;
		}
		dma2d_config.fg_red_blue_swap = DMA2D_RB_SWAP ;
		dma2d_config.bg_red_blue_swap = DMA2D_RB_REGULAR;
		dma2d_config.dst_red_blue_swap = DMA2D_RB_REGULAR;
		
		dma2d_config.fg_frame_width = lcd_blend->xsize;
		dma2d_config.fg_frame_height = lcd_blend->ysize;
		dma2d_config.bg_frame_width = lcd_blend->bg_width;
		dma2d_config.bg_frame_height = lcd_blend->bg_height;
		dma2d_config.dst_frame_width = lcd_blend->bg_width;
		dma2d_config.dst_frame_height = lcd_blend->bg_height;

		dma2d_config.fg_frame_xpos = 0;
		dma2d_config.fg_frame_ypos = 0;
		dma2d_config.bg_frame_xpos = lcd_start_x + lcd_blend->xpos;
		dma2d_config.bg_frame_ypos = lcd_start_y + lcd_blend->ypos;
		dma2d_config.dst_frame_xpos = lcd_start_x + lcd_blend->xpos;
		dma2d_config.dst_frame_ypos = lcd_start_y + lcd_blend->ypos;
		
		dma2d_config.fg_pixel_byte = FOUR_BYTES;
		dma2d_config.bg_pixel_byte = TWO_BYTES;
		dma2d_config.dst_pixel_byte = TWO_BYTES;
		
		dma2d_config.dma2d_width = lcd_blend->xsize;
		dma2d_config.dma2d_height = lcd_blend->ysize;
		dma2d_config.fg_alpha_mode = DMA2D_NO_MODIF_ALPHA;
		dma2d_config.bg_alpha_mode = DMA2D_REPLACE_ALPHA;
		bk_dma2d_offset_blend(&dma2d_config);
		bk_dma2d_start_transfer();
#if (USE_DMA2D_BLEND_ISR_CALLBACKS == 1)
		if (rtos_get_semaphore(&s_blend.dma2d_complete_sem, BEKEN_NEVER_TIMEOUT) != BK_OK)
		{
			LOGE("%s, dma2d_complete_sem get failed: %d\n", __func__);
		}
#else
		while (bk_dma2d_is_transfer_busy()) {}
#endif

#endif  //CONFIG_LCD_DMA2D_BLEN
	return BK_OK;
}

#if CONFIG_LCD_FONT_BLEND
/**
 * @brief This API draw and display font such as "12:30 ÐÇÆÚÒ» sunday"
 *
 * 1: direct draw font(12:30) to yuv bg image
 * 2£ºfont rgb565 data to yuv bg image, convert bg yuv to rgb565, then rgb565 blend, and then rgb565 convert to yuv
 *                             this method performance in condition of direct yuv font not realize.
 * 3:(lcd_font->font_rotate == ROTATE_270) is the demand of lcd rotate 90 but font view still not rotate
 */
bk_err_t lcd_driver_font_blend(lcd_font_config_t *lcd_font)
{
	int ret = BK_OK;
	
	register uint32_t i =0;
	uint8_t * p_yuv_src = lcd_font->pbg_addr;
	uint8_t * p_yuv_dst = blend_addr1;
	uint8_t * p_yuv_rotate_temp = blend_addr2;
	uint8_t *font_addr = NULL;
    uint8_t pixel_bytes;

    if (lcd_font->bg_data_format == PIXEL_FMT_RGB888)
        pixel_bytes = 3;
    else
        pixel_bytes = 2;
#if 0//CONFIG_SOC_BK7258
//	#if CONFIG_CACHE_ENABLE
//	flush_dcache(lcd_font->pbg_addr, lcd_font->bg_height * lcd_font->bg_width * 2);
//	#endif

//	dma2d_memcpy_psram(lcd_font->pbg_addr, p_yuv_dst, lcd_font->xsize, lcd_font->ysize, lcd_font->bg_offline, 0);
	dma2d_config_t dma2d_config = {0};

	/*##-1- Configure the DMA2D Mode, Output Color Mode and output offset #############*/
	dma2d_config.init.mode         = DMA2D_M2M;             /**< Mode Memory To Memory */
	dma2d_config.init.color_mode    = DMA2D_OUTPUT_YUYV; /**< Output color mode is ARGB4444 : 16 bpp */
	dma2d_config.init.output_offset = 0;                   /**< No offset on output */
	dma2d_config.init.red_blue_swap   = DMA2D_RB_REGULAR;     /**< No R&B swap for the output image */
	dma2d_config.init.alpha_inverted = DMA2D_REGULAR_ALPHA;  /**< No alpha inversion for the output image */
	dma2d_config.init.trans_ability = TRANS_16BYTES; 
	dma2d_config.init.out_byte_by_byte_reverse = NO_REVERSE;

	dma2d_config.layer_cfg[DMA2D_FOREGROUND_LAYER].alpha_mode = DMA2D_NO_MODIF_ALPHA;      /**< Keep original Alpha from ARGB4444 input */
	dma2d_config.layer_cfg[DMA2D_FOREGROUND_LAYER].input_alpha = 0xFF;                     /**< Fully opaque */
	dma2d_config.layer_cfg[DMA2D_FOREGROUND_LAYER].input_color_mode = DMA2D_INPUT_YUYV; /**< Input color is ARGB4444 : 16 bpp */
	dma2d_config.layer_cfg[DMA2D_FOREGROUND_LAYER].input_offset = lcd_font->bg_offline;                     /**< No offset in input */
	dma2d_config.layer_cfg[DMA2D_FOREGROUND_LAYER].red_blue_swap   = DMA2D_RB_REGULAR;      /**< No R&B swap for the input image */
	dma2d_config.layer_cfg[DMA2D_FOREGROUND_LAYER].alpha_inverted = DMA2D_REGULAR_ALPHA;   /**< No alpha inversion for the input image */
	dma2d_config.layer_cfg[DMA2D_FOREGROUND_LAYER].input_data_reverse = NO_REVERSE;

	bk_dma2d_init(&dma2d_config);
	bk_dma2d_layer_config(&dma2d_config, DMA2D_FOREGROUND_LAYER);
//	LOGD( "lcd_font->xsize, lcd_font->ysize %d, %d\n", lcd_font->xsize, lcd_font->ysize);
	bk_dma2d_transfer_config(&dma2d_config, (uint32_t)lcd_font->pbg_addr, (uint32_t)p_yuv_dst, lcd_font->xsize, lcd_font->ysize);
	bk_dma2d_start_transfer();
	while (bk_dma2d_is_transfer_busy()) {}
#else   //BK7256 psram can not use dma2d
	for(i = 0; i < lcd_font->ysize; i++)
	{
		os_memcpy((uint32_t *)p_yuv_dst, (uint32_t *)p_yuv_src, lcd_font->xsize * pixel_bytes);
		p_yuv_dst += (lcd_font->xsize * pixel_bytes);
		p_yuv_src += (lcd_font->bg_width * pixel_bytes);
	}
#endif
	p_yuv_dst = blend_addr1;
	if (lcd_font->font_rotate == ROTATE_270)
	{
		if (PIXEL_FMT_VUYY == lcd_font->bg_data_format)
		{
			//step2 rotate area 
			vuyy_rotate_degree90_to_yuyv((unsigned char *)p_yuv_dst, (unsigned char *)p_yuv_rotate_temp, lcd_font->xsize, lcd_font->ysize);
		}
		if (PIXEL_FMT_YUYV == lcd_font->bg_data_format)
		{
			yuyv_rotate_degree90_to_yuyv((unsigned char *)p_yuv_dst, (unsigned char *)p_yuv_rotate_temp, lcd_font->xsize, lcd_font->ysize);
		}
		i = lcd_font->xsize;
		lcd_font->xsize = lcd_font->ysize;
		lcd_font->ysize = i;
		lcd_font->font_format = FONT_YUYV;
		p_yuv_dst = blend_addr2;
	}
	if (lcd_font->font_format == FONT_RGB565)  //font rgb565 data to yuv bg image
	{
		font_addr = blend_addr2;
		if (PIXEL_FMT_VUYY == lcd_font->bg_data_format)
		{
			vuyy_to_rgb565_convert((unsigned char *)blend_addr1, (unsigned char *)blend_addr2, lcd_font->xsize, lcd_font->ysize);
		}
		else if (PIXEL_FMT_YUYV == lcd_font->bg_data_format)
		{
			yuyv_to_rgb565_convert((unsigned char *)blend_addr1, (unsigned char *)blend_addr2, lcd_font->xsize, lcd_font->ysize);
		}
		else if (PIXEL_FMT_RGB565_LE == lcd_font->bg_data_format)
		{
			// bg alreadr rgb565
			font_addr = blend_addr1;
			rgb565_to_rgb565le_convert(blend_addr1, lcd_font->xsize, lcd_font->ysize);
		}
        else if (PIXEL_FMT_RGB888 == lcd_font->bg_data_format)
        {
			rgb888_to_rgb565_convert(blend_addr1, (uint16_t *)blend_addr2, lcd_font->xsize, lcd_font->ysize);
        }
        else
        {
        }
		//	lcd_storage_capture_save("yuv_to_rgb565.rgb", blend_addr2, len);
		font_t font;
		font.info = (ui_display_info_struct){font_addr,0,lcd_font->ysize,0,{0}};
		font.width = lcd_font->xsize;
		font.height = lcd_font->ysize;
		font.font_fmt = lcd_font->font_format;
		for(int i = 0; i < lcd_font->str_num; i++)
		{
			font.digit_info = lcd_font->str[i].font_digit_type;
			font.s = lcd_font->str[i].str;
			font.font_color = lcd_font->str[i].font_color;
			font.x_pos = lcd_font->str[i].x_pos;
			font.y_pos = lcd_font->str[i].y_pos;
			lcd_draw_font(&font);
		}
		if (PIXEL_FMT_VUYY == lcd_font->bg_data_format)
		{
			rgb565_to_vuyy_convert((uint16_t *)blend_addr2, (uint16_t *)blend_addr1, lcd_font->xsize, lcd_font->ysize);
		}
		else if(PIXEL_FMT_YUYV == lcd_font->bg_data_format)
		{
			rgb565_to_yuyv_convert((uint16_t *)blend_addr2, (uint16_t *)blend_addr1, lcd_font->xsize, lcd_font->ysize);
		}
		else if(PIXEL_FMT_RGB565_LE == lcd_font->bg_data_format)
		{
			// USE this api to convert to RGB5655_LE
			rgb565_to_rgb565le_convert(blend_addr1, lcd_font->xsize, lcd_font->ysize);
		}
        else if (PIXEL_FMT_RGB888 == lcd_font->bg_data_format)
        {
			rgb565_to_rgb888_convert((uint16_t *)blend_addr2, blend_addr1, lcd_font->xsize, lcd_font->ysize);
        }
        else
        {
        }
	}
	else  //font yuv data to yuv bg image
	{
		font_t font;
		font.info = (ui_display_info_struct){(unsigned char *)p_yuv_dst,0,lcd_font->ysize,0,{0}};
		font.width = lcd_font->xsize;
		font.height = lcd_font->ysize;
		font.font_fmt = lcd_font->font_format;
		for(int i = 0; i < lcd_font->str_num; i++)
		{
			font.digit_info = lcd_font->str[i].font_digit_type;
			font.s = lcd_font->str[i].str;
			font.font_color = lcd_font->str[i].font_color;
			font.x_pos = lcd_font->str[i].x_pos;
			font.y_pos = lcd_font->str[i].y_pos;
			lcd_draw_font(&font);
		}
	}

	if (lcd_font->font_rotate == ROTATE_270)
	{
		p_yuv_rotate_temp = blend_addr1;
		if (PIXEL_FMT_VUYY == lcd_font->bg_data_format)
		{
			yuyv_rotate_degree270_to_vuyy((unsigned char *)p_yuv_dst, (unsigned char *)p_yuv_rotate_temp, lcd_font->xsize, lcd_font->ysize);
		}
		else
		{
			yuyv_rotate_degree270_to_yuyv((unsigned char *)p_yuv_dst, (unsigned char *)p_yuv_rotate_temp, lcd_font->xsize, lcd_font->ysize);
		}
		// rotate back
		i = lcd_font->xsize;
		lcd_font->xsize = lcd_font->ysize;
		lcd_font->ysize = i;
	}
		p_yuv_src = blend_addr1;
		p_yuv_dst = lcd_font->pbg_addr;
#if 0//CONFIG_SOC_BK7258
//		dma2d_memcpy_psram(p_yuv_src, lcd_font->pbg_addr, lcd_font->xsize, lcd_font->ysize, 0, lcd_font->bg_offline);

		/*##-1- Configure the DMA2D Mode, Output Color Mode and output offset #############*/
		dma2d_config.init.mode         = DMA2D_M2M;             /**< Mode Memory To Memory */
		dma2d_config.init.color_mode    = DMA2D_OUTPUT_YUYV; /**< Output color mode is ARGB4444 : 16 bpp */
		dma2d_config.init.output_offset = lcd_font->bg_offline;                   /**< No offset on output */
		dma2d_config.init.red_blue_swap   = DMA2D_RB_REGULAR;     /**< No R&B swap for the output image */
		dma2d_config.init.alpha_inverted = DMA2D_REGULAR_ALPHA;  /**< No alpha inversion for the output image */
		dma2d_config.init.trans_ability = TRANS_16BYTES; 
		dma2d_config.init.out_byte_by_byte_reverse = NO_REVERSE;

		dma2d_config.layer_cfg[DMA2D_FOREGROUND_LAYER].alpha_mode = DMA2D_NO_MODIF_ALPHA;      /**< Keep original Alpha from ARGB4444 input */
		dma2d_config.layer_cfg[DMA2D_FOREGROUND_LAYER].input_alpha = 0xFF;                     /**< Fully opaque */
		dma2d_config.layer_cfg[DMA2D_FOREGROUND_LAYER].input_color_mode = DMA2D_INPUT_YUYV; /**< Input color is ARGB4444 : 16 bpp */
		dma2d_config.layer_cfg[DMA2D_FOREGROUND_LAYER].input_offset = 0;                     /**< No offset in input */
		dma2d_config.layer_cfg[DMA2D_FOREGROUND_LAYER].red_blue_swap   = DMA2D_RB_REGULAR;      /**< No R&B swap for the input image */
		dma2d_config.layer_cfg[DMA2D_FOREGROUND_LAYER].alpha_inverted = DMA2D_REGULAR_ALPHA;   /**< No alpha inversion for the input image */
		dma2d_config.layer_cfg[DMA2D_FOREGROUND_LAYER].input_data_reverse = NO_REVERSE;

		bk_dma2d_init(&dma2d_config);
		bk_dma2d_layer_config(&dma2d_config, DMA2D_FOREGROUND_LAYER);

		bk_dma2d_transfer_config(&dma2d_config, (uint32_t)p_yuv_src, (uint32_t)lcd_font->pbg_addr, lcd_font->xsize, lcd_font->ysize);
		bk_dma2d_start_transfer();
		while (bk_dma2d_is_transfer_busy()) {}
#else
		for(i = 0; i < lcd_font->ysize; i++)
		{
			os_memcpy((uint32_t *)p_yuv_dst, (uint32_t *)p_yuv_src, lcd_font->xsize * pixel_bytes);
			p_yuv_src += (lcd_font->xsize * pixel_bytes);
			p_yuv_dst += (lcd_font->bg_width * pixel_bytes);
		}
#endif

	return ret;
}
#endif

bk_err_t dma2d_fill_height_head_tail(frame_buffer_t *frame, media_ppi_t jpeg_ppi, media_ppi_t lcd_ppi, uint32_t color)
{
	dma2d_fill_t fill = {0};
	uint8_t fill_height = ((lcd_ppi & 0xFFFF) - (jpeg_ppi & 0xFFFF)) >> 1;

	fill.color = color;
	fill.color_format = DMA2D_OUTPUT_YUYV;
	fill.frameaddr = frame->frame;
	fill.frame_xsize = jpeg_ppi >> 16 ;
	fill.frame_ysize = jpeg_ppi & 0xFFFF;
	fill.width = jpeg_ppi >> 16;
	fill.height = fill_height;
	fill.pixel_byte =TWO_BYTES;
	fill.xpos = 0;
	fill.ypos = 0;
	dma2d_fill(&fill);
	bk_dma2d_start_transfer();

	if (rtos_get_semaphore(&s_blend.dma2d_complete_sem, BEKEN_NEVER_TIMEOUT) != BK_OK)
	{
		LOGE("%s, dma2d_complete_sem get failed: %d\n", __func__);
	}


	fill.xpos = 0;
	fill.ypos = fill_height + ( jpeg_ppi & 0xFFFF);
	dma2d_fill(&fill);
	bk_dma2d_start_transfer();
	
	LOGD("fill.frame_xsize,fill.frame_ysize, fill.width, fill.height= %d %d %d %d\n", fill.frame_xsize,fill.frame_ysize, fill.width, fill.height);

	if (rtos_get_semaphore(&s_blend.dma2d_complete_sem, BEKEN_NEVER_TIMEOUT) != BK_OK)
	{
		LOGE("%s, dma2d_complete_sem get failed: %d\n", __func__);
	}
	return 0;
}



bk_err_t lcd_blend_free_buffer(void)
{
		int ret = BK_FAIL;
#if CONFIG_LCD_DMA2D_BLEND  || CONFIG_LCD_FONT_BLEND
	if (blend_addr1 != NULL)
	{
		os_free(blend_addr1);
		blend_addr1 = NULL;
	}

	if (blend_addr2 != NULL)
	{
		os_free(blend_addr2);
		blend_addr2 = NULL;
	}
	LOGI("%s lcd dma2d deinit, free buffer ok\n", __func__);
#endif
	return ret;
}

bk_err_t lcd_blend_malloc_buffer(void)
{
	int ret = BK_FAIL;
#if CONFIG_LCD_DMA2D_BLEND  || CONFIG_LCD_FONT_BLEND
	if (blend_addr1 == NULL)
	{
		blend_addr1 = (uint8_t *)os_malloc(LCD_BLEND_MALLOC_SIZE);
		if (blend_addr1 == NULL)
			LOGE("lcd blend malloc blend_addr1 error\r\n");
		else
			LOGI("lcd blend malloc blend_addr1 = %p, size = %d \r\n", blend_addr1, LCD_BLEND_MALLOC_SIZE);
	}

	if (blend_addr2 == NULL)
	{
		blend_addr2 = (uint8_t *)os_malloc(LCD_BLEND_MALLOC_RGB_SIZE);
		if (blend_addr2 == NULL)
			LOGE("lcd blend malloc blend_addr2 error\r\n");
		else
			LOGI("lcd blend malloc blend_addr2 = %p, size = %d\r\n", blend_addr2, LCD_BLEND_MALLOC_RGB_SIZE);
	}

	if (blend_addr1 != NULL && blend_addr2 != NULL)
	{
		ret =  BK_OK;
	}
	else
	{
		if (blend_addr1 != NULL)
		{
			os_free(blend_addr1);
			blend_addr1 = NULL;
		}

		if (blend_addr2 != NULL)
		{
			os_free(blend_addr2);
			blend_addr2 = NULL;
		}
		ret =  BK_ERR_NO_MEM;
	}
#endif
	return ret;
}

bk_err_t lcd_dma2d_blend_init(void)
{
	bk_err_t ret = BK_OK;

#if CONFIG_LCD_DMA2D_BLEND
	bk_dma2d_driver_init();
#if (USE_DMA2D_BLEND_ISR_CALLBACKS == 1)
	ret = rtos_init_semaphore_ex(&s_blend.dma2d_complete_sem, 1, 0);
	if (ret != BK_OK)
	{
		LOGE("%s dma2d_sem init failed: %d\n", __func__, ret);
		return ret;
	}
	ret = rtos_init_semaphore_ex(&s_blend.dma2d_err_sem, 1, 0);
	if (ret != BK_OK)
	{
		LOGE("%s dma2d_err_sem init failed: %d\n", __func__, ret);
		return ret;
	}
	bk_dma2d_int_enable(DMA2D_CFG_ERROR | DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE,1);
	bk_dma2d_register_int_callback_isr(DMA2D_CFG_ERROR_ISR, dma2d_config_error);
	bk_dma2d_register_int_callback_isr(DMA2D_TRANS_ERROR_ISR, dma2d_transfer_error);
	bk_dma2d_register_int_callback_isr(DMA2D_TRANS_COMPLETE_ISR, dma2d_transfer_complete);
#endif 
#endif
	return ret;
}


bk_err_t lcd_dma2d_blend_deinit(void)
{
	bk_err_t ret = BK_OK;
#if CONFIG_LCD_DMA2D_BLEND
	bk_dma2d_driver_deinit();
#if USE_DMA2D_BLEND_ISR_CALLBACKS
	ret = rtos_deinit_semaphore(&s_blend.dma2d_complete_sem);
	if (ret != BK_OK)
	{
	LOGE("%s dma2d_complete_sem deinit failed: %d\n", __func__, ret);
	return ret;
	}
	ret = rtos_deinit_semaphore(&s_blend.dma2d_err_sem);
	if (ret != BK_OK)
	{
	LOGE("%s dma2d_err_sem deinit failed: %d\n", __func__, ret);
	return ret;
	}
#endif
#endif

 	return ret;
}

bk_err_t lcd_font_blend_deinit(void)
{
	bk_err_t ret = BK_OK;
 	return ret;
}

bk_err_t lcd_font_blend_init(void)
{
	bk_err_t ret = BK_OK;
 	return ret;
}



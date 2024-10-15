#include "lcd_rgb_device.h"
#include <os/mem.h>
#include "lcd_disp_hal.h"
#include "driver/lcd.h"
#include "frame_buffer.h"
#include <common/bk_include.h>
#include "cli.h"
#include <os/str.h>
#include <os/os.h>
#include <driver/int.h>
#include <common/bk_err.h>
#include <getopt.h>
#include "driver/lcd_types.h"
#include <driver/media_types.h>
#include <stdbool.h> 
#include "media_app.h"

extern int rand(void);
beken_timer_t lcd_rgb_timer;
#define pdata_addr	0x60000000


static void cpu_lcd_fill_test(uint32_t *addr, uint32_t color)
{
	uint32_t *p_addr = addr;
	for(int i=0; i<480*854; i++)
	{
		*(p_addr + i) = color;
	}
}

static void lcd_get_rand_color(uint32_t *color)
{
	uint32_t color_rand = 0;
	uint32_t color_rand_tmp = 0;

	color_rand = (uint32_t)rand();
	color_rand_tmp = (color_rand & 0xffff0000) >> 16; 
	*color = (color_rand & 0xffff0000) | color_rand_tmp;
}


static void lcd_rgb_change_color(void)
{
	static uint8_t state = 0;
	bk_err_t ret = BK_FAIL;
	lcd_open_t lcd_open;
	uint32_t color = 0;
	lcd_display_t lcd_display = {0};
	if(state == 0)
	{
		lcd_open.device_ppi = PPI_480X854;
		lcd_open.device_name = "st7701sn";
		ret = media_app_lcd_open(&lcd_open);
		if(BK_OK != ret)
		{
			os_printf("%s, not found device\n", __func__);
			return;
		}
		state = 1;
	}

	lcd_get_rand_color(&color);
	cpu_lcd_fill_test((uint32_t *)pdata_addr, color);

	lcd_display.display_type = LCD_TYPE_RGB565;
	lcd_display.image_addr = pdata_addr;
	lcd_display.x_start = 0;
	lcd_display.x_end = 479;
	lcd_display.y_start = 0;
	lcd_display.y_end = 853;
	ret = media_app_lcd_display(&lcd_display);
	if(BK_OK != ret)
	{
		os_printf("%s, lcd display fail\n", __func__);
	}

}


void lcd_rgb_display_rand_color(void)
{
	rtos_init_timer(&lcd_rgb_timer, 1000,  (timer_handler_t)lcd_rgb_change_color, 0);
	rtos_start_timer(&lcd_rgb_timer);

}




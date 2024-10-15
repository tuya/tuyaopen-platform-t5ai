#include "lcd_8080_device.h"
#include "lcd_disp_hal.h"
#include <common/bk_include.h>
#include <os/os.h>
#include <common/bk_err.h>
#include "driver/lcd_types.h"
#include <driver/media_types.h>
#include "media_app.h"

static beken_timer_t lcd_mcu_timer;
extern int bk_rand(void);
#define pdata_addr	0x60000000

static void cpu_lcd_fill_test(unsigned int *addr, unsigned int color)
{
	unsigned int *p_addr = addr;
	for(int i=0; i<320*480; i++)
	{
		*(p_addr + i) = color;
	}
}

static void lcd_8080_change_color(void)
{
	static unsigned char state = 0;
	bk_err_t ret = BK_FAIL;
	lcd_open_t lcd_open;
	if(state == 0)
	{
		lcd_open.device_ppi = PPI_320X480;
		lcd_open.device_name = "st7796s";
		ret = media_app_lcd_open(&lcd_open);
		if(BK_OK != ret)
		{
			os_printf("%s, not found device\n", __func__);
			return;
		}
		state = 1;
	}
	


	unsigned short x_start, x_end, y_start, y_end;
	unsigned int color = 0;
	lcd_display_t lcd_display = {0};
#if 0

	os_printf("%s\n", __func__);
	color = 0xf800f800;
	cpu_lcd_fill_test((uint32_t *)pdata_addr, color);
	x_start = 0;
	y_start = 0;
	x_end = 319;
	y_end = 479;
	
	lcd_display.display_type = LCD_TYPE_MCU8080;
	lcd_display.image_addr = (uint32_t)pdata_addr;
	lcd_display.x_start = x_start;
	lcd_display.x_end = x_end;
	lcd_display.y_start = y_start;
	lcd_display.y_end = y_end;	
	ret = media_app_lcd_display(&lcd_display);
	if(BK_OK != ret)
	{
		os_printf("%s, lcd display fail\n", __func__);
	}
	

	lcd_driver_set_display_base_addr((uint32_t)pdata_addr);
	lcd_hal_8080_start_transfer(1);
	lcd_hal_8080_ram_write(0x2c);
	rtos_delay_milliseconds(500);
#endif

	rtos_delay_milliseconds(1000);
	color = (unsigned int)bk_rand();
	color = (color & 0xffff0000) | (color >> 16 & 0xffff);
	cpu_lcd_fill_test((unsigned int *)pdata_addr, color);
	x_start = (unsigned short)((bk_rand() & 0xffff) % 320) /2;
	y_start = (unsigned short)((bk_rand() & 0xffff) % 480) /2;
	x_end = x_start * 2;
	y_end = y_start * 2;
	lcd_display.x_start = x_start;
	lcd_display.x_end = x_end;
	lcd_display.y_start = y_start;
	lcd_display.y_end = y_end;
	lcd_display.display_type = LCD_TYPE_MCU8080;
	lcd_display.image_addr = (unsigned int)pdata_addr;

	ret = media_app_lcd_display(&lcd_display);
	if(BK_OK != ret)
	{
		os_printf("%s, lcd display fail\n", __func__);
	}
#if 0
	lcd_st7796s_set_display_mem_area(x_start, x_end, y_start, y_end);
	lcd_driver_set_display_base_addr((uint32_t)pdata_addr);
	lcd_hal_8080_start_transfer(1);
	lcd_hal_8080_ram_write(0x2c);
#endif
}

void lcd_8080_display_rand_color(void)
{
	os_printf("enter %s\n", __func__);

	rtos_init_timer(&lcd_mcu_timer, 1000,  (timer_handler_t)lcd_8080_change_color, 0);
	rtos_start_timer(&lcd_mcu_timer);
}




#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <driver/int.h>
#include <os/os.h>
#include <common/bk_err.h>

#include "lcd_mcu_8080.h"
#include "lcd_act.h"
#include "media_app.h"
#include <driver/psram.h>
#include <driver/psram_types.h>

#include "psram_mem_slab.h"


#ifndef CONFIG_SOC_BK7256

#define TAG "lcd_8080"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

uint32_t lcd_mcu_psram;
static beken_timer_t lcd_8080_test_timer;
static uint8_t rand_index = 0;
uint8_t lcd_mcu_8080_demo_status = 0;

const lcd_open_t lcd_device =
{
	.device_ppi = PPI_320X480,
	.device_name = "st7796s",
};

void lcd_mcu_8080_open(void)
{
	LOGI("%s\n\r", __func__);
	// init psram on cpu0
	bk_psram_frame_buffer_init();
	uint8_t *psram_lcd_display = NULL;
	psram_lcd_display = bk_psram_frame_buffer_malloc(PSRAM_HEAP_YUV, 300 * 1024);
	if (psram_lcd_display == NULL)
	{
		LOGE("no enough psram for malloc\n\r");
		return;
	}
	LOGI("psram_lcd_display addr: 0x%x\n\r", psram_lcd_display);
	lcd_open_t lcd_open = {0};
	lcd_open.device_ppi = lcd_device.device_ppi;
	lcd_open.device_name = lcd_device.device_name;
	media_app_lcd_open(&lcd_open);

	lcd_mcu_psram = (uint32_t)psram_lcd_display;

	// lcd open set status 1
	lcd_mcu_8080_demo_status = 1;
}

void lcd_mcu_8080_close(void)
{
	LOGI("%s\n\r", __func__);
	if (lcd_mcu_8080_demo_status == 2)
	{
		rand_index = 0;
		rtos_deinit_timer(&lcd_8080_test_timer);
		media_app_lcd_close();
		bk_psram_frame_buffer_free((void*)lcd_mcu_psram);
	}
	else if (lcd_mcu_8080_demo_status == 1)
	{
		media_app_lcd_close();
		bk_psram_frame_buffer_free((void*)lcd_mcu_psram);
	}

	// lcd open set status 0
	lcd_mcu_8080_demo_status = 0;

}

static void cpu_lcd_fill_color(uint32_t *addr, uint32_t color1, uint32_t color2, lcd_display_t lcd_display)
{
	uint32_t *p_addr = addr;
	for(int i=0; i<320*480/2; i++) // for 16bits lcd display
	{
		if (i <= (lcd_display.y_end) * lcd_display.x_end/2 && i >= lcd_display.x_end * lcd_display.y_start/2)
		{
			*(p_addr + i) = color1;
		}
		else
		{
			*(p_addr + i) = color2;
		}
	}
}

static void cpu_lcd_fill_area_color(uint32_t *addr, uint32_t color, lcd_display_t lcd_display)
{
	uint32_t *p_addr = addr;
	for (int i= 0; i < (lcd_display.y_end - lcd_display.y_start) * (lcd_display.x_end - lcd_display.x_start) / 2; i++)
	{
		*(p_addr + i) = color;
	}

}

#define COLOR_RED 0xf800f800
#define COLOR_GREEN 0x0f800f80
#define COLOR_BLUE 0x00f800f8
#define COLOR_WHITE 0xffffffff

static void lcd_8080_change_color(void)
{
	static lcd_display_t lcd_mcu_display;
	uint32_t color1 = 0;
	uint32_t color2 = 0;
	lcd_mcu_display.image_addr = lcd_mcu_psram;
	LOGI("%s rand_index: %d\n\r", __func__, rand_index);
	if (rand_index == 0)
	{
		color1 = COLOR_GREEN;
		lcd_mcu_display.x_start = 80;//0;
		lcd_mcu_display.y_start = 120;//80 * (rand_index - 1);
		lcd_mcu_display.x_end = 240;//319;
		lcd_mcu_display.y_end = 360;//80 * (rand_index);
		// cpu_lcd_fill_color((uint32_t *)lcd_mcu_psram, color1, color2, lcd_mcu_display);
		cpu_lcd_fill_area_color((uint32_t *)lcd_mcu_psram, color1, lcd_mcu_display);
		rand_index = 1;
		lcd_mcu_display.display_type = 2;
	}
	else if (rand_index == 1)
	{
		color1 = COLOR_WHITE;
		color2 = COLOR_WHITE;
		lcd_mcu_display.x_start = 0;
		lcd_mcu_display.y_start = 0;
		lcd_mcu_display.x_end = 319;
		lcd_mcu_display.y_end = 479;
		cpu_lcd_fill_color((uint32_t *)lcd_mcu_psram, color1, color2, lcd_mcu_display);
		rand_index = 2;
		lcd_mcu_display.display_type = 0;
	}
	else if (rand_index == 2)
	{
		color1 = COLOR_BLUE;
		color2 = COLOR_WHITE;
		lcd_mcu_display.x_start = 0;
		lcd_mcu_display.y_start = 80 * (rand_index - 1);
		lcd_mcu_display.x_end = 319;
		lcd_mcu_display.y_end = 80 * (rand_index);
		cpu_lcd_fill_color((uint32_t *)lcd_mcu_psram, color1, color2, lcd_mcu_display);
		rand_index = 3;
		lcd_mcu_display.display_type = 0;
	}
	else if (rand_index == 3)
	{
		color1 = COLOR_RED;
		color2 = COLOR_WHITE;
		lcd_mcu_display.x_start = 0;
		lcd_mcu_display.y_start = 80 * (rand_index - 1);
		lcd_mcu_display.x_end = 319;
		lcd_mcu_display.y_end = 80 * (rand_index);
		cpu_lcd_fill_color((uint32_t *)lcd_mcu_psram, color1, color2, lcd_mcu_display);
		rand_index = 4;
		lcd_mcu_display.display_type = 0;
	}
	else if (rand_index == 4)
	{
		color1 = COLOR_GREEN;
		color2 = COLOR_WHITE;
		lcd_mcu_display.x_start = 0;
		lcd_mcu_display.y_start = 80 * (rand_index - 1);
		lcd_mcu_display.x_end = 319;
		lcd_mcu_display.y_end = 80 * (rand_index);
		cpu_lcd_fill_color((uint32_t *)lcd_mcu_psram, color1, color2, lcd_mcu_display);
		rand_index = 5;
		lcd_mcu_display.display_type = 0;
	}
	else if (rand_index == 5)
	{
		color1 = COLOR_BLUE;
		color2 = COLOR_BLUE;
		lcd_mcu_display.x_start = 0;
		lcd_mcu_display.y_start = 0;
		lcd_mcu_display.x_end = 319;
		lcd_mcu_display.y_end = 479;
		cpu_lcd_fill_color((uint32_t *)lcd_mcu_psram, color1, color2, lcd_mcu_display);
		lcd_mcu_display.display_type = 0;
		rand_index = 6;
	}
	else if (rand_index == 6)
	{
		color1 = COLOR_RED;
		color2 = COLOR_WHITE;
		lcd_mcu_display.x_start = 0;
		lcd_mcu_display.y_start = 240;//80 * (rand_index - 1);
		lcd_mcu_display.x_end = 320;
		lcd_mcu_display.y_end = 480;//80 * (rand_index);
		//cpu_lcd_fill_color((uint32_t *)lcd_mcu_psram, color1, color2, lcd_mcu_display);
		cpu_lcd_fill_area_color((uint32_t *)lcd_mcu_psram, color1, lcd_mcu_display);
		lcd_mcu_display.display_type = 1;
		rand_index = 7;
	}
	else if (rand_index == 7)
	{
		color1 = COLOR_GREEN;
		color2 = COLOR_RED;
		lcd_mcu_display.x_start = 0;
		lcd_mcu_display.y_start = 240;//80 * (rand_index - 1);
		lcd_mcu_display.x_end = 320;
		lcd_mcu_display.y_end = 480;//80 * (rand_index);
		//cpu_lcd_fill_color((uint32_t *)lcd_mcu_psram, color1, color2, lcd_mcu_display);
		cpu_lcd_fill_area_color((uint32_t *)lcd_mcu_psram, color1, lcd_mcu_display);
		lcd_mcu_display.display_type = 1;
		rand_index = 8;
	}
	else if (rand_index == 8)
	{
		color1 = COLOR_WHITE;
		color2 = COLOR_WHITE;
		lcd_mcu_display.x_start = 0;
		lcd_mcu_display.y_start = 0;
		lcd_mcu_display.x_end = 319;
		lcd_mcu_display.y_end = 479;
		cpu_lcd_fill_color((uint32_t *)lcd_mcu_psram, color1, color2, lcd_mcu_display);
		lcd_mcu_display.display_type = 0;
		rand_index = 0;
	}

	if (lcd_mcu_display.display_type == 0)
	{
		lcd_mcu_display.x_start = 0;
		lcd_mcu_display.y_start = 0;
		lcd_mcu_display.x_end = 319;
		lcd_mcu_display.y_end = 479;
	}

	media_app_lcd_display(&lcd_mcu_display);
}

void lcd_mcu_8080_display(void)
{
	if (lcd_mcu_8080_demo_status == 1)
	{
		// lcd open set status 0
		lcd_mcu_8080_demo_status = 2;
		rtos_init_timer(&lcd_8080_test_timer, 5000,  (timer_handler_t)lcd_8080_change_color, 0);
		rtos_start_timer(&lcd_8080_test_timer);
	}
	else
	{
		LOGE("lcd not open or display on now\n\r");
	}
}
#endif


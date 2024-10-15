#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <driver/pwr_clk.h>
#include "cli.h"
#if (CONFIG_SYS_CPU0) || (CONFIG_SYS_CPU1)
#include "lcd_act.h"
#include "media_app.h"
#endif
#if CONFIG_LVGL
#include "lv_vendor.h"
#include "lv_demo_benchmark.h"
#endif
#include "driver/drv_tp.h"
#include <driver/lcd.h>
#include "media_service.h"

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);

#define CMDS_COUNT  (sizeof(s_benchmark_commands) / sizeof(struct cli_command))

const lcd_open_t lcd_open =
{
	.device_ppi = PPI_480X480,
	.device_name = "st7701s",
};

void cli_benchmark_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	os_printf("%s\r\n", __func__);
}

static const struct cli_command s_benchmark_commands[] =
{
	{"benchmark", "benchmark", cli_benchmark_cmd},
};

int cli_benchmark_init(void)
{
	return cli_register_commands(s_benchmark_commands, CMDS_COUNT);
}


#if (CONFIG_SYS_CPU1) && (CONFIG_SOC_BK7258)
#include "yuv_encode.h"

void lvgl_event_handle(media_mailbox_msg_t *msg)
{
	os_printf("%s EVENT_LVGL_OPEN_IND \n", __func__);

	lv_vnd_config_t lv_vnd_config = {0};
	lcd_open_t *lcd_open = (lcd_open_t *)msg->param;

#ifdef CONFIG_LVGL_USE_PSRAM
#define PSRAM_DRAW_BUFFER ((0x60000000UL) + 5 * 1024 * 1024)

	lv_vnd_config.draw_pixel_size = ppi_to_pixel_x(lcd_open->device_ppi) * ppi_to_pixel_y(lcd_open->device_ppi);
	lv_vnd_config.draw_buf_2_1 = (lv_color_t *)PSRAM_DRAW_BUFFER;
	lv_vnd_config.draw_buf_2_2 = (lv_color_t *)(PSRAM_DRAW_BUFFER + lv_vnd_config->draw_pixel_size * sizeof(lv_color_t));
#else

    #if CONFIG_LV_ATTRIBUTE_FAST_MEM_L2
    #define DRAW_BUFFER_SIZE    (124 * 1024 / 2)
    #else
    #define DRAW_BUFFER_SIZE    (128 * 1024 / 2)
    #endif
#define PSRAM_FRAME_BUFFER ((0x60000000UL) + 5 * 1024 * 1024)
    static __attribute__((section(".lvgl_draw"))) uint8_t draw_buf1[DRAW_BUFFER_SIZE];
    static __attribute__((section(".lvgl_draw"))) uint8_t draw_buf2[DRAW_BUFFER_SIZE];
	lv_vnd_config.draw_pixel_size = DRAW_BUFFER_SIZE / sizeof(lv_color_t);
	lv_vnd_config.draw_buf_2_1 = (lv_color_t *)draw_buf1;
	lv_vnd_config.draw_buf_2_2 = (lv_color_t *)draw_buf2;
	lv_vnd_config.frame_buf_1 = (lv_color_t *)PSRAM_FRAME_BUFFER;
	lv_vnd_config.frame_buf_2 = NULL;
#endif
    lv_vnd_config.lcd_hor_res = ppi_to_pixel_x(lcd_open->device_ppi);
    lv_vnd_config.lcd_ver_res = ppi_to_pixel_y(lcd_open->device_ppi);
	lv_vnd_config.rotation = ROTATE_NONE;

	lv_vendor_init(&lv_vnd_config);

	lcd_display_open(lcd_open);

#if (CONFIG_TP)
	drv_tp_open(ppi_to_pixel_x(lcd_open->device_ppi), ppi_to_pixel_y(lcd_open->device_ppi), TP_MIRROR_NONE);
#endif

	lv_vendor_disp_lock();
	lv_demo_benchmark();
	lv_vendor_disp_unlock();

	lv_vendor_start();

    msg_send_rsp_to_media_major_mailbox(msg, BK_OK, APP_MODULE);
}
#endif

#if (CONFIG_SOC_BK7256)
#ifdef CONFIG_CACHE_CUSTOM_SRAM_MAPPING
const unsigned int g_sram_addr_map[4] =
{
	0x38000000,
	0x30020000,
	0x38020000,
	0x30000000
};
#endif
#endif

#if (CONFIG_SYS_CPU0)
void benchmark_init(void)
{
	cli_benchmark_init();

#if (CONFIG_SOC_BK7256)
#ifdef LV_USE_DEMO_BENCHMARK
	bk_err_t ret;
	lv_vnd_config_t lv_vnd_config;

	os_printf("Benchmark Start\r\n");
	cli_benchmark_init();

#ifdef CONFIG_LVGL_USE_PSRAM
#define PSRAM_DRAW_BUFFER ((0x60000000UL) + 5 * 1024 * 1024)

	lv_vnd_config.draw_pixel_size = ppi_to_pixel_x(lcd_open.device_ppi) * ppi_to_pixel_y(lcd_open.device_ppi);
	lv_vnd_config.draw_buf_2_1 = (lv_color_t *)PSRAM_DRAW_BUFFER;
	lv_vnd_config.draw_buf_2_2 = (lv_color_t *)(PSRAM_DRAW_BUFFER + lv_vnd_config.draw_pixel_size * sizeof(lv_color_t));
#else
#define PSRAM_FRAME_BUFFER ((0x60000000UL) + 5 * 1024 * 1024)

	lv_vnd_config.draw_pixel_size = (45 * 1024) / sizeof(lv_color_t);
	lv_vnd_config.draw_buf_2_1 = LV_MEM_CUSTOM_ALLOC(lv_vnd_config.draw_pixel_size * sizeof(lv_color_t));
	lv_vnd_config.draw_buf_2_2 = NULL;
	lv_vnd_config.frame_buf_1 = (lv_color_t *)PSRAM_FRAME_BUFFER;
	lv_vnd_config.frame_buf_2 = NULL;
#endif
    lv_vnd_config.lcd_hor_res = ppi_to_pixel_x(lcd_open.device_ppi);
    lv_vnd_config.lcd_ver_res = ppi_to_pixel_y(lcd_open.device_ppi);
	lv_vnd_config.rotation = ROTATE_NONE;


#if (CONFIG_TP)
	drv_tp_open(ppi_to_pixel_x(lcd_open.device_ppi), ppi_to_pixel_y(lcd_open.device_ppi), TP_MIRROR_NONE);
#endif

	lv_vendor_init(&lv_vnd_config);
	ret = media_app_lcd_open((lcd_open_t *)&lcd_open);
	if (ret != BK_OK)
	{
		os_printf("media_app_lcd_open failed\r\n");
	}
    lv_vendor_start();

    lcd_driver_backlight_open();

	lv_vendor_disp_lock();
	lv_demo_benchmark();
	lv_vendor_disp_unlock();
#endif

#elif (CONFIG_SOC_BK7258)
	bk_err_t ret;
	os_printf("!!!BK7258 LVGL BENCHMARK!!!\r\n");

	ret = media_app_lvgl_open((lcd_open_t *)&lcd_open);
	if (ret != BK_OK)
	{
		os_printf("media_app_lvgl_open failed\r\n");
		return;
	}
#endif
}
#endif

void user_app_main(void)
{
}

int main(void)
{
#if (CONFIG_SYS_CPU0)
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
#if (CONFIG_LV_CODE_LOAD_PSRAM)
    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_LVGL_CODE_RUN,PM_POWER_MODULE_STATE_ON);
#endif
	// bk_set_printf_sync(true);
	// shell_set_log_level(BK_LOG_WARN);
#endif
	bk_init();
    media_service_init();

#if (CONFIG_SYS_CPU0)
	benchmark_init();
#endif

	return 0;
}
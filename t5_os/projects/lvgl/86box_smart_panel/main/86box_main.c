#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>

#include "cli.h"
#include "lcd_act.h"
#include "media_app.h"
#include "driver/drv_tp.h"
#if (CONFIG_LVGL)
#include "lv_vendor.h"
#include "lvgl.h"
#include "page_load_ctrol.h"
#include "img_utility.h"
#endif
#include "modules/pm.h"
#if (CONFIG_WANSON_ASR)
#include "wanson_asr.h"
#endif

#include <driver/lcd.h>
#include "media_service.h"

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);

#define CMDS_COUNT  (sizeof(s_86box_commands) / sizeof(struct cli_command))

const lcd_open_t lcd_open =
{
	.device_ppi = PPI_480X480,
	.device_name = "st7701s",
};

void cli_86box_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    os_printf("%s %d\r\n", __func__, __LINE__);
}

static const struct cli_command s_86box_commands[] =
{
	{"86box", "86box", cli_86box_cmd},
};

int cli_86box_init(void)
{
	return cli_register_commands(s_86box_commands, CMDS_COUNT);
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
#define PSRAM_FRAME_BUFFER ((0x60000000UL) + 5 * 1024 * 1024)
	lv_vnd_config.draw_pixel_size = (45 * 1024) / sizeof(lv_color_t);
	lv_vnd_config.draw_buf_2_1 = LV_MEM_CUSTOM_ALLOC(lv_vnd_config.draw_pixel_size * sizeof(lv_color_t));
	lv_vnd_config.draw_buf_2_2 = NULL;
	lv_vnd_config.frame_buf_1 = (lv_color_t *)PSRAM_FRAME_BUFFER;
	lv_vnd_config.frame_buf_2 = (lv_color_t *)(PSRAM_FRAME_BUFFER + ppi_to_pixel_x(lcd_open->device_ppi) * ppi_to_pixel_y(lcd_open->device_ppi) * sizeof(lv_color_t));
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
	hor_page_load_main();
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
	0x30040000,
	0x30000000
};
#endif
#endif


#if (CONFIG_SYS_CPU0)
void lv_86box_init(void)
{
	cli_86box_init();

#if (CONFIG_SOC_BK7256)
#ifdef CONFIG_LV_USE_DEMO_86BOX
	bk_err_t ret;
	lv_vnd_config_t lv_vnd_config = {0};

#ifdef CONFIG_LVGL_USE_PSRAM
#define PSRAM_DRAW_BUFFER ((0x60000000UL) + 5 * 1024 * 1024)

	lv_vnd_config.draw_pixel_size = ppi_to_pixel_x(lcd_open.device_ppi) * ppi_to_pixel_y(lcd_open.device_ppi);
	lv_vnd_config.draw_buf_2_1 = (lv_color_t *)PSRAM_DRAW_BUFFER;
	lv_vnd_config.draw_buf_2_2 = (lv_color_t *)(PSRAM_DRAW_BUFFER + lv_vnd_config.draw_pixel_size * sizeof(lv_color_t));
#else
	#define PSRAM_FRAME_BUFFER ((0x64000000UL) + 5 * 1024 * 1024)
	lv_vnd_config.draw_pixel_size = (40 * 1024) / sizeof(lv_color_t);
	lv_vnd_config.draw_buf_2_1 = LV_MEM_CUSTOM_ALLOC(lv_vnd_config.draw_pixel_size * sizeof(lv_color_t));
	lv_vnd_config.draw_buf_2_2 = NULL;

	lv_vnd_config.frame_buf_1 = (lv_color_t *)PSRAM_FRAME_BUFFER;
	lv_vnd_config.frame_buf_2 = (lv_color_t *)(PSRAM_FRAME_BUFFER + ppi_to_pixel_x(lcd_open.device_ppi)*ppi_to_pixel_y(lcd_open.device_ppi) * sizeof(lv_color_t));
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
	hor_page_load_main();
	lv_vendor_disp_unlock();
#endif

#elif (CONFIG_SOC_BK7258)
    bk_err_t ret;
	os_printf("!!!BK7258 LVGL 86box Smart Panel!!!\r\n");

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
//	bk_pm_module_vote_cpu_freq(PM_DEV_ID_DISP, PM_CPU_FRQ_320M);
#if (CONFIG_SYS_CPU0)
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
	// bk_set_printf_sync(true);
	// shell_set_log_level(BK_LOG_WARN);
#endif
	bk_init();
    media_service_init();

#if (CONFIG_SYS_CPU0)
	lv_86box_init();
#endif

#if (CONFIG_WANSON_ASR)
#if ((CONFIG_ASDF_WORK_CPU1 && CONFIG_SYS_CPU1) || (CONFIG_ASDF_WORK_CPU0 && CONFIG_SYS_CPU0))
	rtos_delay_milliseconds(1000);
	wanson_asr_init();
	wanson_asr_start();
#endif
#endif

	return 0;
}
#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>

#include "cli.h"

#include "lcd_act.h"
#include "media_app.h"
#include "frame_buffer.h"
#include "lvgl.h"
#include "lv_vendor.h"
#include "driver/drv_tp.h"
#include "lvgl_vfs_init.h"
#include <driver/lcd.h>

#include "media_service.h"

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);

#define PSRAM_ADDRESS          (0x60000000UL)

#define DIS_800X600_COUNT      3
#define DIS_800X600_SIZE       (800 * 600 * 2)
#define DIS_800X600_ADDRESS    (PSRAM_ADDRESS)

#define JPG_800X600_COUNT      4
#define JPG_800X600_SIZE       (1024 * 80)
#define JPG_800X600_ADDRESS    (PSRAM_ADDRESS + (DIS_800X600_SIZE * DIS_800X600_COUNT))

#ifdef CONFIG_EXTENTED_PSRAM_LAYOUT
// psram used: 0x60000000-0x6030F200
// psram_heap: 0x60320000 size: 0xE0000
const fb_layout_t fb_layout[] =
{
	{
		.ppi = PPI_800X600,
		.set = {
			{/* display */
				DIS_800X600_COUNT,
				DIS_800X600_SIZE,
				DIS_800X600_ADDRESS,
			},
			{/* jpeg */
				JPG_800X600_COUNT,
				JPG_800X600_SIZE,
				JPG_800X600_ADDRESS,
			},
		},
	},
};
#endif

#define CMDS_COUNT  (sizeof(s_lvcamera_commands) / sizeof(struct cli_command))


extern void lv_example_meter(void);
extern void lv_example_meter_exit(void);

#define CAMERA_PPI PPI_800X480

const lcd_open_t lcd_open =
{
	.device_ppi = PPI_800X480,
	.device_name = "h050iwv",
};


media_camera_device_t camera_device = {
	.type = UVC_CAMERA,
	.mode = JPEG_MODE,
	.fmt = PIXEL_FMT_JPEG,
	.info.fps = FPS25,
	.info.resolution.width = 800,
	.info.resolution.height = 480,
};

void lvcamera_open(void)
{
	os_printf("%s\r\n", __func__);

#if (CONFIG_TP)
	drv_tp_close();
#endif
	lv_example_meter_exit();
	lv_vendor_stop();
	media_app_camera_open(&camera_device);
}

void lvcamera_close(void)
{
	os_printf("%s\r\n", __func__);

	media_app_camera_close(camera_device.type);

#if (CONFIG_TP)
	drv_tp_open(ppi_to_pixel_x(lcd_open.device_ppi), ppi_to_pixel_y(lcd_open.device_ppi), TP_MIRROR_NONE);
#endif

	lv_vendor_start();
	lv_example_meter();
}


void cli_lvcamera_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	os_printf("%s\r\n", __func__);

	if (os_strcmp(argv[1], "open") == 0)
	{
		lvcamera_open();
	}

	if (os_strcmp(argv[1], "close") == 0)
	{
		lvcamera_close();
	}

}

static const struct cli_command s_lvcamera_commands[] =
{
	{"lvcam", "lvcam", cli_lvcamera_cmd},
};

int cli_lvcamera_init(void)
{
	os_printf("%s\r\n", __func__);
	return cli_register_commands(s_lvcamera_commands, CMDS_COUNT);
}


#ifdef CONFIG_CACHE_CUSTOM_SRAM_MAPPING
const unsigned int g_sram_addr_map[4] =
{
	0x38000000,
	0x30020000,
	0x38020000,
	0x30000000
};
#endif

void lvcamera_main_init(void)
{
	bk_err_t ret;
	lv_vnd_config_t lv_vnd_config;

	os_printf("meter Start\r\n");

	cli_lvcamera_init();

	lvgl_vfs_init();

#ifdef CONFIG_LVGL_USE_PSRAM
#define PSRAM_DRAW_BUFFER (PSRAM_ADDRESS)

	lv_vnd_config.draw_pixel_size = ppi_to_pixel_x(lcd_open.device_ppi) * ppi_to_pixel_y(lcd_open.device_ppi);
	lv_vnd_config.draw_buf_2_1 = (lv_color_t *)PSRAM_DRAW_BUFFER;
	lv_vnd_config.draw_buf_2_2 = (lv_color_t *)(PSRAM_DRAW_BUFFER + lv_vnd_config.draw_pixel_size * sizeof(lv_color_t));
#else
#define PSRAM_FRAME_BUFFER (PSRAM_ADDRESS)

	lv_vnd_config.draw_pixel_size = (30 * 1024) / sizeof(lv_color_t);
	lv_vnd_config.draw_buf_2_1 = LV_MEM_CUSTOM_ALLOC(lv_vnd_config.draw_pixel_size * sizeof(lv_color_t));
	lv_vnd_config.draw_buf_2_2 = NULL;
	lv_vnd_config.frame_buf_1 = (lv_color_t *)PSRAM_FRAME_BUFFER;
	lv_vnd_config.frame_buf_2 = (lv_color_t *)(PSRAM_FRAME_BUFFER + ppi_to_pixel_x(lcd_open.device_ppi) * ppi_to_pixel_y(lcd_open.device_ppi) * sizeof(lv_color_t));
#endif
    lv_vnd_config.lcd_hor_res = ppi_to_pixel_x(lcd_open.device_ppi);
    lv_vnd_config.lcd_ver_res = ppi_to_pixel_y(lcd_open.device_ppi);
	lv_vnd_config.rotation = ROTATE_NONE;

#if (CONFIG_TP)
	drv_tp_open(ppi_to_pixel_x(lcd_open.device_ppi), ppi_to_pixel_y(lcd_open.device_ppi), TP_MIRROR_NONE);
#endif

	lv_vendor_init(&lv_vnd_config);
	media_app_lcd_rotate(BK_TRUE);
	ret = media_app_lcd_open((lcd_open_t *)&lcd_open);
	if (ret != BK_OK)
	{
		os_printf("media_app_lcd_open failed\r\n");
	}

	lv_vendor_start();
    lcd_driver_backlight_open();

	lv_example_meter();
}

void user_app_main(void)
{
}

int main(void)
{
#if (CONFIG_SYS_CPU0)
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
	// bk_set_printf_sync(true);
	// shell_set_log_level(BK_LOG_WARN);
#endif
	bk_init();
    media_service_init();

#ifdef CONFIG_LV_USE_DEMO_METER
	lvcamera_main_init();
#endif

	return 0;
}

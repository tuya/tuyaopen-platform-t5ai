#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "lcd_act.h"
#include "media_app.h"
#include <driver/lcd.h>

#include "media_service.h"

extern void rtos_set_user_app_entry(beken_thread_function_t entry);

void user_app_main(void){

}

//volatile unsigned char face_buf_test[1024 * 50];

int main(void)
{
#if (CONFIG_SYS_CPU0)
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
	// bk_set_printf_sync(true);
	// shell_set_log_level(BK_LOG_WARN);
#endif
	bk_init();
	media_service_init();

	//os_printf("%s\n", face_buf_test);
	
#if (CONFIG_SYS_CPU0)
	lcd_open_t lcd_open;
	media_camera_device_t device = {0};
	lcd_open.device_ppi = PPI_480X272;
	lcd_open.device_name = "NULL";

	device.type = DVP_CAMERA;
	device.mode = JPEG_MODE;
	device.fmt = PIXEL_FMT_JPEG;
	device.info.fps = FPS25;
	device.info.resolution.width = 480;
	device.info.resolution.height = 320;
	media_app_camera_open(&device);
	rtos_delay_milliseconds(100);
	media_app_lcd_open(&lcd_open);
    lcd_driver_backlight_open();
#endif
	return 0;
}

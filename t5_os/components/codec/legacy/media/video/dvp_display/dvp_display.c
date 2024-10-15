#include "modules/pm.h"
#include <driver/int.h>
#include <driver/dvp_camera.h>
#include <driver/dvp_camera_types.h>

extern bk_err_t bk_dvp_camera_open(media_camera_device_t *device);
extern int lcd_act_driver_init(uint32_t lcd_ppi);

void dvp_display_test()
{
	media_camera_device_t device = {0};
	rtos_delay_milliseconds(1000);
#if (CONFIG_MEDIA)
	//frame_buffer_enable(true);
#endif

	device.type = DVP_CAMERA;
	device.mode = JPEG_MODE;
	device.fmt = PIXEL_FMT_JPEG;
	device.info.resolution.width = 640;
	device.info.resolution.height = 480;
	device.info.fps = FPS25;
	bk_dvp_camera_open(&device);

	rtos_delay_milliseconds(1000);
	bk_pm_module_vote_cpu_freq(PM_DEV_ID_DISP, PM_CPU_FRQ_320M);
	lcd_act_driver_init(PPI_480X272);
}



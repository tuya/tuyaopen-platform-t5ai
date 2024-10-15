#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>

#include "media_service.h"
#include "driver/media_types.h"

#include <modules/jpeg_decode_sw.h>
#include <modules/tjpgd.h>
#if (CONFIG_SYS_CPU0)
#include "media_app.h"
#include "media_evt.h"

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);
extern void media_read_frame_callback(frame_buffer_t *frame);
sw_jpeg_dec_res_t result;

media_camera_device_t camera_device = {
	.type = DVP_CAMERA,
	.mode = JPEG_MODE,
	.fmt = PIXEL_FMT_JPEG,
	.info.fps = FPS25,
	.info.resolution.width = 640,
	.info.resolution.height = 480,
};

static void media_read_frame_info_callback(frame_buffer_t *frame)
{
	os_printf("##MJPEG:camera_type:%d(1:dvp 2:uvc) frame_id:%d, length:%d, frame_addr:%p \r\n",frame->type,frame->sequence,
		frame->length, frame->frame);
	bk_jpeg_get_img_info(frame->length, frame->frame, &result);
	os_printf("##DECODE:pixel_x:%d, pixel_y:%d\n\r", result.pixel_x,result.pixel_y);
	os_printf("rotate_angle:%d(0:0 1:90 2:180 3:270)\n\r",jd_get_rotate());
	os_printf("byte_order:%d(0:little endian 1:big endian)\n\r",jd_get_byte_order());
	switch(jd_get_format())
	{
		case JD_FORMAT_RGB888:
			os_printf("out_fmt:RGB888\r\n\n");
			break;

		case JD_FORMAT_RGB565:
			os_printf("out_fmt:RGB565\r\n\n");
			break;

		case JD_FORMAT_Grayscale:
			os_printf("out_fmt:Grayscale\r\n\n");
			break;

		case JD_FORMAT_YUYV:
			os_printf("out_fmt:YUYV\r\n\n");
			break;

		case JD_FORMAT_VYUY:
			os_printf("out_fmt:VYUY\r\n\n");
			break;

		case JD_FORMAT_VUYY:
			os_printf("out_fmt:VUYY\r\n\n");
			break;

		default:
			break;
	}
}

void dvp_debug_init (void)
{
	bk_err_t ret;
	bk_jpeg_dec_sw_init(NULL, 0);

	ret =media_app_camera_open(&camera_device);
	if (ret != BK_OK)
	{
		os_printf("media_app_camera_open failed\r\n");
	}

	ret =media_app_register_read_frame_callback(camera_device.fmt, media_read_frame_info_callback);
	if (ret != BK_OK)
	{
		os_printf("media_app_register_read_frame_callback failed\r\n");
	}
}

void user_app_main(void)
{
	
}
#endif

int main(void)
{
#if (CONFIG_SYS_CPU0)
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
#endif
	bk_init();
    media_service_init();
#if (CONFIG_SYS_CPU0)
	dvp_debug_init();
#endif
	return 0;
}
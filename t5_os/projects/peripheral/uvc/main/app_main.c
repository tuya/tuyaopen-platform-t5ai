#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>

#include "media_service.h"
#include "driver/media_types.h"
#include <modules/jpeg_decode_sw.h>
#include <modules/tjpgd.h>
#include <driver/uvc_camera.h>

#if (CONFIG_SYS_CPU1) || (CONFIG_SYS_CPU0)
#include "media_app.h"
#include "media_evt.h"
#endif

#define TAG "uvc"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);

bk_uvc_config_t uvc_config_info_param = {0};
sw_jpeg_dec_res_t result;

#if (CONFIG_SYS_CPU0)
media_camera_device_t camera_device = {
	.type = UVC_CAMERA,
	.mode = JPEG_MODE,
	.fmt = PIXEL_FMT_JPEG,
	.info.fps = FPS25,
	.info.resolution.width = 800,
	.info.resolution.height = 480,
};

static void media_checkout_uvc_device_info(bk_uvc_device_brief_info_t *info, uvc_state_t state)
{
	if (state == UVC_CONNECTED)
	{
		uint8_t format_index = 0;
		uint8_t frame_num = 0;
		uint8_t index = 0, i = 0;
		uvc_config_info_param.vendor_id = info->vendor_id;
		uvc_config_info_param.product_id = info->product_id;
		LOGI("%s uvc_get_param VID:0x%x\r\n",__func__, info->vendor_id);
		LOGI("%s uvc_get_param PID:0x%x\r\n",__func__, info->product_id);

		format_index = info->format_index.mjpeg_format_index;
		frame_num = info->all_frame.mjpeg_frame_num;
		if(format_index > 0){
			LOGI("%s uvc_get_param MJPEG format_index:%d\r\n",__func__, format_index);
			for(index = 0; index < frame_num; index++)
			{
				LOGI("uvc_get_param MJPEG width:%d heigth:%d index:%d\r\n",
							info->all_frame.mjpeg_frame[index].width,
							info->all_frame.mjpeg_frame[index].height,
							info->all_frame.mjpeg_frame[index].index);
				for(i = 0; i < info->all_frame.mjpeg_frame[index].fps_num; i++)
				{
					LOGI("uvc_get_param MJPEG fps:%d\r\n", info->all_frame.mjpeg_frame[index].fps[i]);
				}

				if (info->all_frame.mjpeg_frame[index].width == camera_device.info.resolution.width && info->all_frame.mjpeg_frame[index].height == camera_device.info.resolution.height)
				{
					uvc_config_info_param.frame_index = info->all_frame.mjpeg_frame[index].index;
					uvc_config_info_param.fps = info->all_frame.mjpeg_frame[index].fps[0];
					uvc_config_info_param.width = camera_device.info.resolution.width;
					uvc_config_info_param.height = camera_device.info.resolution.height;
				}
			}
		}

		uvc_config_info_param.format_index = format_index;

		if (media_app_set_uvc_device_param(&uvc_config_info_param) != BK_OK)
		{
			LOGE("%s, failed\r\n, __func__");
		}
	}
	else
	{
		LOGI("%s, %d\r\n", __func__, state);
	}
}
static void media_read_frame_info_callback(frame_buffer_t *frame)
{
	os_printf("##MJPEG:camera_type:%d(1:dvp 2:uvc) frame_id:%d, length:%d, frame_addr:%p \r\n", frame->type,frame->sequence,
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

void uvc_debug_init (void)
{
	bk_err_t ret;
	bk_jpeg_dec_sw_init(NULL, 0);

	ret =media_app_uvc_register_info_notify_cb(media_checkout_uvc_device_info);
	if (ret != BK_OK)
	{
		os_printf("media_app_uvc_register_info_notify_cb failed\r\n");
	}

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
	uvc_debug_init();
#endif
	return 0;
}

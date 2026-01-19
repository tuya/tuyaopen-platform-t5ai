#include <common/bk_include.h>
#include <components/log.h>
#include "bk_peripheral.h"
#include "media_app.h"
#include "audio_osi_wrapper.h"
#include "video_osi_wrapper.h"
#include "media_cli.h"
#include "frame_buffer.h"
#include "lcd_display_service.h"
#include "uvc_pipeline_act.h"
#include <driver/timer.h>
#include <driver/pm_ap_core.h>

#define TAG "media_sev"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

beken_timer_t media_debug_timer = {0};

media_debug_t *media_debug = NULL;
media_debug_t *media_debug_cached = NULL;
#define DEBUG_INTERVAL (2)

static void media_debug_dump(timer_id_t timer_id)
{
	uint16_t h264 = (media_debug->isr_h264 - media_debug_cached->isr_h264) / DEBUG_INTERVAL;
	uint16_t dec = (media_debug->isr_decoder - media_debug_cached->isr_decoder) / DEBUG_INTERVAL;
	uint16_t lcd = (media_debug->isr_lcd - media_debug_cached->isr_lcd) / DEBUG_INTERVAL;
	uint16_t fps_lcd = (media_debug->fps_lcd - media_debug_cached->fps_lcd) / DEBUG_INTERVAL;
	uint16_t fps_wifi = (media_debug->fps_wifi - media_debug_cached->fps_wifi) / DEBUG_INTERVAL;
	uint32_t jpeg_kps = (media_debug->jpeg_kbps - media_debug_cached->jpeg_kbps) * 8 / DEBUG_INTERVAL / 1024;
	uint32_t h264_kps = (media_debug->h264_kbps - media_debug_cached->h264_kbps) * 8 / DEBUG_INTERVAL / 1024;
	uint32_t wifi_kps = (media_debug->wifi_kbps - media_debug_cached->wifi_kbps) * 8 / DEBUG_INTERVAL / 1024;
	uint32_t meantimes = (media_debug->meantimes - media_debug_cached->meantimes) / DEBUG_INTERVAL / 1000;

	if (h264 == 0 && lcd == 0 && fps_wifi == 0)
	{
		return;
	}

	if (fps_wifi == 0 || meantimes == 0)
	{
		if (media_debug->begin_trs == false)
			meantimes = 0;
		else
			meantimes = 1000;
	}
	uint16_t lvgl = (media_debug->lvgl_draw - media_debug_cached->lvgl_draw) / DEBUG_INTERVAL;

	media_debug_cached->isr_h264 = media_debug->isr_h264;
	media_debug_cached->isr_decoder = media_debug->isr_decoder;
	media_debug_cached->isr_lcd = media_debug->isr_lcd;
	media_debug_cached->fps_lcd = media_debug->fps_lcd;
	media_debug_cached->fps_wifi = media_debug->fps_wifi;
	media_debug_cached->jpeg_kbps = media_debug->jpeg_kbps;
	media_debug_cached->h264_kbps = media_debug->h264_kbps;
	media_debug_cached->wifi_kbps = media_debug->wifi_kbps;
	media_debug_cached->meantimes = media_debug->meantimes;
	media_debug_cached->lvgl_draw = media_debug->lvgl_draw;

	LOGD("h264:%d[%d], dec:%d[%d], lcd:%d[%d], lcd_fps:%d[%d], lvgl:%d[%d]\n",
			h264, media_debug->isr_h264,
			dec, media_debug->isr_decoder,
			lcd, media_debug->isr_lcd,
			fps_lcd, media_debug->fps_lcd,
			lvgl, media_debug->lvgl_draw);

	LOGD("wifi:%d[%d, %dkbps, %dms, %d-%d], jpg:%dKB[%dKbps], h264:%dKB[%dKbps]\n",
			fps_wifi, media_debug->fps_wifi, wifi_kps, meantimes, media_debug->begin_trs, media_debug->end_trs,
			media_debug->jpeg_length / 1024, jpeg_kps,
			media_debug->h264_length / 1024, h264_kps);
}

static bk_err_t media_frame_buffer_list_init(uint32_t param1, uint32_t param2)
{
    return frame_buffer_list_init();
}

static bk_err_t media_frame_buffer_list_deinit(uint32_t param1, uint32_t param2)
{
    return frame_buffer_list_deinit();
}

int media_service_init(void)
{
	bk_err_t ret = BK_OK;

#if (CONFIG_MEDIA)
	bk_peripheral_init();
#endif

	ret = bk_video_osi_funcs_init();

	if (ret != kNoErr)
	{
		LOGE("%s, bk_video_osi_funcs_init failed\n", __func__);
		return ret;
	}
	ret = bk_audio_osi_funcs_init();

	if (ret != kNoErr)
	{
		LOGE("%s, bk_audio_osi_funcs_init failed\n", __func__);
		return ret;
	}
//	media_cli_init();

#if (CONFIG_AUD_INTF_TEST)
	extern int cli_aud_intf_init(void);
	cli_aud_intf_init();
#endif

#if (CONFIG_USB_CDC)
	//extern bk_err_t bk_cdc_acm_demo(void);
	//bk_cdc_acm_demo();
#endif
	//frame_buffer_list_init();

    pm_ap_psram_power_state_callback_info_t  power_state_cb = {0};

    power_state_cb.dev_id = PM_POWER_PSRAM_MODULE_NAME_MEDIA;

    power_state_cb.psram_off_cb_fn = media_frame_buffer_list_deinit;

    power_state_cb.psram_on_cb_fn = media_frame_buffer_list_init;

    power_state_cb.param1 = 0;

    power_state_cb.param2 = 0;

    bk_pm_ap_psram_power_state_register_callback(&power_state_cb);

#ifdef CONFIG_LCD
    lcd_display_service_init();
#endif

    media_app_init();

    if (media_debug == NULL)
    {
        media_debug = (media_debug_t *)os_malloc(sizeof(media_debug_t));

        if (media_debug == NULL)
        {
            LOGE("malloc media_debug fail\n");
        }
    }
    
    if (media_debug_cached == NULL)
    {
        media_debug_cached = (media_debug_t *)os_malloc(sizeof(media_debug_t));
        if (media_debug_cached == NULL)
        {
            LOGE("malloc media_debug_cached fail\n");
        }
    }

    os_memset(media_debug, 0, sizeof(media_debug_t));
	os_memset(media_debug_cached, 0, sizeof(media_debug_t));

	rtos_init_timer(&media_debug_timer, DEBUG_INTERVAL * 1000, (timer_handler_t)media_debug_dump, NULL);
	rtos_start_timer(&media_debug_timer);

	return 0;
}

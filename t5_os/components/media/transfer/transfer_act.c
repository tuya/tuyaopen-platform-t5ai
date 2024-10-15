// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <components/log.h>
#include <components/video_transfer.h>

#include "media_core.h"
#include <driver/int.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_map.h"
#include "gpio_driver.h"

#include <driver/media_types.h>

#include <soc/mapping.h>

#include "transfer_act.h"
#include "media_evt.h"

#include "frame_buffer.h"

#include "bk_wifi.h"

#include "media_app.h"

#if CONFIG_ARCH_RISCV
#include "cache.h"
#endif

#if CONFIG_VIDEO_AVI
#include "app_jpeg2avi.h"
#include "diskio.h"
#endif

#define TAG "transfer"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

//#define DVP_DIAG_DEBUG

#ifdef TRANSFER_DIAG_DEBUG

#define TRANSFER_DIAG_DEBUG_INIT()                  \
	do {                                            \
		gpio_dev_unmap(GPIO_0);                     \
		bk_gpio_disable_pull(GPIO_0);               \
		bk_gpio_enable_output(GPIO_0);              \
		bk_gpio_set_output_low(GPIO_0);             \
		\
		gpio_dev_unmap(GPIO_1);                     \
		bk_gpio_disable_pull(GPIO_1);               \
		bk_gpio_enable_output(GPIO_1);              \
		bk_gpio_set_output_low(GPIO_1);             \
		\
	} while (0)

#define WIFI_TRANSFER_START()                   bk_gpio_set_output_high(GPIO_0)
#define WIFI_TRANSFER_END()                     bk_gpio_set_output_low(GPIO_0)

#define WIFI_DMA_START()                    bk_gpio_set_output_high(GPIO_1)
#define WIFI_DMA_END()                      bk_gpio_set_output_low(GPIO_1)

#else

#define TRANSFER_DIAG_DEBUG_INIT()

#define WIFI_TRANSFER_START()
#define WIFI_TRANSFER_END()

#define WIFI_DMA_START()
#define WIFI_DMA_END()

#endif


typedef struct
{
	uint8_t type;
	uint32_t data;
} trs_task_msg_t;

typedef enum
{
	TRS_TRANSFER_DATA,
	TRS_TRANSFER_EXIT,
} trs_task_msg_type_t;

transfer_info_t transfer_info;

extern media_debug_t *media_debug;

frame_buffer_t *wifi_tranfer_frame = NULL;

beken_semaphore_t transfer_sem;
bool transfer_task_running = false;
static beken_thread_t transfer_task = NULL;
transfer_data_t *transfer_data = NULL;
frame_cb_t frame_read_callback = NULL;

extern u64 riscv_get_mtimer(void);

#if CONFIG_VIDEO_AVI
#define FULL_FILENAME_PAHT_LEN     32
char *filename_get(void)
{
	static char file_name[33] = {0};

	char date[16] = {0};
	extern int get_curtime_str(char *buf,uint8_t opera);
	get_curtime_str(date,0);

	os_memset(file_name,0,sizeof(file_name));
	snprintf(file_name, FULL_FILENAME_PAHT_LEN,"%d:/%s.avi", DISK_NUMBER_SDIO_SD, date);
	LOGI("file_name:%s\r\n",file_name);

	return file_name;
}

static volatile bool g_avi_enable = false;
static volatile uint32_t picture_cnt = 0;
static void avi_open(param_pak_t *param)
{
	if(g_avi_enable)
	{
		LOGI("avi already open\n");
		goto exit;
	}

	LOGI("%s\n", __func__);

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

	g_avi_enable = true;
	picture_cnt = 0;

	GLOBAL_INT_RESTORE();

	jpeg2avi_init();

	jpeg2avi_set_video_param(VIDEO_FRAME_WIDTH, VIDEO_FRAME_HEIGHT, VIDEO_FPS);
	jpeg2avi_set_audio_param(AUDIO_CHANEL_NUM, AUDIO_SAMPLE_RATE, AUDIO_SAMPLE_BITS);

exit:
	if(param)
	{
		MEDIA_EVT_RETURN(param, BK_OK);
	}
}

static void avi_close(param_pak_t *param)
{
	if(!g_avi_enable)
	{
		LOGI("avi already close\n");
		goto exit;
	}

	LOGI("%\n", __func__);

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

	g_avi_enable = false;
	picture_cnt = 0;

	GLOBAL_INT_RESTORE();

	jpeg2avi_stop_record();
	jpeg2avi_deinit();

exit:
	if(param)
	{
		MEDIA_EVT_RETURN(param, BK_OK);
	}

	LOGI("%\n", __func__);
}

static bool avi_is_enable(void)
{
	return g_avi_enable;
}
#endif

static void transfer_task_entry(beken_thread_arg_t data)
{
	frame_buffer_t *encode_frame = NULL;
	uint64 before, after;

	rtos_set_semaphore(&transfer_sem);

	while (transfer_task_running)
	{
		encode_frame = frame_buffer_fb_read(MODULE_WIFI);

		if (encode_frame == NULL)
		{
			LOGE("read jpeg frame NULL\n");
			continue;
		}

#if (CONFIG_ARCH_RISCV)
		before = riscv_get_mtimer();
#else
		before = 0;
#endif

#if CONFIG_VIDEO_AVI
		if(avi_is_enable())
		{
			if(0 == picture_cnt)
			{
				LOGI("jpeg2avi_start_record\r\n");
				jpeg2avi_start_record(filename_get());
			}

			jpeg2avi_input_data(encode_frame->frame,encode_frame->length, eTypeVideo);

			picture_cnt++;
			if(0 == picture_cnt % AVI_VIDEO_FRAMES_MAX)
			{
				picture_cnt = 0;
				jpeg2avi_stop_record();
				LOGI("jpeg2avi_stop_record\r\n");
			}
		}
#endif

		if (frame_read_callback)
		{
			frame_read_callback(encode_frame);
		}

		frame_buffer_fb_free(encode_frame, MODULE_WIFI);

#if (CONFIG_ARCH_RISCV)
		after = riscv_get_mtimer();
#else
		after = 0;
#endif
		LOGD("transfer time: %lu\n", (after - before) / 26000);
	}

	LOGI("transfer task exit\n");

#if CONFIG_VIDEO_AVI
	avi_close(NULL);
#endif

	transfer_task = NULL;
	rtos_set_semaphore(&transfer_sem);
	rtos_delete_thread(NULL);
}

void transfer_task_start(void)
{
	bk_err_t ret;

	TRANSFER_DIAG_DEBUG_INIT();

	if (transfer_task != NULL)
	{
		LOGE("%s transfer_task already running\n", __func__);
		return;
	}

	frame_buffer_fb_register(MODULE_WIFI, FB_INDEX_JPEG);

	ret = rtos_init_semaphore_ex(&transfer_sem, 1, 0);

	if (BK_OK != ret)
	{
		LOGE("%s semaphore init failed\n", __func__);
		return;
	}

	transfer_task_running = true;

	ret = rtos_create_thread(&transfer_task,
	                         4,
	                         "transfer_task",
	                         (beken_thread_function_t)transfer_task_entry,
	                         CONFIG_TRANSFER_TASK_STACK_SIZE,
	                         NULL);

	if (BK_OK != ret)
	{
		LOGE("%s transfer_task init failed\n");
		return;
	}

	ret = rtos_get_semaphore(&transfer_sem, BEKEN_NEVER_TIMEOUT);

	if (BK_OK != ret)
	{
		LOGE("%s transfer_sem get failed\n", __func__);
	}

	LOGI("%s complete\n", __func__);
}



void transfer_task_stop(void)
{
	bk_err_t ret;

	transfer_task_running = false;

	frame_buffer_fb_deregister(MODULE_WIFI);

	ret = rtos_get_semaphore(&transfer_sem, BEKEN_NEVER_TIMEOUT);

	if (BK_OK != ret)
	{
		LOGE("%s transfer get failed\n");
	}

	LOGI("%s complete\n", __func__);

	ret = rtos_deinit_semaphore(&transfer_sem);

	if (BK_OK != ret)
	{
		LOGE("%s transfer deinit failed\n");
	}
}

void transfer_open_handle(param_pak_t *param)
{
	if (get_transfer_state() == TRS_STATE_ENABLED)
	{
		return;
	}

	frame_read_callback = (frame_cb_t)param->param;

	LOGI("%s cb: %p ++\n", __func__, frame_read_callback);

	transfer_task_start();

	set_transfer_state(TRS_STATE_ENABLED);

	MEDIA_EVT_RETURN(param, BK_OK);

	LOGI("%s --\n", __func__);
}

void transfer_close_handle(param_pak_t *param)
{
	LOGI("%s\n", __func__);

	wifi_tranfer_frame = NULL;

	transfer_task_stop();
	set_transfer_state(TRS_STATE_DISABLED);

	MEDIA_EVT_RETURN(param, BK_OK);
}

void transfer_pause_handle(param_pak_t *param)
{
	LOGI("%s, %d\n", __func__, param->param);

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

	transfer_info.pause = param->param;

	GLOBAL_INT_RESTORE();

	MEDIA_EVT_RETURN(param, BK_OK);
}


void transfer_event_handle(uint32_t event, uint32_t param)
{
	switch (event)
	{
		case EVENT_TRANSFER_OPEN_IND:
			transfer_open_handle((param_pak_t *)param);
			break;
		case EVENT_TRANSFER_CLOSE_IND:
			transfer_close_handle((param_pak_t *)param);
			break;
		case EVENT_TRANSFER_PAUSE_IND:
			transfer_pause_handle((param_pak_t *)param);
			break;

#if CONFIG_VIDEO_AVI
		case EVENT_AVI_OPEN_IND:
			avi_open((param_pak_t *)param);
			break;
		case EVENT_AVI_CLOSE_IND:
			avi_close((param_pak_t *)param);
			break;
#endif
		default:
			break;
	}
}

media_trs_state_t get_transfer_state(void)
{
	return transfer_info.state;
}

void set_transfer_state(media_trs_state_t state)
{
	transfer_info.state = state;
}

void transfer_init(void)
{
	transfer_info.state = TRS_STATE_DISABLED;
	transfer_info.debug = false;
	transfer_info.pause = false;
}

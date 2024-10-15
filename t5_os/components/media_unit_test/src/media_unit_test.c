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
#include "cli.h"
#include "media_app.h"

#include <driver/dvp_camera.h>
#include <driver/jpeg_enc.h>
#include "lcd_act.h"
#include "storage_act.h"
#include <driver/audio_ring_buff.h>
#include "aud_intf.h"
#include <driver/uvc_camera.h>
#include <driver/lcd.h>

#include <driver/media_types.h>
#include <lcd_decode.h>
#include <lcd_rotate.h>


/* Test includes. */
#include "unity_fixture.h"
#include "unity.h"

#include "bk_cli.h"
#include "media_evt.h"
#include "media_utils.h"

#include "media_unit_test.h"
#include "media_unit_case.h"

#define TAG "MUT"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define DEFAULT_TEST_TIME	(10)
#define DEFAULT_TEST_DELAY	(5000)


#if 0
TEST_GROUP(TEST_IOT_MEDIA_UT);

TEST_SETUP(TEST_IOT_MEDIA_UT)
{
	//TODO
}

TEST_TEAR_DOWN(TEST_IOT_MEDIA_UT)
{
	//TODO
}
#endif

typedef struct {
	const char *name;
	int (*cb)(int argc, char **argv, int delay, int times);
} media_ut_t;

typedef union {
	uint32_t value;
	char *string;
} media_ut_param_t;


beken_semaphore_t wait_sem;

media_unit_test_config_t *media_unit_test_config = NULL;



const media_ut_t media_ut[] = {
	{"lcd on off", lcd_on_off_test},
	{"h264 on off", h264_on_off_test},
	{"pipeline on off", pipeline_on_off_test},
	{"read frame on off", read_frame_on_off_test},
	{"audio on off", audio_on_off_test},
};

uint8_t media_ut_exit_flag = 0;

void set_media_ut_exit(uint8_t flag)
{
	media_ut_exit_flag = flag;
}

uint8_t get_media_ut_exit(void)
{
	return media_ut_exit_flag;
}

char *media_ut_test_getopt(int argc, char **argv, char *opt)
{
	char *ret = NULL;

	for (int i = 0; i < argc; i++)
	{
		if (os_strcmp(argv[i], opt) == 0
			&& i < argc - 1)
		{
			ret = argv[i + 1];
		}
	}

	return ret;
}


void media_ut_test_help(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	LOGI("%s help: \n", __func__);

	for (int i = 0; i < sizeof(media_ut) / sizeof(media_ut_t); i++)
	{
		LOGI("CASE[%d]: %s\n", i, media_ut[i].name);
	}
}

bk_err_t media_unit_test_task_send_msg(uint8_t type, uint32_t param)
{
	bk_err_t ret = BK_FAIL;
	media_msg_t msg;

	if (media_unit_test_config)
	{
		msg.event = type;
		msg.param = param;
		ret = rtos_push_to_queue(&media_unit_test_config->media_unit_test_queue, &msg, BEKEN_WAIT_FOREVER);
	}

	if (BK_OK != ret)
	{
		LOGE("%s failed, type:%d\r\n", __func__, type);
	}

	return ret;
}


void media_ut_test_run(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret;
	int id = -1;
	int delay = DEFAULT_TEST_DELAY;
	int times = DEFAULT_TEST_TIME;

	if (argc > 3)
	{
		delay = os_strtoul(argv[3], NULL, 10) & 0xFFFFFFFF;
	}

	if (argc > 4)
	{
		times = os_strtoul(argv[4], NULL, 10) & 0xFFFFFFFF;
	}

	if (os_strcmp(argv[2], "all") == 0)
	{
		id = -1;
	}
	else
	{
		id = os_strtoul(argv[2], NULL, 10) & 0xFFFFFFFF;
	}

	media_unit_test_config->id = id;
	media_unit_test_config->delay = delay;
	media_unit_test_config->times = times;

	set_media_ut_exit(0);

	ret = media_unit_test_task_send_msg(MEDIA_UT_START, 0);
	if (BK_OK != ret)
	{
		LOGE("%s failed\r\n", __func__);
	}

}

static void media_unit_test_entry(beken_thread_arg_t data)
{
	int ret = BK_OK;

	rtos_set_semaphore(&media_unit_test_config->media_unit_test_sem);

	while (1)
	{
		media_msg_t msg;
		ret = rtos_pop_from_queue(&media_unit_test_config->media_unit_test_queue, &msg, BEKEN_WAIT_FOREVER);
		if (ret == BK_OK)
		{
			switch (msg.event)
			{
				case MEDIA_UT_START:
				{
					if (media_unit_test_config->id == -1)
					{
						for (int i = 0; i < sizeof(media_ut) / sizeof(media_ut_t); i++)
						{
							LOGI("run case[%d]: %s, delay: %d, times: %d\n", i, media_ut[i].name,
								media_unit_test_config->delay, media_unit_test_config->times);
							ret = media_ut[i].cb(0, NULL, media_unit_test_config->delay, media_unit_test_config->times);
							if (ret != BK_OK)
								LOGI("run case[%d]: %s failed, return %x\n", i, media_ut[i].name, ret);
						}
					
}
					else
					{
						LOGI("run case[%d]: %s, delay: %d, times: %d\n",
							media_unit_test_config->id,
							media_ut[media_unit_test_config->id].name,
							media_unit_test_config->delay,
							media_unit_test_config->times);
						ret = media_ut[media_unit_test_config->id].cb(0, NULL, media_unit_test_config->delay, media_unit_test_config->times);
						if (ret != BK_OK)
						{
							LOGI("run case[%d]: %s failed, return %x\n",
								media_unit_test_config->id,
								media_ut[media_unit_test_config->id].name, ret);
						}
					}
				}
				break;

				case MEDIA_UT_STOP:
					goto out;
					break;
				default:
					break;
			}
		}
	
}

out:
	if (media_unit_test_config->media_unit_test_sem)
	{
		rtos_deinit_semaphore(&media_unit_test_config->media_unit_test_sem);
		media_unit_test_config->media_unit_test_sem = NULL;
	}

	if (media_unit_test_config->media_unit_test_queue)
	{
		rtos_deinit_queue(&media_unit_test_config->media_unit_test_queue);
		media_unit_test_config->media_unit_test_queue = NULL;
	}
	rtos_delete_thread(NULL);
}

void media_ut_test_set_default_config(void)
{
	int ret =BK_OK;

	if (media_unit_test_config == NULL)
	{
		media_unit_test_config = (media_unit_test_config_t*)os_malloc(sizeof(media_unit_test_config_t));

		if (media_unit_test_config == NULL)
		{
			LOGE("%s malloc media_unit_test_config failed\n", __func__);
			return;
		}

		os_memset(media_unit_test_config, 0 , sizeof(media_unit_test_config_t));

		os_memcpy(media_unit_test_config->lcd_name, "st7701sn", strlen("st7701sn"));
		media_unit_test_config->rotate = ROTATE_90;
		media_unit_test_config->camera_ppi = PPI_864X480;
		media_unit_test_config->camera_type = UVC_CAMERA;
		media_unit_test_config->mic_type = AUD_INTF_MIC_TYPE_BOARD;
		media_unit_test_config->spk_type = AUD_INTF_SPK_TYPE_BOARD;

		if (media_unit_test_config->media_unit_test_sem == NULL)
		{
			ret = rtos_init_semaphore(&media_unit_test_config->media_unit_test_sem, 1);
			if (ret != BK_OK)
			{
				LOGE("%s, media_unit_test_sem init error", __func__);
				goto error;
			}
		}

		ret = rtos_init_queue(&media_unit_test_config->media_unit_test_queue,
							"media_ut_queue",
							sizeof(media_msg_t),
							5);

		if (ret != BK_OK)
		{
			LOGE("%s, init rot_queue failed\r\n", __func__);
			goto error;
		}

		ret = rtos_create_thread(&media_unit_test_config->media_unit_test_thread,
							BEKEN_DEFAULT_WORKER_PRIORITY,
							"media_ut_thread",
							(beken_thread_function_t)media_unit_test_entry,
							1024 * 4,
							NULL);

		if (ret != BK_OK)
		{
			LOGE("%s, create media_ut_thread failed\r\n", __func__);
			goto error;
		}
		rtos_get_semaphore(&media_unit_test_config->media_unit_test_sem, BEKEN_NEVER_TIMEOUT);

	}
	return;
error:
	if (media_unit_test_config->media_unit_test_sem)
	{
		rtos_deinit_semaphore(&media_unit_test_config->media_unit_test_sem);
		media_unit_test_config->media_unit_test_sem = NULL;
	}
	if (media_unit_test_config->media_unit_test_queue)
	{
		rtos_deinit_queue(&media_unit_test_config->media_unit_test_queue);
		media_unit_test_config->media_unit_test_queue = NULL;
	}
}

void media_ut_test_set(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *value = NULL;

	media_ut_test_set_default_config();

	value = media_ut_test_getopt(argc, argv, "-uvc");

	if (value)
	{
		media_unit_test_config->camera_ppi = get_string_to_ppi(value);
		media_unit_test_config->camera_type = UVC_CAMERA;
		LOGI("uvc ppi: %s, %dX%d\n", value,
			media_unit_test_config->camera_ppi >> 16,
			media_unit_test_config->camera_ppi & 0xFFFF);
	}

	value = media_ut_test_getopt(argc, argv, "-lcd");

	if (value)
	{
		char *name = get_string_to_lcd_name(value);

		if (name)
		{
			os_memset(media_unit_test_config->lcd_name, 0, sizeof(media_unit_test_config->lcd_name));
			os_memcpy(media_unit_test_config->lcd_name, name,
				strlen(name) < sizeof(media_unit_test_config->lcd_name) ? strlen(name) : sizeof(media_unit_test_config->lcd_name));
		}

		LOGI("lcd name: %s, %s\n", value, media_unit_test_config->lcd_name);

	}

	value = media_ut_test_getopt(argc, argv, "-rotate");

	if (value)
	{
		media_unit_test_config->rotate = get_string_to_angle(value);
		LOGI("rotate: %s, %d\n", value, media_unit_test_config->rotate);
	}

	value = media_ut_test_getopt(argc, argv, "-audio");

	if (value)
	{
		if (os_strcmp(value, "onboard") == 0)
		{
			media_unit_test_config->mic_type = AUD_INTF_MIC_TYPE_BOARD;
			media_unit_test_config->spk_type = AUD_INTF_SPK_TYPE_BOARD;
			LOGI("mic_type: onboard, spk_type: onboard\n");
		}
		else if (os_strcmp(value, "uac") == 0)
		{
			media_unit_test_config->mic_type = AUD_INTF_MIC_TYPE_UAC;
			media_unit_test_config->spk_type = AUD_INTF_SPK_TYPE_UAC;
			LOGI("mic_type: UAC, spk_type: UAC\n");
		}
		else
		{
			LOGE("not support type, set mic type and spk type onboard\n");
			media_unit_test_config->mic_type = AUD_INTF_MIC_TYPE_BOARD;
			media_unit_test_config->spk_type = AUD_INTF_SPK_TYPE_BOARD;
		}
	}
}


void media_ut_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (os_strcmp(argv[1], "help") == 0)
	{
		media_ut_test_help(pcWriteBuffer, xWriteBufferLen, argc, argv);
	}
	else if (os_strcmp(argv[1], "run") == 0)
	{
		media_ut_test_set_default_config();
		media_ut_test_run(pcWriteBuffer, xWriteBufferLen, argc, argv);
	}
	else if (os_strcmp(argv[1], "set") == 0)
	{
		media_ut_test_set(pcWriteBuffer, xWriteBufferLen, argc, argv);
	}
	else if (os_strcmp(argv[1], "stop") == 0)
	{
		set_media_ut_exit(1);
	}
}

#define MEDIA_UNIT_TEST_CMD_CNT   (sizeof(s_media_unit_test_commands) / sizeof(struct cli_command))

static const struct cli_command s_media_unit_test_commands[] =
{
	{"mut", "meida unit test..", media_ut_test_cmd},
};

int media_unit_test_cli_init(void)
{
	return cli_register_commands(s_media_unit_test_commands, MEDIA_UNIT_TEST_CMD_CNT);
}


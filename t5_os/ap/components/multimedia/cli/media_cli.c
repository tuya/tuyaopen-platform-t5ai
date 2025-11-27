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
#include "media_cli.h"

#include "media_app.h"
#include "camera_handle_list.h"
#include "img_service.h"
#include "uvc_pipeline_act.h"
#include "lcd_display_service.h"
#include "media_utils.h"

#define TAG "mcli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define UNKNOW_ERROR (-686)
#define PARAMS_ERROR (-687)

#define CMD_CONTAIN(value) cmd_contain(argc, argv, value)
#define GET_PPI(value)     get_ppi_from_cmd(argc, argv, value)
#define GET_NAME(value)    get_name_from_cmd(argc, argv, value)
#define GET_ROTATE()       get_rotate_from_cmd(argc, argv)
#define GET_H26X_PPI()     get_h26x_ppi_from_cmd(argc, argv)

uint32_t get_ppi_from_cmd(int argc, char **argv, uint32_t pre)
{
	int i;
	uint32_t value = pre;

	for (i = 0; i < argc; i++)
	{
		value = get_string_to_ppi(argv[i]);

		if (value != PPI_DEFAULT)
		{
			break;
		}
	}

	if (value == PPI_DEFAULT)
	{
		value = pre;
	}

	LOGV("%s %d-%d+++\n", __func__, value >> 16, value & 0xFFFF);

	return value;
}

uint32_t get_h26x_ppi_from_cmd(int argc, char **argv)
{
	int i;
	uint32_t value = PPI_DEFAULT;

	for (i = 5; i < argc; i++)
	{
		value = get_string_to_ppi(argv[i]);

		if (value != PPI_DEFAULT)
		{
			break;
		}
	}
	LOGV("%s %d-%d+++\n", __func__, value >> 16, value & 0xFFFF);
	return value;
}


char * get_name_from_cmd(int argc, char **argv, char * pre)
{
	int i;
	char* value = pre;

	for (i = 3; i < argc; i++)
	{
		value = get_string_to_lcd_name(argv[i]);

		if (value != NULL)
		{
			break;
		}
	}

	return value;
}

media_rotate_t get_rotate_from_cmd(int argc, char **argv)
{
	int i;
	media_rotate_t value = ROTATE_90;

	for (i = 3; i < argc; i++)
	{
		value = get_string_to_angle(argv[i]);
	}

	return value;
}

bool cmd_contain(int argc, char **argv, char *string)
{
	int i;
	bool ret = false;

	for (i = 0; i < argc; i++)
	{
		if (os_strcmp(argv[i], string) == 0)
		{
			ret = true;
		}
	}

	return ret;
}

static void media_cli_frame_buffer_out(frame_buffer_t *frame)
{
    if (frame->sequence % 20 == 0)
        LOGI("%s, sequence:%d, length:%d\n", __func__, frame->sequence, frame->length);
}

int open_camera_display(int camera_port, image_format_t fmt)  // uvc 1/ uvc 2/ dvp 3
{
    int ret = BK_FAIL;
    media_camera_device_t device = {0};
    media_ppi_t ppi;
    camera_handle_t handle = NULL;

    frame_list_node_t node;
    ret = media_app_get_main_camera_stream(&node);
    if (ret == BK_OK)
    {
        if(node.camera_id == 8)
        {
            node.camera_id = 0;
        }
        LOGD("%s opened camera id:%x,  switch to id :%x\n", __func__, node.camera_id, camera_port);
        if(node.camera_id == camera_port)
        {
            LOGD("%s open repetition, opened:%x,  switch:%x\n", __func__, node.camera_id, camera_port);
            return BK_OK;
        }
    }

    handle = bk_camera_handle_node_get_by_id_and_fomat(1, IMAGE_MJPEG);
    if (handle != NULL)
    {
        LOGD("%s media_app_get_camera_handle_by_id 1\n", __func__);
        ret = media_app_camera_close(&handle);
    }

    handle = NULL;
    handle = bk_camera_handle_node_get_by_id_and_fomat(2, IMAGE_MJPEG);
    if (handle != NULL)
    {
        LOGD("%s media_app_get_camera_handle_by_id 2\n", __func__);
        ret = media_app_camera_close(&handle);
    }

    ret = media_app_pipeline_h264_close();
    ret = media_app_jdec_close();

    handle = NULL;
    handle = bk_camera_handle_node_get_by_id_and_fomat(0, IMAGE_YUV | IMAGE_MJPEG);
    if (handle != NULL)
    {
        LOGD("%s media_app_get_camera_handle_by_id 0\n", __func__);
        ret = media_app_camera_close(&handle);
    }

    os_memset(&device, 0, sizeof(media_camera_device_t));

    if ((camera_port == 1) || (camera_port == 2))
    {
        ppi = PPI_864X480;
        device.type = UVC_CAMERA;
        device.width = ppi >> 16;
        device.height = ppi & 0xFFFF;
        device.fps = FPS25;
        device.format = IMAGE_MJPEG;
        device.port = camera_port;
        handle = NULL;
        ret = media_app_camera_open(&handle, &device);

        if (fmt == IMAGE_H264)
        {
            media_app_set_rotate(ROTATE_90);
            ret = media_app_pipeline_h264_open(NULL);
        }
        ret = media_app_jdec_open(JPEGDEC_BY_LINE);
    }
    else if (camera_port == 0)
    {
        ppi = PPI_640X480;
        device.type = DVP_CAMERA;
        device.width = ppi >> 16;
        device.height = ppi & 0xFFFF;
        device.fps = FPS25;
        if (fmt == IMAGE_H264)
        {
            device.format = IMAGE_YUV | IMAGE_H264;
        }
        else
        {
            device.format = IMAGE_YUV | IMAGE_MJPEG;
        }
        device.port = 0;
        handle = NULL;
        ret = media_app_camera_open(&handle, &device);
        media_app_set_rotate(ROTATE_90);
        ret = media_app_jdec_open(JPEGDEC_BY_FRAME);
    }
    else
    {
        LOGD("%s not support camera id %d\n", __func__, camera_port);
    }

    return ret;
}

void media_cli_display_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = UNKNOW_ERROR;
    char *msg = NULL;
    image_format_t fmt = IMAGE_MJPEG;
    static uint32_t cnt = 0;
    if (os_strcmp(argv[1], "switch") == 0)
    {
        if (!cnt)
        {
            cnt = 1;
            lcd_open_t lcd_open;
//            lcd_open.device_ppi = PPI_864X480;
//            lcd_open.device_name = "st7701sn";

            lcd_open.device_ppi = PPI_480X480;
            lcd_open.device_name = "st7701s";
            media_app_lcd_disp_open(&lcd_open);
            media_app_set_rotate(ROTATE_90);
        }
        uint8_t port = os_strtoul(argv[2], NULL, 10) & 0xFF;
        if (CMD_CONTAIN("h264"))
        {
            fmt = IMAGE_H264;
        }
        else
        {
            fmt = IMAGE_MJPEG;
        }

        ret = open_camera_display(port, fmt);
    }

    if (ret == UNKNOW_ERROR)
    {
        LOGE("%s unknow cmd\n", __func__);
    }

    if (ret == PARAMS_ERROR)
    {
        LOGE("%s param error cmd\n", __func__);
    }

    if (ret != BK_OK)
    {
        msg = CLI_CMD_RSP_ERROR;
    }
    else
    {
        msg = CLI_CMD_RSP_SUCCEED;
    }

    LOGI("%s ---complete\n", __func__);

    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

void media_cli_camera_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = UNKNOW_ERROR;
    char *msg = NULL;
    static camera_handle_t handle = NULL;
    media_ppi_t ppi = GET_PPI(PPI_640X480);
    media_camera_device_t device = DEFAULT_CAMERA_CONFIG();
    device.type = UVC_CAMERA;
    device.width = ppi >> 16;
    device.height = ppi & 0xFFFF;
    device.fps = FPS30;

    if (CMD_CONTAIN("jpeg"))
    {
        device.format = IMAGE_MJPEG;
    }

    if (CMD_CONTAIN("h264"))
    {
        device.format = IMAGE_H264;
    }

    if (CMD_CONTAIN("h265"))
    {
        device.format = IMAGE_H265;
    }

    if (CMD_CONTAIN("enc_yuv"))
    {
        device.format |= IMAGE_YUV;
    }

    if (CMD_CONTAIN("yuv"))
    {
        device.format = IMAGE_YUV;
    }

    if (CMD_CONTAIN("dual"))
    {
        device.format = IMAGE_MJPEG | IMAGE_H264;
    }

    if (os_strcmp(argv[1], "uvc") == 0)
    {
        if (os_strcmp(argv[2], "open") == 0)
        {
            device.port = os_strtoul(argv[3], NULL, 10);
            //media_app_register_uvc_connect_state_cb(uvc_connect_state_callback);
            ret = media_app_camera_open(&handle, &device);
        }
        else if (os_strcmp(argv[2], "close") == 0)
        {
            uint8_t port = os_strtoul(argv[3], NULL, 10);

            do {
                handle = bk_camera_handle_node_get_by_id_and_fomat(port, IMAGE_MJPEG);
                if (handle)
                {
                    break;
                }

                handle = bk_camera_handle_node_get_by_id_and_fomat(port, IMAGE_H264);
                if (handle)
                {
                    break;
                }

                handle = bk_camera_handle_node_get_by_id_and_fomat(port, IMAGE_H265);
                if (handle != NULL)
                {
                    ret = media_app_camera_close(&handle);
                }

                handle = bk_camera_handle_node_get_by_id_and_fomat(port, IMAGE_MJPEG | IMAGE_H264);
                if (handle != NULL)
                {
                    ret = media_app_camera_close(&handle);
                }
            } while (0);

            if (handle != NULL)
            {
                ret = media_app_camera_close(&handle);
            }
            else
            {
                LOGE("%s, %d handle is null\n", __func__, __LINE__);
            }
        }
        else
        {
            LOGE("%s, %d, not found this cmd!\n", __func__, __LINE__);
        }
    }
    else if (os_strcmp(argv[1], "dvp") == 0)
    {
        device.type = DVP_CAMERA;
        if (os_strcmp(argv[2], "open") == 0)
        {
            ret = media_app_camera_open(&handle, &device);
        }
        else if (os_strcmp(argv[2], "close") == 0)
        {
            ret = media_app_camera_close(&handle);
        }
        else
        {
            LOGE("%s, %d, not found this cmd!\n", __func__, __LINE__);
        }
    }
    else if (os_strcmp(argv[1], "pipeline") == 0)
    {
        if (os_strcmp(argv[2], "h264_open") == 0)
        {
            ret = media_app_pipeline_h264_open(NULL);
        }
        else if (os_strcmp(argv[2], "h264_close") == 0)
        {
            ret = media_app_pipeline_h264_close();
        }
        else
        {
            LOGE("%s, %d, not found this cmd!\n", __func__, __LINE__);
        }
    }
    else if (os_strcmp(argv[1], "read_start") == 0)
    {
        ret = media_app_register_read_frame_callback(device.format, media_cli_frame_buffer_out);
    }
    else if (os_strcmp(argv[1], "read_stop") == 0)
    {
        ret = media_app_unregister_read_frame_callback();
    }
    else
    {
        LOGE("%s, %d, not found this cmd!\n", __func__, __LINE__);
    }

    if (ret == UNKNOW_ERROR)
    {
        LOGE("%s unknow cmd\n", __func__);
    }

    if (ret == PARAMS_ERROR)
    {
        LOGE("%s param error cmd\n", __func__);
    }

    if (ret != BK_OK)
    {
        msg = CLI_CMD_RSP_ERROR;
    }
    else
    {
        msg = CLI_CMD_RSP_SUCCEED;
    }

    LOGI("%s ---complete\n", __func__);

    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

void media_cli_lcd_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = UNKNOW_ERROR;
    char *msg = NULL;
    lcd_open_t lcd_open;
    char *name = "st7792";
    name = GET_NAME(name);
    lcd_open.device_ppi = GET_PPI(PPI_480X272);
    lcd_open.device_name = name;
    media_rotate_t rotate = GET_ROTATE();

    if (os_strcmp(argv[1], "open") == 0)
    {
        //lcd_set_fmt(PIXEL_FMT_RGB888);
        media_app_set_rotate(rotate);
        if (os_strcmp(argv[2], "fb") == 0)
        {
            media_app_jdec_open(JPEGDEC_BY_FRAME);
        }
        else
        {
            media_app_jdec_open(JPEGDEC_BY_LINE);
        }

        ret = media_app_lcd_disp_open(&lcd_open);
    }
    else if (os_strcmp(argv[1], "close") == 0)
    {
        media_app_jdec_close();
        ret = media_app_lcd_disp_close();
    }
    else
    {
        LOGE("%s, %d, not found this cmd!\n", __func__, __LINE__);
    }

    if (ret == UNKNOW_ERROR)
    {
        LOGE("%s unknow cmd\n", __func__);
    }

    if (ret == PARAMS_ERROR)
    {
        LOGE("%s param error cmd\n", __func__);
    }

    if (ret != BK_OK)
    {
        msg = CLI_CMD_RSP_ERROR;
    }
    else
    {
        msg = CLI_CMD_RSP_SUCCEED;
    }

    LOGI("%s ---complete\n", __func__);

    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

void media_cli_storage_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = UNKNOW_ERROR;
    char *msg = NULL;
    uint8_t callback_register = 0;
    image_format_t img_format = IMAGE_MJPEG;

    if (CMD_CONTAIN("cb"))
    {
        callback_register = 1;
    }

    if (CMD_CONTAIN("jpeg"))
    {
        img_format = IMAGE_MJPEG;
    }

    if (CMD_CONTAIN("h264"))
    {
        img_format = IMAGE_H264;
    }

    if (CMD_CONTAIN("h265"))
    {
        img_format = IMAGE_H265;
    }

    if (os_strcmp(argv[1], "open") == 0)
    {
        if (callback_register)
        {
            ret = media_app_storage_open(media_cli_frame_buffer_out);
        }
        else
        {
            ret = media_app_storage_open(NULL);
        }
    }
    else if (os_strcmp(argv[1], "close") == 0)
    {
        ret = media_app_storage_close();
        callback_register = 0;
    }
    else if (os_strcmp(argv[1], "capture") == 0)
    {
        if (argc < 2)
        {
            ret = media_app_capture(img_format, "unknow.jpg");
        }
        else
        {
            ret = media_app_capture(img_format, argv[2]);
        }
    }
    else if (os_strcmp(argv[1], "stream_open") == 0)
    {
        if (argc < 2)
        {
            ret = media_app_save_start(img_format, "unknow.h264");
        }
        else
        {
            ret = media_app_save_start(img_format, argv[2]);
        }
    }
    else if (os_strcmp(argv[1], "stream_close") == 0)
    {
        ret = media_app_save_stop();
    }
    else
    {
        LOGE("%s, %d, not found this cmd!\n", __func__, __LINE__);
    }

    if (ret == UNKNOW_ERROR)
    {
        LOGE("%s unknow cmd\n", __func__);
    }

    if (ret == PARAMS_ERROR)
    {
        LOGE("%s param error cmd\n", __func__);
    }

    if (ret != BK_OK)
    {
        msg = CLI_CMD_RSP_ERROR;
    }
    else
    {
        msg = CLI_CMD_RSP_SUCCEED;
    }

    LOGI("%s ---complete\n", __func__);

    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}


#define MEDIA_CMD_CNT   (sizeof(s_media_commands) / sizeof(struct cli_command))

static const struct cli_command s_media_commands[] =
{
    {"camera_display", "switch 0/1/2 mjpeg/h264", media_cli_display_test_cmd},
    {"media", "dvp/uvc/read open/close ppi port mjpeg/h264", media_cli_camera_test_cmd},
    {"lcd", "open/close fb/line name rotate degree", media_cli_lcd_test_cmd},
    {"storage", "open/close/capture/stream_open/stream_close", media_cli_storage_test_cmd},
};

int media_cli_init(void)
{
	return cli_register_commands(s_media_commands, MEDIA_CMD_CNT);
}


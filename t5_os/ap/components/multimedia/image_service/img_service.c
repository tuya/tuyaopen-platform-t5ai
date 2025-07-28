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
#include <string.h>
#include <stdlib.h>
#include <os/os.h>
#include <os/str.h>
#include <os/mem.h>
#include <components/log.h>
#include "media_evt.h"
#include "img_service.h"
#include "frame_buffer.h"
#include <driver/gpio.h>
#include <driver/lcd.h>
#include <driver/jpeg_dec_types.h>
#include "modules/image_scale.h"
#include "gpio_driver.h"
#include <modules/jpeg_decode_sw.h>
#include <driver/media_types.h>
#include <bk_decode.h>
#include <bk_rotate.h>
#include <bk_scale.h>
#include "yuv_encode.h"
#include "sw_decode.h"
#include "uvc_pipeline_act.h"
#include "lcd_display_service.h"
#include "bk_draw_blend.h"
#include "media_utils.h"
// Modified by TUYA Start
#include "tuya_cloud_types.h"
// Modified by TUYA End

#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif

#define TAG "img_serv"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

//#define IMG_DIAG_DEBUG

#ifdef IMG_DIAG_DEBUG

#define IMG_DIAG_DEBUG_INIT()                   \
    do {                                        \
        gpio_dev_unmap(GPIO_2);                 \
        bk_gpio_disable_input((GPIO_2));        \
        bk_gpio_enable_output(GPIO_2);          \
        bk_gpio_set_output_low(GPIO_2);         \
        \
        gpio_dev_unmap(GPIO_3);                 \
        bk_gpio_disable_input((GPIO_3));        \
        bk_gpio_enable_output(GPIO_3);          \
        bk_gpio_set_output_low(GPIO_3);         \
        \
        gpio_dev_unmap(GPIO_4);                 \
        bk_gpio_disable_input((GPIO_4));        \
        bk_gpio_enable_output(GPIO_4);          \
        bk_gpio_set_output_low(GPIO_4);         \
        \
        gpio_dev_unmap(GPIO_5);                 \
        bk_gpio_disable_pull(GPIO_5);           \
        bk_gpio_enable_output(GPIO_5);          \
        bk_gpio_set_output_low(GPIO_5);         \
        \
        gpio_dev_unmap(GPIO_8);                 \
        bk_gpio_disable_input((GPIO_8));        \
        bk_gpio_enable_output(GPIO_8);          \
        bk_gpio_set_output_low(GPIO_8);         \
        \
    } while (0)

#define IMG_DECODER_START()                 bk_gpio_set_output_high(GPIO_2)
#define IMG_DECODER_END()                   bk_gpio_set_output_low(GPIO_2)

#define IMG_ROTATE_START()                  bk_gpio_set_output_high(GPIO_3)
#define IMG_ROTATE_END()                    bk_gpio_set_output_low(GPIO_3)

#define IMG_RESIZE_END()                    bk_gpio_set_output_high(GPIO_8)
#define IMG_RESIZE_START()                  bk_gpio_set_output_high(GPIO_8)
#else
#define IMG_DIAG_DEBUG_INIT()

#define IMG_DECODER_START()
#define IMG_DECODER_END()

#define IMG_ROTATE_START()
#define IMG_ROTATE_END()

#define IMG_RESIZE_END()
#define IMG_RESIZE_START()
#endif

#if CONFIG_LVGL
uint8_t lvgl_disp_enable = 0;
#endif

extern media_debug_t *media_debug;
/**< image decode */
beken_semaphore_t jpeg_decoder_sem;
bool jpeg_decoder_task_running = false;
static beken_thread_t jpeg_decoder_task = NULL;
frame_list_node_t *jpeg_frame_node;
// Modified by TUYA Start
static void *g_lcd_handle = NULL;

// static  ty_frame_buffer_t g_ty_frame[2] = {0};
static unsigned int g_cnt = 0;
// Modified by TUYA End

/**< image service */
bool img_service_task_running = false;// img_service_task_running = false;
static beken_thread_t img_service_task = NULL;

img_info_t img_info = {0};

bk_err_t bk_img_msg_send(img_msg_t *msg)
{
    bk_err_t ret = BK_FAIL;

    if (img_info.queue && img_service_task_running)
    {
        ret = rtos_push_to_queue(&img_info.queue, msg, BEKEN_NO_WAIT);
        if (ret != BK_OK)
        {
            LOGE("%s push failed\n", __func__);
        }
    }
    return ret;
}

static void img_service_task_entry(beken_thread_arg_t data)
{
    int ret = BK_FAIL ;
    img_service_task_running = true;
    rtos_set_semaphore(&img_info.sem);

    // g_ty_frame[0].frame = psram_malloc(480 * 480 * 2);
    // g_ty_frame[1].frame = psram_malloc(480 * 480 * 2);
    while (img_service_task_running)
    {
        img_msg_t msg;
        ret = rtos_pop_from_queue(&img_info.queue, &msg, BEKEN_WAIT_FOREVER);
        if (ret == BK_OK)
        {
            switch (msg.event)
            {
                case IMG_DISPLAY_REQUEST:
                {
                    frame_buffer_t *frame = msg.ptr;
                    if (frame->fmt == PIXEL_FMT_YUYV || frame->fmt == PIXEL_FMT_RGB565 || frame->fmt == PIXEL_FMT_RGB565_LE)
                    {
                        frame = rotate_frame_handler(frame, img_info.rotate);
                        if (frame != NULL)
                        {
                            #if CONFIG_LVGL
                            if (lvgl_disp_enable) {
                                img_info.fb_free(frame);
                            }
                            else
                            #endif
                            {
                                // Modified by TUYA Start
                                // g_cnt = g_cnt % 2;
                                // ty_frame_buffer_t *ty_frame = &g_ty_frame[g_cnt];
                                // g_cnt++;
                                // if (ty_frame) {
                                //     __builtin_memcpy(ty_frame->frame, frame->frame, frame->length);
                                //     ty_frame->type = 1;
                                //     ty_frame->len = frame->length;
                                //     ty_frame->fmt = 0;
                                //     ty_frame->width = frame->width;
                                //     ty_frame->height = frame->height;
                                //     ty_frame->free_cb = NULL;
                                //     ty_frame->pdata = frame;
                                //     img_info.fb_free(frame);
                                //     // 刷新显示（使用原始buffer的子区域）
                                //     tal_display_flush(g_lcd_handle, ty_frame);
                                //     // Modified by TUYA End
                                // }
                                if (ret != BK_OK)
                                {
                                    img_info.fb_free(frame);
                                }
                            }
                        }
                    }
                    else
                    {
                        LOGE("%s display format %d\n", __func__, frame->fmt);
                        img_info.fb_free(frame);
                    }
                    break;
                }
                case IMG_DISPLAY_EXIT:
                {
                    goto exit;
                    break;
                }
            }
        }
    }
exit:
    img_service_task = NULL;
    rtos_set_semaphore(&img_info.sem);
    LOGD("lcd display task exit\n");
    rtos_delete_thread(NULL);
}


static bk_err_t img_service_task_start(void)
{
    bk_err_t ret = BK_FAIL;

    if (img_service_task != NULL)
    {
        LOGE("%s img_service_task_thread already running\n", __func__);
        goto out;
    }

    ret = rtos_init_queue(&img_info.queue,
                          "img_queue",
                          sizeof(img_msg_t),
                          5);
    if (ret != BK_OK)
    {
        LOGE("%s, init display_queue failed\r\n", __func__);
        goto out;
    }

    ret = rtos_init_semaphore_ex(&img_info.sem, 1, 0);
    if (BK_OK != ret)
    {
        LOGE("%s semaphore init failed\n", __func__);
        goto out;
    }

    ret = rtos_create_thread(&img_service_task,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "lcd_display_thread",
                             (beken_thread_function_t)img_service_task_entry,
                             2 * 1024,
                             (beken_thread_arg_t)NULL);

    if (BK_OK != ret)
    {
        LOGE("%slcd_display_thread init failed\n", __func__);
        goto out;
    }

    ret = rtos_get_semaphore(&img_info.sem, BEKEN_NEVER_TIMEOUT);

    if (BK_OK != ret)
    {
        LOGE("%s decoder_sem get failed\n", __func__);
        goto out;
    }
    return ret;

out:
    if (img_info.queue)
    {
        rtos_deinit_queue(&img_info.queue);
        img_info.queue = NULL;
    }
    if (img_info.sem)
    {
        rtos_deinit_semaphore(&img_info.sem);
        img_info.sem = NULL;
    }

    return ret;
}

static void img_service_task_stop(void)
{
    bk_err_t ret;

    if (img_service_task_running == false)
    {
        LOGD("%s already stop\n", __func__);
        return;
    }
    img_service_task_running = false;

    img_msg_t msg =
    {
        .event = IMG_DISPLAY_EXIT,
    };
    ret = rtos_push_to_queue(&img_info.queue, &msg, BEKEN_WAIT_FOREVER);
    if (ret != BK_OK)
    {
        LOGE("%s %d push failed\n", __func__, __LINE__);
    }

    ret = rtos_get_semaphore(&img_info.sem, BEKEN_NEVER_TIMEOUT);
    if (BK_OK != ret)
    {
        LOGE("%s img_info.sem get failed\n", __func__);
    }

    do
    {
        ret = rtos_pop_from_queue(&img_info.queue, &msg, BEKEN_NO_WAIT);
        if (ret == BK_OK)
        {
            if (msg.event == IMG_DISPLAY_REQUEST)
            {
                LOGD("%s %d free IMG_DISPLAY_REQUEST frame\n", __func__, __LINE__);
                img_info.fb_free((frame_buffer_t *)msg.ptr);
            }
        }
    }
    while (ret == BK_OK);

    ret = rtos_deinit_semaphore(&img_info.sem);
    if (BK_OK != ret)
    {
        LOGE("%s img_info.sem deinit failed\n", __func__);
    }

    if (img_info.queue)
    {
        rtos_deinit_queue(&img_info.queue);
        img_info.queue = NULL;
    }
    LOGD("%s complete\n", __func__);
}

static void decoder_task_entry(beken_thread_arg_t data)
{
    frame_buffer_t *jpeg_frame = NULL;
    frame_buffer_t *dec_frame = NULL;
    rtos_set_semaphore(&jpeg_decoder_sem);

    while (jpeg_decoder_task_running)
    {
        /* if dvp work jpeg_yuv mode, use yuv frame display direct and jpeg data not decoder */
        /* frame buffer_t delect "mix" member */
        if (jpeg_frame_node->camera_type != UVC_CAMERA)
        {
            jpeg_decoder_task_running = false;
            continue;
        }
        /* Normal display workflow */
        jpeg_frame = frame_buffer_fb_read(jpeg_frame_node, MODULE_DECODER, 50);
        if (jpeg_frame == NULL)
        {
            LOGV("%s read jpeg frame NULL\n", __func__);
            continue;
        }

        dec_frame = decoder_frame_handler(jpeg_frame);

        frame_buffer_fb_read_free(jpeg_frame_node, jpeg_frame, MODULE_DECODER);
        if (dec_frame == NULL)
        {
            LOGV("jpeg decoder frame NULL\n");
            continue;
        }

        img_msg_t msg =
        {
            .event = IMG_DISPLAY_REQUEST,
            .ptr = dec_frame,
        };
        bk_err_t ret = bk_img_msg_send(&msg);
        if (ret != BK_OK)
        {
            img_info.fb_free(dec_frame);
        }
    }
    LOGD("camera display task exit\n");
    jpeg_decoder_task = NULL;
    rtos_set_semaphore(&jpeg_decoder_sem);
    rtos_delete_thread(NULL);
}

void decoder_task_start(void)
{
    bk_err_t ret;

    if (jpeg_decoder_task != NULL)
    {
        LOGE("%s lcd decode thread already running\n", __func__);
        goto out;
    }
    jpeg_frame_node = frame_buffer_list_get_by_format(IMAGE_MJPEG);
    if (jpeg_frame_node == NULL)
    {
        LOGE("%s, stream null\n", __func__);
        goto out;
    }
    frame_buffer_fb_register(jpeg_frame_node, MODULE_DECODER);

    ret = rtos_init_semaphore_ex(&jpeg_decoder_sem, 1, 0);

    if (BK_OK != ret)
    {
        LOGE("%s semaphore init failed\n", __func__);
        goto out;
    }

    jpeg_decoder_task_running = true;

    ret = rtos_create_thread(&jpeg_decoder_task,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "lcd_decoder_thread",
                             (beken_thread_function_t)decoder_task_entry,
                             2 * 1024,
                             (beken_thread_arg_t)NULL);

    if (BK_OK != ret)
    {
        LOGE("%s lcd decoder task init failed\n", __func__);
        goto out;
    }

    ret = rtos_get_semaphore(&jpeg_decoder_sem, BEKEN_NEVER_TIMEOUT);
    if (BK_OK != ret)
    {
        LOGE("%s jpeg_decoder_sem get failed\n", __func__);
        goto out;
    }
    return;
out:
    if (jpeg_decoder_sem)
    {
        rtos_deinit_semaphore(&jpeg_decoder_sem);
        jpeg_decoder_sem = NULL;
    }

    if (jpeg_frame_node)
    {
        LOGE("%s, frame_buffer_fb_deregister decoder.\n", __func__);
        frame_buffer_fb_deregister(jpeg_frame_node, MODULE_DECODER);
    }

}

void decoder_task_stop(void)
{
    bk_err_t ret;
    if (jpeg_decoder_task_running == false)
    {
        LOGD("%s already stop\n", __func__);
        return;
    }
    jpeg_decoder_task_running = false;

    frame_buffer_fb_deregister(jpeg_frame_node, MODULE_DECODER);
    ret = rtos_get_semaphore(&jpeg_decoder_sem, BEKEN_NEVER_TIMEOUT);

    if (BK_OK != ret)
    {
        LOGE("%s jpeg_decoder_sem get failed\n", __func__);
    }

    ret = rtos_deinit_semaphore(&jpeg_decoder_sem);

    if (BK_OK != ret)
    {
        LOGE("%s decoder_sem deinit failed\n", __func__);
    }

    LOGD("%s complete\n", __func__);
}


frame_buffer_t *decoder_frame_handler(frame_buffer_t *frame)
{
    bk_err_t ret = BK_FAIL;
    frame_buffer_t *dec_frame = NULL;
    uint64_t before, after;
    
    if (img_info.enable == false)
    {
        return dec_frame;
    }

    rtos_lock_mutex(&img_info.dec_lock);

    if (img_info.decoder_en == false)
    {
        rtos_unlock_mutex(&img_info.dec_lock);
        return dec_frame;
    }

    before = get_current_timestamp();

    img_info.decoder_frame = img_info.fb_malloc(frame->width * frame->height * 2);

    if (img_info.decoder_frame == NULL)
    {
        LOGE("malloc decoder NULL\n");
        goto out;
    }
    IMG_DECODER_START();

    img_info.decoder_frame->sequence = frame->sequence;
    if (img_info.jpg_fmt_check == false)
    {
        img_info.jpg_fmt_check = true;
        yuv_enc_fmt_t yuv_fmt = bk_get_original_jpeg_encode_data_format(frame->frame, frame->length);
        if (yuv_fmt == YUV_422)
        {
            LOGD("%s, FMT:YUV422, use HARDWARE DECODE\r\n", __func__);
            img_info.decode_mode = JPEGDEC_HW_MODE;
        }
        else if (yuv_fmt == YUV_ERR)
        {
            LOGD("%s, FMT:ERR\r\n", __func__);
            img_info.decode_mode = NONE_DECODE;
            img_info.decoder_frame->fmt = PIXEL_FMT_YUYV;
            img_info.fb_free(img_info.decoder_frame);
            img_info.decoder_frame = NULL;
            img_info.jpg_fmt_check = false;
            goto out;
        }
        else
        {
            LOGD("%s, FMT:YUV420, use SOFTWARE DECODE\r\n", __func__);
            img_info.decode_mode = SOFTWARE_DECODING_MAJOR;
        }
    }

    if (img_info.decode_mode == HARDWARE_DECODING)
    {
        img_info.decoder_frame->fmt = PIXEL_FMT_VUYY;
        ret = bk_hw_decode_start(frame, img_info.decoder_frame);
        if (ret != BK_OK)
        {
            LOGV("%s hw decoder error\n", __func__);
            img_info.fb_free(img_info.decoder_frame);
            img_info.decoder_frame = NULL;
            goto out;
        }
    }
    else
    {
        img_info.decoder_frame->fmt = PIXEL_FMT_YUYV;
#if CONFIG_LCD_SW_DECODE
        if (img_info.decode_mode == SOFTWARE_DECODING_MAJOR)
        {
#ifdef CONFIG_MEDIA_PIPELINE
            if (img_info.rotate_en)
            {
                software_decode_set_rotate(img_info.rotate);
            }
#endif
            ret = bk_sw_jpegdec_start(frame, img_info.decoder_frame);
#if (CONFIG_TASK_WDT)
            extern void bk_task_wdt_feed(void);
            bk_task_wdt_feed();
#endif
            if (ret != BK_OK)
            {
                LOGE("%s sw decoder error\n", __func__);
                img_info.fb_free(img_info.decoder_frame);
                img_info.decoder_frame = NULL;
                goto out;
            }
        }
        else
        {
            //ret = bk_sw_minor_jpegdec_start(frame, img_info.decoder_frame);

            if (ret != BK_OK)
            {
                LOGE("%s sw decoder error\n", __func__);
                img_info.fb_free(img_info.decoder_frame);
                img_info.decoder_frame = NULL;
                goto out;
            }
        }
#endif
    }
    IMG_DECODER_END();

out:

    if (img_info.decoder_frame == NULL)
    {
        LOGV("%s decoder failed\n", __func__);
        ret = BK_FAIL;
    }
    else
    {
        dec_frame = img_info.decoder_frame;
        img_info.decoder_frame = NULL;
    }

    rtos_unlock_mutex(&img_info.dec_lock);
    after = get_current_timestamp();
    LOGV("decoder time: %lu\n", (after - before) / 26000);

    return dec_frame;
}


frame_buffer_t *scale_frame_handler(frame_buffer_t *frame, media_ppi_t ppi)
{
    frame_buffer_t *scale_frame = NULL;

    bk_err_t ret = BK_FAIL;
    uint64_t before, after;

    if (img_info.enable == false)
    {
        LOGE("%s img_info.enable == false\r\n", __func__);
        img_info.fb_free(frame);
        return scale_frame;
    }
    if (img_info.scale_en == false)
    {
        LOGE("%s img_info.scale_en == false\r\n", __func__);
        rtos_unlock_mutex(&img_info.scale_lock);
        return frame;
    }

    IMG_RESIZE_START();
    before = get_current_timestamp();

    uint32_t scale_length = (img_info.scale_ppi >> 16) * (img_info.scale_ppi & 0xFFFF) * 2;
    if (img_info.scale_frame == NULL)
    {
        //img_info.scale_frame = img_info.fb_malloc(scale_length);
        img_info.scale_frame = img_info.fb_malloc(frame->width * frame->height * 2);
    }
    if (img_info.scale_frame == NULL)
    {
        LOGE("%s, malloc scale_frame NULL\n", __func__);
        goto out;
    }
    LOGV("scale_ppi: width height %d %d\n", (img_info.scale_ppi >> 16), (img_info.scale_ppi & 0xFFFF));

    img_info.scale_frame->width = (img_info.scale_ppi >> 16);
    img_info.scale_frame->height = (img_info.scale_ppi & 0xFFFF);
    img_info.scale_frame->fmt = frame->fmt;
    img_info.scale_frame->sequence = frame->sequence;
    img_info.scale_frame->length = scale_length;

    ret = bk_hw_scale(frame, img_info.scale_frame);
    if (ret != BK_OK)
    {
        LOGE("%s scale error: %d\n", __func__, ret);
        img_info.fb_free(img_info.scale_frame);
        goto out;
    }
    after = get_current_timestamp();
    LOGV("rotate time: %lu\n", (after - before) / 26000);

    IMG_RESIZE_END();

    scale_frame = img_info.scale_frame;
    img_info.scale_frame = NULL;

out:
    img_info.fb_free(frame);
    rtos_unlock_mutex(&img_info.scale_lock);
    return scale_frame;
}

frame_buffer_t *rotate_frame_handler(frame_buffer_t *frame, media_rotate_t rotate)
{
    frame_buffer_t *rot_frame = NULL;
    uint64_t before, after;
    bk_err_t ret = BK_FAIL;

    if (rotate == ROTATE_NONE)
    {
        LOGV("%s rotate 0 \n", __func__);
        return frame;
    }
    if (frame->height == 720 && frame->width == 1280)
    {
        LOGV("%s 720p is not support rotate\n", __func__);
        return frame;
    }
    IMG_ROTATE_START();

    if (img_info.enable == false)
    {
        LOGE("%s img_info.enable == false\r\n", __func__);
        img_info.fb_free(frame);
        return rot_frame;
    }
    rtos_lock_mutex(&img_info.rot_lock);

    if (img_info.rotate_en == false)
    {
        rtos_unlock_mutex(&img_info.rot_lock);
        return frame;
    }

    before = get_current_timestamp();

    if (img_info.rotate_frame == NULL)
    {
        img_info.rotate_frame = img_info.fb_malloc(frame->width * frame->height * 2);
    }
    if (img_info.rotate_frame == NULL)
    {
        LOGE("%s, malloc rotate_frame NULL\n", __func__);
        goto out;
    }
    if (rotate == ROTATE_180)
    {
        img_info.rotate_frame->height = frame->height;
        img_info.rotate_frame->width = frame->width;
    }
    else
    {
        img_info.rotate_frame->height = frame->width;
        img_info.rotate_frame->width = frame->height;
    }

    img_info.rotate_frame->fmt = frame->fmt;
    img_info.rotate_frame->sequence = frame->sequence;
    img_info.rotate_frame->length = frame->width * frame->height * 2;

    if ((img_info.rotate_mode == HW_ROTATE))
    {
        ret = bk_hw_rotate_yuv2rgb565(frame, img_info.rotate_frame, rotate);
    }
    else
    {
        ret = bk_sw_rotate(frame, img_info.rotate_frame, rotate);
    }
    if (ret != BK_OK)
    {
        LOGE("%s rotate error: %d\n", __func__, ret);
        img_info.fb_free(img_info.rotate_frame);
        goto out;
    }

    after = get_current_timestamp();
    LOGV("rotate time: %lu\n", (after - before) / 26000);

    IMG_ROTATE_END();

out:
    if (img_info.rotate_frame == NULL)
    {
        media_debug->err_rot++;
    }
    else
    {
        rot_frame = img_info.rotate_frame;
        img_info.rotate_frame = NULL;
    }
    img_info.fb_free(frame);
    rtos_unlock_mutex(&img_info.rot_lock);
    return rot_frame;
}

bk_err_t image_rotate_set(media_rotate_t rotate)
{
    int ret = BK_OK;
    img_info.rotate_en = true;
    img_info.rotate = rotate;
    LOGV("%s rotate angle = %d \n", __func__, img_info.rotate);
    return ret;
}

bk_err_t img_service_open(void)
{
    int ret = BK_OK;
    if (true == img_info.enable)
    {
        LOGW("%s already open\n", __func__);
        return ret;
    }
    img_info.fb_free  = frame_buffer_display_free;
    img_info.fb_malloc = frame_buffer_display_malloc;

    img_info.rotate_mode = HW_ROTATE;
    ret = bk_rotate_init(img_info.rotate_mode);
    if (ret != BK_OK)
    {
        LOGE("%s, bk_rotate_init fail\r\n", __func__);
    }
    LOGV("rotate mode(1-sw, 2-hw) = %d, rotate angle(0/1/2/3 -->0/90/180/270) = %d\r\n", img_info.rotate_mode, img_info.rotate);

#if CONFIG_MEDIA_SCALE
    if (img_info.scale_en)
    {
        ret = bk_scale_init();
        if (ret != BK_OK)
        {
            LOGE("%s, bk_hw_scale_driver_init fail\r\n", __func__);
        }
        img_info.scale_ppi = img_info.lcd_device->ppi;
        LOGD("%s, scale ppi: %dX%d %s\n", __func__, img_info.scale_ppi >> 16, img_info.scale_ppi & 0xFFFF, img_info.lcd_device->name);
    }
#endif

    rtos_init_mutex(&img_info.dec_lock);
    rtos_init_mutex(&img_info.rot_lock);
    rtos_init_mutex(&img_info.scale_lock);

#if !CONFIG_MEDIA_PIPELINE
    img_info.decoder_en = true;
    bk_hw_decode_init();
#if CONFIG_LCD_SW_DECODE
    bk_sw_decode_init(img_info.decode_mode);
#endif
    decoder_task_start();
#endif
    ret = img_service_task_start();
    if (ret != BK_OK)
    {
        LOGE("%s img_service_task_start failed: %d\n", __func__, ret);
    }

    img_info.enable = true;

    LOGD("%s complete\n", __func__);
    return ret;
}


bk_err_t img_service_close(void)
{
    int ret = BK_OK;

    LOGV("%s\n", __func__);

    if (false == img_info.enable)
    {
        LOGW("%s already close\n", __func__);
        goto out;
    }

#if !CONFIG_MEDIA_PIPELINE
    decoder_task_stop();
#endif

    img_service_task_stop();

    img_info.enable = false;


    rtos_lock_mutex(&img_info.dec_lock);
    img_info.decoder_en = false;
    rtos_unlock_mutex(&img_info.dec_lock);

    rtos_lock_mutex(&img_info.rot_lock);
    img_info.rotate_en = false;
    rtos_unlock_mutex(&img_info.rot_lock);

    rtos_lock_mutex(&img_info.scale_lock);
    img_info.scale_en = false;
    rtos_unlock_mutex(&img_info.scale_lock);

    bk_hw_decode_deinit();
#if CONFIG_LCD_SW_DECODE
    bk_sw_decode_deinit(img_info.decode_mode);
#endif
    bk_rotate_deinit();

    if (img_info.rotate_frame)
    {
        img_info.fb_free(img_info.rotate_frame);
        img_info.rotate_frame = NULL;
    }

    if (img_info.decoder_frame)
    {
        img_info.fb_free(img_info.decoder_frame);
        img_info.decoder_frame = NULL;
    }

    rtos_deinit_mutex(&img_info.dec_lock);
    rtos_deinit_mutex(&img_info.rot_lock);
    rtos_deinit_mutex(&img_info.scale_lock);

    img_info.rotate = ROTATE_NONE;
    img_info.jpg_fmt_check = false;

    LOGD("%s complete\n", __func__);
out:
    return ret;
}

// Modified by TUYA Start
void im_lcd_handle(void *handle)
{
    g_lcd_handle = handle;  
}
// Modified by TUYA End

#include <stdlib.h>
#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>

#include <driver/jpeg_dec.h>
#include <driver/jpeg_dec_types.h>
#include <driver/timer.h>

#if (CONFIG_LCD_SW_DECODE)
#include <modules/jpeg_decode_sw.h>
#include <modules/tjpgd.h>
#endif

#include <driver/media_types.h>
#include <bk_decode.h>
#include "yuv_encode.h"
#include "sw_decode.h"

#define TAG "lcd_dec"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


extern media_debug_t *media_debug;

static bk_decode_t s_decode = {0};

#if !CONFIG_MEDIA_PIPELINE
static jpeg_dec_handle_t jpeg_dec_handle = NULL;
#endif

static void bk_driver_decoder_timeout(timer_id_t timer_id)
{
    bk_err_t ret = BK_FAIL;

    bk_timer_stop(TIMER_ID3);

    bk_jpeg_dec_stop();
    media_debug->isr_decoder--;

    s_decode.decode_timeout = true;
    LOGE("%s \n", __func__);

    ret = rtos_set_semaphore(&s_decode.hw_dec_sem);
    if (ret != BK_OK)
    {
        LOGV("%s semaphore set failed: %d\n", __func__, ret);
    }
}

static void jpeg_dec_err_cb(jpeg_dec_res_t *result)
{
    bk_err_t ret = BK_FAIL;

    LOGV("%s \n", __func__);
    s_decode.decode_err = true;
    media_debug->isr_decoder--;

    ret = rtos_set_semaphore(&s_decode.hw_dec_sem);

    if (ret != BK_OK)
    {
        LOGE("%s semaphore set failed: %d\n", __func__, ret);
    }
}

static void jpeg_dec_eof_cb(jpeg_dec_res_t *result)
{
    bk_err_t ret = BK_FAIL;
    s_decode.decode_err = false;

    media_debug->isr_decoder++;

    bk_timer_stop(TIMER_ID3);

    if (s_decode.decoder_frame)
    {
        s_decode.decoder_frame->height = result->pixel_y;
        s_decode.decoder_frame->width = result->pixel_x ;
        s_decode.decoder_frame->length = result->pixel_y * result->pixel_x * 2;
    }

    if (result->ok == false)
    {
        s_decode.decode_err = true;
        media_debug->isr_decoder--;
        LOGV("%s decoder error\n", __func__);
    }

    ret = rtos_set_semaphore(&s_decode.hw_dec_sem);

    if (ret != BK_OK)
    {
        LOGE("%s semaphore set failed: %d\n", __func__, ret);
    }
}

bk_err_t bk_hw_decode_start(frame_buffer_t *src_frame, frame_buffer_t *dst_frame)
{
    int ret = BK_OK;
    s_decode.decoder_frame = dst_frame;
    s_decode.decode_timeout = false;
    bk_jpeg_dec_out_format(PIXEL_FMT_YUYV);
    dst_frame->fmt = PIXEL_FMT_YUYV;
    ret = bk_jpeg_dec_hw_start(src_frame->length, src_frame->frame, dst_frame->frame);

    if (ret != BK_OK)
    {
        LOGD("%s, length:%d\r\n", __func__, src_frame->length);
        return ret;
    }
    bk_timer_start(TIMER_ID3, 200, bk_driver_decoder_timeout);

    ret = rtos_get_semaphore(&s_decode.hw_dec_sem, BEKEN_NEVER_TIMEOUT);
    if (ret != BK_OK)
    {
        LOGE("%s semaphore get failed: %d\n", __func__, ret);
    }
    if (s_decode.decode_timeout == true)
    {
        ret = BK_LCD_DECODE_TIMEOUT;
    }
    if (s_decode.decode_err == true)
    {
        ret = BK_LCD_DECODE_ERR;
    }

    return ret;
}

void bk_sw_jpegdec_callback(uint8_t result)
{
    bk_err_t ret = BK_OK;
    if (result == BK_OK)
    {
        media_debug->isr_decoder++;
    }
    else
    {
        LOGE("%s sw decode failed %d\n", __func__, ret);
    }
    ret = rtos_set_semaphore(&s_decode.sw_dec_sem);

    if (ret != BK_OK)
    {
        LOGE("%s semaphore set failed: %d\n", __func__, ret);
    }
}

bk_err_t bk_sw_jpegdec_start(frame_buffer_t *frame, frame_buffer_t *dst_frame)
{
    bk_err_t ret =  BK_OK;

#if CONFIG_MEDIA_PIPELINE
    media_software_decode_info_t info = {0};
    info.in_frame = frame;
    info.out_frame = dst_frame;
    info.cb = &bk_sw_jpegdec_callback;
    software_decode_task_send_msg(JPEGDEC_START, (uint32_t)&info);

    ret = rtos_get_semaphore(&s_decode.sw_dec_sem, BEKEN_NEVER_TIMEOUT);
    if (ret != BK_OK)
    {
        LOGE("%s semaphore get failed: %d\n", __func__, ret);
    }
#else
    jd_output_format format = {0};
    sw_jpeg_dec_res_t result = {0};
    switch (dst_frame->fmt)
    {
        case PIXEL_FMT_RGB565:
            format.format = JD_FORMAT_RGB565;
            format.scale = 1;
            format.byte_order = JD_BIG_ENDIAN;
            break;
        case PIXEL_FMT_YUYV:
            format.format = JD_FORMAT_YUYV;
            format.scale = 0;
            format.byte_order = JD_LITTLE_ENDIAN;
            break;
        case PIXEL_FMT_VUYY:
            format.format = JD_FORMAT_VUYY;
            format.scale = 0;
            format.byte_order = JD_LITTLE_ENDIAN;
            break;
        case PIXEL_FMT_RGB888:
            format.format = JD_FORMAT_RGB888;
            format.scale = 0;
            format.byte_order = JD_LITTLE_ENDIAN;
            break;
        default:
            format.format = JD_FORMAT_VYUY;
            format.scale = 0;
            format.byte_order = JD_LITTLE_ENDIAN;
            break;
    }

    jd_set_output_format_by_handle(jpeg_dec_handle, &format);
    ret = bk_jpeg_dec_sw_start_by_handle(jpeg_dec_handle, JPEGDEC_BY_FRAME, frame->frame, dst_frame->frame, frame->length, dst_frame->size, &result);
    if (ret != BK_OK)
    {
        LOGE("%s sw decoder error\n", __func__);
        media_debug->isr_decoder--;
    }
    else
    {
        dst_frame->height = result.pixel_y;
        dst_frame->width = result.pixel_x ;
    }
#endif

    return ret;
}

bk_err_t bk_hw_decode_init(void)
{
    bk_err_t ret = BK_OK;
    if (s_decode.hw_state != false)
    {
        LOGD("%s, already init\n", __func__);
        return ret;
    }
    s_decode.hw_state = true;

    media_debug->isr_decoder = 0;
    media_debug->err_dec = 0;
    LOGD("%s \r\n", __func__);

    ret = rtos_init_semaphore_ex(&s_decode.hw_dec_sem, 1, 0);

    if (ret != BK_OK)
    {
        LOGE("%s hw  dec_sem init failed: %d\n", __func__, ret);
        return ret;
    }

    ret = bk_jpeg_dec_driver_init();
    bk_jpeg_dec_isr_register(DEC_ERR, jpeg_dec_err_cb);
#if(1)  //enable jpeg complete int isr
    bk_jpeg_dec_isr_register(DEC_END_OF_FRAME, jpeg_dec_eof_cb);
#else   //enable uvc ppi 640X480 jpeg 24 line decode complete int isr
    bk_jpeg_dec_isr_register(DEC_END_OF_LINE_NUM, jpeg_dec_line_cb);
#endif

    return ret;
}
bk_err_t bk_hw_decode_deinit(void)
{
    bk_err_t ret = BK_OK;
    if (s_decode.hw_state == false)
    {
        LOGV("%s, already deinit\n", __func__);
        return ret;
    }
    LOGD("%s \r\n", __func__);

    bk_jpeg_dec_driver_deinit();

    ret = rtos_deinit_semaphore(&s_decode.hw_dec_sem);

    if (ret != BK_OK)
    {
        LOGE("%s dec_sem deinit failed: %d\n", __func__, ret);
        return ret;
    }
    s_decode.hw_state = false;
    return ret;
}

bk_err_t bk_sw_decode_init(media_decode_mode_t sw_dec_mode)
{
    bk_err_t ret = BK_OK;
    LOGD("%s \n", __func__);
    if (s_decode.sw_state != false)
    {
        LOGD("%s, already init\n", __func__);
        return ret;
    }
    s_decode.sw_state = true;

    ret = rtos_init_semaphore_ex(&s_decode.sw_dec_sem, 1, 0);

    if (ret != BK_OK)
    {
        LOGE("%s sw dec_sem init failed: %d\n", __func__, ret);
        goto error;
    }
#if CONFIG_MEDIA_PIPELINE
#if CONFIG_JPEG_SW_DECODE_SUPPORT_BY_FRAME
    ret = software_decode_task_open();
    if (ret != BK_OK)
    {
        LOGE("%s, software_decode_task_open failed\r\n", __func__);
        goto error;
    }
#endif
#else
    ret = bk_jpeg_dec_sw_init_by_handle(&jpeg_dec_handle, NULL, 0);
    if (ret != BK_OK)
    {
        LOGE("%s dec_sem init failed: %d\n", __func__, ret);
        goto error;
    }
#endif
    return ret;

error:
    if (s_decode.sw_dec_sem)
    {
        rtos_deinit_semaphore(&s_decode.sw_dec_sem);
    }
#if CONFIG_MEDIA_PIPELINE
#if CONFIG_JPEG_SW_DECODE_SUPPORT_BY_FRAME
    if (check_software_decode_task_is_open())
    {
        software_decode_task_close();
    }
#endif
#endif
    s_decode.sw_state = false;

    return ret;
}

bk_err_t bk_sw_decode_deinit(media_decode_mode_t sw_dec_mode)
{
    bk_err_t ret = BK_OK;

    if (s_decode.sw_state == false)
    {
        LOGV("%s, already deinit\n", __func__);
        return ret;
    }
    LOGD("%s sw_dec_mode = %d\n", __func__, sw_dec_mode);

#if CONFIG_MEDIA_PIPELINE
#if CONFIG_JPEG_SW_DECODE_SUPPORT_BY_FRAME
    if (check_software_decode_task_is_open())
    {
        software_decode_task_close();
    }
#endif
#else
    ret = bk_jpeg_dec_sw_deinit_by_handle(jpeg_dec_handle);
    if (ret != BK_OK)
    {
        LOGE("%s dec_sem init failed: %d\n", __func__, ret);
    }
#endif
    ret = rtos_deinit_semaphore(&s_decode.sw_dec_sem);

    if (ret != BK_OK)
    {
        LOGE("%s dec_sem deinit failed: %d\n", __func__, ret);
        goto error;
    }

error:
    s_decode.sw_state = false;

    return ret;
}


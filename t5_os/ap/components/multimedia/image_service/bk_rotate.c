#include <stdlib.h>
#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>

#if CONFIG_HW_ROTATE_PFC
#include <driver/rott_driver.h>
#endif
#include <driver/media_types.h>
#include <bk_rotate.h>
#include "modules/image_scale.h"
#include "sw_rotate.h"
#include "media_evt.h"
#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif



#define TAG "lcd_rotate"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)



typedef struct
{
    media_rotate_t rotate_angle;
    beken_semaphore_t rot_sem;
    uint8_t rotate_en;
} rotate_t;
static rotate_t  s_rot = {0};



static void rotate_complete_cb(void)
{
    LOGV("rotate_complete_cb\r\n");
    rtos_set_semaphore(&s_rot.rot_sem);
}

static void rotate_watermark_cb(void)
{
    LOGV("rotate_watermark_cb\r\n");

}
static void rotate_cfg_err_cb(void)
{
    LOGD("rotate_cfg_err_cb\r\n");
}




static void rott_pfc_complete_cb(void)
{
    LOGV("rott_pfc_complete_cb\r\n");
    rtos_set_semaphore(&s_rot.rot_sem);
}


bk_err_t bk_rotate_deinit(void)
{
    bk_err_t ret = BK_OK;
#ifdef CONFIG_HW_ROTATE_PFC
    bk_rott_driver_deinit();
#endif
    if (check_software_rotate_task_is_open())
    {
        software_rotate_task_close();
    }
    ret = rtos_deinit_semaphore(&s_rot.rot_sem);

    if (ret != BK_OK)
    {
        LOGE("%s rot_sem deinit failed: %d\n", __func__, ret);
        return ret;
    }
    return ret;
}

bk_err_t bk_rotate_init(media_rotate_mode_t rotate_mode)
{
    bk_err_t ret = BK_OK;
    ret = rtos_init_semaphore_ex(&s_rot.rot_sem, 1, 0);

    if (ret != BK_OK)
    {
        LOGE("%s rot_sem init failed: %d\n", __func__, ret);
        return ret;
    }

    if (rotate_mode == HW_ROTATE)
    {
#ifdef CONFIG_HW_ROTATE_PFC
        bk_rott_driver_init();
        bk_rott_int_enable(ROTATE_COMPLETE_INT | ROTATE_CFG_ERR_INT, 1);
        bk_rott_isr_register(ROTATE_COMPLETE_INT, rotate_complete_cb);
        bk_rott_isr_register(ROTATE_CFG_ERR_INT, rotate_cfg_err_cb);
#endif
    }
    else
    {
        ret = software_rotate_task_open();
    }
    return ret;
}

bk_err_t bk_hw_rotate_yuv2rgb565(frame_buffer_t *src, frame_buffer_t *dst, media_rotate_t rotate)
{
    bk_err_t ret = BK_OK;

#ifdef CONFIG_HW_ROTATE_PFC
    rott_config_t rott_cfg = {0};

    dst->fmt = PIXEL_FMT_RGB565_LE;
    rott_cfg.input_addr = src->frame;
    rott_cfg.output_addr = dst->frame;
    rott_cfg.rot_mode = rotate;

    switch (src->fmt)
    {
        case PIXEL_FMT_YUYV:
            rott_cfg.input_fmt = src->fmt;
            rott_cfg.input_flow = ROTT_INPUT_NORMAL;
            rott_cfg.output_flow = ROTT_OUTPUT_NORMAL;
            break;
        case PIXEL_FMT_VUYY:
            rott_cfg.input_fmt = src->fmt;
            rott_cfg.input_flow = ROTT_INPUT_NORMAL;
            rott_cfg.output_flow = ROTT_OUTPUT_NORMAL;
            break;
        case PIXEL_FMT_RGB565_LE:
            rott_cfg.input_fmt = src->fmt;
            rott_cfg.input_flow = ROTT_INPUT_REVESE_HALFWORD_BY_HALFWORD;
            rott_cfg.output_flow = ROTT_OUTPUT_NORMAL;
            break;
        case PIXEL_FMT_RGB565:
        default:
            rott_cfg.input_fmt = src->fmt;
            rott_cfg.input_flow = ROTT_INPUT_REVESE_HALFWORD_BY_HALFWORD;
            rott_cfg.output_flow = ROTT_OUTPUT_NORMAL;
            break;
    }
    rott_cfg.picture_xpixel = src->width;
    rott_cfg.picture_ypixel = src->height;
    //  rott_cfg.block_xpixel = ROTT_XBLOCK;
    //  rott_cfg.block_ypixel = ROTT_YBLOCK
    //  rott_cfg.block_cnt = ROTT_BLOCK_NUM;
    ret = rott_config(&rott_cfg);
    if (ret != BK_OK)
    {
        LOGE(" rott_config ERR\n");
    }
    bk_rott_enable();

    ret = rtos_get_semaphore(&s_rot.rot_sem, BEKEN_NEVER_TIMEOUT);

    if (ret != BK_OK)
    {
        LOGE("%s semaphore get failed: %d\n", __func__, ret);
    }
#endif
    return ret;
}

static int rotate_flag = 0;
void lcd_sw_rotate_callback(uint8_t result)
{
    bk_err_t ret = BK_OK;
    if (result != BK_OK)
    {
        LOGE("%s sw rotate failed %d\n", __func__, ret);
    }
    rotate_flag ++;
    if (rotate_flag == 2)
    {
        ret = rtos_set_semaphore(&s_rot.rot_sem);

        if (ret != BK_OK)
        {
            LOGE("%s semaphore set failed: %d\n", __func__, ret);
        }
    }
}

bk_err_t bk_sw_rotate(frame_buffer_t *src, frame_buffer_t *dst, uint8_t rotate)
{
    bk_err_t ret = BK_OK;

#if 1
    register uint16_t src_width, src_height;

    frame_buffer_t *decoder_frame = (frame_buffer_t *)src;
    frame_buffer_t *rotate_frame = (frame_buffer_t *)dst;

    int (*func)(unsigned char *vuyy, unsigned char *rotatedVuyy, int width, int height);
    src_width = decoder_frame->width;
    src_height = decoder_frame->height;

    register uint8_t *dst_frame_temp = rotate_frame->frame ;
    register uint8_t *src_frame_temp = decoder_frame->frame;

#if  CONFIG_CACHE_ENABLE
    flush_dcache(src_frame_temp, decoder_frame->length);
    flush_dcache(dst_frame_temp, rotate_frame->length);
#endif

    switch (rotate_frame->fmt)
    {
        case PIXEL_FMT_VUYY:

            if (rotate == ROTATE_90)
            {
                func = vuyy_rotate_degree90_to_yuyv;
            }
            else
            {
                func = vuyy_rotate_degree270_to_yuyv;
            }

            rotate_frame->fmt = PIXEL_FMT_YUYV;
            break;
        case PIXEL_FMT_YUYV:
        default:
            if (rotate == ROTATE_90)
            {
                func = yuyv_rotate_degree90_to_yuyv;
            }
            else
            {
                func = yuyv_rotate_degree270_to_yuyv;
            }
            rotate_frame->fmt = PIXEL_FMT_YUYV;
            break;
    }

#if  CONFIG_CACHE_ENABLE
    flush_dcache(src_frame_temp, decoder_frame->length);
    flush_dcache(dst_frame_temp, rotate_frame->length);
#endif

    func(src_frame_temp, dst_frame_temp, src_width, src_height);
#else
    rotate_flag = 0;

    media_software_rotate_info_t info = {0};
    media_software_rotate_info_t info_2 = {0};
    info_2.src_yuv = info.src_yuv = src;
    info_2.dst_yuv = info.dst_yuv = dst;
    info_2.rot_angle = info.rot_angle = rotate;
    info_2.cb = info.cb = &lcd_sw_rotate_callback;
    info.start_line = 0;
    info.end_line = src->height >> 1;
    ret = software_rotate_task_send_msg(EVENT_YUV_ROTATE_START_NOTIFY, (uint32_t)&info);

    info_2.start_line = src->height >> 1;
    info_2.end_line = src->height;

//    ret = software_rotate_task_send_msg(EVENT_YUV_ROTATE_START_NOTIFY, (uint32_t)&info_2);
    media_mailbox_msg_t msg = {0};
    msg.event = EVENT_YUV_ROTATE_START_NOTIFY;
    msg.param = (uint32_t)&info_2;
    msg_send_notify_to_media_major_mailbox(&msg, MINOR_MODULE);
    ret = rtos_get_semaphore(&s_rot.rot_sem, BEKEN_NEVER_TIMEOUT);
    if (ret != BK_OK)
    {
        LOGE("%s semaphore get failed: %d\n", __func__, ret);
    }

#endif
    return ret;
}



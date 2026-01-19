#include "os/os.h"
#include "os/mem.h"
#include "modules/jpeg_decode_sw.h"
#include "driver/lcd.h"
#include "driver/media_types.h"
#include "driver/psram.h"
#include "driver/jpeg_dec.h"
#include "driver/jpeg_dec_types.h"
#include "driver/dma2d.h"
#include "lv_jpeg_hw_decode.h"
#include "lvgl.h"
#include "mux_pipeline.h"


static beken_semaphore_t g_hw_decode_sem = NULL;
static beken2_timer_t g_hw_decode_timer;
static u8 *g_dec_frame_data = NULL;
static beken_semaphore_t lv_dma2d_sem = NULL;
static JPEG_HW_OUTPUT_FMT_T g_jpeg_output_fmt = JH_OUTPUT_RGB565;

static void lv_jpeg_hw_dec_err_cb(jpeg_dec_res_t *result)
{
    bk_err_t ret = BK_FAIL;

    rtos_stop_oneshot_timer(&g_hw_decode_timer);

    BK_LOGD(NULL, "%s:%x \n", __func__, result->ok);
    ret = rtos_set_semaphore(&g_hw_decode_sem);
    if (ret != BK_OK)
    {
        BK_LOGD(NULL, "%s semaphore set failed: %d\n", __func__, ret);
    }
}

static void lv_jpeg_hw_dec_eof_cb(jpeg_dec_res_t *result)
{
    bk_err_t ret = BK_FAIL;

    rtos_stop_oneshot_timer(&g_hw_decode_timer);

    if (result->ok == false)
    {
        BK_LOGD(NULL, "%s decoder error\n", __func__);
    }

    ret = rtos_set_semaphore(&g_hw_decode_sem);
    if (ret != BK_OK)
    {
        BK_LOGD(NULL, "%s semaphore set failed: %d\n", __func__, ret);
    }
}

static void lv_jpeg_hw_dec_timeout(void *Larg, void *Rarg)
{
    bk_err_t ret = BK_FAIL;

    BK_LOGD(NULL, "%s \n", __func__);
    ret = rtos_set_semaphore(&g_hw_decode_sem);
    if (ret != BK_OK)
    {
        BK_LOGD(NULL, "%s semaphore set failed: %d\n", __func__, ret);
    }
}

static void lv_dma2d_config_error(void)
{
    BK_LOGD(NULL, "%s \n", __func__);
}

static void lv_dma2d_transfer_error(void)
{
    BK_LOGD(NULL, "%s \n", __func__);
}

static void lv_dma2d_transfer_complete(void)
{
    rtos_set_semaphore(&lv_dma2d_sem);
}

bk_err_t lv_dma2d_yuyv2rgb565_init(void)
{
    bk_err_t ret;

    ret = rtos_init_semaphore_ex(&lv_dma2d_sem, 1, 0);
    if (BK_OK != ret) {
        BK_LOGD(NULL, "%s %d lv_dma2d_sem init failed\n", __func__, __LINE__);
        return ret;
    }

    bk_dma2d_driver_init();
    bk_dma2d_register_int_callback_isr(DMA2D_CFG_ERROR_ISR, lv_dma2d_config_error);
    bk_dma2d_register_int_callback_isr(DMA2D_TRANS_ERROR_ISR, lv_dma2d_transfer_error);
    bk_dma2d_register_int_callback_isr(DMA2D_TRANS_COMPLETE_ISR, lv_dma2d_transfer_complete);
    bk_dma2d_int_enable(DMA2D_CFG_ERROR | DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE, 1);

    return ret;
}

bk_err_t lv_dma2d_yuyv2rgb565_deinit(void)
{
    bk_err_t ret;

    bk_dma2d_int_enable(DMA2D_CFG_ERROR | DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE, 0);
    bk_dma2d_driver_deinit();
    ret = rtos_deinit_semaphore(&lv_dma2d_sem);
    if (BK_OK != ret) {
        BK_LOGD(NULL, "%s %d lv_dma2d_sem deinit failed\n", __func__, __LINE__);
    }

    return ret;
}

static void lv_dma2d_yuyv2rgb565(void *src, const void *dst, uint16_t width, uint16_t height)
{
    dma2d_memcpy_pfc_t dma2d_memcpy_pfc = {0};

    dma2d_memcpy_pfc.input_addr = (char *)src;
    dma2d_memcpy_pfc.output_addr = (char *)dst;
    dma2d_memcpy_pfc.mode = DMA2D_M2M_PFC;
    dma2d_memcpy_pfc.input_color_mode = DMA2D_INPUT_YUYV;
    dma2d_memcpy_pfc.output_color_mode = DMA2D_OUTPUT_RGB565;
    dma2d_memcpy_pfc.src_pixel_byte = TWO_BYTES;
    dma2d_memcpy_pfc.dst_pixel_byte = TWO_BYTES;
    dma2d_memcpy_pfc.dma2d_width = width;
    dma2d_memcpy_pfc.dma2d_height = height;
    dma2d_memcpy_pfc.src_frame_width = width;
    dma2d_memcpy_pfc.src_frame_height = height;
    dma2d_memcpy_pfc.dst_frame_width = width;
    dma2d_memcpy_pfc.dst_frame_height = height;
    dma2d_memcpy_pfc.src_frame_xpos = 0;
    dma2d_memcpy_pfc.src_frame_ypos = 0;
    dma2d_memcpy_pfc.dst_frame_xpos = 0;
    dma2d_memcpy_pfc.dst_frame_ypos = 0;
    dma2d_memcpy_pfc.input_red_blue_swap = 0;
    dma2d_memcpy_pfc.output_red_blue_swap = 0;

    bk_dma2d_memcpy_or_pixel_convert(&dma2d_memcpy_pfc);
    bk_dma2d_start_transfer();

    rtos_get_semaphore(&lv_dma2d_sem, BEKEN_NEVER_TIMEOUT);
}

s32 lv_jpeg_hw_decode(frame_buffer_t *jpeg_frame, lv_img_dsc_t *img_dst)
{
    bk_err_t ret = BK_FAIL;
    int flag = 0;

    do {
        ret = rtos_init_semaphore_ex(&g_hw_decode_sem, 1, 0);
        if (ret != BK_OK)
        {
            BK_LOGD(NULL, "[%s][%d] init sem fail\n", __FUNCTION__, __LINE__);
            break;
        }

        if (!rtos_is_oneshot_timer_init(&g_hw_decode_timer))
        {
            ret = rtos_init_oneshot_timer(&g_hw_decode_timer, 1000, lv_jpeg_hw_dec_timeout, NULL, NULL);
            if (ret != BK_OK)
            {
                BK_LOGD(NULL, "[%s][%d] create timer fail\n", __FUNCTION__, __LINE__);
                break;
            }

            ret = rtos_start_oneshot_timer(&g_hw_decode_timer);
            if(ret != BK_OK)
            {
                BK_LOGD(NULL, "[%s][%d] start timer fail\n", __FUNCTION__, __LINE__);
                break;
            }
        }
        else
        {
            ret = rtos_oneshot_reload_timer(&g_hw_decode_timer);
            if(ret != BK_OK)
            {
                BK_LOGD(NULL, "[%s][%d] reload timer fail\n", __FUNCTION__, __LINE__);
                break;
            }
        }

        g_dec_frame_data = psram_malloc(img_dst->data_size);
        if (!g_dec_frame_data)
        {
            BK_LOGD(NULL, "[%s][%d] malloc psram size %d fail\r\n", __FUNCTION__, __LINE__, img_dst->data_size);
            ret = BK_ERR_NO_MEM;
            break;
        }

        bk_jpeg_dec_driver_init();
        bk_jpeg_dec_isr_register(DEC_ERR, lv_jpeg_hw_dec_err_cb);
        bk_jpeg_dec_isr_register(DEC_END_OF_FRAME, lv_jpeg_hw_dec_eof_cb);
        bk_jpeg_dec_out_format(PIXEL_FMT_YUYV);

        ret = bk_jpeg_dec_hw_start(jpeg_frame->length, jpeg_frame->frame, g_dec_frame_data);
        if (ret != BK_OK)
        {
            BK_LOGD(NULL, "%s hw decode start fail %d\n", __func__, ret);
            break;
        }

        ret = rtos_get_semaphore(&g_hw_decode_sem, 1000);
        if (ret != BK_OK)
        {
            BK_LOGD(NULL, "%s semaphore get failed: %d\n", __func__, ret);
            break;
        }

        if (JH_OUTPUT_RGB565 == g_jpeg_output_fmt) {
            if(img_dst->data == NULL)
            {
                img_dst->data = psram_malloc(img_dst->data_size);
                if (!img_dst->data)
                {
                    BK_LOGD(NULL, "[%s][%d] malloc psram size %d fail\r\n", __FUNCTION__, __LINE__, img_dst->data_size);
                    ret = BK_ERR_NO_MEM;
                    break;
                }

                flag = 1;
            }

            lv_dma2d_yuyv2rgb565(g_dec_frame_data, img_dst->data, img_dst->header.w, img_dst->header.h);
        } else if (JH_OUTPUT_YUYV == g_jpeg_output_fmt) {
            img_dst->data = g_dec_frame_data;
        }

        img_dst->header.always_zero = 0;
        img_dst->header.cf = LV_IMG_CF_TRUE_COLOR;
        ret = BK_OK;
    }while(0);

    if (rtos_is_oneshot_timer_init(&g_hw_decode_timer))
    {
        rtos_deinit_oneshot_timer(&g_hw_decode_timer);
    }

    if (g_hw_decode_sem)
    {
        rtos_deinit_semaphore(&g_hw_decode_sem);
        g_hw_decode_sem = NULL;
    }

    if (g_dec_frame_data && JH_OUTPUT_RGB565 == g_jpeg_output_fmt)
    {
        psram_free(g_dec_frame_data);
        g_dec_frame_data  = NULL;
    }

    bk_jpeg_dec_stop();
    bk_jpeg_dec_driver_deinit();

    if (BK_OK != ret)
    {
        if (flag)
        {
            psram_free((void *)img_dst->data);
            img_dst->data = NULL;
        }
    }

    return ret;
}

void bk_jpeg_hw_decode_to_mem_init(void)
{
    bk_err_t ret = BK_FAIL;

    ret = rtos_init_semaphore_ex(&g_hw_decode_sem, 1, 0);
    if (ret != BK_OK)
    {
        BK_LOGD(NULL, "[%s][%d] init sem fail\n", __FUNCTION__, __LINE__);
        return;
    }

    ret = rtos_init_oneshot_timer(&g_hw_decode_timer, 1000, lv_jpeg_hw_dec_timeout, NULL, NULL);
    if (ret != BK_OK)
    {
        BK_LOGD(NULL, "[%s][%d] create timer fail\n", __FUNCTION__, __LINE__);
        return;
    }

    bk_jpeg_dec_driver_init();
    bk_jpeg_dec_isr_register(DEC_ERR, lv_jpeg_hw_dec_err_cb);
    bk_jpeg_dec_isr_register(DEC_END_OF_FRAME, lv_jpeg_hw_dec_eof_cb);
    bk_jpeg_dec_out_format(PIXEL_FMT_YUYV);

    lv_dma2d_yuyv2rgb565_init();
}

void bk_jpeg_hw_decode_to_mem_deinit(void)
{
    bk_err_t ret = BK_FAIL;

    bk_jpeg_dec_stop();
    bk_jpeg_dec_driver_deinit();

    bk_dma2d_stop_transfer();
    lv_dma2d_yuyv2rgb565_deinit();

    if (rtos_is_oneshot_timer_init(&g_hw_decode_timer))
    {
        ret = rtos_deinit_oneshot_timer(&g_hw_decode_timer);
        if (ret != BK_OK)
        {
            BK_LOGD(NULL, "[%s][%d] deinit timer fail\n", __FUNCTION__, __LINE__);
            return;
        }
    }

    if (g_hw_decode_sem)
    {
        ret = rtos_deinit_semaphore(&g_hw_decode_sem);
        if (ret != BK_OK)
        {
            BK_LOGD(NULL, "[%s][%d] deint sem fail\n", __FUNCTION__, __LINE__);
            return;
        }
        g_hw_decode_sem = NULL;
    }
}

bk_err_t bk_jpeg_hw_decode_to_mem(uint8_t *src_addr, uint8_t *dst_addr, uint32_t src_size, uint16_t dst_width, uint16_t dst_height)
{
    bk_err_t ret = BK_FAIL;

    g_dec_frame_data = psram_malloc(dst_width * dst_height * 2);
    if (!g_dec_frame_data)
    {
        BK_LOGD(NULL, "[%s][%d] malloc psram fail\r\n", __FUNCTION__, __LINE__);
        ret = BK_ERR_NO_MEM;
        return ret;
    }

    do {
        if (!rtos_is_oneshot_timer_init(&g_hw_decode_timer)) {
            ret = rtos_start_oneshot_timer(&g_hw_decode_timer);
            if(ret != BK_OK)
            {
                BK_LOGD(NULL, "[%s][%d] start timer fail\n", __FUNCTION__, __LINE__);
                break;
            }
        } else {
            ret = rtos_oneshot_reload_timer(&g_hw_decode_timer);
            if(ret != BK_OK)
            {
                BK_LOGD(NULL, "[%s][%d] reload timer fail\n", __FUNCTION__, __LINE__);
                break;
            }
        }

        ret = bk_jpeg_dec_hw_start(src_size, src_addr, g_dec_frame_data);
        if (ret != BK_OK)
        {
            BK_LOGD(NULL, "%s hw decode start fail %d\n", __func__, ret);
            break;
        }

        ret = rtos_get_semaphore(&g_hw_decode_sem, 1000);
        if (ret != BK_OK)
        {
            BK_LOGD(NULL, "%s semaphore get failed: %d\n", __func__, ret);
            break;
        }

        lv_dma2d_yuyv2rgb565(g_dec_frame_data, dst_addr, dst_width, dst_height);
    }while(0);

    if (g_dec_frame_data)
    {
        psram_free(g_dec_frame_data);
        g_dec_frame_data = NULL;
    }

    return ret;

}

void lv_jpeg_hw_decode_output_fmt_set(JPEG_HW_OUTPUT_FMT_T jpeg_output_fmt)
{
    g_jpeg_output_fmt = jpeg_output_fmt;
}


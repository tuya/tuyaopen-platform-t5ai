/**
 * @file tkl_codec.c
 * @brief default weak implements of tuya pin
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */
#include "tkl_image_codec.h"
#include "sdkconfig.h"

#include "driver/dma.h"
#include "driver/yuv_buf.h"
#include <driver/dma2d.h>
#include <driver/jpeg_dec.h>
#include <modules/jpeg_decode_sw.h>
#include <modules/tjpgd.h>

#define CPU_FREQ_320M     (5)
#define CODE_TYPE_NUM      (4)

static uint8_t g_curr_irq_type = 0;
static TKL_IMAGE_CODEC_EVT_CB sg_event_cb = NULL; 

#if 0
static jpeg_dec_handle_t jpeg_dec_handle;
extern jd_workbuf_t jpeg_decode_workbuf_cp1;
#endif

typedef struct
{
    uint8_t irq_enable;
    TUYA_IMAGE_CODEC_IRQ_T irq_cfg;
} TUYA_IMAGE_IRQ_T;

TUYA_IMAGE_IRQ_T image_irq[CODE_TYPE_NUM] = {0};

static void tkl_dma2d_config_error(void)
{
    bk_printf("%s, trigger dma2d config error\n", __func__);
    if(sg_event_cb) {
        sg_event_cb(TKL_IMAGE_EVT_CONFIG_TIMEOUT);
    }
}

static void tkl_dma2d_transfer_error(void)
{
    bk_printf("%s, trigger dma2d transfer error\n", __func__);

    if(sg_event_cb) {
        sg_event_cb(TKL_IMAGE_EVT_TRANS_ERROR);
    }
}

static void tkl_dma2d_transfer_complete(void)
{
    if(sg_event_cb) {
        sg_event_cb(TKL_IMAGE_EVT_TRANS_COMPLETE);
    }
}

static OPERATE_RET __dma2d_init(void)
{
    OPERATE_RET ret = OPRT_OK;

    ret = bk_dma2d_driver_init();
    if (ret != BK_OK)
    {
        ret = OPRT_COM_ERROR;
        return ret;
    }

    bk_dma2d_register_int_callback_isr(DMA2D_CFG_ERROR_ISR, tkl_dma2d_config_error);
    bk_dma2d_register_int_callback_isr(DMA2D_TRANS_ERROR_ISR, tkl_dma2d_transfer_error);
    bk_dma2d_register_int_callback_isr(DMA2D_TRANS_COMPLETE_ISR, tkl_dma2d_transfer_complete);
    bk_dma2d_int_enable(DMA2D_CFG_ERROR | DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE, 1);

    return ret;
}

static OPERATE_RET __jpeg_dec_sw_init(void)
{
	OPERATE_RET ret = 0;
#if 0
	ret = bk_jpeg_dec_sw_init_by_handle(&jpeg_dec_handle,
						(uint8_t *)&jpeg_decode_workbuf_cp1, sizeof(jd_workbuf_t));

	ret = bk_jpeg_dec_sw_init_by_handle(&jpeg_dec_handle,
						NULL, 0);
    printf(">>> %s init, ret %d\r\n", __func__, ret);
#endif
	return ret;
}

OPERATE_RET tkl_image_codec_init(TKL_CODEC_BASE_CFG_T *para)
{
    OPERATE_RET ret = OPRT_OK;
    if (para == NULL)
    {
        return OPRT_INVALID_PARM;
    }

    sg_event_cb = para->cb;

    switch (para->type)
    {
    case TKL_IMAGE_CODEC_TYPE_DMA2D:
        ret = __dma2d_init();
        break;
    case TKL_IMAGE_CODEC_TYPE_JPEG:
        ret = __jpeg_dec_sw_init();
    default:
        ret = OPRT_COM_ERROR;
        break;
    }

    return ret;
}

static OPERATE_RET __dma2d_deinit(void)
{
    bk_dma2d_stop_transfer();
    bk_dma2d_int_enable(DMA2D_CFG_ERROR | DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE, 0);
    bk_dma2d_driver_deinit();

    return OPRT_OK;
}

OPERATE_RET tkl_image_codec_deinit(TKL_IMAGE_CODEC_INIT_E type)
{
    OPERATE_RET ret = OPRT_OK;
    switch (type)
    {
    case TKL_IMAGE_CODEC_TYPE_DMA2D:
        ret = __dma2d_init();
        break;
    default:
        ret = OPRT_COM_ERROR;
        break;
    }

    return ret;
}

OPERATE_RET tkl_image_codec_convert(TKL_CODEC_FRAME_T *in_frame, TKL_CODEC_FRAME_T *out_frame)
{
    OPERATE_RET ret = 0;
    if (in_frame == NULL || out_frame == NULL || in_frame->pbuf == NULL || out_frame->pbuf == NULL)
    {
        return OPRT_INVALID_PARM;
    }

    if ((in_frame->width != out_frame->width) || (in_frame->height != out_frame->height))
    {
        bk_printf("%s the src img size(%d, %d) != dest img size(%d, %d)\r\n", __func__,
            in_frame->width, in_frame->height, out_frame->width, out_frame->height);
        return OPRT_INVALID_PARM;
    }

    if (in_frame->type == TKL_CODEC_YUV422)
    {
        switch (out_frame->type)
        {
        case TKL_CODEC_RGB565:
            yuyv_to_rgb565_convert((UINT8_T *)in_frame->pbuf, (UINT8_T *)out_frame->pbuf, 
                in_frame->width, in_frame->height);
            break;
        default:
            bk_printf("%s not support type(%d) convert to type(%d)\r\n", __func__, in_frame->type, out_frame->type);
            return OPRT_INVALID_PARM;
        }
    }

    if (in_frame->type == TKL_CODEC_RGB565)
    {
        switch (out_frame->type)
        {
        case TKL_CODEC_YUV422:
            rgb565_to_yuyv_convert((UINT8_T *)in_frame->pbuf, (UINT8_T *)out_frame->pbuf, 
                in_frame->width, in_frame->height);
            break;
        default:
            bk_printf("%s not support type(%d) convert to type(%d)\r\n", __func__, in_frame->type, out_frame->type);
            return OPRT_INVALID_PARM;
        }
    }

#if 0
    if (in_frame->type == TKL_CODEC_JPEG)
    {
        sw_jpeg_dec_res_t result;
        ret = bk_jpeg_get_img_info(in_frame->size, in_frame->pbuf, &result, NULL);

        switch (out_frame->type)
        {
        case TKL_CODEC_YUV422:
            jd_set_format_by_handle(jpeg_dec_handle, JD_FORMAT_YUYV);
            break;
        case TKL_CODEC_RGB565:
            jd_set_format_by_handle(jpeg_dec_handle, JD_FORMAT_RGB565);
            break;
        case TKL_CODEC_RGB888:
            jd_set_format_by_handle(jpeg_dec_handle, JD_FORMAT_RGB888);
            break;
        default:
            bk_printf("%s not support type(%d) convert to type(%d)\r\n", __func__, in_frame->type, out_frame->type);
            return OPRT_INVALID_PARM;
        }
        printf("%s in_frame->pbuf %p, out_frame->pbuf %p, in_frame->size %d, out_frame->size %d\r\n", 
            __func__, in_frame->pbuf, out_frame->pbuf, in_frame->size, out_frame->size);
        ret = bk_jpeg_dec_sw_start_by_handle(jpeg_dec_handle, JPEGDEC_BY_FRAME, in_frame->pbuf, out_frame->pbuf,
                    in_frame->size, out_frame->size, &result);
    }
#endif

    return OPRT_OK;
}

static OPERATE_RET __dma2d_memcpy_cfg_init(TKL_CODEC_FRAME_T *in_frame, TKL_CODEC_FRAME_T *out_frame, dma2d_memcpy_pfc_t *dma2d_memcpy_cfg)
{
    switch (in_frame->type)
    {
    case TKL_CODEC_YUV422:
        dma2d_memcpy_cfg->input_color_mode = DMA2D_INPUT_YUYV;
        dma2d_memcpy_cfg->src_pixel_byte = TWO_BYTES;
        dma2d_memcpy_cfg->output_color_mode = DMA2D_OUTPUT_YUYV;
        dma2d_memcpy_cfg->dst_pixel_byte = TWO_BYTES;
        break;
    case TKL_CODEC_RGB565:
        dma2d_memcpy_cfg->input_color_mode = DMA2D_INPUT_RGB565;
        dma2d_memcpy_cfg->src_pixel_byte = TWO_BYTES;
        dma2d_memcpy_cfg->output_color_mode  = DMA2D_OUTPUT_RGB565;
        dma2d_memcpy_cfg->dst_pixel_byte = TWO_BYTES;
        break;
    default:
        bk_printf("%s not support type(%d) frame cpy\r\n", __func__, in_frame->type);
        return OPRT_INVALID_PARM;
    }

	dma2d_memcpy_cfg->dma2d_width = in_frame->width;
	dma2d_memcpy_cfg->dma2d_height = in_frame->height;
	dma2d_memcpy_cfg->src_frame_width = in_frame->width;
	dma2d_memcpy_cfg->src_frame_height = in_frame->height;
    dma2d_memcpy_cfg->src_frame_xpos = 0;
	dma2d_memcpy_cfg->src_frame_ypos = 0;
	dma2d_memcpy_cfg->dst_frame_width = out_frame->width;
	dma2d_memcpy_cfg->dst_frame_height = out_frame->height;
	dma2d_memcpy_cfg->dst_frame_xpos = out_frame->xpos;
	dma2d_memcpy_cfg->dst_frame_ypos = out_frame->ypos;

    dma2d_memcpy_cfg->input_addr = (void *)in_frame->pbuf;
    dma2d_memcpy_cfg->output_addr = (void *)out_frame->pbuf;

	dma2d_memcpy_cfg->mode = DMA2D_M2M;

    return OPRT_OK;
}

OPERATE_RET tkl_image_codec_memcopy(TKL_CODEC_FRAME_T *in_frame, TKL_CODEC_FRAME_T *out_frame)
{
    if (in_frame == NULL || out_frame == NULL)
    {
        return OPRT_INVALID_PARM;
    }

    if (in_frame->type != out_frame->type)
    {
        bk_printf("%s the src img fmt(%d) != dest img fmt(%d)\r\n", __func__,
            in_frame->type, out_frame->type);
        return OPRT_INVALID_PARM;
    }

    OPERATE_RET ret = OPRT_OK;
	dma2d_memcpy_pfc_t dma2d_memcpy_pfc = {0};   
    ret = __dma2d_memcpy_cfg_init(in_frame, out_frame, &dma2d_memcpy_pfc);
    if (ret)
        OPRT_INVALID_PARM;

	bk_dma2d_memcpy_or_pixel_convert(&dma2d_memcpy_pfc);
    bk_dma2d_start_transfer();

    return OPRT_OK;
}

OPERATE_RET tkl_image_codec_irq_init(TKL_IMAGE_CODEC_INIT_E type, const TUYA_IMAGE_CODEC_IRQ_T *cfg)
{
    image_irq[type].irq_cfg.cb = cfg->cb;
    image_irq[type].irq_cfg.arg = cfg->arg;
    image_irq[type].irq_enable = 0;
    return OPRT_OK;
}

OPERATE_RET tkl_image_codec_irq_enable(TKL_IMAGE_CODEC_INIT_E handle)
{
    image_irq[handle].irq_enable = 1;
    return OPRT_OK;
}

OPERATE_RET tkl_image_codec_irq_disable(TKL_IMAGE_CODEC_INIT_E handle)
{
    image_irq[handle].irq_enable = 0;
    return OPRT_OK;
}
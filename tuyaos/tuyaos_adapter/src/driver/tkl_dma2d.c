#include "tkl_dma2d.h"
#include "sdkconfig.h"

#include "driver/dma.h"
#include <driver/dma2d.h>

typedef struct
{
    TUYA_DMA2D_IRQ_CB dma2d_isr_cb;
    void *arg;
} TUYA_DMA2D_MANAGE_T;

TUYA_DMA2D_MANAGE_T g_dma2d_manage;

static void tkl_dma2d_config_error(void)
{
    bk_printf("%s, trigger dma2d config error\n", __func__);
}

static void tkl_dma2d_transfer_error(void)
{
    if (g_dma2d_manage.dma2d_isr_cb)
        g_dma2d_manage.dma2d_isr_cb(TUYA_DMA2D_TRANS_ERROR_ISR, g_dma2d_manage.arg);
}

static void tkl_dma2d_transfer_complete(void)
{
    bk_dma2d_int_status_clear(DMA2D_TRANS_COMPLETE_STATUS);

    if (g_dma2d_manage.dma2d_isr_cb)
        g_dma2d_manage.dma2d_isr_cb(TUYA_DMA2D_TRANS_COMPLETE_ISR, g_dma2d_manage.arg);
}

OPERATE_RET tkl_dma2d_init(CONST TUYA_DMA2D_BASE_CFG_T *cfg)
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

    if (cfg->cb)
    {
        g_dma2d_manage.dma2d_isr_cb = cfg->cb;
        g_dma2d_manage.arg = cfg->arg;
    }

    return ret;
}

OPERATE_RET tkl_dma2d_deinit()
{
    bk_dma2d_stop_transfer();
    bk_dma2d_int_enable(DMA2D_CFG_ERROR | DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE, 0);
    bk_dma2d_driver_deinit();

    g_dma2d_manage.dma2d_isr_cb = NULL;
    g_dma2d_manage.arg = NULL;

    return OPRT_OK;
}

static OPERATE_RET __dma2d_cfg_init(TKL_DMA2D_FRAME_INFO_T *in_frame, TKL_DMA2D_FRAME_INFO_T *out_frame, dma2d_memcpy_pfc_t *dma2d_cfg)
{
    if (in_frame->axis.x_axis >= in_frame->width || in_frame->axis.y_axis >= in_frame->height
        || in_frame->axis.x_axis >= out_frame->width || in_frame->axis.y_axis >= out_frame->height)
        return OPRT_INVALID_PARM;

    if (out_frame->axis.x_axis >= out_frame->width || out_frame->axis.y_axis >= out_frame->height)
        return OPRT_INVALID_PARM;

    switch (in_frame->type)
    {
    case TUYA_FRAME_FMT_YUV422:
        dma2d_cfg->input_color_mode = DMA2D_INPUT_YUYV;
        dma2d_cfg->src_pixel_byte = TWO_BYTES;
        break;
    case TUYA_FRAME_FMT_RGB565:
        dma2d_cfg->input_color_mode = DMA2D_INPUT_RGB565;
        dma2d_cfg->src_pixel_byte = TWO_BYTES;
        break;
    case TUYA_FRAME_FMT_RGB888:
        dma2d_cfg->input_color_mode = DMA2D_INPUT_RGB888;
        dma2d_cfg->src_pixel_byte = THREE_BYTES;
        break;
    default:
        bk_printf("%s not support type(%d) frame cpy\r\n", __func__, in_frame->type);
        return OPRT_INVALID_PARM;
    }

    switch (out_frame->type)
    {
    case TUYA_FRAME_FMT_YUV422:
        dma2d_cfg->output_color_mode = DMA2D_OUTPUT_YUYV;
        dma2d_cfg->dst_pixel_byte = TWO_BYTES;
        break;
    case TUYA_FRAME_FMT_RGB565:
        dma2d_cfg->output_color_mode  = DMA2D_OUTPUT_RGB565;
        dma2d_cfg->dst_pixel_byte = TWO_BYTES;
        break;
    case TUYA_FRAME_FMT_RGB888:
        dma2d_cfg->output_color_mode  = DMA2D_OUTPUT_RGB888;
        dma2d_cfg->dst_pixel_byte = THREE_BYTES;
        break;
    default:
        bk_printf("%s not support type(%d) frame cpy\r\n", __func__, in_frame->type);
        return OPRT_INVALID_PARM;
    }

	dma2d_cfg->src_frame_width = in_frame->width;
	dma2d_cfg->src_frame_height = in_frame->height;

    dma2d_cfg->src_frame_xpos = in_frame->axis.x_axis;
	dma2d_cfg->src_frame_ypos = in_frame->axis.y_axis;

    dma2d_cfg->dst_frame_width = out_frame->width;
	dma2d_cfg->dst_frame_height =  out_frame->height;

    dma2d_cfg->dst_frame_xpos = out_frame->axis.x_axis;
	dma2d_cfg->dst_frame_ypos = out_frame->axis.y_axis;

    dma2d_cfg->input_addr = (UINT8_T *)in_frame->pbuf;
    dma2d_cfg->output_addr = (UINT8_T *)out_frame->pbuf;


    if (in_frame->width_cp == 0 || in_frame->height_cp == 0)
    {
        dma2d_cfg->dma2d_width = in_frame->width;
        dma2d_cfg->dma2d_height = in_frame->height;
        goto out;
    }

    UINT16_T dst_frame_xpos_eof = dma2d_cfg->dst_frame_xpos + in_frame->width_cp;
    if (dst_frame_xpos_eof > out_frame->width)
        dma2d_cfg->dma2d_width = out_frame->width - dma2d_cfg->dst_frame_xpos;
    else
        dma2d_cfg->dma2d_width = in_frame->width_cp;

    UINT16_T dst_frame_ypos_eof = dma2d_cfg->dst_frame_ypos + in_frame->height_cp;
    if (dst_frame_ypos_eof > out_frame->height)
        dma2d_cfg->dma2d_height = out_frame->height - dma2d_cfg->dst_frame_xpos;
    else
        dma2d_cfg->dma2d_height = in_frame->height_cp;

out:
    return OPRT_OK;
}

OPERATE_RET tkl_dma2d_convert(TKL_DMA2D_FRAME_INFO_T *src, TKL_DMA2D_FRAME_INFO_T *dst)
{
    if (src == NULL || dst == NULL)
    {
        return OPRT_INVALID_PARM;
    }

    if (src->type == dst->type)
    {
        return tkl_dma2d_memcpy(src, dst);
    }

    OPERATE_RET ret = OPRT_OK;
	dma2d_memcpy_pfc_t dma2d_convert_pfc = {0};
    dma2d_convert_pfc.mode = DMA2D_M2M_PFC;
    ret = __dma2d_cfg_init(src, dst, &dma2d_convert_pfc);
    if (ret)
        OPRT_INVALID_PARM;

	bk_dma2d_memcpy_or_pixel_convert(&dma2d_convert_pfc);
    bk_dma2d_start_transfer();

    return OPRT_OK;
}

OPERATE_RET tkl_dma2d_memcpy(TKL_DMA2D_FRAME_INFO_T *src, TKL_DMA2D_FRAME_INFO_T *dst)
{
    if (src == NULL || dst == NULL)
    {
        return OPRT_INVALID_PARM;
    }

    if (src->type != dst->type)
    {
        return tkl_dma2d_convert(src, dst);
    }

    OPERATE_RET ret = OPRT_OK;
	dma2d_memcpy_pfc_t dma2d_memcpy_pfc = {0};
    dma2d_memcpy_pfc.mode = DMA2D_M2M;
    ret = __dma2d_cfg_init(src, dst, &dma2d_memcpy_pfc);
    if (ret)
        OPRT_INVALID_PARM;

	bk_dma2d_memcpy_or_pixel_convert(&dma2d_memcpy_pfc);
    bk_dma2d_start_transfer();

    return OPRT_OK;
}
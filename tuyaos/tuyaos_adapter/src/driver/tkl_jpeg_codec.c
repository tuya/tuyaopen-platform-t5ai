#include "tkl_jpeg_codec.h"
#include "tkl_semaphore.h"
#include <bk_decode.h>
#include <driver/jpeg_dec.h>
#include <modules/jpeg_decode_sw.h>
#include <modules/tjpgd.h>

typedef struct {
    TKL_SEM_HANDLE hw_dec_sem;
    uint8_t hw_dec_inited;
    uint8_t hw_dec_err;
} JPEG_CODEC_MANAGE_T;

JPEG_CODEC_MANAGE_T g_jpeg_codec_manage =
{
    .hw_dec_sem = NULL,
    .hw_dec_inited = FALSE,
    .hw_dec_err = FALSE,
};

static void jpeg_dec_err_cb(jpeg_dec_res_t *result)
{
    g_jpeg_codec_manage.hw_dec_err = TRUE;
    if (g_jpeg_codec_manage.hw_dec_sem)
    {
        tkl_semaphore_post(g_jpeg_codec_manage.hw_dec_sem);
    }
}

static void jpeg_dec_eof_cb(jpeg_dec_res_t *result)
{
    g_jpeg_codec_manage.hw_dec_err = FALSE;

    if (result->ok == FALSE)
    {
        g_jpeg_codec_manage.hw_dec_err = TRUE;
    }

    if (g_jpeg_codec_manage.hw_dec_sem)
    {
        tkl_semaphore_post(g_jpeg_codec_manage.hw_dec_sem);
    }
}

static OPERATE_RET __jpeg_codec_sw_init()
{
    return OPRT_OK;
}

static OPERATE_RET __jpeg_codec_hw_init(JPEG_CODEC_MANAGE_T *codec_manage)
{
    OPERATE_RET ret = OPRT_OK;

    ret = tkl_semaphore_create_init(&codec_manage->hw_dec_sem, 0, 1);
    if (ret)
    {
        bk_printf("%s, create hw_dec_sem failed\r\n", __func__);
        return OPRT_COM_ERROR;
    }
    
    ret = bk_jpeg_dec_driver_init();
    if (ret)
    {
        bk_printf("%s, init hw_dec failed\r\n", __func__);
        return OPRT_COM_ERROR;
    }

    ret |= bk_jpeg_dec_isr_register(DEC_ERR, jpeg_dec_err_cb);
    ret |= bk_jpeg_dec_isr_register(DEC_END_OF_FRAME, jpeg_dec_eof_cb);
    if (ret)
    {
        bk_printf("%s, register isr failed\r\n", __func__);
        return OPRT_COM_ERROR;
    }

    codec_manage->hw_dec_inited = TRUE;

    return OPRT_OK;
}

OPERATE_RET tkl_jpeg_codec_init()
{
    __jpeg_codec_sw_init();

    __jpeg_codec_hw_init(&g_jpeg_codec_manage);

    return OPRT_OK;
}

static OPERATE_RET __jpeg_dec_sw_deinit(void)
{
    return OPRT_OK;
}

static OPERATE_RET __jpeg_dec_hw_deinit(JPEG_CODEC_MANAGE_T *codec_manage)
{
    bk_hw_decode_deinit();

    if (codec_manage->hw_dec_sem)
    {
        tkl_semaphore_release(codec_manage->hw_dec_sem);
        codec_manage->hw_dec_sem = NULL;
    }

    codec_manage->hw_dec_inited = FALSE;
    codec_manage->hw_dec_err = FALSE;

    return OPRT_OK;
}

OPERATE_RET tkl_jpeg_codec_deinit()
{
    OPERATE_RET ret = 0;

    __jpeg_dec_sw_deinit();

    if (g_jpeg_codec_manage.hw_dec_inited)
        __jpeg_dec_hw_deinit(&g_jpeg_codec_manage);

    return ret;
}

OPERATE_RET tkl_jpeg_codec_img_info_get(uint8_t *jpeg_buf, uint32_t jpeg_size, TKL_JPEG_CODEC_INFO_T *jpeg_info)
{
    if (!jpeg_buf || !jpeg_info)
        return OPRT_INVALID_PARM;

    OPERATE_RET ret = OPRT_OK;
    sw_jpeg_dec_res_t result;
    ret = bk_jpeg_get_img_info(jpeg_size, jpeg_buf, &result, NULL);
    if (ret)
    {
        bk_printf("%s %d, get img info fail: %d\r\n", __FUNCTION__, __LINE__, ret);
        return OPRT_COM_ERROR;
    }

    jpeg_info->in_size = jpeg_size;
    jpeg_info->out_width = result.pixel_x;
    jpeg_info->out_height = result.pixel_y;

    return OPRT_OK;
}

OPERATE_RET __jpeg_dec_sw_convert(uint8_t *src_buf, uint8_t *dst_buf, TKL_JPEG_CODEC_INFO_T *jpeg_codec_info, JPEG_DEC_OUT_FMT fmt)
{
    OPERATE_RET ret = OPRT_OK;
    sw_jpeg_dec_res_t result;
    uint32_t out_size = 0;
    JD_FORMAT_OUTPUT output_fmt;
    uint8_t width_must_be_multiple_of_2 = false;
    uint8_t width_must_be_multiple_of_4 = false;

    switch (fmt)
    {
    case JPEG_DEC_OUT_YUV422:
        output_fmt = JD_FORMAT_YUYV;
        out_size = jpeg_codec_info->out_width * jpeg_codec_info->out_height * 2;
        width_must_be_multiple_of_2 = true;
        break;
    case JPEG_DEC_OUT_RGB565:
        output_fmt = JD_FORMAT_RGB565;
        out_size = jpeg_codec_info->out_width * jpeg_codec_info->out_height * 2;
        width_must_be_multiple_of_2 = true;
        break;
    case JPEG_DEC_OUT_RGB888:
        output_fmt = JD_FORMAT_RGB888;
        out_size = jpeg_codec_info->out_width * jpeg_codec_info->out_height * 3;
        width_must_be_multiple_of_4 = true;
        break;
    default:
        bk_printf("%s not support jpeg convert to type(%d)\r\n", __func__, fmt);
        return OPRT_INVALID_PARM;
    }

    if (width_must_be_multiple_of_2 && 
        (jpeg_codec_info->out_width % 2) != 0 )
    {
        bk_printf("[%s][%d] 16 color depth can't support image[%d*%d]\r\n", __func__, __LINE__, 
            jpeg_codec_info->out_width, jpeg_codec_info->out_height);
        return OPRT_COM_ERROR;
    }

    if (width_must_be_multiple_of_4 && 
        (jpeg_codec_info->out_width & 3) != 0 )
    {
        bk_printf("[%s][%d] 24/32 color depth can't support image[%d*%d]\r\n", __func__, __LINE__, 
            jpeg_codec_info->out_width, jpeg_codec_info->out_height);
        return OPRT_COM_ERROR;
    }

    ret = bk_jpeg_dec_sw_start_one_time(JPEGDEC_BY_FRAME, src_buf, dst_buf, 
            jpeg_codec_info->in_size, out_size, (sw_jpeg_dec_res_t *)&result, 0, output_fmt, 0, NULL, NULL);

    return ret;
}

OPERATE_RET __jpeg_dec_hw_convert(uint8_t *src_buf, uint8_t *dst_buf, TKL_JPEG_CODEC_INFO_T *jpeg_codec_info, JPEG_CODEC_MANAGE_T *codec_manage)
{
    bk_err_t ret = OPRT_OK;

    ret |= bk_jpeg_dec_out_format(PIXEL_FMT_YUYV);
    ret |= bk_jpeg_dec_hw_start(jpeg_codec_info->in_size, src_buf, dst_buf);
    if (ret)
    {
        bk_printf("%s, start hw_dec failed\r\n", __func__);
        return OPRT_COM_ERROR;
    }

    if (codec_manage->hw_dec_sem)
    {
        if (tkl_semaphore_wait(codec_manage->hw_dec_sem, 500) != OPRT_OK)
        {
            bk_printf("%s, wait hw_dec_sem timeout\r\n", __func__);
            codec_manage->hw_dec_err = TRUE;
        }
    }

    if (codec_manage->hw_dec_err)
    {
        bk_jpeg_dec_stop();
        bk_printf("%s, hw_dec error\r\n", __func__);
        codec_manage->hw_dec_err = FALSE;
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

OPERATE_RET tkl_jpeg_codec_convert(uint8_t *src_buf, uint8_t *dst_buf, TKL_JPEG_CODEC_INFO_T *jpeg_codec_info, JPEG_DEC_OUT_FMT out_fmt)
{
    OPERATE_RET ret = OPRT_OK;

    if (!src_buf || !dst_buf || !jpeg_codec_info)
        return OPRT_INVALID_PARM;

    if (!jpeg_codec_info->in_size || !jpeg_codec_info->out_height || !jpeg_codec_info->out_width)
    {
        bk_printf("%s, dont know jpeg, please get img info first\r\n", __func__);
        return OPRT_INVALID_PARM;
    }

    if (g_jpeg_codec_manage.hw_dec_inited && out_fmt == JPEG_DEC_OUT_YUV422
        && !(jpeg_codec_info->out_width % 32) && !(jpeg_codec_info->out_height % 8))
        ret = __jpeg_dec_hw_convert(src_buf, dst_buf, jpeg_codec_info, &g_jpeg_codec_manage);
    else
        ret = __jpeg_dec_sw_convert(src_buf, dst_buf, jpeg_codec_info, out_fmt);

    return ret;
}



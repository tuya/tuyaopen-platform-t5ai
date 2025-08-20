#include "tkl_jpeg_codec.h"
#include "tkl_semaphore.h"
#include <driver/jpeg_dec.h>
#include <modules/jpeg_decode_sw.h>
#include <modules/tjpgd.h>

static OPERATE_RET __jpeg_codec_sw_init()
{
    return OPRT_OK;
}

OPERATE_RET tkl_jpeg_codec_init()
{
    OPERATE_RET ret = 0;

    __jpeg_codec_sw_init();

    return ret;
}

static OPERATE_RET __jpeg_dec_sw_deint(void)
{
    return OPRT_OK;
}

OPERATE_RET tkl_jpeg_codec_deinit()
{
    OPERATE_RET ret = 0;

    __jpeg_dec_sw_deint();

    return ret;
}

OPERATE_RET tkl_jpeg_codec_img_info_get(UINT8_T *jpeg_buf, UINT32_T jpeg_size, TKL_JPEG_CODEC_INFO_T *jpeg_info)
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

OPERATE_RET __jpeg_dec_sw_convert(UINT8_T *src_buf, UINT8_T *dst_buf, TKL_JPEG_CODEC_INFO_T *jpeg_codec_info, JPEG_DEC_OUT_FMT fmt)
{
    OPERATE_RET ret = OPRT_OK;
    sw_jpeg_dec_res_t result;
    UINT32_T out_size = 0;
    JD_FORMAT_OUTPUT output_fmt;
    UINT8_T width_must_be_multiple_of_2 = false;
    UINT8_T width_must_be_multiple_of_4 = false;

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
        bk_printf("%s not support jpeg convert to type(%d)\r\n", __func__, jpeg_codec_info->out_fmt);
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

OPERATE_RET tkl_jpeg_codec_convert(UINT8_T *src_buf, UINT8_T *dst_buf, TKL_JPEG_CODEC_INFO_T *jpeg_codec_info, JPEG_DEC_OUT_FMT out_fmt)
{
    OPERATE_RET ret = OPRT_OK;

    if (!src_buf || !dst_buf || !jpeg_codec_info)
        return OPRT_INVALID_PARM;

    if (!jpeg_codec_info->in_size || !jpeg_codec_info->out_height || !jpeg_codec_info->out_width)
    {
        bk_printf("%s, dont know jpeg, please get img info first\r\n", __func__);
        return OPRT_INVALID_PARM;
    }

    ret = __jpeg_dec_sw_convert(src_buf, dst_buf, jpeg_codec_info, out_fmt);

    return ret;
}



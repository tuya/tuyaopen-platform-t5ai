#include "common/bk_err.h"
#include "os/mem.h"
#include "os/os.h"
#include <common/bk_typedef.h>
#include "driver/media_types.h"
#include "modules/image_scale_types.h"
#include "modules/image_scale.h"

int image_convert(codec_img_t *src)
{
    int ret = BK_OK;
    if ((src->rotate == 0) && (!src->if_resize))
    {
        switch (src->input_type)
        {
            case PIXEL_MSB_YUYV:
                switch (src->output_type)
                {
                    case PIXEL_MSB_RGB565:
                    {
                        yuyv_to_rgb565_convert(src->src_buf, src->dst_buf, src->width, src->height);
                    }
                    break;
                    default:
                        ret = BK_ERR_NOT_SUPPORT;
                        break;
                }
                break;
            case PIXEL_MSB_VUYY:
                switch (src->output_type)
                {
                    case PIXEL_MSB_RGB565:
                    {
                        vuyy_to_rgb565_convert(src->src_buf, src->dst_buf, src->width, src->height);
                    }
                    break;
                    case PIXEL_MSB_RGB888:
                    {
                        vuyy_to_rgb888(src->src_buf, src->dst_buf, src->width, src->height);
                    }
                    break;
                    default:
                        ret = BK_ERR_NOT_SUPPORT;
                        break;
                }
                break;
            case PIXEL_MSB_UYVY:
                switch (src->output_type)
                {
                    case PIXEL_MSB_RGB565:
                    {
                        uyvy_to_rgb565_convert(src->src_buf, src->dst_buf, src->width, src->height);
                    }
                    break;
                    default:
                        ret = BK_ERR_NOT_SUPPORT;
                        break;
                }
                break;
            case PIXEL_MSB_YYUV:
                switch (src->output_type)
                {
                    case PIXEL_MSB_RGB888:
                    {
                        yyuv_to_rgb888(src->src_buf, src->dst_buf, src->width, src->height);
                    }
                    break;
                    default:
                        ret = BK_ERR_NOT_SUPPORT;
                        break;
                }
                break;
            case PIXEL_MSB_VYUY:
                switch (src->output_type)
                {
                    case PIXEL_MSB_RGB888:
                    {
                        vyuy_to_rgb888_convert(src->src_buf, src->dst_buf, src->width, src->height);
                    }
                    break;
                    default:
                        ret = BK_ERR_NOT_SUPPORT;
                        break;
                }
                break;
            case PIXEL_MSB_RGB565:
                switch (src->output_type)
                {
                    case PIXEL_MSB_UYVY:
                    {
                        rgb565_to_uyvy_convert((unsigned short *)src->src_buf, (unsigned short *)src->dst_buf, src->width, src->height);
                    }
                    break;
                    case PIXEL_MSB_YUYV:
                    {
                        rgb565_to_yuyv_convert((unsigned short *)src->src_buf, (unsigned short *)src->dst_buf, src->width, src->height);
                    }
                    break;
                    case PIXEL_MSB_VUYY:
                    {
                        rgb565_to_vuyy_convert((unsigned short *)src->src_buf, (unsigned short *)src->dst_buf, src->width, src->height);
                    }
                    break;
                    default:
                        ret = BK_ERR_NOT_SUPPORT;
                        break;
                }
                break;
            case PIXEL_MSB_ARGB8888:
                switch (src->output_type)
                {
                    case PIXEL_MSB_VUYY:
                    {
                        argb8888_to_vuyy_blend(src->src_buf, src->dst_buf, src->width, src->height);
                    }
                    break;
                    case PIXEL_MSB_YUYV:
                    {
                        argb8888_to_yuyv_blend(src->src_buf, src->dst_buf, src->width, src->height);
                    }
                    break;
                    default:
                        ret = BK_ERR_NOT_SUPPORT;
                        break;
                }
                break;
            default:
                ret = BK_ERR_NOT_SUPPORT;
                break;
        }
    }
    else if (src->rotate && (!src->if_resize))
    {
        switch (src->input_type)
        {
            case PIXEL_MSB_VUYY:
                switch (src->rotate)
                {
                    case ROTATE_90:
                        if (src->output_type == PIXEL_MSB_YUYV)
                        {
                            vuyy_rotate_degree90_to_yuyv(src->src_buf, src->dst_buf, src->width, src->height);
                            break;
                        }
                        else if (src->output_type == PIXEL_MSB_RGB565)
                        {
                            vuyy2rgb_rotate_degree90(src->src_buf, src->dst_buf, src->width, src->height);
                            break;
                        }
                        else
                        {
                            ret = BK_ERR_NOT_SUPPORT;
                            break;
                        }
                    case ROTATE_270:
                        if (src->output_type == PIXEL_MSB_YUYV)
                        {
                            vuyy_rotate_degree270_to_yuyv(src->src_buf, src->dst_buf, src->width, src->height);
                            break;
                        }
                        else
                        {
                            ret = BK_ERR_NOT_SUPPORT;
                            break;
                        }
                    default:
                        ret = BK_ERR_NOT_SUPPORT;
                        break;
                }
                break;
            case PIXEL_MSB_YUYV:
                switch (src->rotate)
                {
                    case ROTATE_90:
                        if (src->output_type == PIXEL_MSB_YUYV)
                        {
                            yuyv_rotate_degree90_to_yuyv(src->src_buf, src->dst_buf, src->width, src->height);
                            break;
                        }
                        else if (src->output_type == PIXEL_MSB_RGB565)
                        {
                            yuyv2rgb_rotate_degree90(src->src_buf, src->dst_buf, src->width, src->height);
                            break;
                        }
                        else
                        {
                            ret = BK_ERR_NOT_SUPPORT;
                            break;
                        }
                    case ROTATE_270:
                        if (src->output_type == PIXEL_MSB_YUYV)
                        {
                            yuyv_rotate_degree270_to_yuyv(src->src_buf, src->dst_buf, src->width, src->height);
                            break;
                        }
                        else if (src->output_type == PIXEL_MSB_VUYY)
                        {
                            yuyv_rotate_degree270_to_vuyy(src->src_buf, src->dst_buf, src->width, src->height);
                            break;

                        }
                    default:
                        ret = BK_ERR_NOT_SUPPORT;
                        break;
                }
                break;
            default:
                ret = BK_ERR_NOT_SUPPORT;
                break;
        }
    }
    return ret;
}


uint8_t *codec_image(codec_img_t *src)
{
    uint32_t ret = BK_OK;
    codec_img_t *codec_img = NULL;
    codec_img = os_malloc(sizeof(codec_img_t));
    os_memset(codec_img, 0, sizeof(codec_img_t));
    codec_img->src_buf = src->src_buf;
    codec_img->dst_buf = src->dst_buf;
    codec_img->width = src->width;
    codec_img->height = src->height;
    codec_img->rotate = src->rotate;
    codec_img->input_type = src->input_type;
    codec_img->output_type = src->output_type;
    codec_img->if_resize = src->if_resize;
    if (src->if_resize != 0)
    {
        codec_img->new_width = src->new_width;
        codec_img->new_height = src->new_height;
    }
    ret = image_convert(codec_img);
    if (ret != BK_OK)
    {
        os_printf("format not support \r\n");
        return 0 ;
    }
    return codec_img->dst_buf;
}


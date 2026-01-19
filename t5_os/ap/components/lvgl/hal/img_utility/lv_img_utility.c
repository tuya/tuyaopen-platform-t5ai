#include "os/os.h"
#include "os/mem.h"
#include "driver/lcd.h"
#include "driver/media_types.h"
#include "modules/jpeg_decode_sw.h"
#include "driver/psram.h"
#include "lv_jpeg_hw_decode.h"
#include "lvgl.h"
#include "bk_posix.h"

uint8_t lv_dma2d_is_init = 0;
static jpeg_dec_handle_t jpeg_dec_handle;

static s32 lv_img_jpeg_sw_decode(frame_buffer_t *jpeg_frame, lv_img_dsc_t *img_dst)
{
    s32 ret = BK_FAIL;
    sw_jpeg_dec_res_t result;
    int flag = 0;

    do {
        ret = bk_jpeg_get_img_info(jpeg_frame->length, jpeg_frame->frame, &result, NULL);
        if (ret != BK_OK)
        {
            BK_LOGD(NULL, "[%s][%d] get img info fail:%d\r\n", __FUNCTION__, __LINE__, ret);
            ret = BK_FAIL;
            break;
        }

        img_dst->header.w = result.pixel_x;
        img_dst->header.h = result.pixel_y;
        img_dst->data_size = img_dst->header.w * img_dst->header.h * 2;
        if (img_dst->data == NULL)
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

        ret = bk_jpeg_dec_sw_start_by_handle(jpeg_dec_handle, JPEGDEC_BY_FRAME, jpeg_frame->frame, (uint8_t *)img_dst->data, jpeg_frame->length, img_dst->data_size, (sw_jpeg_dec_res_t *)&result);
        if (ret != BK_OK)
        {
            BK_LOGD(NULL, "[%s][%d] sw decoder error\r\n", __FUNCTION__, __LINE__);
            if(flag)
            {
                psram_free((void *)img_dst->data);
                img_dst->data = NULL;
            }
            break;
        }

        img_dst->header.always_zero = 0;
        img_dst->header.cf = LV_IMG_CF_TRUE_COLOR;
    }while(0);

    return ret;
}

bk_err_t lv_img_read_file_to_mem(char *filename, uint32* paddr)
{
    uint8 *sram_addr = NULL;
    uint32 once_read_len = 1024 * 4;
    int fd = -1;
    int read_len = 0;
    bk_err_t ret = BK_FAIL;

    do {
        fd = open(filename, O_RDONLY);
        if(fd < 0)
        {
            BK_LOGD(NULL, "[%s][%d] open fail:%s\r\n", __FUNCTION__, __LINE__, filename);
            ret = BK_FAIL;
            break;
        }

        sram_addr = os_malloc(once_read_len);
        if (sram_addr == NULL) {
            BK_LOGD(NULL, "[%s][%d] malloc fail\r\n", __FUNCTION__, __LINE__);
            ret = BK_FAIL;
            break;
        }

        while(1)
        {
            read_len = read(fd, sram_addr, once_read_len);
            if (read_len < 0)
            {
                BK_LOGD(NULL, "[%s][%d] read file fail.\r\n", __FUNCTION__, __LINE__);
                ret= BK_FAIL;
                break;
            }

            if (read_len == 0)
            {
                ret = BK_OK;
                break;
            }

            if (once_read_len != read_len)
            {
                if (read_len % 4)
                {
                    read_len = (read_len / 4 + 1) * 4;
                }
                bk_psram_word_memcpy(paddr, sram_addr, read_len);
            }
            else
            {
                bk_psram_word_memcpy(paddr, sram_addr, once_read_len);
                paddr += (once_read_len / 4);
            }
        }
    }while(0);

    if (sram_addr)
    {
        os_free(sram_addr);
    }

    if (fd > 0)
    {
        close(fd);
    }

    return ret;
}

int lv_img_read_filelen(char *filename)
{
    int ret = BK_FAIL;
    struct stat statbuf;

    do {
        if(!filename)
        {
            BK_LOGD(NULL, "[%s][%d]param is null.\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_PARAM;
            break;
        }

        ret = stat(filename, &statbuf);
        if(BK_OK != ret)
        {
            BK_LOGD(NULL, "[%s][%d] sta fail:%s\r\n", __FUNCTION__, __LINE__, filename);
            break;
        }

        ret = statbuf.st_size;
        BK_LOGD(NULL, "[%s][%d] %s size:%d\r\n", __FUNCTION__, __LINE__, filename, ret);
    } while(0);

    return ret;
}

static frame_buffer_t *lv_img_read_file(char *file_name)
{
    frame_buffer_t *jpeg_frame = NULL;
    int file_len = 0;
    int ret = 0;

    do {
        file_len = lv_img_read_filelen(file_name);
        if (file_len <= 0)
        {
            BK_LOGD(NULL, "[%s][%d] %s don't exit in fatfs\r\n", __FUNCTION__, __LINE__, file_name);
            break;
        }

        jpeg_frame = os_malloc(sizeof(frame_buffer_t));
        if (!jpeg_frame)
        {
            BK_LOGD(NULL, "[%s][%d] malloc fail\r\n", __FUNCTION__, __LINE__);
            break;
        }

        memset(jpeg_frame, 0, sizeof(frame_buffer_t));
        jpeg_frame->frame = psram_malloc(file_len);
        jpeg_frame->length = file_len;
        if (!jpeg_frame->frame)
        {
            os_free(jpeg_frame);
            jpeg_frame = NULL;
            BK_LOGD(NULL, "[%s][%d] psram malloc fail\r\n", __FUNCTION__, __LINE__);
            break;
        }

        ret = lv_img_read_file_to_mem((char *)file_name, (uint32 *)jpeg_frame->frame);
        if (BK_OK != ret)
        {
            psram_free(jpeg_frame->frame);
            jpeg_frame->frame = NULL;

            os_free(jpeg_frame);
            jpeg_frame = NULL;
        }
    }while(0);

    return jpeg_frame;
}

static s32 lv_img_file_jpeg_sw_dec(char *file_name, lv_img_dsc_t *img_dst)
{
    int ret = BK_FAIL;
    frame_buffer_t *jpeg_frame = NULL;

    do {
        jpeg_frame = lv_img_read_file(file_name);
        if (jpeg_frame == NULL)
        {
            ret = BK_FAIL;
            break;
        }

        ret = lv_img_jpeg_sw_decode(jpeg_frame, img_dst);
        if (BK_OK == ret)
        {
            BK_LOGD(NULL, "[%s][%d] decode success, width:%d, height:%d, size:%d\r\n",
                                                __FUNCTION__, __LINE__,
                         img_dst->header.w, img_dst->header.h, img_dst->data_size);
        }
    } while(0);

    if (jpeg_frame)
    {
        if (jpeg_frame->frame)
        {
            psram_free(jpeg_frame->frame);
            jpeg_frame->frame = NULL;
        }

        os_free(jpeg_frame);
        jpeg_frame = NULL;
    }

    return ret;
}

static s32 lv_img_file_jpeg_hw_dec(char *file_name, lv_img_dsc_t *img_dst)
{
    int ret = BK_FAIL;
    frame_buffer_t *jpeg_frame = NULL;

    do{
        jpeg_frame = lv_img_read_file(file_name);
        if (jpeg_frame == NULL)
        {
            ret = BK_FAIL;
            break;
        }

        //通过软解获取图片信息
        sw_jpeg_dec_res_t result;
        ret = bk_jpeg_get_img_info(jpeg_frame->length, jpeg_frame->frame, &result, NULL);
        if (ret != BK_OK)
        {
            BK_LOGD(NULL, "[%s][%d] get img info fail:%d\r\n", __FUNCTION__, __LINE__, ret);
            ret = BK_FAIL;
            break;
        }

        img_dst->header.w = result.pixel_x;
        img_dst->header.h = result.pixel_y;
        img_dst->data_size = img_dst->header.w * img_dst->header.h * 2;

        ret = lv_jpeg_hw_decode(jpeg_frame, img_dst);
        if(BK_OK == ret)
        {
            BK_LOGD(NULL, "[%s][%d] hw decode success, width:%d, height:%d, size:%d\r\n",
                            __FUNCTION__, __LINE__,
                            img_dst->header.w, img_dst->header.h, img_dst->data_size);
        }
    }while(0);

    if (jpeg_frame)
    {
        if (jpeg_frame->frame)
        {
            psram_free(jpeg_frame->frame);
            jpeg_frame->frame = NULL;
        }

        os_free(jpeg_frame);
        jpeg_frame = NULL;
    }

    return ret;
}


/**
 * @brief load jpg file from file system, and store data in img_dst
 * @param[in] filename: only filename
 * @param[in] img_dst: if the data of img_dst is NULL, will use memory of psram_malloc, so when you don't use, should free this ram
 * @retval  BK_OK:success
 * @retval  <0: decode fail or file don't exit in sd
 */
s32 lv_jpeg_img_load_with_sw_dec(char *filename, lv_img_dsc_t *img_dst)
{
    int ret = BK_FAIL;

    do {
        if (!filename || !img_dst)
        {
            ret = BK_ERR_NULL_PARAM;
            break;
        }

        bk_jpeg_dec_sw_init_by_handle(&jpeg_dec_handle, NULL, 0);
        jd_set_format_by_handle(jpeg_dec_handle, JD_FORMAT_RGB565);
        ret = lv_img_file_jpeg_sw_dec(filename, img_dst);
        if (ret != BK_OK) {
            BK_LOGD(NULL, "%s jpeg sw decode fail\r\n", __func__);
        }
        jd_set_format_by_handle(jpeg_dec_handle, JD_FORMAT_VYUY);
        bk_jpeg_dec_sw_deinit_by_handle(jpeg_dec_handle);
        jpeg_dec_handle = NULL;
    } while(0);

    return ret;
}

s32 lv_jpeg_img_load_yuyv(char *filename, lv_img_dsc_t *img_dst)
{
    int ret = BK_FAIL;

    do {
        if (!filename || !img_dst)
        {
            ret = BK_ERR_NULL_PARAM;
            break;
        }

        bk_jpeg_dec_sw_init_by_handle(&jpeg_dec_handle, NULL, 0);
        jd_set_format_by_handle(jpeg_dec_handle, JD_FORMAT_YUYV);
        ret = lv_img_file_jpeg_sw_dec(filename, img_dst);
        if (ret != BK_OK) {
            BK_LOGD(NULL, "%s jpeg sw decode fail\r\n", __func__);
        }
        jd_set_format_by_handle(jpeg_dec_handle, JD_FORMAT_VYUY);
        bk_jpeg_dec_sw_deinit_by_handle(jpeg_dec_handle);
        jpeg_dec_handle = NULL;
    } while(0);

    return ret;
}

s32 lv_jpeg_img_load_with_hw_dec(char *filename, lv_img_dsc_t *img_dst)
{
    int ret = BK_FAIL;

    do {
        if(!filename || !img_dst)
        {
            ret = BK_ERR_NULL_PARAM;
            break;
        }

        if (lv_dma2d_is_init == 0) {
            lv_dma2d_yuyv2rgb565_init();
            lv_dma2d_is_init = 1;
        }

        lv_jpeg_hw_decode_output_fmt_set(JH_OUTPUT_RGB565);
        ret = lv_img_file_jpeg_hw_dec(filename, img_dst);
        if (ret != BK_OK) {
            BK_LOGD(NULL, "%s jpeg hw decode fail\r\n", __func__);
        }
    } while(0);

    return ret;
}

s32 lv_png_img_load(char *filename, lv_img_dsc_t *img_dst)
{
    int ret = BK_FAIL;
    lv_img_decoder_dsc_t img_decoder_dsc;

    if (!filename || !img_dst) {
        ret = BK_ERR_NULL_PARAM;
        BK_LOGD(NULL, "[%s][%d]param invalid\r\n", __FUNCTION__, __LINE__);
        return ret;
    }

    memset((char *)&img_decoder_dsc, 0, sizeof(img_decoder_dsc));
    img_decoder_dsc.src_type = LV_IMG_SRC_FILE;
    ret = lv_img_decoder_open(&img_decoder_dsc, filename, img_decoder_dsc.color, img_decoder_dsc.frame_id);
    if (ret != LV_RES_OK) {
        BK_LOGD(NULL, "[%s][%d] decode fail:%d\r\n", __FUNCTION__, __LINE__, ret);
        ret = BK_FAIL;
        return ret;
    }

    memcpy(&img_dst->header, &img_decoder_dsc.header, sizeof(lv_img_header_t));
    img_dst->data_size = lv_img_read_filelen(filename);
    img_dst->data = img_decoder_dsc.img_data;
    lv_mem_free((void *)img_decoder_dsc.src);

    return ret;
}

void lv_img_decode_unload(lv_img_dsc_t *img_dst)
{
    psram_free((void *)img_dst->data);
    img_dst->data = NULL;

    if (lv_dma2d_is_init) {
        lv_dma2d_yuyv2rgb565_deinit();
        lv_dma2d_is_init = 0;
    }
}


#include "tkl_dvp.h"
#include "tkl_memory.h"
#include <driver/int.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <driver/media_types.h>
#include <driver/yuv_buf.h>
#include <driver/h264.h>
#include <driver/video_common_driver.h>

#define MODULE_ON       (1)
#define MODULE_OFF      (0)

#define HARDWARE_BLOCK_WDITH_BYTE   8
#define HARDWARE_BLOCK_LINE         8
#define YUV_PIXEL_SIZE_BYTE         2 //YUV422一个像素点占两个字节

#define BLOCK_WIDTH           8   // 像素处理块宽
#define BLOCK_HEIGHT          8   // 像素处理块高

#define clk_m(a) (a * 1000 * 1000)

#define YUV422_PER_PIXEL_BYTE   (2)

#define DVP_H264_SEI_SIZE       (96)
#define DVP_DMA_CACHE           (1024 * 10)

static DVP_FRAME_ASSIGN_CB dvp_frame_assign_cb = NULL;
static DVP_FRAME_UNASSIGN_CB dvp_frame_unassign_cb = NULL;
static DVP_FRAME_POST_CB dvp_frame_post_cb = NULL;

typedef struct
{
    UINT32_T in_addr;
    UINT32_T in_channel;
    UINT32_T in_line_size;
    UINT32_T in_offset;
    UINT32_T out_addr;
    UINT32_T out_channel;
    UINT32_T out_offset;
#if 0
    UINT8_T sei[H264_SELF_DEFINE_SEI_SIZE]; // save frame infomation
#endif
    UINT8_T sequence;
    UINT8_T is_i_frame_flag;
} ENCODER_MANAGE_T;

typedef struct
{
    yuv_mode_t cur_work_mode;
    UINT8_T is_mix_mode;
    UINT8_T *pingpong_buf;
    UINT32_T pingpong_len;
    TUYA_DVP_FRAME_MANAGE_T *base_frame;
    TUYA_FRAME_FMT_E base_frame_fmt;
    UINT32_T base_frame_len;
    TUYA_DVP_FRAME_MANAGE_T *encoded_frame;
    TUYA_FRAME_FMT_E encoded_frame_fmt;
    UINT32_T encoded_frame_len;
    ENCODER_MANAGE_T encoder_manage;
    yuv_buf_config_t yuv_buf_module_config;
    jpeg_config_t jpeg_module_config;
    UINT8_T module_stat;
    UINT8_T frame_id;
    UINT8_T error_flag;
    mclk_freq_t bk_clk;
} DVP_MODULE_MANAGE_T;

DVP_MODULE_MANAGE_T g_dvp_module_manage =
{
    .yuv_buf_module_config =
    {
        .mclk_div = YUV_MCLK_DIV_3,
        .yuv_mode_cfg =
        {
            .hsync = 1,
            .vsync = 1,
            .yuv_format = YUV_FORMAT_YUYV,
        },
    },
    .jpeg_module_config =
    {
        .hsync = 1,
        .vsync = 1,
        .mode = JPEG_MODE,
        .sensor_fmt = YUV_FORMAT_YUYV,
    },

    .is_mix_mode = false,
    .bk_clk = MCLK_24M,
    .frame_id = 0,
    .module_stat = MODULE_OFF,
    .error_flag = false,
};

static OPERATE_RET __ty_output_mode_to_bk_work_mode(TUYA_DVP_OUTPUT_MODE output_mode, yuv_mode_t *bk_work_mode)
{
    switch (output_mode)
    {
        case TUYA_DVP_OUTPUT_YUV422:
            (*bk_work_mode) = YUV_MODE;
            break;
        case TUYA_DVP_OUTPUT_JPEG:
        case TUYA_DVP_OUTPUT_JPEG_YUV422_BOTH:
            (*bk_work_mode) = JPEG_MODE;
            break;
        case TUYA_DVP_OUTPUT_H264:
        case TUYA_DVP_OUTPUT_H264_YUV422_BOTH:
            (*bk_work_mode) = H264_MODE;
            break;
        default:
            bk_printf("%s this device dont support mode(%d)\r\n", __func__, output_mode);
            return OPRT_NOT_SUPPORTED;
    }

    return OPRT_OK;
}

static OPERATE_RET __dvp_output_mode_check(TUYA_DVP_BASE_CFG_T *base_cfg)
{
    OPERATE_RET ret = OPRT_OK;
    uint16_t width = base_cfg->width;
    uint16_t height = base_cfg->height;
    TUYA_DVP_OUTPUT_MODE output_mode = base_cfg->output_mode;

    ret = __ty_output_mode_to_bk_work_mode(output_mode, &(g_dvp_module_manage.cur_work_mode));
    if (ret)
        return OPRT_NOT_SUPPORTED;


    if (output_mode == TUYA_DVP_OUTPUT_YUV422
        || output_mode == TUYA_DVP_OUTPUT_JPEG_YUV422_BOTH 
        || output_mode == TUYA_DVP_OUTPUT_H264_YUV422_BOTH)
    {
        g_dvp_module_manage.base_frame_fmt = TUYA_FRAME_FMT_YUV422;
        g_dvp_module_manage.base_frame_len = width * height * YUV422_PER_PIXEL_BYTE;

        g_dvp_module_manage.is_mix_mode = (output_mode != TUYA_DVP_OUTPUT_YUV422) ? true : false;
    }

    if (g_dvp_module_manage.cur_work_mode == H264_MODE)
    {
        g_dvp_module_manage.encoded_frame_fmt = TUYA_FRAME_FMT_H264;
        g_dvp_module_manage.encoded_frame_len = CONFIG_H264_FRAME_SIZE;
    }

    if (g_dvp_module_manage.cur_work_mode == JPEG_MODE)
    {
        g_dvp_module_manage.encoded_frame_fmt = TUYA_FRAME_FMT_JPEG;
        g_dvp_module_manage.encoded_frame_len = CONFIG_JPEG_FRAME_SIZE;
    }

    return OPRT_OK;
}

static OPERATE_RET __ty_clk_to_bk_clk(UINT32_T clk, mclk_freq_t *outclk)
{
    switch (clk)
    {
        case clk_m(15):
            (*outclk) = MCLK_15M;
            break;
        case clk_m(16):
            (*outclk) = MCLK_16M;
            break;
        case clk_m(20):
            (*outclk) = MCLK_20M;
            break;
        case clk_m(24):
            (*outclk) = MCLK_24M;
            break;
        case clk_m(30):
            (*outclk) = MCLK_30M;
            break;
        case clk_m(32):
            (*outclk) = MCLK_32M;
            break;
        case clk_m(40):
            (*outclk) = MCLK_40M;
            break;
        case clk_m(48):
            (*outclk) = MCLK_48M;
            break;
        default:
            (*outclk) = MCLK_24M;
            break;
    }

    return OPRT_OK;
}

static OPERATE_RET __dvp_yuv_buf_module_init(TUYA_DVP_BASE_CFG_T *base_cfg)
{
    yuv_buf_config_t yuv_buf_config_cur = {0};
    memcpy(&yuv_buf_config_cur, &(g_dvp_module_manage.yuv_buf_module_config), sizeof(yuv_buf_config_t));

    yuv_buf_config_cur.work_mode = g_dvp_module_manage.cur_work_mode;

    // 横向width的像素块个数
    yuv_buf_config_cur.x_pixel = base_cfg->width / BLOCK_WIDTH; // 除以BLOCK_WIDTH得到横向块数

    // 纵向像素块个数
    yuv_buf_config_cur.y_pixel = base_cfg->height / BLOCK_HEIGHT; // 除以BLOCK_HEIGHT得到纵向块数

    if (g_dvp_module_manage.cur_work_mode == YUV_MODE)
        goto init_yuv_buf;

    // 申请pingpong buf
    if (g_dvp_module_manage.pingpong_buf != NULL)
        goto init_yuv_buf;

    if (g_dvp_module_manage.cur_work_mode == H264_MODE)
    {
        // h264编码器每16行一输入，16 * pixel_per_size * pingpong
        g_dvp_module_manage.pingpong_len = base_cfg->width * 32 * 2; 
        g_dvp_module_manage.pingpong_buf = (uint8_t *)os_malloc(g_dvp_module_manage.pingpong_len);
    }
    else if (g_dvp_module_manage.cur_work_mode == JPEG_MODE)
    {
        // jepg编码器每8行一输入，8 * pixel_per_size * pingpong
        g_dvp_module_manage.pingpong_len = base_cfg->width * 16 * 2;
        g_dvp_module_manage.pingpong_buf = (uint8_t *)os_malloc(g_dvp_module_manage.pingpong_len);
    }

    if (g_dvp_module_manage.pingpong_buf == NULL)
    {
        bk_printf("%s malloc pingpong buf failed\r\n", __func__);
        return OPRT_MALLOC_FAILED;
    }

init_yuv_buf:
    yuv_buf_config_cur.base_addr = (g_dvp_module_manage.pingpong_buf == NULL) ? NULL : g_dvp_module_manage.pingpong_buf;

    UINT8_T ret = bk_yuv_buf_init(&yuv_buf_config_cur);
	if (ret != BK_OK)
	{
		bk_printf("yuv_buf yuv mode init error\n");
		return OPRT_MALLOC_FAILED;
	}

    return OPRT_OK;
}

static OPERATE_RET __dvp_jpeg_module_init(TUYA_DVP_BASE_CFG_T *base_cfg)
{
    jpeg_config_t jpeg_config_cur = {0};
    memcpy(&jpeg_config_cur, &(g_dvp_module_manage.jpeg_module_config), sizeof(jpeg_config_t));

    jpeg_config_cur.x_pixel = base_cfg->width / BLOCK_WIDTH;
    jpeg_config_cur.y_pixel = base_cfg->height / BLOCK_WIDTH;

    jpeg_config_cur.clk = g_dvp_module_manage.bk_clk;
    jpeg_config_cur.mode = JPEG_MODE;

    UINT8_T ret = bk_jpeg_enc_init(&jpeg_config_cur);
    if (ret != BK_OK)
    {
        bk_printf("jpeg init error\n");
        return OPRT_MALLOC_FAILED;
    }

    return OPRT_OK;
}

static void __dvp_dma_finish_cb(dma_id_t id)
{
    g_dvp_module_manage.encoder_manage.out_offset += DVP_DMA_CACHE;
}

static OPERATE_RET __dvp_encoder_output_dma_config(TUYA_DVP_BASE_CFG_T *base_cfg, yuv_mode_t work_mode)
{
    if (!dvp_frame_assign_cb)
        return OPRT_COM_ERROR;

    dma_config_t dma_config = {0};
    ENCODER_MANAGE_T *coder_manage = &(g_dvp_module_manage.encoder_manage);

    coder_manage->out_offset = 0;
    if (work_mode == H264_MODE)
    {
        bk_h264_get_fifo_addr(&(coder_manage->out_addr));
        coder_manage->out_channel = bk_fixed_dma_alloc(DMA_DEV_H264, DMA_ID_8);
    }
    else if (work_mode == JPEG_MODE)
    {
        bk_jpeg_enc_get_fifo_addr(&(coder_manage->out_addr));
        coder_manage->out_channel = bk_fixed_dma_alloc(DMA_DEV_JPEG, DMA_ID_8);
    }
    if (coder_manage->out_channel >= DMA_ID_MAX)
    {
        bk_printf("malloc dma fail \r\n");
        return OPRT_MALLOC_FAILED;
    }

    g_dvp_module_manage.encoded_frame = dvp_frame_assign_cb(g_dvp_module_manage.encoded_frame_fmt);
    if (g_dvp_module_manage.encoded_frame ==NULL)
    {
        bk_printf("%s, assign idle frame fail\r\n", __func__);
        return OPRT_MALLOC_FAILED;
    }
    g_dvp_module_manage.encoded_frame->frame_fmt = g_dvp_module_manage.encoded_frame_fmt;
    g_dvp_module_manage.encoded_frame->width = base_cfg->width;
    g_dvp_module_manage.encoded_frame->height = base_cfg->height;

    dma_config.mode = DMA_WORK_MODE_REPEAT;
    dma_config.chan_prio = 0;
    dma_config.src.width = DMA_DATA_WIDTH_32BITS;
    dma_config.src.start_addr = coder_manage->out_addr;
    dma_config.dst.dev = DMA_DEV_DTCM;
    dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
    dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;

    if (work_mode == H264_MODE)
    {
        dma_config.src.dev = DMA_DEV_H264;
    }
    else if (work_mode == JPEG_MODE)
    {
        dma_config.src.dev = DMA_DEV_JPEG;
    }

    dma_config.dst.start_addr = (uint32_t)g_dvp_module_manage.encoded_frame->data;
    dma_config.dst.end_addr = (uint32_t)(g_dvp_module_manage.encoded_frame->data + g_dvp_module_manage.encoded_frame_len);

    bk_dma_init(coder_manage->out_channel, &dma_config);
    bk_dma_set_transfer_len(coder_manage->out_channel, DVP_DMA_CACHE);
    bk_dma_register_isr(coder_manage->out_channel, NULL, __dvp_dma_finish_cb);
    bk_dma_enable_finish_interrupt(coder_manage->out_channel);
#if (CONFIG_SPE)
    bk_dma_set_src_burst_len(coder_manage->out_channel, BURST_LEN_SINGLE);
    bk_dma_set_dest_burst_len(coder_manage->out_channel, BURST_LEN_INC16);
    bk_dma_set_dest_sec_attr(coder_manage->out_channel, DMA_ATTR_SEC);
    bk_dma_set_src_sec_attr(coder_manage->out_channel, DMA_ATTR_SEC);
#endif
    bk_dma_start(coder_manage->out_channel);

    return OPRT_OK;
}

static OPERATE_RET __dvp_encoder_input_dma_config(TUYA_DVP_BASE_CFG_T *base_cfg)
{
    if (!dvp_frame_assign_cb)
        return OPRT_COM_ERROR;

    dma_config_t dma_config = {0};
    ENCODER_MANAGE_T *coder_manage = &(g_dvp_module_manage.encoder_manage);

    coder_manage->in_offset = 0;
    coder_manage->in_addr = bk_yuv_buf_get_em_base_addr();
    coder_manage->in_line_size = (g_dvp_module_manage.pingpong_len >> 1);
    coder_manage->in_channel = bk_dma_alloc(DMA_DEV_DTCM);
    if (coder_manage->in_channel >= DMA_ID_MAX)
    {
        bk_printf("malloc dma fail \r\n");
        return OPRT_MALLOC_FAILED;
    }

    g_dvp_module_manage.base_frame = dvp_frame_assign_cb(g_dvp_module_manage.base_frame_fmt);
    if (g_dvp_module_manage.base_frame == NULL)
    {
        bk_printf("%s, assign idle frame fail\r\n", __func__);
        return OPRT_MALLOC_FAILED;
    }

    g_dvp_module_manage.base_frame->width = base_cfg->width;
    g_dvp_module_manage.base_frame->height = base_cfg->height;
    g_dvp_module_manage.base_frame->frame_fmt = g_dvp_module_manage.base_frame_fmt;
    g_dvp_module_manage.base_frame->data_len = g_dvp_module_manage.base_frame_len;

    dma_config.mode = DMA_WORK_MODE_SINGLE;
    dma_config.chan_prio = 1;
    dma_config.src.dev = DMA_DEV_DTCM;
    dma_config.src.width = DMA_DATA_WIDTH_32BITS;
    dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.src.start_addr = (uint32_t)coder_manage->in_addr;
    dma_config.src.end_addr = (uint32_t)(coder_manage->in_addr + coder_manage->in_line_size);

    dma_config.dst.dev = DMA_DEV_DTCM;
    dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
    dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.dst.start_addr = (uint32_t)g_dvp_module_manage.base_frame->data;
    dma_config.dst.end_addr = (uint32_t)(g_dvp_module_manage.base_frame->data + coder_manage->in_line_size);

    bk_dma_init(coder_manage->in_channel, &dma_config);
    bk_dma_set_transfer_len(coder_manage->in_channel, coder_manage->in_line_size);
#if (CONFIG_SPE)
    bk_dma_set_src_burst_len(coder_manage->in_channel, 3);
    bk_dma_set_dest_burst_len(coder_manage->in_channel, 3);
    bk_dma_set_dest_sec_attr(coder_manage->in_channel, DMA_ATTR_SEC);
    bk_dma_set_src_sec_attr(coder_manage->in_channel, DMA_ATTR_SEC);
#endif

    return OPRT_OK;
}

static OPERATE_RET __dvp_yuv_mode_init(TUYA_DVP_BASE_CFG_T *base_cfg)
{
    if (!dvp_frame_assign_cb)
        return OPRT_COM_ERROR;

    g_dvp_module_manage.base_frame = dvp_frame_assign_cb(g_dvp_module_manage.base_frame_fmt);
    if (g_dvp_module_manage.base_frame == NULL)
    {
        bk_printf("%s, assign idle frame fail\r\n", __func__);
        return OPRT_MALLOC_FAILED;
    }

    g_dvp_module_manage.base_frame->width = base_cfg->width;
    g_dvp_module_manage.base_frame->height = base_cfg->height;
    g_dvp_module_manage.base_frame->frame_fmt = g_dvp_module_manage.base_frame_fmt;
    g_dvp_module_manage.base_frame->data_len = g_dvp_module_manage.base_frame_len;
    bk_yuv_buf_set_em_base_addr((uint32_t)g_dvp_module_manage.base_frame->data);

    return OPRT_OK;
}

static OPERATE_RET __dvp_h264_mode_init(TUYA_DVP_BASE_CFG_T *base_cfg)
{
    OPERATE_RET ret = 0;

    ret = bk_h264_init(base_cfg->width, base_cfg->height);
    if (ret)
        return OPRT_COM_ERROR;

    ret = __dvp_encoder_output_dma_config(base_cfg, H264_MODE);
    if (ret)
        return OPRT_COM_ERROR;

#if 0
    os_memset(&coder_manage->sei[0], 0xFF, DVP_H264_SEI_SIZE);

    h264_encode_sei_init(&coder_manage->sei[0]);
#endif

    if (base_cfg->output_mode == TUYA_DVP_OUTPUT_H264_YUV422_BOTH)
    {
        ret = __dvp_encoder_input_dma_config(base_cfg);
        if (ret)
            return OPRT_COM_ERROR;
    }

    return ret;
}

static OPERATE_RET __dvp_jpeg_mode_init(TUYA_DVP_BASE_CFG_T *base_cfg)
{
    OPERATE_RET ret = 0;

    ret = __dvp_jpeg_module_init(base_cfg);
    if (ret)
        return OPRT_COM_ERROR;

    ret = __dvp_encoder_output_dma_config(base_cfg, JPEG_MODE);
    if (ret)
        return OPRT_COM_ERROR;

    if (base_cfg->output_mode == TUYA_DVP_OUTPUT_JPEG_YUV422_BOTH)
    {
        ret = __dvp_encoder_input_dma_config(base_cfg);
        if (ret)
            return OPRT_COM_ERROR;
    }

    return ret;
}

static OPERATE_RET __dvp_hardware_init(TUYA_DVP_BASE_CFG_T *base_cfg)
{
    OPERATE_RET ret = 0;

    ret = __dvp_yuv_buf_module_init(base_cfg);
    if (ret)
        return ret;

    if (g_dvp_module_manage.cur_work_mode == YUV_MODE)
    {
        ret =  __dvp_yuv_mode_init(base_cfg);
        goto end;
    }

    if (g_dvp_module_manage.cur_work_mode == H264_MODE)
    {
        ret = __dvp_h264_mode_init(base_cfg);
    }

    if (g_dvp_module_manage.cur_work_mode == JPEG_MODE)
    {
        ret = __dvp_jpeg_mode_init(base_cfg);
    }

end:
    return ret;
}

static void __dvp_yuv_eof_handler(yuv_buf_unit_t id, void *param)
{
    if (!g_dvp_module_manage.module_stat || !dvp_frame_assign_cb || !param)
		return;

    TUYA_DVP_BASE_CFG_T *base_cfg = (TUYA_DVP_BASE_CFG_T *)param;

    g_dvp_module_manage.base_frame->is_frame_complete = true;
    g_dvp_module_manage.base_frame->total_frame_len = g_dvp_module_manage.base_frame_len;
    TUYA_DVP_FRAME_MANAGE_T *new_frame = dvp_frame_assign_cb(g_dvp_module_manage.base_frame_fmt);
    if (new_frame)
    {
		new_frame->width = base_cfg->width;
		new_frame->height = base_cfg->height;
		new_frame->frame_fmt = g_dvp_module_manage.base_frame_fmt;
		new_frame->data_len = g_dvp_module_manage.base_frame_len;
        if (dvp_frame_post_cb)
            dvp_frame_post_cb(g_dvp_module_manage.base_frame);

		g_dvp_module_manage.base_frame = new_frame;
    }

    bk_yuv_buf_set_em_base_addr((UINT32_T)g_dvp_module_manage.base_frame->data);
}

static void __dvp_yuv_line_done(yuv_buf_unit_t id, void *param)
{
    if (!g_dvp_module_manage.module_stat || !param)
		return;

    UINT8_T *line_idx = (UINT8_T *)param;
    ENCODER_MANAGE_T *coder_manage = &(g_dvp_module_manage.encoder_manage);

    if ((coder_manage->in_offset + coder_manage->in_line_size) > g_dvp_module_manage.base_frame_len)
    {
        coder_manage->in_offset = 0;
    }

    while(bk_dma_get_enable_status(coder_manage->in_channel));
    bk_dma_stop(coder_manage->in_channel);
    bk_dma_set_src_start_addr(coder_manage->in_channel,
                              (uint32_t)coder_manage->in_addr + (*line_idx) * coder_manage->in_line_size);
    bk_dma_set_dest_start_addr(coder_manage->in_channel,
                               (uint32_t)(g_dvp_module_manage.base_frame->data + coder_manage->in_offset));
    bk_dma_start(coder_manage->in_channel);
    coder_manage->in_offset += coder_manage->in_line_size;
}

static void __dvp_h264_eof_handler(h264_unit_t id, void *param)
{
    if (!g_dvp_module_manage.module_stat || !dvp_frame_assign_cb || !param)
		return;

    TUYA_DVP_BASE_CFG_T *base_cfg = (TUYA_DVP_BASE_CFG_T *)param;

    if (g_dvp_module_manage.encoded_frame == NULL
        || g_dvp_module_manage.encoded_frame->data == NULL)
    {
        bk_printf("g_dvp_module_manage->encode_frame NULL error\n");
        goto error;
    }

    UINT32_T real_length = bk_h264_get_encode_count() * 4;
    UINT32_T remain_length = 0;
    ENCODER_MANAGE_T *coder_manage = &(g_dvp_module_manage.encoder_manage);

    coder_manage->sequence++;

    if (coder_manage->sequence > H264_GOP_FRAME_CNT)
    {
        coder_manage->sequence = 1;
    }

    coder_manage->is_i_frame_flag = (coder_manage->sequence == 1) ? 1 : 0;

#if (CONFIG_H264_GOP_START_IDR_FRAME)
    if (coder_manage->sequence == H264_GOP_FRAME_CNT)
    {
        bk_h264_soft_reset();
        coder_manage->sequence = 0;
    }
#endif

    if (real_length > CONFIG_H264_FRAME_SIZE - 0x20)
    {
        bk_printf("%s size over h264 buffer range, %d\r\n", __func__, real_length);
        g_dvp_module_manage.error_flag = true;
    }

    bk_dma_flush_src_buffer(coder_manage->out_channel);

    remain_length = DVP_DMA_CACHE - bk_dma_get_remain_len(coder_manage->out_channel);

    bk_dma_stop(coder_manage->out_channel);

    coder_manage->out_offset += remain_length;

    if (coder_manage->out_offset != real_length)
    {
        UINT32_T left_length = real_length - coder_manage->out_offset;
        bk_printf("%s size no match:%d-%d=%d\r\n", __func__, real_length, coder_manage->out_offset, left_length);
        if (left_length != DVP_DMA_CACHE)
        {
            g_dvp_module_manage.error_flag = true;
        }
    }

    coder_manage->out_offset = 0;

    if (g_dvp_module_manage.error_flag)
    {
        g_dvp_module_manage.encoded_frame->data_len = 0;
        coder_manage->sequence = 0;
        goto out;
    }

    g_dvp_module_manage.encoded_frame->frame_id = g_dvp_module_manage.frame_id++;
    g_dvp_module_manage.encoded_frame->data_len = real_length;
    g_dvp_module_manage.encoded_frame->total_frame_len = real_length;
    g_dvp_module_manage.encoded_frame->is_frame_complete = true;

#if 0
    handle->encode_frame->crc = hnd_crc8(handle->encode_frame->frame, handle->encode_frame->length, 0xFF);
    handle->encode_frame->length += H264_SELF_DEFINE_SEI_SIZE;
    os_memcpy(&handle->sei[23], (uint8_t *)handle->encode_frame, sizeof(frame_buffer_t));
    os_memcpy(&handle->encode_frame->frame[handle->encode_frame->length - H264_SELF_DEFINE_SEI_SIZE], &handle->sei[0], H264_SELF_DEFINE_SEI_SIZE);
#endif

    TUYA_DVP_FRAME_MANAGE_T *new_frame = dvp_frame_assign_cb(TUYA_FRAME_FMT_H264);
    if (new_frame)
    {
        new_frame->width = base_cfg->width;
        new_frame->height = base_cfg->height;
        new_frame->frame_fmt = TUYA_FRAME_FMT_H264;
        if (dvp_frame_post_cb)
            dvp_frame_post_cb(g_dvp_module_manage.encoded_frame);

        g_dvp_module_manage.encoded_frame = new_frame;
    }
    else
    {
        bk_h264_soft_reset();
        g_dvp_module_manage.encoded_frame->data_len = 0;
        coder_manage->sequence = 0;
    }

out:
    bk_dma_set_dest_addr(coder_manage->out_channel, (uint32_t)g_dvp_module_manage.encoded_frame->data,
        (uint32_t)g_dvp_module_manage.encoded_frame->data + g_dvp_module_manage.encoded_frame_len);
    bk_dma_start(coder_manage->out_channel);

    if (!g_dvp_module_manage.error_flag && 
        base_cfg->output_mode == TUYA_DVP_OUTPUT_H264_YUV422_BOTH)
    {
        coder_manage->in_offset = 0;
        bk_dma_flush_src_buffer(coder_manage->in_channel);
        g_dvp_module_manage.base_frame->is_frame_complete = true;
        g_dvp_module_manage.base_frame->total_frame_len = g_dvp_module_manage.base_frame_len;
        new_frame = dvp_frame_assign_cb(g_dvp_module_manage.base_frame_fmt);
        if (new_frame)
        {
            new_frame->width = base_cfg->width;
            new_frame->height = base_cfg->height;
            new_frame->frame_fmt = g_dvp_module_manage.base_frame_fmt;
            new_frame->data_len = g_dvp_module_manage.base_frame_len;
            if (dvp_frame_post_cb)
                dvp_frame_post_cb(g_dvp_module_manage.base_frame);

            g_dvp_module_manage.base_frame = new_frame;
        }
    }

    return;

error:
    bk_dma_stop(coder_manage->out_channel);
    bk_yuv_buf_stop(H264_MODE);
}

static void __dvp_jpeg_eof_handler(h264_unit_t id, void *param)
{
    if (!g_dvp_module_manage.module_stat || !dvp_frame_assign_cb || !param)
		return;

    TUYA_DVP_BASE_CFG_T *base_cfg = (TUYA_DVP_BASE_CFG_T *)param;
    ENCODER_MANAGE_T *coder_manage = &(g_dvp_module_manage.encoder_manage);

    if (g_dvp_module_manage.error_flag)
    {
        g_dvp_module_manage.encoded_frame->data_len = 0;
        coder_manage->out_offset = 0;
        bk_dma_stop(coder_manage->out_channel);
        bk_dma_start(coder_manage->out_channel);
        return;
    }

    if (g_dvp_module_manage.encoded_frame == NULL
        || g_dvp_module_manage.encoded_frame->data == NULL)
    {
        bk_printf("g_dvp_module_manage->encode_frame NULL error\n");
        goto error;
    }

    bk_dma_flush_src_buffer(coder_manage->out_channel);

    UINT32_T real_length = bk_jpeg_enc_get_frame_size();
    UINT32_T remain_length = 0;

    remain_length = DVP_DMA_CACHE - bk_dma_get_remain_len(coder_manage->out_channel);

    bk_dma_stop(coder_manage->out_channel);

    UINT32_T tmp_flag = false;
    coder_manage->out_offset = coder_manage->out_offset + remain_length - JPEG_CRC_SIZE;

    if (coder_manage->out_offset != real_length)
    {
        bk_printf("%s size no match:%u-%u=%u\r\n", __func__, real_length, coder_manage->out_offset, real_length - coder_manage->out_offset);
    }

    coder_manage->out_offset = 0;

    UINT8_T *jpeg_buf = g_dvp_module_manage.encoded_frame->data;
    UINT8_T eof_flag = false;
    for (UINT32_T i = real_length; i > real_length - 10; i--)
    {
        if (jpeg_buf[i - 1] == 0xD9 && jpeg_buf[i - 2] == 0xFF)
        {
            real_length = i;
            eof_flag = true;
            break;
        }
        eof_flag = false;
    }

    if (!eof_flag)
    {
        g_dvp_module_manage.encoded_frame->data_len = 0;
        goto out;
    }

    g_dvp_module_manage.encoded_frame->frame_id = g_dvp_module_manage.frame_id++;
    g_dvp_module_manage.encoded_frame->data_len = real_length;
    g_dvp_module_manage.encoded_frame->total_frame_len = real_length;
    g_dvp_module_manage.encoded_frame->is_frame_complete = true;

    TUYA_DVP_FRAME_MANAGE_T *new_frame = dvp_frame_assign_cb(TUYA_FRAME_FMT_JPEG);
    if (new_frame)
    {
        new_frame->width = base_cfg->width;
        new_frame->height = base_cfg->height;
        new_frame->frame_fmt = TUYA_FRAME_FMT_JPEG;
        if (dvp_frame_post_cb)
            dvp_frame_post_cb(g_dvp_module_manage.encoded_frame);

        g_dvp_module_manage.encoded_frame = new_frame;
    }
    else
    {
        g_dvp_module_manage.encoded_frame->data_len = 0;
    }

out:
    bk_dma_set_dest_addr(coder_manage->out_channel, (uint32_t)g_dvp_module_manage.encoded_frame->data,
        (uint32_t)g_dvp_module_manage.encoded_frame->data + g_dvp_module_manage.encoded_frame_len);
    bk_dma_start(coder_manage->out_channel);

    if (!g_dvp_module_manage.error_flag &&
        base_cfg->output_mode == TUYA_DVP_OUTPUT_JPEG_YUV422_BOTH)
    {
        coder_manage->in_offset = 0;
        bk_dma_flush_src_buffer(coder_manage->in_channel);
        g_dvp_module_manage.base_frame->is_frame_complete = true;
        g_dvp_module_manage.base_frame->total_frame_len = g_dvp_module_manage.base_frame_len;
        new_frame = dvp_frame_assign_cb(g_dvp_module_manage.base_frame_fmt);
        if (new_frame)
        {
            new_frame->width = base_cfg->width;
            new_frame->height = base_cfg->height;
            new_frame->frame_fmt = g_dvp_module_manage.base_frame_fmt;
            new_frame->data_len = g_dvp_module_manage.base_frame_len;
            if (dvp_frame_post_cb)
                dvp_frame_post_cb(g_dvp_module_manage.base_frame);

            g_dvp_module_manage.base_frame = new_frame;
        }
    }

    return;

error:
    bk_dma_stop(coder_manage->out_channel);
    bk_yuv_buf_stop(JPEG_MODE);
}

static void __dvp_hardware_vsync_negedge_handler(jpeg_unit_t id, void *param)
{
    if (!g_dvp_module_manage.error_flag)
        return;

    yuv_mode_t mode = g_dvp_module_manage.cur_work_mode;
	if (mode == JPEG_MODE || mode == JPEG_YUV_MODE)
	{
		bk_jpeg_enc_soft_reset();
        bk_yuv_buf_start(JPEG_MODE);
	}
	else if (mode == H264_MODE || mode == H264_YUV_MODE)
	{
		bk_h264_config_reset();
		bk_yuv_buf_start(H264_MODE);
        bk_h264_encode_enable();
	}

    bk_yuv_buf_soft_reset();

    g_dvp_module_manage.error_flag = false;

    ENCODER_MANAGE_T *coder_manage = &(g_dvp_module_manage.encoder_manage);
    coder_manage->sequence = 0;

    if (g_dvp_module_manage.is_mix_mode)
    {
        coder_manage->in_offset = 0;
    }

    if (coder_manage->out_channel < DMA_ID_MAX)
    {
        bk_dma_stop(coder_manage->out_channel);
        if (g_dvp_module_manage.encoded_frame != NULL)
        {
            g_dvp_module_manage.encoded_frame->data_len = 0;
        }
        bk_dma_start(coder_manage->out_channel);
    }
}

static void __dvp_error_handler(jpeg_unit_t id, void *param)
{
    if (!g_dvp_module_manage.error_flag)
        g_dvp_module_manage.error_flag = true;
}

static void __dvp_isr_register(TUYA_DVP_BASE_CFG_T *base_cfg)
{
    switch (base_cfg->output_mode)
    {
    case TUYA_DVP_OUTPUT_YUV422:
        bk_yuv_buf_register_isr(YUV_BUF_YUV_ARV, __dvp_yuv_eof_handler, (void *)base_cfg);
        break;
    case TUYA_DVP_OUTPUT_JPEG:
    case TUYA_DVP_OUTPUT_JPEG_YUV422_BOTH:
        bk_jpeg_enc_register_isr(JPEG_EOF, __dvp_jpeg_eof_handler, (void *)base_cfg);
        bk_jpeg_enc_register_isr(JPEG_FRAME_ERR, __dvp_error_handler, NULL);
        break;
    case TUYA_DVP_OUTPUT_H264:
    case TUYA_DVP_OUTPUT_H264_YUV422_BOTH:
        bk_h264_register_isr(H264_FINAL_OUT, __dvp_h264_eof_handler, (void *)base_cfg);
        break;
    default:
        break;
    }

    if (base_cfg->output_mode == TUYA_DVP_OUTPUT_H264_YUV422_BOTH
        || base_cfg->output_mode == TUYA_DVP_OUTPUT_JPEG_YUV422_BOTH)
    {
        static UINT8_T line_0 = 0;
        bk_yuv_buf_register_isr(YUV_BUF_SM0_WR, __dvp_yuv_line_done, (void *)(&line_0));
        static UINT8_T line_1 = 1;
        bk_yuv_buf_register_isr(YUV_BUF_SM1_WR, __dvp_yuv_line_done, (void *)(&line_1));
    }

    bk_yuv_buf_register_isr(YUV_BUF_VSYNC_NEGEDGE, __dvp_hardware_vsync_negedge_handler, NULL);
	bk_yuv_buf_register_isr(YUV_BUF_SEN_RESL, __dvp_error_handler, NULL);
    bk_yuv_buf_register_isr(YUV_BUF_FULL, __dvp_error_handler, NULL);
    bk_yuv_buf_register_isr(YUV_BUF_H264_ERR, __dvp_error_handler, NULL);
    bk_yuv_buf_register_isr(YUV_BUF_ENC_SLOW, __dvp_error_handler, NULL);
}

OPERATE_RET tkl_dvp_init(TUYA_DVP_BASE_CFG_T *base_cfg, UINT32_T clk)
{
    OPERATE_RET ret = 0;

    if (!base_cfg)
        return OPRT_INVALID_PARM;

    ret = __dvp_output_mode_check(base_cfg);
    if (ret)
        return OPRT_NOT_SUPPORTED;

    g_dvp_module_manage.bk_clk = MCLK_24M;
    __ty_clk_to_bk_clk(clk, &g_dvp_module_manage.bk_clk);

    // SMP版本gpio功能都在usr_gpio_cfg配好
    // bk_video_gpio_init(DVP_GPIO_ALL);

	//enable mclk
    bk_video_dvp_mclk_enable(YUV_MODE);

    __dvp_hardware_init(base_cfg);

    __dvp_isr_register(base_cfg);

    //update mclk config
    bk_video_set_mclk(g_dvp_module_manage.bk_clk);

    yuv_mode_t work_mode = g_dvp_module_manage.cur_work_mode;
    bk_yuv_buf_start(work_mode);
    if (work_mode == H264_MODE)
        bk_h264_encode_enable();

    g_dvp_module_manage.module_stat = MODULE_ON;

    return OPRT_OK;
}

OPERATE_RET tkl_dvp_deinit()
{
    // step 1: stop module work
    if (g_dvp_module_manage.module_stat == MODULE_ON)
    {
        bk_yuv_buf_stop(YUV_MODE);
		bk_yuv_buf_stop(JPEG_MODE);
		bk_yuv_buf_stop(H264_MODE);
        g_dvp_module_manage.module_stat = MODULE_OFF;
    }

	// SMP版本gpio功能都在usr_gpio_cfg配好
	// bk_video_gpio_deinit(DVP_GPIO_ALL);

    // step 2: deinit hardware
	bk_yuv_buf_deinit();
	bk_h264_encode_disable();
	bk_h264_deinit();
	bk_jpeg_enc_deinit();
    bk_video_dvp_mclk_disable();

    // step 3: deinit dma
    ENCODER_MANAGE_T *coder_manage = &(g_dvp_module_manage.encoder_manage);
    if (g_dvp_module_manage.cur_work_mode == H264_MODE)
    {
        bk_dma_stop(coder_manage->out_channel);
        bk_dma_deinit(coder_manage->out_channel);
        bk_dma_free(DMA_DEV_H264, coder_manage->out_channel);
    }

    if (g_dvp_module_manage.cur_work_mode == JPEG_MODE)
    {
        bk_dma_stop(coder_manage->out_channel);
        bk_dma_deinit(coder_manage->out_channel);
        bk_dma_free(DMA_DEV_JPEG, coder_manage->out_channel);
    }

    if (g_dvp_module_manage.is_mix_mode)
    {
        bk_dma_stop(coder_manage->in_channel);
        bk_dma_deinit(coder_manage->in_channel);
        bk_dma_free(DMA_DEV_DTCM, coder_manage->in_channel);

        g_dvp_module_manage.is_mix_mode = false;
    }

    if (g_dvp_module_manage.pingpong_buf)
    {
        os_free(g_dvp_module_manage.pingpong_buf);
        g_dvp_module_manage.pingpong_buf = NULL;
    }

    if (g_dvp_module_manage.base_frame)
    {
        dvp_frame_unassign_cb(g_dvp_module_manage.base_frame);
        g_dvp_module_manage.base_frame_len = 0;
        g_dvp_module_manage.base_frame = NULL;
    }

    if (g_dvp_module_manage.encoded_frame)
    {
        dvp_frame_unassign_cb(g_dvp_module_manage.encoded_frame);
        g_dvp_module_manage.encoded_frame_len = 0;
        g_dvp_module_manage.encoded_frame = NULL;
    }

    g_dvp_module_manage.error_flag = false;

    return OPRT_OK;
}

OPERATE_RET tkl_dvp_frame_assign_cb_register(DVP_FRAME_ASSIGN_CB func)
{
    dvp_frame_assign_cb = func;
    return OPRT_OK;
}

OPERATE_RET tkl_dvp_frame_unassign_cb_register(DVP_FRAME_UNASSIGN_CB func)
{
    dvp_frame_unassign_cb = func;
    return OPRT_OK;
}

OPERATE_RET tkl_dvp_frame_post_cb_register(DVP_FRAME_POST_CB func)
{
    dvp_frame_post_cb = func;
    return OPRT_OK;
}

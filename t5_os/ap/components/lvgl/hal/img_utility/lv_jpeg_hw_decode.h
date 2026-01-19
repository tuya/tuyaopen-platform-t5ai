#ifndef __LV_JPEG_HW_DECODE_H_
#define __LV_JPEG_HW_DECODE_H_

#include "lvgl.h"

typedef enum {
    JH_OUTPUT_RGB565,
    JH_OUTPUT_YUYV,
} JPEG_HW_OUTPUT_FMT_T;

bk_err_t lv_dma2d_yuyv2rgb565_init(void);

bk_err_t lv_dma2d_yuyv2rgb565_deinit(void);

s32 lv_jpeg_hw_decode(frame_buffer_t *jpeg_frame, lv_img_dsc_t *img_dst);

void lv_jpeg_hw_decode_output_fmt_set(JPEG_HW_OUTPUT_FMT_T jpeg_output_fmt);

void bk_jpeg_hw_decode_to_mem_init(void);

void bk_jpeg_hw_decode_to_mem_deinit(void);

bk_err_t bk_jpeg_hw_decode_to_mem(uint8_t *src_addr, uint8_t *dst_addr, uint32_t src_size, uint16_t dst_width, uint16_t dst_height);

#endif


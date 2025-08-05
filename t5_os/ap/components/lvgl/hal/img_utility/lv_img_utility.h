#ifndef __LV_IMG_UTILITY_H
#define __LV_IMG_UTILITY_H

#include "lvgl.h"

int lv_img_read_filelen(char *filename);
s32 lv_jpeg_img_load_with_sw_dec(char *filename, lv_img_dsc_t *img_dst);
s32 lv_jpeg_img_load_yuyv(char *filename, lv_img_dsc_t *img_dst);
s32 lv_jpeg_img_load_with_hw_dec(char *filename, lv_img_dsc_t *img_dst);
s32 lv_png_img_load(char *filename, lv_img_dsc_t *img_dst);
void lv_img_decode_unload(lv_img_dsc_t *img_dst);

#endif

